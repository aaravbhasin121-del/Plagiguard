package com.plagiguard.model;

import java.util.ArrayList;
import java.util.List;

/**
 * Contains the complete similarity analysis result between two documents.
 */
public class SimilarityResult {
    private int id;
    private int analysisId;
    private int documentAId;
    private int documentBId;
    private String documentAName;
    private String documentBName;
    private double jaccardScore;         // |A∩B|/|A∪B| * 100
    private double directionalScore;     // |A∩B|/|A| * 100
    private int matchingNgramCount;      // number of matching n-grams
    private int uniqueNgramsA;
    private int uniqueNgramsB;
    private int totalNgramsA;
    private int totalNgramsB;
    private int matchingPassageCount;
    private long processingTimeMs;
    private List<MatchResult> matchingPassages;

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /** No-arg constructor. */
    public SimilarityResult() {
        this.matchingPassages = new ArrayList<>();
    }

    /** Full constructor. */
    public SimilarityResult(int id, int analysisId,
                            int documentAId, int documentBId,
                            String documentAName, String documentBName,
                            double jaccardScore, double directionalScore,
                            int matchingNgramCount,
                            int uniqueNgramsA, int uniqueNgramsB,
                            int totalNgramsA, int totalNgramsB,
                            int matchingPassageCount, long processingTimeMs,
                            List<MatchResult> matchingPassages) {
        this.id = id;
        this.analysisId = analysisId;
        this.documentAId = documentAId;
        this.documentBId = documentBId;
        this.documentAName = documentAName;
        this.documentBName = documentBName;
        this.jaccardScore = jaccardScore;
        this.directionalScore = directionalScore;
        this.matchingNgramCount = matchingNgramCount;
        this.uniqueNgramsA = uniqueNgramsA;
        this.uniqueNgramsB = uniqueNgramsB;
        this.totalNgramsA = totalNgramsA;
        this.totalNgramsB = totalNgramsB;
        this.matchingPassageCount = matchingPassageCount;
        this.processingTimeMs = processingTimeMs;
        this.matchingPassages = matchingPassages != null ? matchingPassages : new ArrayList<>();
    }

    // -----------------------------------------------------------------------
    // Getters
    // -----------------------------------------------------------------------

    public int getId() { return id; }
    public int getAnalysisId() { return analysisId; }
    public int getDocumentAId() { return documentAId; }
    public int getDocumentBId() { return documentBId; }
    public String getDocumentAName() { return documentAName; }
    public String getDocumentBName() { return documentBName; }
    public double getJaccardScore() { return jaccardScore; }
    public double getDirectionalScore() { return directionalScore; }
    public int getMatchingNgramCount() { return matchingNgramCount; }
    public int getUniqueNgramsA() { return uniqueNgramsA; }
    public int getUniqueNgramsB() { return uniqueNgramsB; }
    public int getTotalNgramsA() { return totalNgramsA; }
    public int getTotalNgramsB() { return totalNgramsB; }
    public int getMatchingPassageCount() { return matchingPassageCount; }
    public long getProcessingTimeMs() { return processingTimeMs; }
    public List<MatchResult> getMatchingPassages() { return matchingPassages; }

    // -----------------------------------------------------------------------
    // Setters
    // -----------------------------------------------------------------------

    public void setId(int id) { this.id = id; }
    public void setAnalysisId(int analysisId) { this.analysisId = analysisId; }
    public void setDocumentAId(int documentAId) { this.documentAId = documentAId; }
    public void setDocumentBId(int documentBId) { this.documentBId = documentBId; }
    public void setDocumentAName(String documentAName) { this.documentAName = documentAName; }
    public void setDocumentBName(String documentBName) { this.documentBName = documentBName; }
    public void setJaccardScore(double jaccardScore) { this.jaccardScore = jaccardScore; }
    public void setDirectionalScore(double directionalScore) { this.directionalScore = directionalScore; }
    public void setMatchingNgramCount(int matchingNgramCount) { this.matchingNgramCount = matchingNgramCount; }
    public void setUniqueNgramsA(int uniqueNgramsA) { this.uniqueNgramsA = uniqueNgramsA; }
    public void setUniqueNgramsB(int uniqueNgramsB) { this.uniqueNgramsB = uniqueNgramsB; }
    public void setTotalNgramsA(int totalNgramsA) { this.totalNgramsA = totalNgramsA; }
    public void setTotalNgramsB(int totalNgramsB) { this.totalNgramsB = totalNgramsB; }
    public void setMatchingPassageCount(int matchingPassageCount) { this.matchingPassageCount = matchingPassageCount; }
    public void setProcessingTimeMs(long processingTimeMs) { this.processingTimeMs = processingTimeMs; }
    public void setMatchingPassages(List<MatchResult> matchingPassages) { this.matchingPassages = matchingPassages; }

    // -----------------------------------------------------------------------
    // Business logic
    // -----------------------------------------------------------------------

    /**
     * Returns a human-readable similarity level based on Jaccard score.
     *
     * @return "High" (>=70%), "Medium" (>=40%), "Low" (>=15%), or "Minimal"
     */
    public String getSimilarityLevel() {
        if (jaccardScore >= 70.0) return "High";
        if (jaccardScore >= 40.0) return "Medium";
        if (jaccardScore >= 15.0) return "Low";
        return "Minimal";
    }

    // -----------------------------------------------------------------------
    // toString
    // -----------------------------------------------------------------------

    @Override
    public String toString() {
        return "SimilarityResult{" +
               "id=" + id +
               ", analysisId=" + analysisId +
               ", documentAName='" + documentAName + '\'' +
               ", documentBName='" + documentBName + '\'' +
               ", jaccardScore=" + String.format("%.2f", jaccardScore) + "%" +
               ", directionalScore=" + String.format("%.2f", directionalScore) + "%" +
               ", similarityLevel='" + getSimilarityLevel() + '\'' +
               ", matchingNgramCount=" + matchingNgramCount +
               ", matchingPassageCount=" + matchingPassageCount +
               ", processingTimeMs=" + processingTimeMs +
               '}';
    }
}
