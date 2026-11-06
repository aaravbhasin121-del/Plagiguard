package com.plagiguard.service;

import com.plagiguard.model.Analysis;
import com.plagiguard.model.Document;
import com.plagiguard.model.MatchResult;
import com.plagiguard.model.SimilarityResult;
import com.plagiguard.detector.PlagiarismDetector;
import com.plagiguard.dsa.DocumentGraph;
import com.plagiguard.dsa.MaxHeap;
import com.plagiguard.database.AnalysisRepository;
import com.plagiguard.database.DatabaseManager;
import com.plagiguard.database.DocumentRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.sql.SQLException;
import java.time.LocalDateTime;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.Consumer;

/**
 * Coordinates complete plagiarism analysis sessions.
 *
 * Analysis types:
 * - TWO_DOCUMENT: Compare document A against document B
 * - BATCH: Compare all uploaded documents against each other
 * - LIBRARY_COMPARE: Compare new document against all library documents
 * - VERSION_COMPARE: Compare two versions of same document
 *
 * Uses SwingWorker-compatible progress reporting via Consumer<ProgressUpdate>.
 */
public class AnalysisService {
    private static final Logger logger = LoggerFactory.getLogger(AnalysisService.class);

    private final PlagiarismDetector detector;
    private final AnalysisRepository analysisRepo;
    private final DocumentRepository docRepo;
    private final DocumentGraph documentGraph;
    private final MaxHeap rankingHeap;

    private volatile boolean cancelled = false;

    public AnalysisService(DatabaseManager dbManager) {
        this.detector      = new PlagiarismDetector();
        this.analysisRepo  = new AnalysisRepository(dbManager);
        this.docRepo       = new DocumentRepository(dbManager);
        this.documentGraph = new DocumentGraph();
        this.rankingHeap   = new MaxHeap();
    }

    // -------------------------------------------------------------------------
    // Two-document comparison
    // -------------------------------------------------------------------------

    /**
     * Run a two-document comparison.
     */
    public Analysis analyzeTwoDocuments(Document docA, Document docB, int ngramSize,
                                        Consumer<ProgressUpdate> progressCallback) throws SQLException {
        cancelled = false;
        long startTime = System.currentTimeMillis();
        detector.setNgramSize(ngramSize);

        Analysis analysis = new Analysis();
        analysis.setAnalysisType("TWO_DOCUMENT");
        analysis.setAnalysisDate(LocalDateTime.now());
        analysis.setDocumentCount(2);
        analysis.setStatus("RUNNING");
        int analysisId = analysisRepo.saveAnalysis(analysis);
        analysisRepo.saveAnalysisDocuments(analysisId, List.of(docA.getId(), docB.getId()));

        notifyProgress(progressCallback, "Initializing", 5, "Preparing documents...");

        try {
            if (cancelled) return finishCancelled(analysis);

            notifyProgress(progressCallback, "Detecting", 30, "Running Rabin-Karp matching...");
            SimilarityResult result = detector.detect(docA, docB, analysisId);

            if (cancelled) return finishCancelled(analysis);

            notifyProgress(progressCallback, "Saving", 80, "Persisting results...");
            int resultId = analysisRepo.saveSimilarityResult(result);
            List<MatchResult> matches = result.getMatchingPassages();
            if (matches != null && !matches.isEmpty()) {
                analysisRepo.saveMatchingPassages(resultId, matches);
            }

            // Update graph and heap
            documentGraph.addEdge(docA.getId(), docA.getName(),
                                  docB.getId(), docB.getName(), result.getJaccardScore());
            rankingHeap.insert(new MaxHeap.HeapEntry(docA, docB, result.getJaccardScore()));

            long elapsed = System.currentTimeMillis() - startTime;
            analysis.setProcessingTimeMs(elapsed);
            analysis.setStatus("COMPLETED");
            analysisRepo.updateAnalysisStatus(analysisId, "COMPLETED", null);

            notifyProgress(progressCallback, "Complete", 100, "Analysis finished.");
            logger.info("TWO_DOCUMENT analysis {} completed in {}ms", analysisId, elapsed);

            // Attach results for caller
            analysis.setSimilarityResults(List.of(result));
            return analysis;

        } catch (Exception e) {
            logger.error("Analysis {} failed", analysisId, e);
            analysisRepo.updateAnalysisStatus(analysisId, "FAILED", e.getMessage());
            analysis.setStatus("FAILED");
            analysis.setErrorMessage(e.getMessage());
            throw new SQLException("Analysis failed: " + e.getMessage(), e);
        }
    }

    // -------------------------------------------------------------------------
    // Batch analysis
    // -------------------------------------------------------------------------

    /**
     * Run batch analysis across all provided documents. Compares every pair.
     */
    public Analysis analyzeBatch(List<Document> documents, int ngramSize,
                                 Consumer<ProgressUpdate> progressCallback) throws SQLException {
        cancelled = false;
        long startTime = System.currentTimeMillis();
        detector.setNgramSize(ngramSize);

        int n = documents.size();
        int totalPairs = n * (n - 1) / 2;

        Analysis analysis = new Analysis();
        analysis.setAnalysisType("BATCH");
        analysis.setAnalysisDate(LocalDateTime.now());
        analysis.setDocumentCount(n);
        analysis.setStatus("RUNNING");
        int analysisId = analysisRepo.saveAnalysis(analysis);

        List<Integer> docIds = new ArrayList<>();
        for (Document d : documents) docIds.add(d.getId());
        analysisRepo.saveAnalysisDocuments(analysisId, docIds);

        notifyProgress(progressCallback, "Initializing", 2,
                "Preparing " + n + " documents for batch comparison...");

        List<SimilarityResult> allResults = new ArrayList<>();
        int pairsDone = 0;

        try {
            for (int i = 0; i < n; i++) {
                for (int j = i + 1; j < n; j++) {
                    if (cancelled) return finishCancelled(analysis);

                    Document docA = documents.get(i);
                    Document docB = documents.get(j);

                    int progress = 5 + (int) ((double) pairsDone / totalPairs * 85);
                    notifyProgress(progressCallback, "Comparing",
                            progress, "Comparing: " + docA.getName() + " vs " + docB.getName());

                    SimilarityResult result = detector.detect(docA, docB, analysisId);
                    int resultId = analysisRepo.saveSimilarityResult(result);
                    List<MatchResult> matches = result.getMatchingPassages();
                    if (matches != null && !matches.isEmpty()) {
                        analysisRepo.saveMatchingPassages(resultId, matches);
                    }

                    documentGraph.addEdge(docA.getId(), docA.getName(),
                                         docB.getId(), docB.getName(), result.getJaccardScore());
                    rankingHeap.insert(new MaxHeap.HeapEntry(docA, docB, result.getJaccardScore()));
                    allResults.add(result);
                    pairsDone++;
                }
            }

            long elapsed = System.currentTimeMillis() - startTime;
            analysis.setProcessingTimeMs(elapsed);
            analysis.setStatus("COMPLETED");
            analysisRepo.updateAnalysisStatus(analysisId, "COMPLETED", null);
            analysis.setSimilarityResults(allResults);

            notifyProgress(progressCallback, "Complete", 100,
                    "Batch analysis complete — " + pairsDone + " pairs compared.");
            logger.info("BATCH analysis {} completed: {} pairs in {}ms", analysisId, pairsDone, elapsed);
            return analysis;

        } catch (Exception e) {
            logger.error("Batch analysis {} failed", analysisId, e);
            analysisRepo.updateAnalysisStatus(analysisId, "FAILED", e.getMessage());
            analysis.setStatus("FAILED");
            analysis.setErrorMessage(e.getMessage());
            throw new SQLException("Batch analysis failed: " + e.getMessage(), e);
        }
    }

    // -------------------------------------------------------------------------
    // Library comparison
    // -------------------------------------------------------------------------

    /**
     * Compare a document against the document library.
     */
    public Analysis analyzeAgainstLibrary(Document newDoc, List<Document> library, int ngramSize,
                                          Consumer<ProgressUpdate> progressCallback) throws SQLException {
        cancelled = false;
        long startTime = System.currentTimeMillis();
        detector.setNgramSize(ngramSize);

        Analysis analysis = new Analysis();
        analysis.setAnalysisType("LIBRARY_COMPARE");
        analysis.setAnalysisDate(LocalDateTime.now());
        analysis.setDocumentCount(library.size() + 1);
        analysis.setStatus("RUNNING");
        int analysisId = analysisRepo.saveAnalysis(analysis);

        List<Integer> ids = new ArrayList<>();
        ids.add(newDoc.getId());
        for (Document d : library) ids.add(d.getId());
        analysisRepo.saveAnalysisDocuments(analysisId, ids);

        notifyProgress(progressCallback, "Initializing", 5,
                "Comparing against " + library.size() + " library documents...");

        List<SimilarityResult> results = new ArrayList<>();
        int done = 0;

        try {
            for (Document libDoc : library) {
                if (cancelled) return finishCancelled(analysis);

                int progress = 5 + (int) ((double) done / library.size() * 85);
                notifyProgress(progressCallback, "Comparing", progress,
                        "Comparing vs: " + libDoc.getName());

                SimilarityResult result = detector.detect(newDoc, libDoc, analysisId);
                int resultId = analysisRepo.saveSimilarityResult(result);
                List<MatchResult> matches = result.getMatchingPassages();
                if (matches != null && !matches.isEmpty()) {
                    analysisRepo.saveMatchingPassages(resultId, matches);
                }

                documentGraph.addEdge(newDoc.getId(), newDoc.getName(),
                                      libDoc.getId(), libDoc.getName(), result.getJaccardScore());
                rankingHeap.insert(new MaxHeap.HeapEntry(newDoc, libDoc, result.getJaccardScore()));
                results.add(result);
                done++;
            }

            long elapsed = System.currentTimeMillis() - startTime;
            analysis.setProcessingTimeMs(elapsed);
            analysis.setStatus("COMPLETED");
            analysisRepo.updateAnalysisStatus(analysisId, "COMPLETED", null);
            analysis.setSimilarityResults(results);

            notifyProgress(progressCallback, "Complete", 100, "Library comparison complete.");
            logger.info("LIBRARY_COMPARE analysis {} completed in {}ms", analysisId, elapsed);
            return analysis;

        } catch (Exception e) {
            logger.error("Library analysis {} failed", analysisId, e);
            analysisRepo.updateAnalysisStatus(analysisId, "FAILED", e.getMessage());
            analysis.setStatus("FAILED");
            analysis.setErrorMessage(e.getMessage());
            throw new SQLException("Library analysis failed: " + e.getMessage(), e);
        }
    }

    // -------------------------------------------------------------------------
    // Version comparison
    // -------------------------------------------------------------------------

    /**
     * Version comparison using LCS and Edit Distance.
     */
    public Analysis compareVersions(Document docA, Document docB,
                                    Consumer<ProgressUpdate> progressCallback) throws SQLException {
        cancelled = false;
        long startTime = System.currentTimeMillis();

        Analysis analysis = new Analysis();
        analysis.setAnalysisType("VERSION_COMPARE");
        analysis.setAnalysisDate(LocalDateTime.now());
        analysis.setDocumentCount(2);
        analysis.setStatus("RUNNING");
        int analysisId = analysisRepo.saveAnalysis(analysis);
        analysisRepo.saveAnalysisDocuments(analysisId, List.of(docA.getId(), docB.getId()));

        notifyProgress(progressCallback, "Initializing", 10, "Preparing version comparison...");

        try {
            notifyProgress(progressCallback, "LCS Analysis", 40, "Computing longest common subsequence...");
            SimilarityResult result = detector.detectVersions(docA, docB, analysisId);

            if (cancelled) return finishCancelled(analysis);

            notifyProgress(progressCallback, "Saving", 80, "Persisting results...");
            int resultId = analysisRepo.saveSimilarityResult(result);
            List<MatchResult> matches = result.getMatchingPassages();
            if (matches != null && !matches.isEmpty()) {
                analysisRepo.saveMatchingPassages(resultId, matches);
            }

            long elapsed = System.currentTimeMillis() - startTime;
            analysis.setProcessingTimeMs(elapsed);
            analysis.setStatus("COMPLETED");
            analysisRepo.updateAnalysisStatus(analysisId, "COMPLETED", null);
            analysis.setSimilarityResults(List.of(result));

            notifyProgress(progressCallback, "Complete", 100, "Version comparison finished.");
            logger.info("VERSION_COMPARE analysis {} completed in {}ms", analysisId, elapsed);
            return analysis;

        } catch (Exception e) {
            logger.error("Version comparison {} failed", analysisId, e);
            analysisRepo.updateAnalysisStatus(analysisId, "FAILED", e.getMessage());
            analysis.setStatus("FAILED");
            analysis.setErrorMessage(e.getMessage());
            throw new SQLException("Version comparison failed: " + e.getMessage(), e);
        }
    }

    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------

    public void cancel() {
        cancelled = true;
        logger.info("Analysis cancellation requested.");
    }

    // -------------------------------------------------------------------------
    // History & Stats
    // -------------------------------------------------------------------------

    public List<Analysis> getAnalysisHistory() throws SQLException {
        return analysisRepo.findAllAnalyses();
    }

    public boolean deleteAnalysis(int id) throws SQLException {
        return analysisRepo.deleteAnalysis(id);
    }

    public DocumentGraph getDocumentGraph() {
        return documentGraph;
    }

    public List<MaxHeap.HeapEntry> getTopSimilarDocuments(int k) {
        return rankingHeap.topK(k);
    }

    public DashboardStats getDashboardStats() throws SQLException {
        DashboardStats stats = new DashboardStats();
        stats.totalDocuments       = docRepo.count();
        stats.totalAnalyses        = analysisRepo.getAnalysisCount();
        stats.averageSimilarity    = analysisRepo.getAverageSimilarityScore();
        stats.totalMatchingPassages = analysisRepo.getTotalMatchingPassages();
        stats.recentAnalyses       = analysisRepo.getRecentAnalyses(5);
        return stats;
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    private void notifyProgress(Consumer<ProgressUpdate> cb, String stage, int pct, String detail) {
        if (cb != null) {
            try {
                cb.accept(new ProgressUpdate(stage, pct, detail));
            } catch (Exception e) {
                logger.debug("Progress callback threw an exception", e);
            }
        }
    }

    private Analysis finishCancelled(Analysis analysis) throws SQLException {
        analysis.setStatus("CANCELLED");
        analysis.setErrorMessage("Cancelled by user.");
        analysisRepo.updateAnalysisStatus(analysis.getId(), "CANCELLED", "Cancelled by user.");
        logger.info("Analysis {} cancelled by user.", analysis.getId());
        return analysis;
    }

    // -------------------------------------------------------------------------
    // Nested types
    // -------------------------------------------------------------------------

    public static class ProgressUpdate {
        public final String stage;
        public final int percentComplete;
        public final String detail;

        public ProgressUpdate(String stage, int percentComplete, String detail) {
            this.stage           = stage;
            this.percentComplete = percentComplete;
            this.detail          = detail;
        }
    }

    public static class DashboardStats {
        public int totalDocuments;
        public int totalAnalyses;
        public double averageSimilarity;
        public int totalMatchingPassages;
        public List<Analysis> recentAnalyses;
    }
}
