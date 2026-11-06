#pragma once

#include "service/DocumentService.hpp"
#include "service/AnalysisService.hpp"
#include "service/ReportService.hpp"
#include "util/JsonUtils.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <atomic>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace plagiguard::ui {

/**
 * @brief HttpServer - Embedded high-performance C++ HTTP & REST API server.
 * Serves the modern frontend and exposes the C++ DSA plagiarism detection engine.
 */
class HttpServer {
private:
    int port;
    int serverFd;
    std::atomic<bool> running;
    std::string webRoot;
    service::DocumentService& docService;
    service::AnalysisService& analysisService;
    std::chrono::steady_clock::time_point serverStartTime;

    static std::vector<uint8_t> base64Decode(const std::string& in) {
        std::vector<uint8_t> out;
        std::vector<int> T(256, -1);
        const std::string b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; i++) T[static_cast<uint8_t>(b64[i])] = i;

        int val = 0, valb = -8;
        for (uint8_t c : in) {
            if (c == '=') break;
            if (T[c] == -1) continue;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    std::string readStaticFile(const std::string& path) {
        std::ifstream file(webRoot + path, std::ios::in | std::ios::binary);
        if (!file.is_open()) return "";
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    std::string getContentType(const std::string& path) {
        if (path.ends_with(".html")) return "text/html; charset=utf-8";
        if (path.ends_with(".css")) return "text/css; charset=utf-8";
        if (path.ends_with(".js")) return "application/javascript; charset=utf-8";
        if (path.ends_with(".json")) return "application/json; charset=utf-8";
        if (path.ends_with(".svg")) return "image/svg+xml";
        if (path.ends_with(".png")) return "image/png";
        if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
        if (path.ends_with(".ico")) return "image/x-icon";
        return "text/plain; charset=utf-8";
    }

    void handleClient(int clientSocket) {
#ifdef SO_NOSIGPIPE
        int set = 1;
        setsockopt(clientSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
#endif
        std::string request;
        char buffer[16384];
        size_t contentLength = 0;
        bool headerParsed = false;
        size_t headerEnd = std::string::npos;

        while (true) {
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) break;
            request.append(buffer, bytesRead);

            if (!headerParsed) {
                headerEnd = request.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    headerParsed = true;
                    // Find Content-Length
                    std::string headerPart = request.substr(0, headerEnd);
                    size_t clPos = headerPart.find("Content-Length:");
                    if (clPos == std::string::npos) clPos = headerPart.find("content-length:");
                    if (clPos != std::string::npos) {
                        size_t valStart = clPos + 15;
                        size_t valEnd = headerPart.find("\r\n", valStart);
                        std::string clStr = headerPart.substr(valStart, valEnd - valStart);
                        size_t first = clStr.find_first_not_of(" \t");
                        size_t last = clStr.find_last_not_of(" \t");
                        if (first != std::string::npos && last != std::string::npos) {
                            try {
                                contentLength = std::stoul(clStr.substr(first, last - first + 1));
                            } catch (...) {
                                contentLength = 0;
                            }
                        }
                    }
                }
            }

            if (headerParsed) {
                size_t bodyReceived = request.size() - (headerEnd + 4);
                if (bodyReceived >= contentLength) {
                    break;
                }
            }
        }

        if (request.empty()) {
            close(clientSocket);
            return;
        }

        std::istringstream reqStream(request);
        std::string method, path, protocol;
        if (!(reqStream >> method >> path >> protocol)) {
            close(clientSocket);
            return;
        }

        try {
            std::string responseBody;
            std::string contentType = "text/plain";
            int statusCode = 200;

            // Parse query string if present
            std::string query;
            size_t qPos = path.find('?');
            if (qPos != std::string::npos) {
                query = path.substr(qPos + 1);
                path = path.substr(0, qPos);
            }

        // --- REST API ROUTING ---
        if (method == "OPTIONS") {
            std::ostringstream response;
            response << "HTTP/1.1 204 No Content\r\n";
            response << "Access-Control-Allow-Origin: *\r\n";
            response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
            response << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
            response << "Access-Control-Max-Age: 86400\r\n";
            response << "Connection: close\r\n\r\n";
            std::string respStr = response.str();
            send(clientSocket, respStr.c_str(), respStr.size(), 0);
            close(clientSocket);
            return;
        }

        if (path == "/api/status" || path == "/api/health") {
            auto now = std::chrono::steady_clock::now();
            long uptime = std::chrono::duration_cast<std::chrono::seconds>(now - serverStartTime).count();
            auto stats = analysisService.getDashboardStats();
            std::ostringstream json;
            json << "{"
                 << "\"status\":\"ONLINE\","
                 << "\"engine\":\"PlagiGuard C++20 Core\","
                 << "\"version\":\"v2.4-AcademicNative\","
                 << "\"build\":\"Apple Clang 16.0 (C++20)\","
                 << "\"uptimeSeconds\":" << uptime << ","
                 << "\"totalDocuments\":" << stats.totalDocuments << ","
                 << "\"totalAnalyses\":" << stats.totalAnalyses << ","
                 << "\"activeAlgorithms\":["
                 << "\"CustomHashTable (Separate Chaining)\","
                 << "\"Polynomial Rolling Double Hash (Moduli 10^9+7, 998244353)\","
                 << "\"Rabin-Karp String Matcher & Collision Verifier\","
                 << "\"Word N-Gram Generator (Sliding Window)\","
                 << "\"Jaccard & Directional Similarity Metrics\","
                 << "\"Max-Heap Document Ranking\","
                 << "\"Document Graph (Adjacency List & Force Layout)\","
                 << "\"Breadth-First Search (Queue Traversal)\","
                 << "\"Depth-First Search (Stack Traversal)\","
                 << "\"Dynamic Programming (LCS & Levenshtein Distance)\","
                 << "\"Trie (Prefix Indexing)\","
                 << "\"Inverted Index (Postings List)\","
                 << "\"Apple Vision Neural OCR (VNRecognizeTextRequest)\""
                 << "]}";
            responseBody = json.str();
            contentType = "application/json";
        }
        else if (path == "/api/stats") {
            auto stats = analysisService.getDashboardStats();
            auto hist = analysisService.getSimilarityHistogram();
            std::ostringstream json;
            json << "{"
                 << "\"totalDocuments\":" << stats.totalDocuments << ","
                 << "\"totalAnalyses\":" << stats.totalAnalyses << ","
                 << "\"averageSimilarity\":" << stats.averageSimilarity << ","
                 << "\"totalMatchingPassages\":" << stats.totalMatchingPassages << ","
                 << "\"histogram\":[";
            for (size_t i = 0; i < hist.size(); ++i) {
                if (i > 0) json << ",";
                json << hist[i];
            }
            json << "]}";
            responseBody = json.str();
            contentType = "application/json";
        }
        else if (path == "/api/document") {
            int docId = 0;
            if (!query.empty() && query.find("id=") != std::string::npos) {
                size_t idPos = query.find("id=");
                size_t ampPos = query.find('&', idPos);
                std::string idStr = (ampPos != std::string::npos) ?
                                    query.substr(idPos + 3, ampPos - (idPos + 3)) :
                                    query.substr(idPos + 3);
                try { docId = std::stoi(idStr); } catch (...) { docId = 0; }
            }
            auto docOpt = docService.getDocumentById(docId);
            if (docOpt.has_value()) {
                const auto& d = docOpt.value();
                std::ostringstream json;
                json << "{"
                     << "\"id\":" << d.id << ","
                     << "\"name\":\"" << util::JsonUtils::escape(d.name) << "\","
                     << "\"type\":\"" << util::JsonUtils::escape(d.fileType) << "\","
                     << "\"size\":" << d.fileSize << ","
                     << "\"words\":" << d.wordCount << ","
                     << "\"chars\":" << d.originalContent.size() << ","
                     << "\"fingerprint\":\"" << d.fingerprint << "\","
                     << "\"uploadDate\":\"" << util::JsonUtils::escape(d.uploadDate) << "\","
                     << "\"content\":\"" << util::JsonUtils::escape(d.originalContent.substr(0, 10000)) << "\""
                     << "}";
                responseBody = json.str();
                contentType = "application/json";
            } else {
                statusCode = 404;
                responseBody = "{\"error\":\"Document not found\"}";
                contentType = "application/json";
            }
        }
        else if (path == "/api/delete-doc" && method == "POST") {
            size_t bodyPos = request.find("\r\n\r\n");
            std::string body = (bodyPos != std::string::npos) ? request.substr(bodyPos + 4) : "";
            size_t idPos = body.find("\"id\":");
            int docId = 0;
            if (idPos != std::string::npos) {
                try { docId = std::stoi(body.substr(idPos + 5)); } catch (...) {}
            }
            if (docId > 0 && docService.deleteDocument(docId)) {
                responseBody = "{\"status\":\"success\",\"message\":\"Document deleted successfully.\"}";
                contentType = "application/json";
            } else {
                statusCode = 400;
                responseBody = "{\"status\":\"error\",\"message\":\"Failed to delete document.\"}";
                contentType = "application/json";
            }
        }
        else if (path == "/api/documents") {
            auto docs = docService.getAllDocuments();
            std::ostringstream json;
            json << "[";
            for (size_t i = 0; i < docs.size(); ++i) {
                if (i > 0) json << ",";
                json << "{"
                     << "\"id\":" << docs[i].id << ","
                     << "\"name\":\"" << util::JsonUtils::escape(docs[i].name) << "\","
                     << "\"type\":\"" << util::JsonUtils::escape(docs[i].fileType) << "\","
                     << "\"size\":" << docs[i].fileSize << ","
                     << "\"words\":" << docs[i].wordCount << ","
                     << "\"fingerprint\":\"" << docs[i].fingerprint << "\","
                     << "\"uploadDate\":\"" << util::JsonUtils::escape(docs[i].uploadDate) << "\""
                     << "}";
            }
            json << "]";
            responseBody = json.str();
            contentType = "application/json";
        }
        else if (path == "/api/history") {
            auto history = analysisService.getHistory();
            std::ostringstream json;
            json << "[";
            for (size_t i = 0; i < history.size(); ++i) {
                if (i > 0) json << ",";
                std::string docNames = "";
                if (!history[i].results.empty()) {
                    docNames = history[i].results[0].docAName + " vs " + history[i].results[0].docBName;
                }
                json << "{"
                     << "\"id\":" << history[i].id << ","
                     << "\"type\":\"" << util::JsonUtils::escape(history[i].typeString) << "\","
                     << "\"date\":\"" << util::JsonUtils::escape(history[i].analysisDate) << "\","
                     << "\"documentCount\":" << history[i].documentCount << ","
                     << "\"documents\":\"" << util::JsonUtils::escape(docNames) << "\","
                     << "\"processingTime\":" << history[i].processingTimeMs << ","
                     << "\"status\":\"" << util::JsonUtils::escape(history[i].status) << "\","
                     << "\"avgSimilarity\":" << history[i].getAverageSimilarity() << ","
                     << "\"matchingPassages\":" << history[i].getTotalMatchingPassages()
                     << "}";
            }
            json << "]";
            responseBody = json.str();
            contentType = "application/json";
        }
        else if (path == "/api/analysis-detail") {
            int analysisId = 1;
            if (!query.empty() && query.find("id=") != std::string::npos) {
                size_t idPos = query.find("id=");
                size_t ampPos = query.find('&', idPos);
                std::string idStr = (ampPos != std::string::npos) ?
                                    query.substr(idPos + 3, ampPos - (idPos + 3)) :
                                    query.substr(idPos + 3);
                try { analysisId = std::stoi(idStr); } catch (...) {}
            }
            auto anOpt = analysisService.getAnalysisById(analysisId);
            if (anOpt.has_value() && !anOpt.value().results.empty()) {
                const auto& a = anOpt.value();
                const auto& res = a.results[0];
                std::string contentA, contentB;
                for (const auto& d : a.documents) {
                    if (d.id == res.docAId) contentA = d.originalContent;
                    if (d.id == res.docBId) contentB = d.originalContent;
                }
                // Fallback to fetch docs if not embedded in a.documents
                if (contentA.empty()) {
                    auto docAOpt = docService.getDocumentById(res.docAId);
                    if (docAOpt.has_value()) contentA = docAOpt.value().originalContent;
                }
                if (contentB.empty()) {
                    auto docBOpt = docService.getDocumentById(res.docBId);
                    if (docBOpt.has_value()) contentB = docBOpt.value().originalContent;
                }

                std::ostringstream json;
                json << "{"
                     << "\"analysisId\":" << a.id << ","
                     << "\"docAId\":" << res.docAId << ","
                     << "\"docBId\":" << res.docBId << ","
                     << "\"docAName\":\"" << util::JsonUtils::escape(res.docAName) << "\","
                     << "\"docBName\":\"" << util::JsonUtils::escape(res.docBName) << "\","
                     << "\"jaccardScore\":" << res.jaccardScore << ","
                     << "\"directionalAtoB\":" << res.directionalScoreAtoB << ","
                     << "\"directionalBtoA\":" << res.directionalScoreBtoA << ","
                     << "\"matchingNgrams\":" << res.matchingNgramCount << ","
                     << "\"uniqueNgramsA\":" << res.uniqueNgramsA << ","
                     << "\"uniqueNgramsB\":" << res.uniqueNgramsB << ","
                     << "\"matchingPassagesCount\":" << res.matchingPassages.size() << ","
                     << "\"processingTimeMs\":" << a.processingTimeMs << ","
                     << "\"analysisDate\":\"" << util::JsonUtils::escape(a.analysisDate) << "\","
                     << "\"docAContent\":\"" << util::JsonUtils::escape(contentA) << "\","
                     << "\"docBContent\":\"" << util::JsonUtils::escape(contentB) << "\","
                     << "\"interpretation\":\"" << util::JsonUtils::escape(res.getInterpretation()) << "\","
                     << "\"heatmap\":[";
                for (size_t h = 0; h < res.paragraphHeatmapA.size(); ++h) {
                    if (h > 0) json << ",";
                    json << res.paragraphHeatmapA[h];
                }
                json << "],\"passages\":[";
                for (size_t p = 0; p < res.matchingPassages.size(); ++p) {
                    if (p > 0) json << ",";
                    const auto& m = res.matchingPassages[p];
                    json << "{"
                         << "\"sourceCharStart\":" << m.sourceStartToken << ","
                         << "\"sourceCharEnd\":" << m.sourceEndToken << ","
                         << "\"targetCharStart\":" << m.targetStartToken << ","
                         << "\"targetCharEnd\":" << m.targetEndToken << ","
                         << "\"length\":" << m.matchLength << ","
                         << "\"text\":\"" << util::JsonUtils::escape(m.matchingText) << "\""
                         << "}";
                }
                json << "]}";
                responseBody = json.str();
                contentType = "application/json";
            } else {
                statusCode = 404;
                responseBody = "{\"error\":\"Analysis not found\"}";
                contentType = "application/json";
            }
        }
        else if (path == "/api/graph") {
            double threshold = 0.0;
            if (!query.empty() && query.find("threshold=") != std::string::npos) {
                size_t tPos = query.find("threshold=");
                size_t ampPos = query.find('&', tPos);
                std::string tStr = (ampPos != std::string::npos) ?
                                   query.substr(tPos + 10, ampPos - (tPos + 10)) :
                                   query.substr(tPos + 10);
                try { threshold = std::stod(tStr); } catch (...) { threshold = 15.0; }
            }
            analysisService.syncGraphFromDatabase(threshold);
            auto& graph = analysisService.getDocumentGraph();
            auto nodeIds = graph.getVertexIds();
            auto components = graph.getConnectedComponents();

            std::ostringstream json;
            json << "{\"threshold\":" << threshold << ",\"nodes\":[";
            for (size_t i = 0; i < nodeIds.size(); ++i) {
                if (i > 0) json << ",";
                dsa::GraphVertex v;
                graph.getVertex(nodeIds[i], v);
                json << "{\"id\":" << v.id << ",\"name\":\"" << util::JsonUtils::escape(v.name)
                     << "\",\"x\":" << v.x << ",\"y\":" << v.y << ",\"wordCount\":" << v.wordCount << "}";
            }
            json << "],\"edges\":[";
            bool firstEdge = true;
            for (int u : nodeIds) {
                const auto* neighbors = graph.getNeighbors(u);
                if (neighbors) {
                    for (const auto& e : *neighbors) {
                        if (u < e.targetId) { // avoid duplicate undirected edges
                            if (!firstEdge) json << ",";
                            json << "{\"source\":" << u << ",\"target\":" << e.targetId
                                 << ",\"weight\":" << e.similarity << "}";
                            firstEdge = false;
                        }
                    }
                }
            }
            json << "],\"components\":[";
            for (size_t i = 0; i < components.size(); ++i) {
                if (i > 0) json << ",";
                json << "[";
                for (size_t j = 0; j < components[i].size(); ++j) {
                    if (j > 0) json << ",";
                    json << components[i][j];
                }
                json << "]";
            }
            json << "]}";
            responseBody = json.str();
            contentType = "application/json";
        }
        else if (path == "/api/graph/bfs") {
            int startNode = 1;
            double threshold = 0.0;
            if (!query.empty()) {
                size_t sPos = query.find("start=");
                if (sPos != std::string::npos) {
                    size_t ampPos = query.find('&', sPos);
                    std::string sStr = (ampPos != std::string::npos) ?
                                       query.substr(sPos + 6, ampPos - (sPos + 6)) :
                                       query.substr(sPos + 6);
                    try { startNode = std::stoi(sStr); } catch (...) {}
                }
                size_t tPos = query.find("threshold=");
                if (tPos != std::string::npos) {
                    size_t ampPos = query.find('&', tPos);
                    std::string tStr = (ampPos != std::string::npos) ?
                                       query.substr(tPos + 10, ampPos - (tPos + 10)) :
                                       query.substr(tPos + 10);
                    try { threshold = std::stod(tStr); } catch (...) {}
                }
            }
            analysisService.syncGraphFromDatabase(threshold);
            auto& graph = analysisService.getDocumentGraph();
            auto traversal = graph.bfs(startNode);

            std::ostringstream json;
            json << "{\"algorithm\":\"Breadth-First Search (Queue-based)\","
                 << "\"startNode\":" << startNode << ","
                 << "\"visitedCount\":" << traversal.size() << ","
                 << "\"traversal\":[";
            for (size_t i = 0; i < traversal.size(); ++i) {
                if (i > 0) json << ",";
                json << traversal[i];
            }
            json << "]}";
            responseBody = json.str();
            contentType = "application/json";
        }
        else if (path == "/api/graph/dfs") {
            int startNode = 1;
            double threshold = 0.0;
            if (!query.empty()) {
                size_t sPos = query.find("start=");
                if (sPos != std::string::npos) {
                    size_t ampPos = query.find('&', sPos);
                    std::string sStr = (ampPos != std::string::npos) ?
                                       query.substr(sPos + 6, ampPos - (sPos + 6)) :
                                       query.substr(sPos + 6);
                    try { startNode = std::stoi(sStr); } catch (...) {}
                }
                size_t tPos = query.find("threshold=");
                if (tPos != std::string::npos) {
                    size_t ampPos = query.find('&', tPos);
                    std::string tStr = (ampPos != std::string::npos) ?
                                       query.substr(tPos + 10, ampPos - (tPos + 10)) :
                                       query.substr(tPos + 10);
                    try { threshold = std::stod(tStr); } catch (...) {}
                }
            }
            analysisService.syncGraphFromDatabase(threshold);
            auto& graph = analysisService.getDocumentGraph();
            auto traversal = graph.dfs(startNode);

            std::ostringstream json;
            json << "{\"algorithm\":\"Depth-First Search (Stack-based)\","
                 << "\"startNode\":" << startNode << ","
                 << "\"visitedCount\":" << traversal.size() << ","
                 << "\"traversal\":[";
            for (size_t i = 0; i < traversal.size(); ++i) {
                if (i > 0) json << ",";
                json << traversal[i];
            }
            json << "]}";
            responseBody = json.str();
            contentType = "application/json";
        }
        else if (path == "/api/analyze-batch" && method == "POST") {
            size_t bodyPos = request.find("\r\n\r\n");
            std::string body = (bodyPos != std::string::npos) ? request.substr(bodyPos + 4) : "";
            int windowSize = 5;
            size_t wPos = body.find("\"windowSize\":");
            if (wPos != std::string::npos) {
                try { windowSize = std::stoi(body.substr(wPos + 13)); } catch (...) {}
            }
            auto allDocs = docService.getAllDocuments();
            if (allDocs.size() < 2) {
                statusCode = 400;
                responseBody = "{\"error\":\"Need at least 2 documents in library to run batch analysis.\"}";
                contentType = "application/json";
            } else {
                for (auto& d : allDocs) {
                    auto toks = processing::TextPreprocessor::tokenizeDetailed(d.originalContent);
                    for (const auto& t : toks) {
                        d.tokens.push_back(t.text);
                        d.tokenCharOffsets.push_back({t.charStart, t.charEnd});
                        d.tokenParagraphs.push_back(t.paragraphIndex);
                    }
                    d.paragraphs = processing::TextPreprocessor::extractParagraphs(d.originalContent);
                }
                auto batchAnalysis = analysisService.analyzeBatch(allDocs, windowSize);
                auto topMatches = analysisService.getTopRankedMatches(10);

                std::ostringstream json;
                json << "{"
                     << "\"analysisId\":" << batchAnalysis.id << ","
                     << "\"documentCount\":" << batchAnalysis.documentCount << ","
                     << "\"totalComparisons\":" << batchAnalysis.results.size() << ","
                     << "\"processingTimeMs\":" << batchAnalysis.processingTimeMs << ","
                     << "\"topRanked\":[";
                for (size_t i = 0; i < topMatches.size(); ++i) {
                    if (i > 0) json << ",";
                    const auto& tm = topMatches[i];
                    json << "{"
                         << "\"docAId\":" << tm.docAId << ","
                         << "\"docBId\":" << tm.docBId << ","
                         << "\"docAName\":\"" << util::JsonUtils::escape(tm.docAName) << "\","
                         << "\"docBName\":\"" << util::JsonUtils::escape(tm.docBName) << "\","
                         << "\"similarity\":" << tm.similarityScore
                         << "}";
                }
                json << "],\"allResults\":[";
                for (size_t i = 0; i < batchAnalysis.results.size(); ++i) {
                    if (i > 0) json << ",";
                    const auto& r = batchAnalysis.results[i];
                    json << "{"
                         << "\"docAId\":" << r.docAId << ","
                         << "\"docBId\":" << r.docBId << ","
                         << "\"docAName\":\"" << util::JsonUtils::escape(r.docAName) << "\","
                         << "\"docBName\":\"" << util::JsonUtils::escape(r.docBName) << "\","
                         << "\"jaccardScore\":" << r.jaccardScore << ","
                         << "\"directionalAtoB\":" << r.directionalScoreAtoB << ","
                         << "\"directionalBtoA\":" << r.directionalScoreBtoA << ","
                         << "\"matchingPassagesCount\":" << r.matchingPassageCount
                         << "}";
                }
                json << "]}";
                responseBody = json.str();
                contentType = "application/json";
            }
        }
        else if (path == "/api/upload" && method == "POST") {
            size_t bodyPos = request.find("\r\n\r\n");
            std::string body = (bodyPos != std::string::npos) ? request.substr(bodyPos + 4) : "";

            std::string filename = "uploaded_doc.txt";
            size_t fPos = body.find("\"filename\":");
            if (fPos != std::string::npos) {
                size_t startQ = body.find('"', fPos + 11);
                size_t endQ = body.find('"', startQ + 1);
                if (startQ != std::string::npos && endQ != std::string::npos) {
                    filename = body.substr(startQ + 1, endQ - startQ - 1);
                }
            }

            // Clean filename
            for (char& c : filename) {
                if (c == '/' || c == '\\' || c == ':' || c == '?' || c == '*') c = '_';
            }

            std::string destPath = "uploads/" + filename;
            bool saved = false;

            // Check if base64 data was sent: "data":"..."
            size_t dPos = body.find("\"data\":");
            if (dPos != std::string::npos) {
                size_t startQ = body.find('"', dPos + 7);
                size_t endQ = body.find('"', startQ + 1);
                if (startQ != std::string::npos && endQ != std::string::npos) {
                    std::string rawData = body.substr(startQ + 1, endQ - startQ - 1);
                    size_t comma = rawData.find(',');
                    if (comma != std::string::npos) {
                        rawData = rawData.substr(comma + 1);
                    }
                    auto bytes = base64Decode(rawData);
                    if (!bytes.empty()) {
                        std::ofstream outFile(destPath, std::ios::binary);
                        if (outFile.is_open()) {
                            outFile.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                            outFile.close();
                            saved = true;
                        }
                    }
                }
            }

            if (!saved) {
                // Fallback: raw text content
                size_t cPos = body.find("\"content\":");
                if (cPos != std::string::npos) {
                    size_t startQ = body.find('"', cPos + 10);
                    size_t endQ = body.rfind('"');
                    if (startQ != std::string::npos && endQ > startQ) {
                        std::string rawContent = body.substr(startQ + 1, endQ - startQ - 1);
                        std::string content;
                        for (size_t i = 0; i < rawContent.size(); ++i) {
                            if (rawContent[i] == '\\' && i + 1 < rawContent.size()) {
                                char next = rawContent[i + 1];
                                if (next == 'n') { content += '\n'; i++; }
                                else if (next == 'r') { content += '\r'; i++; }
                                else if (next == 't') { content += '\t'; i++; }
                                else if (next == '"') { content += '"'; i++; }
                                else if (next == '\\') { content += '\\'; i++; }
                                else { content += rawContent[i]; }
                            } else {
                                content += rawContent[i];
                            }
                        }
                        std::ofstream outFile(destPath, std::ios::binary);
                        if (outFile.is_open()) {
                            outFile << content;
                            outFile.close();
                            saved = true;
                        }
                    }
                }
            }

            if (saved) {
                auto res = docService.processDocument(destPath);
                if (res.success) {
                    responseBody = "{\"status\":\"success\",\"message\":\"Document uploaded and indexed successfully.\",\"id\":" + std::to_string(res.document.id) + ",\"name\":\"" + util::JsonUtils::escape(res.document.name) + "\"}";
                    contentType = "application/json";
                } else if (res.isDuplicate) {
                    responseBody = "{\"status\":\"duplicate\","
                                   "\"message\":\"" + util::JsonUtils::escape(res.message) + "\","
                                   "\"duplicateType\":\"EXACT_TEXT_DUPLICATE\","
                                   "\"duplicateOf\":\"" + util::JsonUtils::escape(res.duplicateOf.name) + "\","
                                   "\"existingId\":" + std::to_string(res.duplicateOf.id) + ","
                                   "\"fingerprint\":\"" + res.duplicateOf.fingerprint + "\","
                                   "\"wordCount\":" + std::to_string(res.duplicateOf.wordCount) + ","
                                   "\"uploadDate\":\"" + util::JsonUtils::escape(res.duplicateOf.uploadDate) + "\"}";
                    contentType = "application/json";
                } else {
                    responseBody = "{\"status\":\"error\",\"message\":\"" + util::JsonUtils::escape(res.message) + "\"}";
                    contentType = "application/json";
                }
            } else {
                statusCode = 400;
                responseBody = "{\"status\":\"error\",\"message\":\"Failed to parse and save file content.\"}";
                contentType = "application/json";
            }
        }
        else if (path == "/api/analyze-two" && method == "POST") {
            // Find body start
            size_t bodyPos = request.find("\r\n\r\n");
            std::string body = (bodyPos != std::string::npos) ? request.substr(bodyPos + 4) : "";

            // Parse docAId and docBId from json body
            int docAId = 0, docBId = 0, windowSize = 5;
            auto parseField = [&](const std::string& key) -> int {
                size_t p = body.find("\"" + key + "\":");
                if (p != std::string::npos) {
                    return std::stoi(body.substr(p + key.size() + 3));
                }
                return 0;
            };
            docAId = parseField("docAId");
            docBId = parseField("docBId");
            int w = parseField("windowSize");
            if (w > 0) windowSize = w;

            auto allDocs = docService.getAllDocuments();
            model::Document docA, docB;
            bool foundA = false, foundB = false;
            for (const auto& d : allDocs) {
                if (d.id == docAId) { docA = d; foundA = true; }
                if (d.id == docBId) { docB = d; foundB = true; }
            }

            if (foundA && foundB) {
                // Prepare tokens & detailed info
                auto toksA = processing::TextPreprocessor::tokenizeDetailed(docA.originalContent);
                for (const auto& t : toksA) {
                    docA.tokens.push_back(t.text);
                    docA.tokenCharOffsets.push_back({t.charStart, t.charEnd});
                    docA.tokenParagraphs.push_back(t.paragraphIndex);
                }
                docA.paragraphs = processing::TextPreprocessor::extractParagraphs(docA.originalContent);

                auto toksB = processing::TextPreprocessor::tokenizeDetailed(docB.originalContent);
                for (const auto& t : toksB) {
                    docB.tokens.push_back(t.text);
                    docB.tokenCharOffsets.push_back({t.charStart, t.charEnd});
                    docB.tokenParagraphs.push_back(t.paragraphIndex);
                }
                docB.paragraphs = processing::TextPreprocessor::extractParagraphs(docB.originalContent);

                auto analysis = analysisService.analyzeTwoDocuments(docA, docB, windowSize);
                const auto& res = analysis.results[0];

                std::ostringstream json;
                json << "{"
                     << "\"analysisId\":" << analysis.id << ","
                     << "\"jaccardScore\":" << res.jaccardScore << ","
                     << "\"directionalAtoB\":" << res.directionalScoreAtoB << ","
                     << "\"directionalBtoA\":" << res.directionalScoreBtoA << ","
                     << "\"matchingNgrams\":" << res.matchingNgramCount << ","
                     << "\"uniqueNgramsA\":" << res.uniqueNgramsA << ","
                     << "\"uniqueNgramsB\":" << res.uniqueNgramsB << ","
                     << "\"matchingPassagesCount\":" << res.matchingPassageCount << ","
                     << "\"processingTimeMs\":" << res.processingTimeMs << ","
                     << "\"docAContent\":\"" << util::JsonUtils::escape(docA.originalContent) << "\","
                     << "\"docBContent\":\"" << util::JsonUtils::escape(docB.originalContent) << "\","
                     << "\"interpretation\":\"" << util::JsonUtils::escape(res.getInterpretation()) << "\","
                     << "\"heatmap\":[";
                for (size_t h = 0; h < res.paragraphHeatmapA.size(); ++h) {
                    if (h > 0) json << ",";
                    json << res.paragraphHeatmapA[h];
                }
                json << "],\"passages\":[";
                for (size_t p = 0; p < res.matchingPassages.size(); ++p) {
                    if (p > 0) json << ",";
                    const auto& m = res.matchingPassages[p];
                    json << "{"
                         << "\"sourceCharStart\":" << m.sourceCharStart << ","
                         << "\"sourceCharEnd\":" << m.sourceCharEnd << ","
                         << "\"targetCharStart\":" << m.targetCharStart << ","
                         << "\"targetCharEnd\":" << m.targetCharEnd << ","
                         << "\"length\":" << m.matchLength << ","
                         << "\"text\":\"" << util::JsonUtils::escape(m.matchingText) << "\""
                         << "}";
                }
                json << "]}";
                responseBody = json.str();
                contentType = "application/json";
            } else {
                statusCode = 404;
                responseBody = "{\"error\":\"Document not found\"}";
                contentType = "application/json";
            }
        }
        else if (path == "/api/version-diff" && method == "POST") {
            size_t bodyPos = request.find("\r\n\r\n");
            std::string body = (bodyPos != std::string::npos) ? request.substr(bodyPos + 4) : "";
            int docAId = 0, docBId = 0;
            auto parseField = [&](const std::string& key) -> int {
                size_t p = body.find("\"" + key + "\":");
                if (p != std::string::npos) return std::stoi(body.substr(p + key.size() + 3));
                return 0;
            };
            docAId = parseField("docAId");
            docBId = parseField("docBId");

            auto allDocs = docService.getAllDocuments();
            model::Document docA, docB;
            for (const auto& d : allDocs) {
                if (d.id == docAId) docA = d;
                if (d.id == docBId) docB = d;
            }

            auto detailedA = processing::TextPreprocessor::tokenizeDetailed(docA.originalContent);
            for (const auto& t : detailedA) docA.tokens.push_back(t.text);
            auto detailedB = processing::TextPreprocessor::tokenizeDetailed(docB.originalContent);
            for (const auto& t : detailedB) docB.tokens.push_back(t.text);

            auto diffResult = analysisService.compareVersions(docA, docB);
            auto alignResult = analysisService.alignDocuments(docA, docB);

            std::ostringstream json;
            json << "{"
                 << "\"lcsLength\":" << diffResult.lcsLength << ","
                 << "\"similarityRatio\":" << (diffResult.similarityRatio * 100.0) << ","
                 << "\"additions\":" << diffResult.additions << ","
                 << "\"deletions\":" << diffResult.deletions << ","
                 << "\"unchanged\":" << diffResult.unchanged << ","
                 << "\"editDistance\":" << alignResult.distance << ","
                 << "\"diffs\":[";
            for (size_t i = 0; i < diffResult.diffs.size(); ++i) {
                if (i > 0) json << ",";
                const auto& d = diffResult.diffs[i];
                std::string typeStr = (d.type == dsa::DiffType::ADDED) ? "ADDED" :
                                      ((d.type == dsa::DiffType::REMOVED) ? "REMOVED" : "UNCHANGED");
                json << "{\"type\":\"" << typeStr << "\",\"text\":\"" << util::JsonUtils::escape(d.text) << "\"}";
            }
            json << "]}";
            responseBody = json.str();
            contentType = "application/json";
        }
        else if (path == "/api/report") {
            int analysisId = 1;
            if (!query.empty() && query.find("id=") != std::string::npos) {
                size_t idPos = query.find("id=");
                size_t ampPos = query.find('&', idPos);
                std::string idStr = (ampPos != std::string::npos) ?
                                    query.substr(idPos + 3, ampPos - (idPos + 3)) :
                                    query.substr(idPos + 3);
                try { analysisId = std::stoi(idStr); } catch (...) { analysisId = 1; }
            }
            auto history = analysisService.getHistory();
            for (const auto& a : history) {
                if (a.id == analysisId) {
                    if (query.find("format=pdf") != std::string::npos) {
                        std::string pdfPath = "reports/report_" + std::to_string(analysisId) + ".pdf";
                        service::ReportService::generatePdfReport(a, pdfPath);
                        std::ifstream pFile(pdfPath, std::ios::binary);
                        if (pFile.is_open()) {
                            std::ostringstream ss;
                            ss << pFile.rdbuf();
                            responseBody = ss.str();
                            contentType = "application/pdf";
                        } else {
                            responseBody = service::ReportService::generateHtmlReport(a);
                            contentType = "text/html; charset=utf-8";
                        }
                    } else {
                        responseBody = service::ReportService::generateHtmlReport(a);
                        contentType = "text/html; charset=utf-8";
                    }
                    break;
                }
            }
            if (responseBody.empty()) {
                responseBody = "<h1>Report Not Found</h1>";
                statusCode = 404;
            }
        }
        else {
            // Serve static files from web/ directory
            if (path == "/" || path.empty()) path = "/index.html";
            std::string content = readStaticFile(path);
            if (!content.empty()) {
                responseBody = content;
                contentType = getContentType(path);
            } else {
                statusCode = 404;
                responseBody = "404 Not Found";
            }
        }

        // Send HTTP Response
        std::ostringstream response;
        response << "HTTP/1.1 " << statusCode << " " << (statusCode == 200 ? "OK" : "Not Found") << "\r\n";
        response << "Content-Type: " << contentType << "\r\n";
        response << "Content-Length: " << responseBody.size() << "\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Connection: close\r\n\r\n";
        response << responseBody;

        std::string respStr = response.str();
        send(clientSocket, respStr.c_str(), respStr.size(), 0);
        close(clientSocket);
        } catch (const std::exception& e) {
            std::cerr << "[HttpServer] Error processing request: " << e.what() << "\n";
            close(clientSocket);
        } catch (...) {
            std::cerr << "[HttpServer] Unknown error processing request\n";
            close(clientSocket);
        }
    }

public:
    HttpServer(int p, const std::string& root,
               service::DocumentService& ds, service::AnalysisService& as)
        : port(p), serverFd(-1), running(false), webRoot(root),
          docService(ds), analysisService(as),
          serverStartTime(std::chrono::steady_clock::now()) {}

    ~HttpServer() {
        stop();
    }

    bool start() {
        signal(SIGPIPE, SIG_IGN);
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd < 0) return false;

        int opt = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            close(serverFd);
            return false;
        }

        if (listen(serverFd, 128) < 0) {
            close(serverFd);
            return false;
        }

        running = true;
        std::cout << "[PlagiGuard] Web Server listening at http://127.0.0.1:" << port << std::endl;

        std::thread([this]() {
            while (running) {
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                int clientSocket = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientSocket >= 0) {
                    std::thread(&HttpServer::handleClient, this, clientSocket).detach();
                }
            }
        }).detach();

        return true;
    }

    void stop() {
        if (running) {
            running = false;
            if (serverFd >= 0) {
                close(serverFd);
                serverFd = -1;
            }
        }
    }
};

} // namespace plagiguard::ui

