package com.plagiguard.processing;

import org.apache.pdfbox.pdmodel.PDDocument;
import org.apache.pdfbox.pdmodel.PDPage;
import org.apache.pdfbox.text.PDFTextStripper;

import java.io.*;
import java.util.ArrayList;
import java.util.List;

/**
 * Extracts text from PDF files using Apache PDFBox.
 * Detects scanned PDFs (no extractable text layer).
 */
public class PDFExtractor {

    /**
     * Minimum average characters per page to consider a PDF as having extractable text.
     * PDFs with fewer characters per page are likely scanned images.
     */
    private static final int MIN_CHARS_PER_PAGE_THRESHOLD = 50;

    // -----------------------------------------------------------------------
    // extract
    // -----------------------------------------------------------------------

    /**
     * Extract text from PDF file.
     *
     * @param file PDF file
     * @return extracted text, or empty string if scanned
     * @throws IOException if the file cannot be read or parsed
     */
    public String extract(File file) throws IOException {
        try (PDDocument document = PDDocument.load(file)) {
            if (document.isEncrypted()) {
                throw new IOException("PDF is password-protected and cannot be read.");
            }

            PDFTextStripper stripper = new PDFTextStripper();
            stripper.setSortByPosition(true);
            stripper.setLineSeparator("\n");
            stripper.setParagraphStart("\n");

            String text = stripper.getText(document);
            return (text != null) ? text : "";
        }
    }

    // -----------------------------------------------------------------------
    // hasExtractableText
    // -----------------------------------------------------------------------

    /**
     * Check if PDF has extractable text (not just scanned images).
     * A PDF is considered scanned if it has pages but very little extractable text.
     *
     * @param file PDF file
     * @return {@code true} if the PDF has a usable text layer
     * @throws IOException if the file cannot be read
     */
    public boolean hasExtractableText(File file) throws IOException {
        try (PDDocument document = PDDocument.load(file)) {
            if (document.isEncrypted()) return false;

            int numberOfPages = document.getNumberOfPages();
            if (numberOfPages == 0) return false;

            PDFTextStripper stripper = new PDFTextStripper();
            // Only strip a small portion for the check (first 3 pages max)
            stripper.setStartPage(1);
            stripper.setEndPage(Math.min(3, numberOfPages));

            String sample = stripper.getText(document);
            if (sample == null || sample.isBlank()) return false;

            int samplePages = Math.min(3, numberOfPages);
            double avgCharsPerPage = (double) sample.length() / samplePages;
            return avgCharsPerPage >= MIN_CHARS_PER_PAGE_THRESHOLD;
        }
    }

    // -----------------------------------------------------------------------
    // extractByPage
    // -----------------------------------------------------------------------

    /**
     * Extract text page by page for position tracking.
     *
     * @param file PDF file
     * @return list of strings, one per page (never null; empty string for blank pages)
     * @throws IOException if the file cannot be read
     */
    public List<String> extractByPage(File file) throws IOException {
        List<String> pages = new ArrayList<>();

        try (PDDocument document = PDDocument.load(file)) {
            if (document.isEncrypted()) {
                throw new IOException("PDF is password-protected and cannot be read.");
            }

            int numberOfPages = document.getNumberOfPages();
            PDFTextStripper stripper = new PDFTextStripper();
            stripper.setSortByPosition(true);
            stripper.setLineSeparator("\n");

            for (int pageNum = 1; pageNum <= numberOfPages; pageNum++) {
                stripper.setStartPage(pageNum);
                stripper.setEndPage(pageNum);
                String pageText = stripper.getText(document);
                pages.add(pageText != null ? pageText : "");
            }
        }

        return pages;
    }
}
