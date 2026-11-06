package com.plagiguard.database;

import com.plagiguard.model.Analysis;
import com.plagiguard.model.MatchResult;
import com.plagiguard.model.SimilarityResult;

import java.sql.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;

/**
 * Repository for Analysis, SimilarityResult, and MatchingPassage persistence.
 */
public class AnalysisRepository {

    private static final DateTimeFormatter DTF = DateTimeFormatter.ISO_LOCAL_DATE_TIME;
    private final DatabaseManager dbManager;

    public AnalysisRepository(DatabaseManager dbManager) {
        this.dbManager = dbManager;
    }

    // -------------------------------------------------------------------------
    // Analysis CRUD
    // -------------------------------------------------------------------------

    public int saveAnalysis(Analysis analysis) throws SQLException {
        String sql = """
                INSERT INTO ANALYSES
                    (analysis_type, analysis_date, document_count, processing_time_ms, status, error_message)
                VALUES (?, ?, ?, ?, ?, ?)
                """;
        try (PreparedStatement ps = dbManager.getConnection()
                .prepareStatement(sql, Statement.RETURN_GENERATED_KEYS)) {
            ps.setString(1, analysis.getAnalysisType());
            ps.setString(2, analysis.getAnalysisDate() != null
                    ? analysis.getAnalysisDate().format(DTF) : LocalDateTime.now().format(DTF));
            ps.setInt(3, analysis.getDocumentCount());
            ps.setLong(4, analysis.getProcessingTimeMs());
            ps.setString(5, analysis.getStatus());
            ps.setString(6, analysis.getErrorMessage());
            ps.executeUpdate();
            try (ResultSet keys = ps.getGeneratedKeys()) {
                if (keys.next()) {
                    int id = keys.getInt(1);
                    analysis.setId(id);
                    return id;
                }
            }
        }
        throw new SQLException("Failed to save Analysis — no generated key.");
    }

    public Optional<Analysis> findAnalysisById(int id) throws SQLException {
        String sql = "SELECT * FROM ANALYSES WHERE analysis_id = ?";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, id);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    return Optional.of(mapAnalysis(rs));
                }
            }
        }
        return Optional.empty();
    }

    /** Return all analyses, most recent first. */
    public List<Analysis> findAllAnalyses() throws SQLException {
        String sql = "SELECT * FROM ANALYSES ORDER BY analysis_date DESC";
        List<Analysis> list = new ArrayList<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql);
             ResultSet rs = ps.executeQuery()) {
            while (rs.next()) {
                list.add(mapAnalysis(rs));
            }
        }
        return list;
    }

    public boolean deleteAnalysis(int id) throws SQLException {
        String sql = "DELETE FROM ANALYSES WHERE analysis_id = ?";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, id);
            return ps.executeUpdate() > 0;
        }
    }

    public void updateAnalysisStatus(int id, String status, String errorMessage) throws SQLException {
        String sql = "UPDATE ANALYSES SET status = ?, error_message = ? WHERE analysis_id = ?";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setString(1, status);
            ps.setString(2, errorMessage);
            ps.setInt(3, id);
            ps.executeUpdate();
        }
    }

    public int getAnalysisCount() throws SQLException {
        String sql = "SELECT COUNT(*) FROM ANALYSES";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql);
             ResultSet rs = ps.executeQuery()) {
            if (rs.next()) return rs.getInt(1);
        }
        return 0;
    }

    // -------------------------------------------------------------------------
    // Analysis Documents (join table)
    // -------------------------------------------------------------------------

    public void saveAnalysisDocuments(int analysisId, List<Integer> documentIds) throws SQLException {
        String sql = "INSERT INTO ANALYSIS_DOCUMENTS (analysis_id, document_id) VALUES (?, ?)";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            for (int docId : documentIds) {
                ps.setInt(1, analysisId);
                ps.setInt(2, docId);
                ps.addBatch();
            }
            ps.executeBatch();
        }
    }

    public List<Integer> getDocumentIdsForAnalysis(int analysisId) throws SQLException {
        String sql = "SELECT document_id FROM ANALYSIS_DOCUMENTS WHERE analysis_id = ?";
        List<Integer> ids = new ArrayList<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, analysisId);
            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) {
                    ids.add(rs.getInt("document_id"));
                }
            }
        }
        return ids;
    }

    // -------------------------------------------------------------------------
    // Similarity Results
    // -------------------------------------------------------------------------

    public int saveSimilarityResult(SimilarityResult result) throws SQLException {
        String sql = """
                INSERT INTO SIMILARITY_RESULTS
                    (analysis_id, document_a_id, document_b_id, jaccard_score, directional_score,
                     matching_count, matching_passage_count, unique_ngrams_a, unique_ngrams_b, processing_time_ms)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """;
        try (PreparedStatement ps = dbManager.getConnection()
                .prepareStatement(sql, Statement.RETURN_GENERATED_KEYS)) {
            ps.setInt(1, result.getAnalysisId());
            ps.setInt(2, result.getDocumentAId());
            ps.setInt(3, result.getDocumentBId());
            ps.setDouble(4, result.getJaccardScore());
            ps.setDouble(5, result.getDirectionalScore());
            ps.setInt(6, result.getMatchingCount());
            ps.setInt(7, result.getMatchingPassageCount());
            ps.setInt(8, result.getUniqueNgramsA());
            ps.setInt(9, result.getUniqueNgramsB());
            ps.setLong(10, result.getProcessingTimeMs());
            ps.executeUpdate();
            try (ResultSet keys = ps.getGeneratedKeys()) {
                if (keys.next()) {
                    int id = keys.getInt(1);
                    result.setId(id);
                    return id;
                }
            }
        }
        throw new SQLException("Failed to save SimilarityResult — no generated key.");
    }

    public List<SimilarityResult> findResultsByAnalysisId(int analysisId) throws SQLException {
        String sql = "SELECT * FROM SIMILARITY_RESULTS WHERE analysis_id = ?";
        List<SimilarityResult> list = new ArrayList<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, analysisId);
            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) {
                    list.add(mapSimilarityResult(rs));
                }
            }
        }
        return list;
    }

    public Optional<SimilarityResult> findResultById(int id) throws SQLException {
        String sql = "SELECT * FROM SIMILARITY_RESULTS WHERE result_id = ?";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, id);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    return Optional.of(mapSimilarityResult(rs));
                }
            }
        }
        return Optional.empty();
    }

    // -------------------------------------------------------------------------
    // Matching Passages
    // -------------------------------------------------------------------------

    public void saveMatchingPassages(int resultId, List<MatchResult> matches) throws SQLException {
        if (matches == null || matches.isEmpty()) return;
        String sql = """
                INSERT INTO MATCHING_PASSAGES
                    (result_id, source_document_id, target_document_id,
                     source_start_position, source_end_position,
                     target_start_position, target_end_position,
                     matching_text, match_length, match_type)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """;
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            for (MatchResult m : matches) {
                ps.setInt(1, resultId);
                ps.setInt(2, m.getSourceDocumentId());
                ps.setInt(3, m.getTargetDocumentId());
                ps.setInt(4, m.getSourceStartPosition());
                ps.setInt(5, m.getSourceEndPosition());
                ps.setInt(6, m.getTargetStartPosition());
                ps.setInt(7, m.getTargetEndPosition());
                ps.setString(8, m.getMatchingText());
                ps.setInt(9, m.getMatchLength());
                ps.setString(10, m.getMatchType());
                ps.addBatch();
            }
            ps.executeBatch();
        }
    }

    public List<MatchResult> findMatchesByResultId(int resultId) throws SQLException {
        String sql = "SELECT * FROM MATCHING_PASSAGES WHERE result_id = ?";
        List<MatchResult> list = new ArrayList<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, resultId);
            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) {
                    list.add(mapMatchResult(rs));
                }
            }
        }
        return list;
    }

    // -------------------------------------------------------------------------
    // Dashboard stats
    // -------------------------------------------------------------------------

    public double getAverageSimilarityScore() throws SQLException {
        String sql = "SELECT AVG(jaccard_score) FROM SIMILARITY_RESULTS";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql);
             ResultSet rs = ps.executeQuery()) {
            if (rs.next()) {
                double val = rs.getDouble(1);
                return rs.wasNull() ? 0.0 : val;
            }
        }
        return 0.0;
    }

    public int getTotalMatchingPassages() throws SQLException {
        String sql = "SELECT COUNT(*) FROM MATCHING_PASSAGES";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql);
             ResultSet rs = ps.executeQuery()) {
            if (rs.next()) return rs.getInt(1);
        }
        return 0;
    }

    public List<Analysis> getRecentAnalyses(int limit) throws SQLException {
        String sql = "SELECT * FROM ANALYSES ORDER BY analysis_date DESC LIMIT ?";
        List<Analysis> list = new ArrayList<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, limit);
            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) {
                    list.add(mapAnalysis(rs));
                }
            }
        }
        return list;
    }

    // -------------------------------------------------------------------------
    // Settings
    // -------------------------------------------------------------------------

    public void saveSetting(String key, String value) throws SQLException {
        String sql = "INSERT INTO USER_SETTINGS (key, value) VALUES (?, ?) ON CONFLICT(key) DO UPDATE SET value = excluded.value";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setString(1, key);
            ps.setString(2, value);
            ps.executeUpdate();
        }
    }

    public Optional<String> getSetting(String key) throws SQLException {
        String sql = "SELECT value FROM USER_SETTINGS WHERE key = ?";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setString(1, key);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    return Optional.ofNullable(rs.getString("value"));
                }
            }
        }
        return Optional.empty();
    }

    public Map<String, String> getAllSettings() throws SQLException {
        String sql = "SELECT key, value FROM USER_SETTINGS";
        Map<String, String> settings = new LinkedHashMap<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql);
             ResultSet rs = ps.executeQuery()) {
            while (rs.next()) {
                settings.put(rs.getString("key"), rs.getString("value"));
            }
        }
        return settings;
    }

    // -------------------------------------------------------------------------
    // Mapping helpers
    // -------------------------------------------------------------------------

    private Analysis mapAnalysis(ResultSet rs) throws SQLException {
        Analysis a = new Analysis();
        a.setId(rs.getInt("analysis_id"));
        a.setAnalysisType(rs.getString("analysis_type"));
        a.setDocumentCount(rs.getInt("document_count"));
        a.setProcessingTimeMs(rs.getLong("processing_time_ms"));
        a.setStatus(rs.getString("status"));
        a.setErrorMessage(rs.getString("error_message"));
        String dateStr = rs.getString("analysis_date");
        if (dateStr != null && !dateStr.isBlank()) {
            try {
                a.setAnalysisDate(LocalDateTime.parse(dateStr, DTF));
            } catch (Exception e) {
                a.setAnalysisDate(LocalDateTime.now());
            }
        }
        return a;
    }

    private SimilarityResult mapSimilarityResult(ResultSet rs) throws SQLException {
        SimilarityResult sr = new SimilarityResult();
        sr.setId(rs.getInt("result_id"));
        sr.setAnalysisId(rs.getInt("analysis_id"));
        sr.setDocumentAId(rs.getInt("document_a_id"));
        sr.setDocumentBId(rs.getInt("document_b_id"));
        sr.setJaccardScore(rs.getDouble("jaccard_score"));
        sr.setDirectionalScore(rs.getDouble("directional_score"));
        sr.setMatchingCount(rs.getInt("matching_count"));
        sr.setMatchingPassageCount(rs.getInt("matching_passage_count"));
        sr.setUniqueNgramsA(rs.getInt("unique_ngrams_a"));
        sr.setUniqueNgramsB(rs.getInt("unique_ngrams_b"));
        sr.setProcessingTimeMs(rs.getLong("processing_time_ms"));
        return sr;
    }

    private MatchResult mapMatchResult(ResultSet rs) throws SQLException {
        MatchResult m = new MatchResult();
        m.setId(rs.getInt("match_id"));
        m.setResultId(rs.getInt("result_id"));
        m.setSourceDocumentId(rs.getInt("source_document_id"));
        m.setTargetDocumentId(rs.getInt("target_document_id"));
        m.setSourceStartPosition(rs.getInt("source_start_position"));
        m.setSourceEndPosition(rs.getInt("source_end_position"));
        m.setTargetStartPosition(rs.getInt("target_start_position"));
        m.setTargetEndPosition(rs.getInt("target_end_position"));
        m.setMatchingText(rs.getString("matching_text"));
        m.setMatchLength(rs.getInt("match_length"));
        m.setMatchType(rs.getString("match_type"));
        return m;
    }
}
