package com.plagiguard.processing;

import org.apache.poi.xwpf.usermodel.*;

import java.io.*;
import java.util.ArrayList;
import java.util.List;

/**
 * Extracts text from DOCX files using Apache POI.
 * Preserves paragraph boundaries.
 */
public class DOCXExtractor {

    // -----------------------------------------------------------------------
    // extract
    // -----------------------------------------------------------------------

    /**
     * Extract full text from DOCX file.
     * Paragraphs are joined with newlines so the caller can detect boundaries.
     *
     * @param file DOCX file
     * @return full extracted text (paragraphs separated by "\n\n")
     * @throws IOException if the file cannot be read or is not a valid DOCX
     */
    public String extract(File file) throws IOException {
        List<String> paragraphs = extractParagraphs(file);
        if (paragraphs.isEmpty()) return "";

        StringBuilder sb = new StringBuilder();
        for (String para : paragraphs) {
            if (!para.isBlank()) {
                sb.append(para.strip()).append("\n\n");
            }
        }
        // Remove trailing double-newline
        String result = sb.toString();
        if (result.endsWith("\n\n")) {
            result = result.substring(0, result.length() - 2);
        }
        return result;
    }

    // -----------------------------------------------------------------------
    // extractParagraphs
    // -----------------------------------------------------------------------

    /**
     * Extract text with paragraph boundaries preserved.
     * Each element in the returned list is the text of one paragraph.
     * Empty/whitespace-only paragraphs are included as empty strings.
     *
     * @param file DOCX file
     * @return list of paragraph strings
     * @throws IOException if the file cannot be read or is not a valid DOCX
     */
    public List<String> extractParagraphs(File file) throws IOException {
        List<String> paragraphs = new ArrayList<>();

        try (FileInputStream fis = new FileInputStream(file);
             XWPFDocument document = new XWPFDocument(fis)) {

            // Body paragraphs
            for (XWPFParagraph paragraph : document.getParagraphs()) {
                paragraphs.add(extractParagraphText(paragraph));
            }

            // Also extract text from tables (cell by cell, row by row)
            for (XWPFTable table : document.getTables()) {
                for (XWPFTableRow row : table.getRows()) {
                    StringBuilder rowText = new StringBuilder();
                    List<XWPFTableCell> cells = row.getTableCells();
                    for (int i = 0; i < cells.size(); i++) {
                        XWPFTableCell cell = cells.get(i);
                        // Collect all paragraphs inside the cell
                        StringBuilder cellText = new StringBuilder();
                        for (XWPFParagraph para : cell.getParagraphs()) {
                            String text = extractParagraphText(para);
                            if (!text.isBlank()) {
                                if (cellText.length() > 0) cellText.append(' ');
                                cellText.append(text.strip());
                            }
                        }
                        if (cellText.length() > 0) {
                            if (rowText.length() > 0) rowText.append('\t');
                            rowText.append(cellText);
                        }
                    }
                    if (rowText.length() > 0) {
                        paragraphs.add(rowText.toString());
                    }
                }
            }

        } catch (org.apache.poi.openxml4j.exceptions.InvalidFormatException e) {
            throw new IOException("File is not a valid DOCX document: " + file.getName(), e);
        }

        return paragraphs;
    }

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /**
     * Extract plain text from a single XWPFParagraph, including all runs,
     * hyperlinks, and footnote references.
     */
    private String extractParagraphText(XWPFParagraph paragraph) {
        if (paragraph == null) return "";
        StringBuilder sb = new StringBuilder();

        for (XWPFRun run : paragraph.getRuns()) {
            String runText = run.getText(0);
            if (runText != null) {
                sb.append(runText);
            }
        }

        // Attempt to get hyperlink text as well
        for (XWPFHyperlink link : paragraph.getDocument().getHyperlinks()) {
            // Hyperlink text is already included in runs via paragraph.getRuns()
            // so no additional processing needed here.
        }

        return sb.toString();
    }
}
