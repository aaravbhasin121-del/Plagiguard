#pragma once

#include "AppleOcrExtractor.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <cstdio>
#include <array>
#include <memory>

namespace plagiguard::processing {

struct ExtractionResult {
    bool success;
    std::string text;
    std::string fileType;
    size_t fileSize;
    bool isScannedPdf;
    std::string errorMessage;

    ExtractionResult()
        : success(false), fileSize(0), isScannedPdf(false) {}
};

/**
 * @brief TextExtractor - Extracts text from TXT, PDF (including Scanned PDF with Neural OCR), and DOCX files.
 *
 * Supported formats:
 * - Plain Text (.txt, .md)
 * - Adobe PDF (.pdf): Uses Apple PDFKit + Neural Vision OCR for scanned pages
 * - Microsoft Word (.docx, .doc): Uses native textutil + XML package extractor
 * - Scanned Images (.png, .jpg, .jpeg, .tiff): Native Vision Neural OCR
 */
class TextExtractor {
public:
    static ExtractionResult extract(const std::string& filePath) {
        ExtractionResult result;
        std::filesystem::path p(filePath);

        if (!std::filesystem::exists(p)) {
            result.success = false;
            result.errorMessage = "File does not exist: " + filePath;
            return result;
        }

        result.fileSize = std::filesystem::file_size(p);
        std::string ext = p.extension().string();
        for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (ext == ".txt" || ext == ".md" || ext == ".text") {
            result.fileType = "TXT";
            std::ifstream file(filePath, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                result.success = false;
                result.errorMessage = "Unable to open text file";
                return result;
            }
            std::ostringstream ss;
            ss << file.rdbuf();
            result.text = ss.str();
            result.success = true;
            return result;
        } else if (ext == ".pdf") {
            result.fileType = "PDF";
            bool usedOcr = false;
            std::string extracted = NativeOcrHelper::extractPdfWithOcr(filePath, usedOcr);

            if (extracted.empty() || extracted.find_first_not_of(" \t\n\r") == std::string::npos) {
                result.isScannedPdf = true;
                result.success = false;
                result.errorMessage = "Unable to extract text from PDF even with Vision OCR.";
                return result;
            }

            result.isScannedPdf = usedOcr;
            result.text = extracted;
            result.success = true;
            return result;
        } else if (ext == ".docx" || ext == ".doc") {
            result.fileType = "DOCX";
            std::string text = NativeOcrHelper::extractDocx(filePath);

            if (text.empty() || text.find_first_not_of(" \t\n\r") == std::string::npos) {
                result.success = false;
                result.errorMessage = "Failed to parse DOCX document content.";
                return result;
            }

            result.text = text;
            result.success = true;
            return result;
        } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tiff") {
            result.fileType = "IMAGE";
            std::string text = NativeOcrHelper::extractImageOcr(filePath);
            if (text.empty() || text.find_first_not_of(" \t\n\r") == std::string::npos) {
                result.success = false;
                result.errorMessage = "Neural OCR could not detect readable text in image.";
                return result;
            }
            result.text = text;
            result.success = true;
            return result;
        } else {
            result.fileType = "UNSUPPORTED";
            result.success = false;
            result.errorMessage = "Unsupported file extension: " + ext + ". Supported: .txt, .pdf, .docx, .png, .jpg";
            return result;
        }
    }
};

} // namespace plagiguard::processing

