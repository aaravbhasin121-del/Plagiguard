#pragma once

#include "DatabaseManager.hpp"
#include "model/Analysis.hpp"
#include <vector>
#include <string>

namespace plagiguard::database {

/**
 * @brief AnalysisRepository - Persists analysis runs, similarities, and matched passages.
 */
class AnalysisRepository {
private:
    DatabaseManager& dbManager;

public:
    AnalysisRepository() : dbManager(DatabaseManager::getInstance()) {}

    int saveAnalysis(model::Analysis& analysis) {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = R"(
            INSERT INTO ANALYSES (analysis_type, document_count, processing_time_ms, status, error_message)
            VALUES (?, ?, ?, ?, ?);
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare analysis insert");
        }

        sqlite3_bind_text(stmt, 1, analysis.typeString.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(analysis.documentCount));
        sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(analysis.processingTimeMs));
        sqlite3_bind_text(stmt, 4, analysis.status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, analysis.errorMessage.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("Failed to execute analysis insert");
        }
        sqlite3_finalize(stmt);

        analysis.id = static_cast<int>(sqlite3_last_insert_rowid(db));

        // Save Analysis-Document associations
        for (const auto& doc : analysis.documents) {
            const char* linkSql = "INSERT INTO ANALYSIS_DOCUMENTS (analysis_id, document_id) VALUES (?, ?);";
            sqlite3_stmt* linkStmt = nullptr;
            if (sqlite3_prepare_v2(db, linkSql, -1, &linkStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(linkStmt, 1, analysis.id);
                sqlite3_bind_int(linkStmt, 2, doc.id);
                sqlite3_step(linkStmt);
                sqlite3_finalize(linkStmt);
            }
        }

        // Save Similarity Results and Matching Passages
        for (auto& sim : analysis.results) {
            sim.analysisId = analysis.id;
            const char* simSql = R"(
                INSERT INTO SIMILARITY_RESULTS (analysis_id, document_a_id, document_b_id,
                                                document_a_name, document_b_name, jaccard_score,
                                                directional_score, matching_count, matching_passage_count,
                                                unique_ngrams_a, unique_ngrams_b, processing_time_ms)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
            )";

            sqlite3_stmt* simStmt = nullptr;
            if (sqlite3_prepare_v2(db, simSql, -1, &simStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(simStmt, 1, analysis.id);
                sqlite3_bind_int(simStmt, 2, sim.docAId);
                sqlite3_bind_int(simStmt, 3, sim.docBId);
                sqlite3_bind_text(simStmt, 4, sim.docAName.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(simStmt, 5, sim.docBName.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_double(simStmt, 6, sim.jaccardScore);
                sqlite3_bind_double(simStmt, 7, sim.directionalScoreAtoB);
                sqlite3_bind_int64(simStmt, 8, static_cast<sqlite3_int64>(sim.matchingNgramCount));
                sqlite3_bind_int64(simStmt, 9, static_cast<sqlite3_int64>(sim.matchingPassageCount));
                sqlite3_bind_int64(simStmt, 10, static_cast<sqlite3_int64>(sim.uniqueNgramsA));
                sqlite3_bind_int64(simStmt, 11, static_cast<sqlite3_int64>(sim.uniqueNgramsB));
                sqlite3_bind_int64(simStmt, 12, static_cast<sqlite3_int64>(sim.processingTimeMs));

                if (sqlite3_step(simStmt) == SQLITE_DONE) {
                    sim.id = static_cast<int>(sqlite3_last_insert_rowid(db));

                    // Save passages
                    for (auto& match : sim.matchingPassages) {
                        match.resultId = sim.id;
                        const char* matchSql = R"(
                            INSERT INTO MATCHING_PASSAGES (result_id, source_document_id, target_document_id,
                                                           source_start_position, source_end_position,
                                                           target_start_position, target_end_position,
                                                           matching_text, match_length, match_type)
                            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
                        )";

                        sqlite3_stmt* mStmt = nullptr;
                        if (sqlite3_prepare_v2(db, matchSql, -1, &mStmt, nullptr) == SQLITE_OK) {
                            sqlite3_bind_int(mStmt, 1, sim.id);
                            sqlite3_bind_int(mStmt, 2, match.sourceDocId);
                            sqlite3_bind_int(mStmt, 3, match.targetDocId);
                            sqlite3_bind_int64(mStmt, 4, static_cast<sqlite3_int64>(match.sourceStartToken));
                            sqlite3_bind_int64(mStmt, 5, static_cast<sqlite3_int64>(match.sourceEndToken));
                            sqlite3_bind_int64(mStmt, 6, static_cast<sqlite3_int64>(match.targetStartToken));
                            sqlite3_bind_int64(mStmt, 7, static_cast<sqlite3_int64>(match.targetEndToken));
                            sqlite3_bind_text(mStmt, 8, match.matchingText.c_str(), -1, SQLITE_TRANSIENT);
                            sqlite3_bind_int64(mStmt, 9, static_cast<sqlite3_int64>(match.matchLength));
                            sqlite3_bind_text(mStmt, 10, "EXACT", -1, SQLITE_TRANSIENT);
                            sqlite3_step(mStmt);
                            sqlite3_finalize(mStmt);
                        }
                    }
                }
                sqlite3_finalize(simStmt);
            }
        }

        return analysis.id;
    }

    std::vector<model::Analysis> findAll() {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        std::vector<model::Analysis> list;
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = R"(
            SELECT analysis_id, analysis_type, analysis_date, document_count,
                   processing_time_ms, status, error_message
            FROM ANALYSES ORDER BY analysis_date DESC;
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            model::Analysis a;
            a.id = sqlite3_column_int(stmt, 0);
            const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (type) a.typeString = type;
            const char* date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            if (date) a.analysisDate = date;
            a.documentCount = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
            a.processingTimeMs = sqlite3_column_int64(stmt, 4);
            const char* status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            if (status) a.status = status;
            const char* err = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            if (err) a.errorMessage = err;

            // Fetch results for this analysis
            const char* rSql = R"(
                SELECT result_id, document_a_id, document_b_id, document_a_name, document_b_name,
                       jaccard_score, directional_score, matching_count, matching_passage_count,
                       unique_ngrams_a, unique_ngrams_b, processing_time_ms
                FROM SIMILARITY_RESULTS WHERE analysis_id = ?;
            )";
            sqlite3_stmt* rStmt = nullptr;
            if (sqlite3_prepare_v2(db, rSql, -1, &rStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(rStmt, 1, a.id);
                while (sqlite3_step(rStmt) == SQLITE_ROW) {
                    model::SimilarityResult res;
                    res.id = sqlite3_column_int(rStmt, 0);
                    res.analysisId = a.id;
                    res.docAId = sqlite3_column_int(rStmt, 1);
                    res.docBId = sqlite3_column_int(rStmt, 2);
                    const char* na = reinterpret_cast<const char*>(sqlite3_column_text(rStmt, 3));
                    if (na) res.docAName = na;
                    const char* nb = reinterpret_cast<const char*>(sqlite3_column_text(rStmt, 4));
                    if (nb) res.docBName = nb;
                    res.jaccardScore = sqlite3_column_double(rStmt, 5);
                    res.directionalScoreAtoB = sqlite3_column_double(rStmt, 6);
                    res.matchingNgramCount = static_cast<size_t>(sqlite3_column_int64(rStmt, 7));
                    res.matchingPassageCount = static_cast<size_t>(sqlite3_column_int64(rStmt, 8));
                    res.uniqueNgramsA = static_cast<size_t>(sqlite3_column_int64(rStmt, 9));
                    res.uniqueNgramsB = static_cast<size_t>(sqlite3_column_int64(rStmt, 10));
                    res.processingTimeMs = sqlite3_column_int64(rStmt, 11);
                    a.results.push_back(res);
                }
                sqlite3_finalize(rStmt);
            }

            list.push_back(a);
        }

        sqlite3_finalize(stmt);
        return list;
    }

    std::optional<model::Analysis> findById(int id) {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = R"(
            SELECT analysis_id, analysis_type, analysis_date, document_count,
                   processing_time_ms, status, error_message
            FROM ANALYSES WHERE analysis_id = ? LIMIT 1;
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return std::nullopt;
        }

        model::Analysis a;
        a.id = sqlite3_column_int(stmt, 0);
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (type) a.typeString = type;
        const char* date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (date) a.analysisDate = date;
        a.documentCount = static_cast<size_t>(sqlite3_column_int64(stmt, 3));
        a.processingTimeMs = sqlite3_column_int64(stmt, 4);
        const char* status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        if (status) a.status = status;
        const char* err = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        if (err) a.errorMessage = err;
        sqlite3_finalize(stmt);

        // Fetch associated documents
        const char* docSql = R"(
            SELECT d.document_id, d.document_name, d.file_type, d.file_size, d.word_count,
                   d.original_content, d.normalized_content, d.document_fingerprint, d.upload_date, d.file_path
            FROM DOCUMENTS d
            JOIN ANALYSIS_DOCUMENTS ad ON d.document_id = ad.document_id
            WHERE ad.analysis_id = ?;
        )";
        sqlite3_stmt* dStmt = nullptr;
        if (sqlite3_prepare_v2(db, docSql, -1, &dStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(dStmt, 1, a.id);
            while (sqlite3_step(dStmt) == SQLITE_ROW) {
                model::Document d;
                d.id = sqlite3_column_int(dStmt, 0);
                d.name = reinterpret_cast<const char*>(sqlite3_column_text(dStmt, 1));
                d.fileType = reinterpret_cast<const char*>(sqlite3_column_text(dStmt, 2));
                d.fileSize = static_cast<size_t>(sqlite3_column_int64(dStmt, 3));
                d.wordCount = static_cast<size_t>(sqlite3_column_int64(dStmt, 4));
                d.originalContent = reinterpret_cast<const char*>(sqlite3_column_text(dStmt, 5));
                d.normalizedContent = reinterpret_cast<const char*>(sqlite3_column_text(dStmt, 6));
                d.fingerprint = reinterpret_cast<const char*>(sqlite3_column_text(dStmt, 7));
                const char* uDate = reinterpret_cast<const char*>(sqlite3_column_text(dStmt, 8));
                if (uDate) d.uploadDate = uDate;
                a.documents.push_back(d);
            }
            sqlite3_finalize(dStmt);
        }

        // Fetch similarity results & matching passages
        const char* rSql = R"(
            SELECT result_id, document_a_id, document_b_id, document_a_name, document_b_name,
                   jaccard_score, directional_score, matching_count, matching_passage_count,
                   unique_ngrams_a, unique_ngrams_b, processing_time_ms
            FROM SIMILARITY_RESULTS WHERE analysis_id = ?;
        )";
        sqlite3_stmt* rStmt = nullptr;
        if (sqlite3_prepare_v2(db, rSql, -1, &rStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(rStmt, 1, a.id);
            while (sqlite3_step(rStmt) == SQLITE_ROW) {
                model::SimilarityResult res;
                res.id = sqlite3_column_int(rStmt, 0);
                res.analysisId = a.id;
                res.docAId = sqlite3_column_int(rStmt, 1);
                res.docBId = sqlite3_column_int(rStmt, 2);
                const char* na = reinterpret_cast<const char*>(sqlite3_column_text(rStmt, 3));
                if (na) res.docAName = na;
                const char* nb = reinterpret_cast<const char*>(sqlite3_column_text(rStmt, 4));
                if (nb) res.docBName = nb;
                res.jaccardScore = sqlite3_column_double(rStmt, 5);
                res.directionalScoreAtoB = sqlite3_column_double(rStmt, 6);
                res.matchingNgramCount = static_cast<size_t>(sqlite3_column_int64(rStmt, 7));
                res.matchingPassageCount = static_cast<size_t>(sqlite3_column_int64(rStmt, 8));
                res.uniqueNgramsA = static_cast<size_t>(sqlite3_column_int64(rStmt, 9));
                res.uniqueNgramsB = static_cast<size_t>(sqlite3_column_int64(rStmt, 10));
                res.processingTimeMs = sqlite3_column_int64(rStmt, 11);

                // Fetch passages for this result
                const char* pSql = R"(
                    SELECT match_id, source_document_id, target_document_id,
                           source_start_position, source_end_position,
                           target_start_position, target_end_position,
                           matching_text, match_length, match_type
                    FROM MATCHING_PASSAGES WHERE result_id = ?;
                )";
                sqlite3_stmt* pStmt = nullptr;
                if (sqlite3_prepare_v2(db, pSql, -1, &pStmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_int(pStmt, 1, res.id);
                    while (sqlite3_step(pStmt) == SQLITE_ROW) {
                        model::MatchResult match;
                        match.id = sqlite3_column_int(pStmt, 0);
                        match.resultId = res.id;
                        match.sourceDocId = sqlite3_column_int(pStmt, 1);
                        match.targetDocId = sqlite3_column_int(pStmt, 2);
                        match.sourceStartToken = static_cast<size_t>(sqlite3_column_int64(pStmt, 3));
                        match.sourceEndToken = static_cast<size_t>(sqlite3_column_int64(pStmt, 4));
                        match.targetStartToken = static_cast<size_t>(sqlite3_column_int64(pStmt, 5));
                        match.targetEndToken = static_cast<size_t>(sqlite3_column_int64(pStmt, 6));
                        const char* mt = reinterpret_cast<const char*>(sqlite3_column_text(pStmt, 7));
                        if (mt) match.matchingText = mt;
                        match.matchLength = static_cast<size_t>(sqlite3_column_int64(pStmt, 8));
                        res.matchingPassages.push_back(match);
                    }
                    sqlite3_finalize(pStmt);
                }

                a.results.push_back(res);
            }
            sqlite3_finalize(rStmt);
        }

        return a;
    }

    std::vector<int> getSimilarityHistogram() {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        std::vector<int> histogram(5, 0); // 0-20, 21-40, 41-60, 61-80, 81-100
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = R"(
            SELECT
                CASE
                    WHEN jaccard_score <= 20.0 THEN 0
                    WHEN jaccard_score <= 40.0 THEN 1
                    WHEN jaccard_score <= 60.0 THEN 2
                    WHEN jaccard_score <= 80.0 THEN 3
                    ELSE 4
                END as bucket,
                COUNT(*) as count
            FROM SIMILARITY_RESULTS
            GROUP BY bucket;
        )";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int bucket = sqlite3_column_int(stmt, 0);
                int count = sqlite3_column_int(stmt, 1);
                if (bucket >= 0 && bucket < 5) {
                    histogram[bucket] = count;
                }
            }
            sqlite3_finalize(stmt);
        }
        return histogram;
    }

    size_t count() {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = "SELECT COUNT(*) FROM ANALYSES;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
        size_t c = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            c = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
        }
        sqlite3_finalize(stmt);
        return c;
    }

    double getAverageSimilarity() {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = "SELECT AVG(jaccard_score) FROM SIMILARITY_RESULTS;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0.0;
        double avg = 0.0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            avg = sqlite3_column_double(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return avg;
    }

    size_t getTotalMatchingPassages() {
        std::lock_guard<std::mutex> lock(dbManager.getMutex());
        sqlite3* db = dbManager.getDbHandle();
        const char* sql = "SELECT COUNT(*) FROM MATCHING_PASSAGES;";
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

