#pragma once

#include <sqlite3.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <mutex>

namespace plagiguard::database {

/**
 * @brief DatabaseManager - Manages SQLite relational persistence for PlagiGuard.
 *
 * Implements:
 * - Thread-safe singleton access
 * - Robust relational schema with foreign keys and indexes
 * - ACID transactions for multi-row writes
 * - Prepared statements to prevent SQL injection vulnerabilities
 */
class DatabaseManager {
private:
    sqlite3* db;
    std::string dbPath;
    std::mutex dbMutex;

    DatabaseManager() : db(nullptr), dbPath("plagiguard.db") {}

    void executeRaw(const std::string& sql) {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::string err = errMsg ? errMsg : "Unknown SQL error";
            sqlite3_free(errMsg);
            throw std::runtime_error("SQLite error: " + err + " executing: " + sql);
        }
    }

public:
    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    ~DatabaseManager() {
        close();
    }

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void initialize(const std::string& path = "plagiguard.db") {
        std::lock_guard<std::mutex> lock(dbMutex);
        dbPath = path;

        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::string err = sqlite3_errmsg(db);
            sqlite3_close(db);
            db = nullptr;
            throw std::runtime_error("Failed to open SQLite database: " + err);
        }

        // Enable Foreign Keys and WAL journal mode for high performance
        executeRaw("PRAGMA foreign_keys = ON;");
        executeRaw("PRAGMA journal_mode = WAL;");

        createSchema();
    }

    void createSchema() {
        const char* schema = R"(
            CREATE TABLE IF NOT EXISTS DOCUMENTS (
                document_id INTEGER PRIMARY KEY AUTOINCREMENT,
                document_name TEXT NOT NULL,
                file_type TEXT NOT NULL,
                file_size INTEGER NOT NULL,
                word_count INTEGER NOT NULL,
                original_content TEXT NOT NULL,
                normalized_content TEXT NOT NULL,
                document_fingerprint TEXT NOT NULL,
                upload_date DATETIME DEFAULT CURRENT_TIMESTAMP,
                file_path TEXT
            );

            CREATE INDEX IF NOT EXISTS idx_doc_fingerprint ON DOCUMENTS(document_fingerprint);

            CREATE TABLE IF NOT EXISTS ANALYSES (
                analysis_id INTEGER PRIMARY KEY AUTOINCREMENT,
                analysis_type TEXT NOT NULL,
                analysis_date DATETIME DEFAULT CURRENT_TIMESTAMP,
                document_count INTEGER NOT NULL,
                processing_time_ms INTEGER NOT NULL,
                status TEXT NOT NULL,
                error_message TEXT
            );

            CREATE TABLE IF NOT EXISTS ANALYSIS_DOCUMENTS (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                analysis_id INTEGER NOT NULL,
                document_id INTEGER NOT NULL,
                FOREIGN KEY (analysis_id) REFERENCES ANALYSES(analysis_id) ON DELETE CASCADE,
                FOREIGN KEY (document_id) REFERENCES DOCUMENTS(document_id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS SIMILARITY_RESULTS (
                result_id INTEGER PRIMARY KEY AUTOINCREMENT,
                analysis_id INTEGER NOT NULL,
                document_a_id INTEGER NOT NULL,
                document_b_id INTEGER NOT NULL,
                document_a_name TEXT NOT NULL,
                document_b_name TEXT NOT NULL,
                jaccard_score REAL NOT NULL,
                directional_score REAL NOT NULL,
                matching_count INTEGER NOT NULL,
                matching_passage_count INTEGER NOT NULL,
                unique_ngrams_a INTEGER NOT NULL,
                unique_ngrams_b INTEGER NOT NULL,
                processing_time_ms INTEGER NOT NULL,
                FOREIGN KEY (analysis_id) REFERENCES ANALYSES(analysis_id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS MATCHING_PASSAGES (
                match_id INTEGER PRIMARY KEY AUTOINCREMENT,
                result_id INTEGER NOT NULL,
                source_document_id INTEGER NOT NULL,
                target_document_id INTEGER NOT NULL,
                source_start_position INTEGER NOT NULL,
                source_end_position INTEGER NOT NULL,
                target_start_position INTEGER NOT NULL,
                target_end_position INTEGER NOT NULL,
                matching_text TEXT NOT NULL,
                match_length INTEGER NOT NULL,
                match_type TEXT NOT NULL,
                FOREIGN KEY (result_id) REFERENCES SIMILARITY_RESULTS(result_id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS USER_SETTINGS (
                setting_key TEXT PRIMARY KEY,
                setting_value TEXT NOT NULL
            );
        )";

        executeRaw(schema);
    }

    void executeTransaction(const std::function<void(sqlite3*)>& callback) {
        std::lock_guard<std::mutex> lock(dbMutex);
        executeRaw("BEGIN TRANSACTION;");
        try {
            callback(db);
            executeRaw("COMMIT;");
        } catch (...) {
            executeRaw("ROLLBACK;");
            throw;
        }
    }

    sqlite3* getDbHandle() {
        return db;
    }

    std::mutex& getMutex() {
        return dbMutex;
    }

    void close() {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (db != nullptr) {
            sqlite3_close(db);
            db = nullptr;
        }
    }
};

} // namespace plagiguard::database

