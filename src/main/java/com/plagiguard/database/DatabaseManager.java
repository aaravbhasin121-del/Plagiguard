package com.plagiguard.database;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.sql.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Manages the SQLite database connection and schema for PlagiGuard.
 *
 * Database file: plagiguard.db in the application data directory.
 * Uses JDBC with prepared statements exclusively (no string concatenation in queries).
 * Handles connection pooling via a single connection (desktop app pattern).
 */
public class DatabaseManager {
    private static final Logger logger = LoggerFactory.getLogger(DatabaseManager.class);
    private static DatabaseManager instance;
    private Connection connection;
    private static final String DB_FILE = "plagiguard.db";

    private DatabaseManager() {}

    /**
     * Return singleton instance, creating it if necessary.
     */
    public static synchronized DatabaseManager getInstance() {
        if (instance == null) {
            instance = new DatabaseManager();
        }
        return instance;
    }

    /**
     * Initialize the database, create tables if they don't exist.
     */
    public void initialize() throws SQLException {
        try {
            // Resolve database path in user home / plagiguard data directory
            Path dataDir = Paths.get(System.getProperty("user.home"), ".plagiguard");
            if (!Files.exists(dataDir)) {
                Files.createDirectories(dataDir);
            }
            String dbUrl = "jdbc:sqlite:" + dataDir.resolve(DB_FILE).toAbsolutePath();
            logger.info("Opening SQLite database at: {}", dbUrl);

            connection = DriverManager.getConnection(dbUrl);
            // Enable WAL mode and foreign keys
            try (Statement stmt = connection.createStatement()) {
                stmt.execute("PRAGMA journal_mode=WAL");
                stmt.execute("PRAGMA foreign_keys=ON");
                stmt.execute("PRAGMA synchronous=NORMAL");
            }
            createTables();
            logger.info("Database initialized successfully.");
        } catch (Exception e) {
            throw new SQLException("Failed to initialize database: " + e.getMessage(), e);
        }
    }

    /**
     * Get the database connection.
     */
    public Connection getConnection() {
        return connection;
    }

    /**
     * Close the database connection.
     */
    public void close() {
        if (connection != null) {
            try {
                connection.close();
                logger.info("Database connection closed.");
            } catch (SQLException e) {
                logger.error("Error closing database connection", e);
            } finally {
                connection = null;
            }
        }
    }

    /**
     * Create all required tables with proper indexes.
     */
    private void createTables() throws SQLException {
        try (Statement stmt = connection.createStatement()) {

            // ---- DOCUMENTS ----
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS DOCUMENTS (
                    document_id         INTEGER PRIMARY KEY AUTOINCREMENT,
                    document_name       TEXT    NOT NULL,
                    file_type           TEXT,
                    file_size           INTEGER,
                    word_count          INTEGER,
                    original_content    TEXT,
                    normalized_content  TEXT,
                    document_fingerprint TEXT,
                    upload_date         TEXT,
                    file_path           TEXT
                )
            """);

            // ---- ANALYSES ----
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS ANALYSES (
                    analysis_id         INTEGER PRIMARY KEY AUTOINCREMENT,
                    analysis_type       TEXT,
                    analysis_date       TEXT,
                    document_count      INTEGER,
                    processing_time_ms  INTEGER,
                    status              TEXT,
                    error_message       TEXT
                )
            """);

            // ---- ANALYSIS_DOCUMENTS (join table) ----
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS ANALYSIS_DOCUMENTS (
                    id              INTEGER PRIMARY KEY AUTOINCREMENT,
                    analysis_id     INTEGER NOT NULL,
                    document_id     INTEGER NOT NULL,
                    FOREIGN KEY (analysis_id) REFERENCES ANALYSES(analysis_id) ON DELETE CASCADE,
                    FOREIGN KEY (document_id) REFERENCES DOCUMENTS(document_id) ON DELETE CASCADE
                )
            """);

            // ---- SIMILARITY_RESULTS ----
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS SIMILARITY_RESULTS (
                    result_id               INTEGER PRIMARY KEY AUTOINCREMENT,
                    analysis_id             INTEGER NOT NULL,
                    document_a_id           INTEGER NOT NULL,
                    document_b_id           INTEGER NOT NULL,
                    jaccard_score           REAL,
                    directional_score       REAL,
                    matching_count          INTEGER,
                    matching_passage_count  INTEGER,
                    unique_ngrams_a         INTEGER,
                    unique_ngrams_b         INTEGER,
                    processing_time_ms      INTEGER,
                    FOREIGN KEY (analysis_id)   REFERENCES ANALYSES(analysis_id)   ON DELETE CASCADE,
                    FOREIGN KEY (document_a_id) REFERENCES DOCUMENTS(document_id)  ON DELETE CASCADE,
                    FOREIGN KEY (document_b_id) REFERENCES DOCUMENTS(document_id)  ON DELETE CASCADE
                )
            """);

            // ---- MATCHING_PASSAGES ----
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS MATCHING_PASSAGES (
                    match_id                INTEGER PRIMARY KEY AUTOINCREMENT,
                    result_id               INTEGER NOT NULL,
                    source_document_id      INTEGER,
                    target_document_id      INTEGER,
                    source_start_position   INTEGER,
                    source_end_position     INTEGER,
                    target_start_position   INTEGER,
                    target_end_position     INTEGER,
                    matching_text           TEXT,
                    match_length            INTEGER,
                    match_type              TEXT,
                    FOREIGN KEY (result_id) REFERENCES SIMILARITY_RESULTS(result_id) ON DELETE CASCADE
                )
            """);

            // ---- USER_SETTINGS ----
            stmt.execute("""
                CREATE TABLE IF NOT EXISTS USER_SETTINGS (
                    key   TEXT PRIMARY KEY,
                    value TEXT
                )
            """);

            // ---- INDEXES ----
            stmt.execute("""
                CREATE INDEX IF NOT EXISTS idx_doc_fingerprint
                    ON DOCUMENTS(document_fingerprint)
            """);
            stmt.execute("""
                CREATE INDEX IF NOT EXISTS idx_analysis_date
                    ON ANALYSES(analysis_date)
            """);
            stmt.execute("""
                CREATE INDEX IF NOT EXISTS idx_result_doc_a
                    ON SIMILARITY_RESULTS(document_a_id)
            """);
            stmt.execute("""
                CREATE INDEX IF NOT EXISTS idx_result_doc_b
                    ON SIMILARITY_RESULTS(document_b_id)
            """);

            logger.info("All tables and indexes created/verified.");
        }
    }

    /**
     * Execute a callback inside a transaction. Rolls back on failure.
     */
    public void executeTransaction(TransactionCallback callback) throws SQLException {
        boolean autoCommit = connection.getAutoCommit();
        connection.setAutoCommit(false);
        try {
            callback.execute(connection);
            connection.commit();
        } catch (SQLException e) {
            logger.error("Transaction failed, rolling back", e);
            try {
                connection.rollback();
            } catch (SQLException rb) {
                logger.error("Rollback failed", rb);
            }
            throw e;
        } finally {
            try {
                connection.setAutoCommit(autoCommit);
            } catch (SQLException ignored) {}
        }
    }

    @FunctionalInterface
    public interface TransactionCallback {
        void execute(Connection conn) throws SQLException;
    }

    /**
     * Check if database is initialized and accessible.
     */
    public boolean isHealthy() {
        try {
            if (connection == null || connection.isClosed()) return false;
            try (Statement stmt = connection.createStatement();
                 ResultSet rs = stmt.executeQuery("SELECT 1")) {
                return rs.next();
            }
        } catch (SQLException e) {
            logger.warn("Database health check failed", e);
            return false;
        }
    }

    /**
     * Get database file size in bytes.
     */
    public long getDatabaseSize() {
        try {
            Path dbPath = Paths.get(System.getProperty("user.home"), ".plagiguard", DB_FILE);
            File dbFile = dbPath.toFile();
            return dbFile.exists() ? dbFile.length() : 0L;
        } catch (Exception e) {
            logger.warn("Could not determine database file size", e);
            return -1L;
        }
    }

    /**
     * Clear all data from all tables (used in testing).
     */
    public void clearAllData() throws SQLException {
        executeTransaction(conn -> {
            try (Statement stmt = conn.createStatement()) {
                stmt.execute("DELETE FROM MATCHING_PASSAGES");
                stmt.execute("DELETE FROM SIMILARITY_RESULTS");
                stmt.execute("DELETE FROM ANALYSIS_DOCUMENTS");
                stmt.execute("DELETE FROM ANALYSES");
                stmt.execute("DELETE FROM DOCUMENTS");
                stmt.execute("DELETE FROM USER_SETTINGS");
            }
        });
        logger.info("All data cleared from database.");
    }
}
