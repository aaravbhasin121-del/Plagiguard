package com.plagiguard.model;

/**
 * Represents a matching passage found between two documents.
 */
public class MatchResult {
    private int id;
    private int resultId;                  // FK to SimilarityResult
    private int sourceDocumentId;
    private int targetDocumentId;
    private String sourceDocumentName;
    private String targetDocumentName;
    private int sourceStartPosition;       // token position in source
    private int sourceEndPosition;
    private int targetStartPosition;       // token position in target
    private int targetEndPosition;
    private String matchingText;           // the actual matched text
    private int matchLength;               // word count of match
    private MatchType matchType;           // EXACT, PARTIAL, NGRAM
    private double similarityContribution; // how much this match contributes to overall score

    public enum MatchType { EXACT, PARTIAL, NGRAM }

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /** No-arg constructor. */
    public MatchResult() {}

    /** Full constructor. */
    public MatchResult(int id, int resultId,
                       int sourceDocumentId, int targetDocumentId,
                       String sourceDocumentName, String targetDocumentName,
                       int sourceStartPosition, int sourceEndPosition,
                       int targetStartPosition, int targetEndPosition,
                       String matchingText, int matchLength,
                       MatchType matchType, double similarityContribution) {
        this.id = id;
        this.resultId = resultId;
        this.sourceDocumentId = sourceDocumentId;
        this.targetDocumentId = targetDocumentId;
        this.sourceDocumentName = sourceDocumentName;
        this.targetDocumentName = targetDocumentName;
        this.sourceStartPosition = sourceStartPosition;
        this.sourceEndPosition = sourceEndPosition;
        this.targetStartPosition = targetStartPosition;
        this.targetEndPosition = targetEndPosition;
        this.matchingText = matchingText;
        this.matchLength = matchLength;
        this.matchType = matchType;
        this.similarityContribution = similarityContribution;
    }

    // -----------------------------------------------------------------------
    // Getters
    // -----------------------------------------------------------------------

    public int getId() { return id; }
    public int getResultId() { return resultId; }
    public int getSourceDocumentId() { return sourceDocumentId; }
    public int getTargetDocumentId() { return targetDocumentId; }
    public String getSourceDocumentName() { return sourceDocumentName; }
    public String getTargetDocumentName() { return targetDocumentName; }
    public int getSourceStartPosition() { return sourceStartPosition; }
    public int getSourceEndPosition() { return sourceEndPosition; }
    public int getTargetStartPosition() { return targetStartPosition; }
    public int getTargetEndPosition() { return targetEndPosition; }
    public String getMatchingText() { return matchingText; }
    public int getMatchLength() { return matchLength; }
    public MatchType getMatchType() { return matchType; }
    public double getSimilarityContribution() { return similarityContribution; }

    // -----------------------------------------------------------------------
    // Setters
    // -----------------------------------------------------------------------

    public void setId(int id) { this.id = id; }
    public void setResultId(int resultId) { this.resultId = resultId; }
    public void setSourceDocumentId(int sourceDocumentId) { this.sourceDocumentId = sourceDocumentId; }
    public void setTargetDocumentId(int targetDocumentId) { this.targetDocumentId = targetDocumentId; }
    public void setSourceDocumentName(String sourceDocumentName) { this.sourceDocumentName = sourceDocumentName; }
    public void setTargetDocumentName(String targetDocumentName) { this.targetDocumentName = targetDocumentName; }
    public void setSourceStartPosition(int sourceStartPosition) { this.sourceStartPosition = sourceStartPosition; }
    public void setSourceEndPosition(int sourceEndPosition) { this.sourceEndPosition = sourceEndPosition; }
    public void setTargetStartPosition(int targetStartPosition) { this.targetStartPosition = targetStartPosition; }
    public void setTargetEndPosition(int targetEndPosition) { this.targetEndPosition = targetEndPosition; }
    public void setMatchingText(String matchingText) { this.matchingText = matchingText; }
    public void setMatchLength(int matchLength) { this.matchLength = matchLength; }
    public void setMatchType(MatchType matchType) { this.matchType = matchType; }
    public void setSimilarityContribution(double similarityContribution) { this.similarityContribution = similarityContribution; }

    // -----------------------------------------------------------------------
    // toString
    // -----------------------------------------------------------------------

    @Override
    public String toString() {
        return "MatchResult{" +
               "id=" + id +
               ", resultId=" + resultId +
               ", sourceDocumentId=" + sourceDocumentId +
               ", targetDocumentId=" + targetDocumentId +
               ", sourceStartPosition=" + sourceStartPosition +
               ", sourceEndPosition=" + sourceEndPosition +
               ", targetStartPosition=" + targetStartPosition +
               ", targetEndPosition=" + targetEndPosition +
               ", matchLength=" + matchLength +
               ", matchType=" + matchType +
               ", similarityContribution=" + similarityContribution +
               ", matchingText='" + (matchingText != null && matchingText.length() > 50
                   ? matchingText.substring(0, 50) + "..." : matchingText) + '\'' +
               '}';
    }
}
