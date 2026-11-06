package com.plagiguard.model;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

/**
 * Represents an academic document in PlagiGuard.
 * Stores both original text (for display) and normalized text (for comparison).
 */
public class Document {
    private int id;
    private String name;                   // original filename
    private String filePath;               // path to uploaded file
    private String fileType;               // TXT, PDF, DOCX
    private long fileSize;                 // bytes
    private String originalContent;        // raw extracted text
    private String normalizedContent;      // lowercase, normalized whitespace
    private String[] tokens;              // tokenized words
    private String fingerprint;            // document fingerprint hash
    private int wordCount;
    private LocalDateTime uploadDate;
    // paragraph positions: List<int[]> each int[]{startTokenIndex, endTokenIndex}
    private List<int[]> paragraphBoundaries;

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /** No-arg constructor. */
    public Document() {
        this.paragraphBoundaries = new ArrayList<>();
    }

    /** Full-arg constructor. */
    public Document(int id, String name, String filePath, String fileType, long fileSize,
                    String originalContent, String normalizedContent, String[] tokens,
                    String fingerprint, int wordCount, LocalDateTime uploadDate,
                    List<int[]> paragraphBoundaries) {
        this.id = id;
        this.name = name;
        this.filePath = filePath;
        this.fileType = fileType;
        this.fileSize = fileSize;
        this.originalContent = originalContent;
        this.normalizedContent = normalizedContent;
        this.tokens = tokens;
        this.fingerprint = fingerprint;
        this.wordCount = wordCount;
        this.uploadDate = uploadDate;
        this.paragraphBoundaries = paragraphBoundaries != null ? paragraphBoundaries : new ArrayList<>();
    }

    // -----------------------------------------------------------------------
    // Getters
    // -----------------------------------------------------------------------

    public int getId() { return id; }
    public String getName() { return name; }
    public String getFilePath() { return filePath; }
    public String getFileType() { return fileType; }
    public long getFileSize() { return fileSize; }
    public String getOriginalContent() { return originalContent; }
    public String getNormalizedContent() { return normalizedContent; }
    public String[] getTokens() { return tokens; }
    public String getFingerprint() { return fingerprint; }
    public int getWordCount() { return wordCount; }
    public LocalDateTime getUploadDate() { return uploadDate; }
    public List<int[]> getParagraphBoundaries() { return paragraphBoundaries; }

    // -----------------------------------------------------------------------
    // Setters
    // -----------------------------------------------------------------------

    public void setId(int id) { this.id = id; }
    public void setName(String name) { this.name = name; }
    public void setFilePath(String filePath) { this.filePath = filePath; }
    public void setFileType(String fileType) { this.fileType = fileType; }
    public void setFileSize(long fileSize) { this.fileSize = fileSize; }
    public void setOriginalContent(String originalContent) { this.originalContent = originalContent; }
    public void setNormalizedContent(String normalizedContent) { this.normalizedContent = normalizedContent; }
    public void setTokens(String[] tokens) { this.tokens = tokens; }
    public void setFingerprint(String fingerprint) { this.fingerprint = fingerprint; }
    public void setWordCount(int wordCount) { this.wordCount = wordCount; }
    public void setUploadDate(LocalDateTime uploadDate) { this.uploadDate = uploadDate; }
    public void setParagraphBoundaries(List<int[]> paragraphBoundaries) {
        this.paragraphBoundaries = paragraphBoundaries;
    }

    // -----------------------------------------------------------------------
    // equals / hashCode / toString
    // -----------------------------------------------------------------------

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof Document other)) return false;
        return id == other.id && Objects.equals(fingerprint, other.fingerprint);
    }

    @Override
    public int hashCode() {
        return Objects.hash(id, fingerprint);
    }

    @Override
    public String toString() {
        return "Document{" +
               "id=" + id +
               ", name='" + name + '\'' +
               ", fileType='" + fileType + '\'' +
               ", fileSize=" + fileSize +
               ", wordCount=" + wordCount +
               ", uploadDate=" + uploadDate +
               ", fingerprint='" + fingerprint + '\'' +
               '}';
    }
}
