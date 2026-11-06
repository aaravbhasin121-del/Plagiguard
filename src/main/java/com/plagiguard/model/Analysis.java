package com.plagiguard.model;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;

/**
 * Represents a complete analysis session in PlagiGuard.
 */
public class Analysis {
    private int id;
    private String analysisType;    // TWO_DOCUMENT, BATCH, VERSION_COMPARE, LIBRARY_COMPARE
    private LocalDateTime analysisDate;
    private int documentCount;
    private long processingTimeMs;
    private String status;          // PENDING, RUNNING, COMPLETED, FAILED, CANCELLED
    private List<Document> documents;
    private List<SimilarityResult> results;
    private String errorMessage;

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /** No-arg constructor. */
    public Analysis() {
        this.documents = new ArrayList<>();
        this.results = new ArrayList<>();
        this.status = "PENDING";
    }

    /** Full constructor. */
    public Analysis(int id, String analysisType, LocalDateTime analysisDate,
                    int documentCount, long processingTimeMs, String status,
                    List<Document> documents, List<SimilarityResult> results,
                    String errorMessage) {
        this.id = id;
        this.analysisType = analysisType;
        this.analysisDate = analysisDate;
        this.documentCount = documentCount;
        this.processingTimeMs = processingTimeMs;
        this.status = status;
        this.documents = documents != null ? documents : new ArrayList<>();
        this.results = results != null ? results : new ArrayList<>();
        this.errorMessage = errorMessage;
    }

    // -----------------------------------------------------------------------
    // Getters
    // -----------------------------------------------------------------------

    public int getId() { return id; }
    public String getAnalysisType() { return analysisType; }
    public LocalDateTime getAnalysisDate() { return analysisDate; }
    public int getDocumentCount() { return documentCount; }
    public long getProcessingTimeMs() { return processingTimeMs; }
    public String getStatus() { return status; }
    public List<Document> getDocuments() { return documents; }
    public List<SimilarityResult> getResults() { return results; }
    public String getErrorMessage() { return errorMessage; }

    // -----------------------------------------------------------------------
    // Setters
    // -----------------------------------------------------------------------

    public void setId(int id) { this.id = id; }
    public void setAnalysisType(String analysisType) { this.analysisType = analysisType; }
    public void setAnalysisDate(LocalDateTime analysisDate) { this.analysisDate = analysisDate; }
    public void setDocumentCount(int documentCount) { this.documentCount = documentCount; }
    public void setProcessingTimeMs(long processingTimeMs) { this.processingTimeMs = processingTimeMs; }
    public void setStatus(String status) { this.status = status; }
    public void setDocuments(List<Document> documents) { this.documents = documents; }
    public void setResults(List<SimilarityResult> results) { this.results = results; }
    public void setErrorMessage(String errorMessage) { this.errorMessage = errorMessage; }

    // -----------------------------------------------------------------------
    // Business logic
    // -----------------------------------------------------------------------

    /**
     * Computes the average Jaccard score across all similarity results.
     *
     * @return average similarity as a percentage (0.0 if no results)
     */
    public double getAverageSimilarity() {
        if (results == null || results.isEmpty()) return 0.0;
        double sum = 0.0;
        for (SimilarityResult r : results) {
            sum += r.getJaccardScore();
        }
        return sum / results.size();
    }

    // -----------------------------------------------------------------------
    // toString
    // -----------------------------------------------------------------------

    @Override
    public String toString() {
        return "Analysis{" +
               "id=" + id +
               ", analysisType='" + analysisType + '\'' +
               ", analysisDate=" + analysisDate +
               ", documentCount=" + documentCount +
               ", status='" + status + '\'' +
               ", processingTimeMs=" + processingTimeMs +
               ", resultCount=" + (results != null ? results.size() : 0) +
               ", averageSimilarity=" + String.format("%.2f", getAverageSimilarity()) + "%" +
               '}';
    }
}
