package com.plagiguard.database;

import com.plagiguard.model.Document;

import java.sql.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;

/**
 * Repository for Document CRUD operations.
 * All queries use prepared statements.
 */
public class DocumentRepository {

    private static final DateTimeFormatter DTF = DateTimeFormatter.ISO_LOCAL_DATE_TIME;
    private final DatabaseManager dbManager;

    public DocumentRepository(DatabaseManager dbManager) {
        this.dbManager = dbManager;
    }

    /**
     * Save a new document, return generated ID.
     */
    public int save(Document document) throws SQLException {
        String sql = """
                INSERT INTO DOCUMENTS
                    (document_name, file_type, file_size, word_count,
                     original_content, normalized_content, document_fingerprint,
                     upload_date, file_path)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                """;
        try (PreparedStatement ps = dbManager.getConnection()
                .prepareStatement(sql, Statement.RETURN_GENERATED_KEYS)) {
            ps.setString(1, document.getName());
            ps.setString(2, document.getFileType());
            ps.setLong(3, document.getFileSize());
            ps.setInt(4, document.getWordCount());
            ps.setString(5, document.getOriginalContent());
            ps.setString(6, document.getNormalizedContent());
            ps.setString(7, document.getFingerprint());
            ps.setString(8, document.getUploadDate() != null
                    ? document.getUploadDate().format(DTF) : null);
            ps.setString(9, document.getFilePath());
            ps.executeUpdate();
            try (ResultSet keys = ps.getGeneratedKeys()) {
                if (keys.next()) {
                    int generatedId = keys.getInt(1);
                    document.setId(generatedId);
                    return generatedId;
                }
            }
        }
        throw new SQLException("Document save failed — no generated key returned.");
    }

    /**
     * Find document by ID.
     */
    public Optional<Document> findById(int id) throws SQLException {
        String sql = "SELECT * FROM DOCUMENTS WHERE document_id = ?";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, id);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    return Optional.of(mapResultSet(rs));
                }
            }
        }
        return Optional.empty();
    }

    /**
     * Find document by fingerprint (duplicate detection).
     */
    public Optional<Document> findByFingerprint(String fingerprint) throws SQLException {
        String sql = "SELECT * FROM DOCUMENTS WHERE document_fingerprint = ? LIMIT 1";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setString(1, fingerprint);
            try (ResultSet rs = ps.executeQuery()) {
                if (rs.next()) {
                    return Optional.of(mapResultSet(rs));
                }
            }
        }
        return Optional.empty();
    }

    /**
     * Get all documents ordered by upload date descending.
     */
    public List<Document> findAll() throws SQLException {
        String sql = "SELECT * FROM DOCUMENTS ORDER BY upload_date DESC";
        List<Document> docs = new ArrayList<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql);
             ResultSet rs = ps.executeQuery()) {
            while (rs.next()) {
                docs.add(mapResultSet(rs));
            }
        }
        return docs;
    }

    /**
     * Find documents by name (partial match, case-insensitive).
     */
    public List<Document> findByName(String name) throws SQLException {
        String sql = "SELECT * FROM DOCUMENTS WHERE LOWER(document_name) LIKE ? ORDER BY upload_date DESC";
        List<Document> docs = new ArrayList<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setString(1, "%" + name.toLowerCase() + "%");
            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) {
                    docs.add(mapResultSet(rs));
                }
            }
        }
        return docs;
    }

    /**
     * Delete a document and its associated data (cascades via FK).
     */
    public boolean delete(int id) throws SQLException {
        String sql = "DELETE FROM DOCUMENTS WHERE document_id = ?";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setInt(1, id);
            int affected = ps.executeUpdate();
            return affected > 0;
        }
    }

    /**
     * Get total document count.
     */
    public int count() throws SQLException {
        String sql = "SELECT COUNT(*) FROM DOCUMENTS";
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql);
             ResultSet rs = ps.executeQuery()) {
            if (rs.next()) {
                return rs.getInt(1);
            }
        }
        return 0;
    }

    /**
     * Update document metadata.
     */
    public void update(Document document) throws SQLException {
        String sql = """
                UPDATE DOCUMENTS SET
                    document_name        = ?,
                    file_type            = ?,
                    file_size            = ?,
                    word_count           = ?,
                    original_content     = ?,
                    normalized_content   = ?,
                    document_fingerprint = ?,
                    upload_date          = ?,
                    file_path            = ?
                WHERE document_id = ?
                """;
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setString(1, document.getName());
            ps.setString(2, document.getFileType());
            ps.setLong(3, document.getFileSize());
            ps.setInt(4, document.getWordCount());
            ps.setString(5, document.getOriginalContent());
            ps.setString(6, document.getNormalizedContent());
            ps.setString(7, document.getFingerprint());
            ps.setString(8, document.getUploadDate() != null
                    ? document.getUploadDate().format(DTF) : null);
            ps.setString(9, document.getFilePath());
            ps.setInt(10, document.getId());
            ps.executeUpdate();
        }
    }

    /**
     * Get documents uploaded in a date range (inclusive).
     */
    public List<Document> findByDateRange(LocalDateTime from, LocalDateTime to) throws SQLException {
        String sql = """
                SELECT * FROM DOCUMENTS
                WHERE upload_date >= ? AND upload_date <= ?
                ORDER BY upload_date DESC
                """;
        List<Document> docs = new ArrayList<>();
        try (PreparedStatement ps = dbManager.getConnection().prepareStatement(sql)) {
            ps.setString(1, from.format(DTF));
            ps.setString(2, to.format(DTF));
            try (ResultSet rs = ps.executeQuery()) {
                while (rs.next()) {
                    docs.add(mapResultSet(rs));
                }
            }
        }
        return docs;
    }

    // Helper to map ResultSet to Document
    private Document mapResultSet(ResultSet rs) throws SQLException {
        Document doc = new Document();
        doc.setId(rs.getInt("document_id"));
        doc.setName(rs.getString("document_name"));
        doc.setFileType(rs.getString("file_type"));
        doc.setFileSize(rs.getLong("file_size"));
        doc.setWordCount(rs.getInt("word_count"));
        doc.setOriginalContent(rs.getString("original_content"));
        doc.setNormalizedContent(rs.getString("normalized_content"));
        doc.setFingerprint(rs.getString("document_fingerprint"));
        doc.setFilePath(rs.getString("file_path"));

        String dateStr = rs.getString("upload_date");
        if (dateStr != null && !dateStr.isBlank()) {
            try {
                doc.setUploadDate(LocalDateTime.parse(dateStr, DTF));
            } catch (Exception e) {
                doc.setUploadDate(LocalDateTime.now());
            }
        }

        // Tokens are not persisted — must be regenerated from normalizedContent if needed
        String normalized = doc.getNormalizedContent();
        if (normalized != null && !normalized.isBlank()) {
            doc.setTokens(normalized.split("\\s+"));
        } else {
            doc.setTokens(new String[0]);
        }
        return doc;
    }
}
