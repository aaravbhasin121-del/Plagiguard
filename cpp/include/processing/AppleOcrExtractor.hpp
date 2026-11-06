#pragma once

#import <Foundation/Foundation.h>
#import <PDFKit/PDFKit.h>
#import <Vision/Vision.h>
#import <AppKit/AppKit.h>

#include <string>
#include <vector>
#include <iostream>
#include <array>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace plagiguard::processing {

/**
 * @brief NativeOcrHelper - Apple Native Neural OCR & PDF/DOCX Extractor.
 *
 * Implements:
 * 1. Apple PDFKit direct text extraction
 * 2. Apple Vision Framework Neural OCR (VNRecognizeTextRequest) for scanned PDFs and images
 * 3. macOS native textutil for Microsoft Word (.docx)
 */
class NativeOcrHelper {
private:
    static std::string execCommand(const std::string& cmd) {
        std::array<char, 512> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

public:
    static std::string extractDocx(const std::string& filePath) {
        // 1. Try macOS native textutil
        std::string cmd = "/usr/bin/textutil -convert txt \"" + filePath + "\" -stdout 2>/dev/null";
        std::string text = execCommand(cmd);

        if (!text.empty() && text.find_first_not_of(" \t\n\r") != std::string::npos &&
            text.rfind("PK\x03\x04", 0) != 0 && text.rfind("PK\\u0003", 0) != 0) {
            return text;
        }

        // 2. Fallback: unzip word/document.xml
        std::string unzipCmd = "/usr/bin/unzip -p \"" + filePath + "\" word/document.xml 2>/dev/null";
        std::string xml = execCommand(unzipCmd);
        if (!xml.empty()) {
            std::string extracted;
            bool insideTag = false;
            for (size_t i = 0; i < xml.size(); ++i) {
                if (xml[i] == '<') {
                    if (i + 4 < xml.size() && xml.substr(i, 4) == "<w:p") {
                        if (!extracted.empty() && extracted.back() != '\n') extracted += "\n\n";
                    }
                    insideTag = true;
                } else if (xml[i] == '>') {
                    insideTag = false;
                } else if (!insideTag) {
                    extracted += xml[i];
                }
            }
            return extracted;
        }

        return "";
    }

    static std::string extractPdfWithOcr(const std::string& filePath, bool& outUsedOcr) {
        outUsedOcr = false;
        @autoreleasepool {
            NSString* nsPath = [NSString stringWithUTF8String:filePath.c_str()];
            NSURL* url = [NSURL fileURLWithPath:nsPath];
            PDFDocument* pdf = [[PDFDocument alloc] initWithURL:url];
            if (!pdf) return "";

            // 1. Try direct text extraction
            NSString* rawText = [pdf string];
            if (rawText != nil) {
                NSString* trimmed = [rawText stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
                if ([trimmed length] > 50) {
                    return std::string([rawText UTF8String]);
                }
            }

            // 2. PDF has no text layer or very sparse text -> Run Apple Vision Neural OCR
            outUsedOcr = true;
            NSMutableString* ocrResult = [NSMutableString string];
            NSUInteger pageCount = [pdf pageCount];

            for (NSUInteger i = 0; i < pageCount; i++) {
                PDFPage* page = [pdf pageAtIndex:i];
                if (!page) continue;

                NSRect bounds = [page boundsForBox:kPDFDisplayBoxMediaBox];
                if (bounds.size.width <= 0 || bounds.size.height <= 0) continue;

                // Scale up image 2x for high OCR precision
                NSSize renderSize = NSMakeSize(bounds.size.width * 2.0, bounds.size.height * 2.0);
                NSImage* image = [[NSImage alloc] initWithSize:renderSize];
                [image lockFocus];
                CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
                CGContextScaleCTM(ctx, 2.0, 2.0);
                [page drawWithBox:kPDFDisplayBoxMediaBox toContext:ctx];
                [image unlockFocus];

                CGImageRef cgImage = [image CGImageForProposedRect:NULL context:nil hints:nil];
                if (cgImage) {
                    VNRecognizeTextRequest* request = [[VNRecognizeTextRequest alloc] init];
                    request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
                    request.usesLanguageCorrection = YES;

                    VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithCGImage:cgImage options:@{}];
                    NSError* err = nil;
                    [handler performRequests:@[request] error:&err];

                    for (VNRecognizedTextObservation* obs in request.results) {
                        VNRecognizedText* candidate = [[obs topCandidates:1] firstObject];
                        if (candidate) {
                            [ocrResult appendString:candidate.string];
                            [ocrResult appendString:@"\n"];
                        }
                    }
                    [ocrResult appendString:@"\n\n"];
                }
            }

            return std::string([ocrResult UTF8String]);
        }
    }

    static std::string extractImageOcr(const std::string& filePath) {
        @autoreleasepool {
            NSString* nsPath = [NSString stringWithUTF8String:filePath.c_str()];
            NSURL* url = [NSURL fileURLWithPath:nsPath];
            NSImage* image = [[NSImage alloc] initWithContentsOfURL:url];
            if (!image) return "";

            CGImageRef cgImage = [image CGImageForProposedRect:NULL context:nil hints:nil];
            if (!cgImage) return "";

            NSMutableString* ocrResult = [NSMutableString string];
            VNRecognizeTextRequest* request = [[VNRecognizeTextRequest alloc] init];
            request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
            request.usesLanguageCorrection = YES;

            VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithCGImage:cgImage options:@{}];
            NSError* err = nil;
            [handler performRequests:@[request] error:&err];

            for (VNRecognizedTextObservation* obs in request.results) {
                VNRecognizedText* candidate = [[obs topCandidates:1] firstObject];
                if (candidate) {
                    [ocrResult appendString:candidate.string];
                    [ocrResult appendString:@"\n"];
                }
            }

            return std::string([ocrResult UTF8String]);
        }
    }
};

} // namespace plagiguard::processing

