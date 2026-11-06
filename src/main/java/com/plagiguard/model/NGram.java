package com.plagiguard.model;

import java.util.Arrays;
import java.util.Objects;

/**
 * Represents a single N-gram token sequence with position information.
 */
public class NGram {
    private String[] tokens;      // the words in this n-gram
    private String joined;        // space-joined string
    private int startPosition;    // start token index in document
    private int endPosition;      // end token index
    private long hash1;           // first rolling hash
    private long hash2;           // second rolling hash (double hashing)
    private int documentId;       // which document this belongs to

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /** No-arg constructor. */
    public NGram() {}

    /**
     * Convenience constructor: computes joined from tokens automatically.
     */
    public NGram(String[] tokens, int startPosition, int endPosition,
                 long hash1, long hash2, int documentId) {
        this.tokens = tokens;
        this.joined = tokens != null ? String.join(" ", tokens) : "";
        this.startPosition = startPosition;
        this.endPosition = endPosition;
        this.hash1 = hash1;
        this.hash2 = hash2;
        this.documentId = documentId;
    }

    /** Full constructor including explicit joined value. */
    public NGram(String[] tokens, String joined, int startPosition, int endPosition,
                 long hash1, long hash2, int documentId) {
        this.tokens = tokens;
        this.joined = joined;
        this.startPosition = startPosition;
        this.endPosition = endPosition;
        this.hash1 = hash1;
        this.hash2 = hash2;
        this.documentId = documentId;
    }

    // -----------------------------------------------------------------------
    // Getters
    // -----------------------------------------------------------------------

    public String[] getTokens() { return tokens; }
    public String getJoined() { return joined; }
    public int getStartPosition() { return startPosition; }
    public int getEndPosition() { return endPosition; }
    public long getHash1() { return hash1; }
    public long getHash2() { return hash2; }
    public int getDocumentId() { return documentId; }

    // -----------------------------------------------------------------------
    // Setters
    // -----------------------------------------------------------------------

    public void setTokens(String[] tokens) {
        this.tokens = tokens;
        this.joined = tokens != null ? String.join(" ", tokens) : "";
    }
    public void setJoined(String joined) { this.joined = joined; }
    public void setStartPosition(int startPosition) { this.startPosition = startPosition; }
    public void setEndPosition(int endPosition) { this.endPosition = endPosition; }
    public void setHash1(long hash1) { this.hash1 = hash1; }
    public void setHash2(long hash2) { this.hash2 = hash2; }
    public void setDocumentId(int documentId) { this.documentId = documentId; }

    // -----------------------------------------------------------------------
    // equals / hashCode / toString  — based on joined string for set operations
    // -----------------------------------------------------------------------

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof NGram other)) return false;
        return Objects.equals(joined, other.joined);
    }

    @Override
    public int hashCode() {
        return Objects.hashCode(joined);
    }

    @Override
    public String toString() {
        return "NGram{" +
               "joined='" + joined + '\'' +
               ", startPosition=" + startPosition +
               ", endPosition=" + endPosition +
               ", hash1=" + hash1 +
               ", hash2=" + hash2 +
               ", documentId=" + documentId +
               '}';
    }
}
