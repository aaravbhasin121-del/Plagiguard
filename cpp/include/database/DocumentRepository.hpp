#pragma once

#include "DatabaseManager.hpp"
#include "model/Document.hpp"
#include <vector>
#include <optional>
#include <string>

namespace plagiguard::database {

/**
 * @brief DocumentRepository - Data access object for academic documents.
 */
class DocumentRepository {
private:
    DatabaseManager& dbManager;

public:
    DocumentRepository() : dbManager(DatabaseManager::getInstance()) {}

    int save(model::Document& doc) {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = R"(
            INSERT INTO DOCUMENTS (document_name, file_type, file_size, word_count,
                                   original_content, normalized_content, document_fingerprint,
                                   file_path)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?);
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare insert document statement");
        }

        sqlite3_bind_text(stmt, 1, doc.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, doc.fileType.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(doc.fileSize));
        sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(doc.wordCount));
        sqlite3_bind_text(stmt, 5, doc.originalContent.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, doc.normalizedContent.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, doc.fingerprint.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8, doc.filePath.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("Failed to execute insert document");
        }
        sqlite3_finalize(stmt);

        doc.id = static_cast<int>(sqlite3_last_insert_rowid(db));
        return doc.id;
    }

    std::optional<model::Document> findByFingerprint(const std::string& fingerprint) {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = R"(
            SELECT document_id, document_name, file_type, file_size, word_count,
                   original_content, normalized_content, document_fingerprint, upload_date, file_path
            FROM DOCUMENTS WHERE document_fingerprint = ? LIMIT 1;
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return std::nullopt;
        }

        sqlite3_bind_text(stmt, 1, fingerprint.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Document doc;
            doc.id = sqlite3_column_int(stmt, 0);
            doc.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            doc.fileType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            doc.fileSize = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
            doc.wordCount = static_cast<size_t>(sqlite3_column_int64(stmt, 4));
            doc.originalContent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            doc.normalizedContent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            doc.fingerprint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            const char* uDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            if (uDate) doc.uploadDate = uDate;
            const char* fPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            if (fPath) doc.filePath = fPath;

            sqlite3_finalize(stmt);
            return doc;
        }

        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    std::optional<model::Document> findById(int id) {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = R"(
            SELECT document_id, document_name, file_type, file_size, word_count,
                   original_content, normalized_content, document_fingerprint, upload_date, file_path
            FROM DOCUMENTS WHERE document_id = ? LIMIT 1;
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return std::nullopt;
        }

        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Document doc;
            doc.id = sqlite3_column_int(stmt, 0);
            doc.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            doc.fileType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            doc.fileSize = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
            doc.wordCount = static_cast<size_t>(sqlite3_column_int64(stmt, 4));
            doc.originalContent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            doc.normalizedContent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            doc.fingerprint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            const char* uDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            if (uDate) doc.uploadDate = uDate;
            const char* fPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            if (fPath) doc.filePath = fPath;

            sqlite3_finalize(stmt);
            return doc;
        }

        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    std::vector<model::Document> findAll() {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        std::vector<model::Document> docs;
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = R"(
            SELECT document_id, document_name, file_type, file_size, word_count,
                   original_content, normalized_content, document_fingerprint, upload_date, file_path
            FROM DOCUMENTS ORDER BY upload_date DESC;
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return docs;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Document doc;
            doc.id = sqlite3_column_int(stmt, 0);
            doc.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            doc.fileType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            doc.fileSize = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
            doc.wordCount = static_cast<size_t>(sqlite3_column_int64(stmt, 4));
            doc.originalContent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            doc.normalizedContent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            doc.fingerprint = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            const char* uDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            if (uDate) doc.uploadDate = uDate;
            const char* fPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            if (fPath) doc.filePath = fPath;

            docs.push_back(doc);
        }

        sqlite3_finalize(stmt);
        return docs;
    }

    bool deleteById(int id) {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = "DELETE FROM DOCUMENTS WHERE document_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_int(stmt, 1, id);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    size_t count() {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = "SELECT COUNT(*) FROM DOCUMENTS;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
        size_t c = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            c = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
        }
        sqlite3_finalize(stmt);
        return c;
    }
};

} // namespace plagiguard::database

