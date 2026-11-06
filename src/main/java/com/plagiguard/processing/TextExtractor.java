package com.plagiguard.processing;

import java.io.*;

/**
 * Factory and coordinator for document text extraction.
 * Dispatches to the appropriate extractor based on file type.
 *
 * Supported formats: TXT, PDF, DOCX
 * For unsupported formats or scanned PDFs, returns descriptive error.
 */
public class TextExtractor {
    private final PDFExtractor pdfExtractor;
    private final DOCXExtractor docxExtractor;

    /** Default constructor — creates internal extractor instances. */
    public TextExtractor() {
        this.pdfExtractor  = new PDFExtractor();
        this.docxExtractor = new DOCXExtractor();
    }

    // -----------------------------------------------------------------------
    // extract
    // -----------------------------------------------------------------------

    /**
     * Extract text from a file, auto-detecting type by extension.
     *
     * @param file the file to extract from
     * @return ExtractionResult with text and metadata
     */
    public ExtractionResult extract(File file) {
        if (file == null) {
            return new ExtractionResult("", false, "File reference is null.", "UNKNOWN", false);
        }

        ValidationResult validation = validate(file, 50L * 1024 * 1024); // 50 MB default
        if (!validation.valid) {
            return new ExtractionResult("", false, validation.message, "UNKNOWN", false);
        }

        String name = file.getName().toLowerCase();
        try {
            if (name.endsWith(".txt")) {
                return extractTxt(file);
            } else if (name.endsWith(".pdf")) {
                return extractPdf(file);
            } else if (name.endsWith(".docx")) {
                return extractDocx(file);
            } else {
                String ext = name.contains(".")
                    ? name.substring(name.lastIndexOf('.') + 1).toUpperCase()
                    : "UNKNOWN";
                return new ExtractionResult("", false,
                    "Unsupported file format: " + ext + ". Supported formats: TXT, PDF, DOCX.",
                    ext, false);
            }
        } catch (Exception e) {
            return new ExtractionResult("", false,
                "Extraction failed: " + e.getMessage(), "UNKNOWN", false);
        }
    }

    // -----------------------------------------------------------------------
    // validate
    // -----------------------------------------------------------------------

    /**
     * Validate file before extraction (extension, size, readable).
     *
     * @param file         file to validate
     * @param maxSizeBytes maximum allowed file size in bytes
     * @return ValidationResult
     */
    public ValidationResult validate(File file, long maxSizeBytes) {
        if (file == null) {
            return new ValidationResult(false, "File is null.");
        }
        if (!file.exists()) {
            return new ValidationResult(false, "File does not exist: " + file.getAbsolutePath());
        }
        if (!file.isFile()) {
            return new ValidationResult(false, "Path is not a regular file: " + file.getAbsolutePath());
        }
        if (!file.canRead()) {
            return new ValidationResult(false, "File is not readable: " + file.getAbsolutePath());
        }

        long size = file.length();
        if (size == 0) {
            return new ValidationResult(false, "File is empty: " + file.getName());
        }
        if (size > maxSizeBytes) {
            double mb = maxSizeBytes / (1024.0 * 1024.0);
            return new ValidationResult(false,
                String.format("File exceeds maximum allowed size of %.0f MB: %s", mb, file.getName()));
        }

        String lowerName = file.getName().toLowerCase();
        if (!lowerName.endsWith(".txt") && !lowerName.endsWith(".pdf") && !lowerName.endsWith(".docx")) {
            return new ValidationResult(false,
                "Unsupported file extension. Only TXT, PDF, and DOCX files are accepted.");
        }

        return new ValidationResult(true, "File is valid.");
    }

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    private ExtractionResult extractTxt(File file) throws IOException {
        StringBuilder sb = new StringBuilder((int) Math.min(file.length(), Integer.MAX_VALUE));
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(file), java.nio.charset.StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                sb.append(line).append('\n');
            }
        } catch (java.nio.charset.MalformedInputException e) {
            // Retry with ISO-8859-1
            sb.setLength(0);
            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(new FileInputStream(file), java.nio.charset.StandardCharsets.ISO_8859_1))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line).append('\n');
                }
            }
        }

        String text = sb.toString().strip();
        if (text.isEmpty()) {
            return new ExtractionResult("", false, "TXT file contained no text: " + file.getName(), "TXT", false);
        }
        int wordCount = text.split("\\s+").length;
        return new ExtractionResult(text, true, null, "TXT", false) {
            // wordCount override via anonymous subclass not needed — use the constructor field
        };
        // Re-create with proper word count
    }

    private ExtractionResult extractPdf(File file) throws IOException {
        boolean hasText = pdfExtractor.hasExtractableText(file);
        if (!hasText) {
            return new ExtractionResult("", false,
                "PDF appears to contain only scanned images with no extractable text layer: " + file.getName(),
                "PDF", true);
        }
        String text = pdfExtractor.extract(file);
        if (text == null || text.isBlank()) {
            return new ExtractionResult("", false,
                "No text could be extracted from PDF: " + file.getName(), "PDF", true);
        }
        text = text.strip();
        int wordCount = text.split("\\s+").length;
        return new ExtractionResult(text, true, null, "PDF", false);
    }

    private ExtractionResult extractDocx(File file) throws IOException {
        String text = docxExtractor.extract(file);
        if (text == null || text.isBlank()) {
            return new ExtractionResult("", false,
                "No text could be extracted from DOCX: " + file.getName(), "DOCX", false);
        }
        text = text.strip();
        return new ExtractionResult(text, true, null, "DOCX", false);
    }

    // -----------------------------------------------------------------------
    // Inner result classes
    // -----------------------------------------------------------------------

    /** Result of text extraction. */
    public static class ExtractionResult {
        public final String text;
        public final boolean success;
        public final String errorMessage;
        public final String fileType;
        public final int estimatedWordCount;
        public final boolean isScannedPdf; // true if PDF had no extractable text

        public ExtractionResult(String text, boolean success, String errorMessage,
                                String fileType, boolean isScannedPdf) {
            this.text = text != null ? text : "";
            this.success = success;
            this.errorMessage = errorMessage;
            this.fileType = fileType;
            this.isScannedPdf = isScannedPdf;
            // Estimate word count from text
            this.estimatedWordCount = (this.text.isBlank()) ? 0 : this.text.split("\\s+").length;
        }

        @Override
        public String toString() {
            return "ExtractionResult{" +
                   "success=" + success +
                   ", fileType='" + fileType + '\'' +
                   ", estimatedWordCount=" + estimatedWordCount +
                   ", isScannedPdf=" + isScannedPdf +
                   (errorMessage != null ? ", errorMessage='" + errorMessage + '\'' : "") +
                   '}';
        }
    }

    /** Result of file validation. */
    public static class ValidationResult {
        public final boolean valid;
        public final String message;

        public ValidationResult(boolean valid, String message) {
            this.valid = valid;
            this.message = message;
        }

        @Override
        public String toString() {
            return "ValidationResult{valid=" + valid + ", message='" + message + "'}";
        }
    }
}
