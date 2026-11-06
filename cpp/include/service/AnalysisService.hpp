#pragma once

#include "database/AnalysisRepository.hpp"
#include "database/DocumentRepository.hpp"
#include "detector/PlagiarismDetector.hpp"
#include "dsa/DocumentGraph.hpp"
#include "dsa/MaxHeap.hpp"
#include "dsa/LCS.hpp"
#include "dsa/EditDistance.hpp"
#include "model/Analysis.hpp"
#include <vector>
#include <chrono>
#include <functional>

namespace plagiguard::service {

struct DashboardStats {
    size_t totalDocuments;
    size_t totalAnalyses;
    double averageSimilarity;
    size_t totalMatchingPassages;
    std::vector<model::Analysis> recentAnalyses;
};

/**
 * @brief AnalysisService - Coordinates multi-mode plagiarism analyses and graph construction.
 */
class AnalysisService {
private:
    database::AnalysisRepository analysisRepo;
    database::DocumentRepository docRepo;
    dsa::DocumentGraph documentGraph;
    dsa::MaxHeap rankingHeap;

public:
    AnalysisService() : documentGraph(15.0) {}

    model::Analysis analyzeTwoDocuments(const model::Document& docA,
                                       const model::Document& docB,
                                       size_t windowSize = 5,
                                       const std::function<void(int, const std::string&)>& progressCallback = nullptr) {
        auto startTime = std::chrono::high_resolution_clock::now();

        if (progressCallback) progressCallback(10, "Extracting and tokenizing documents");
        if (progressCallback) progressCallback(30, "Generating N-grams and computing rolling hashes");

        detector::PlagiarismDetector detector(windowSize);
        if (progressCallback) progressCallback(60, "Executing Rabin-Karp matching and collision verification");

        model::SimilarityResult sim = detector.compare(docA, docB);

        if (progressCallback) progressCallback(85, "Calculating Jaccard similarity and directional overlap");

        model::Analysis analysis;
        analysis.type = model::AnalysisType::TWO_DOCUMENT;
        analysis.typeString = "TWO_DOCUMENT";
        analysis.documentCount = 2;
        analysis.documents = {docA, docB};
        analysis.results = {sim};
        analysis.status = "COMPLETED";

        auto endTime = std::chrono::high_resolution_clock::now();
        analysis.processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        if (progressCallback) progressCallback(95, "Persisting results to database");
        analysisRepo.saveAnalysis(analysis);

        // Update Graph and Heap
        documentGraph.addUndirectedEdge(docA.id, docA.name, docB.id, docB.name, sim.jaccardScore);
        rankingHeap.insert(dsa::HeapEntry(docA.id, docA.name, docB.id, docB.name, sim.jaccardScore));

        if (progressCallback) progressCallback(100, "Analysis complete");
        return analysis;
    }

    model::Analysis analyzeBatch(const std::vector<model::Document>& docs,
                                 size_t windowSize = 5,
                                 const std::function<void(int, const std::string&)>& progressCallback = nullptr) {
        auto startTime = std::chrono::high_resolution_clock::now();
        detector::PlagiarismDetector detector(windowSize);

        model::Analysis analysis;
        analysis.type = model::AnalysisType::BATCH;
        analysis.typeString = "BATCH";
        analysis.documentCount = docs.size();
        analysis.documents = docs;
        analysis.status = "RUNNING";

        size_t totalPairs = (docs.size() * (docs.size() - 1)) / 2;
        size_t currentPair = 0;

        for (size_t i = 0; i < docs.size(); ++i) {
            documentGraph.addVertex(docs[i].id, docs[i].name, docs[i].wordCount);
            for (size_t j = i + 1; j < docs.size(); ++j) {
                currentPair++;
                int pct = static_cast<int>((static_cast<double>(currentPair) / static_cast<double>(totalPairs > 0 ? totalPairs : 1)) * 90.0);
                if (progressCallback) {
                    progressCallback(pct, "Comparing " + docs[i].name + " vs " + docs[j].name);
                }

                model::SimilarityResult sim = detector.compare(docs[i], docs[j]);
                analysis.results.push_back(sim);

                documentGraph.addUndirectedEdge(docs[i].id, docs[i].name, docs[j].id, docs[j].name, sim.jaccardScore);
                rankingHeap.insert(dsa::HeapEntry(docs[i].id, docs[i].name, docs[j].id, docs[j].name, sim.jaccardScore));
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        analysis.processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        analysis.status = "COMPLETED";

        if (progressCallback) progressCallback(95, "Saving batch analysis records");
        analysisRepo.saveAnalysis(analysis);

        documentGraph.computeLayout(800.0, 600.0);

        if (progressCallback) progressCallback(100, "Batch analysis complete");
        return analysis;
    }

    dsa::VersionDiffResult compareVersions(const model::Document& docA, const model::Document& docB) {
        return dsa::LCS::computeDiff(docA.tokens, docB.tokens);
    }

    dsa::AlignmentResult alignDocuments(const model::Document& docA, const model::Document& docB) {
        return dsa::EditDistance::align(docA.tokens, docB.tokens);
    }

    std::vector<model::Analysis> getHistory() {
        return analysisRepo.findAll();
    }

    std::optional<model::Analysis> getAnalysisById(int id) {
        return analysisRepo.findById(id);
    }

    std::vector<int> getSimilarityHistogram() {
        return analysisRepo.getSimilarityHistogram();
    }

    void syncGraphFromDatabase(double threshold = 0.0) {
        documentGraph = dsa::DocumentGraph(threshold);
        rankingHeap.clear();
        auto allDocs = docRepo.findAll();
        for (const auto& doc : allDocs) {
            documentGraph.addVertex(doc.id, doc.name, doc.wordCount);
        }

        // 1. Ingest all prior saved analyses
        auto allAnalyses = analysisRepo.findAll();
        for (const auto& an : allAnalyses) {
            for (const auto& res : an.results) {
                documentGraph.addUndirectedEdge(res.docAId, res.docAName,
                                               res.docBId, res.docBName,
                                               res.jaccardScore);
                rankingHeap.insert(dsa::HeapEntry(res.docAId, res.docAName,
                                                  res.docBId, res.docBName,
                                                  res.jaccardScore));
            }
        }

        // 2. Compute pairwise similarity for all documents to guarantee connected graph topology
        for (size_t i = 0; i < allDocs.size(); ++i) {
            for (size_t j = i + 1; j < allDocs.size(); ++j) {
                int aId = allDocs[i].id;
                int bId = allDocs[j].id;

                const auto* neighbors = documentGraph.getNeighbors(aId);
                bool hasEdge = false;
                if (neighbors) {
                    for (const auto& e : *neighbors) {
                        if (e.targetId == bId) { hasEdge = true; break; }
                    }
                }

                if (!hasEdge) {
                    detector::PlagiarismDetector detectorInst(5);
                    model::SimilarityResult sim = detectorInst.compare(allDocs[i], allDocs[j]);
                    documentGraph.addUndirectedEdge(aId, allDocs[i].name,
                                                   bId, allDocs[j].name,
                                                   sim.jaccardScore);
                    rankingHeap.insert(dsa::HeapEntry(aId, allDocs[i].name,
                                                      bId, allDocs[j].name,
                                                      sim.jaccardScore));
                }
            }
        }

        documentGraph.computeLayout(850.0, 520.0);
    }

    dsa::DocumentGraph& getDocumentGraph() {
        return documentGraph;
    }

    std::vector<dsa::HeapEntry> getTopRankedMatches(size_t k) {
        return rankingHeap.getTopK(k);
    }

    DashboardStats getDashboardStats() {
        DashboardStats stats;
        stats.totalDocuments = docRepo.count();
        stats.totalAnalyses = analysisRepo.count();
        stats.averageSimilarity = analysisRepo.getAverageSimilarity();
        stats.totalMatchingPassages = analysisRepo.getTotalMatchingPassages();
        stats.recentAnalyses = analysisRepo.findAll();
        if (stats.recentAnalyses.size() > 5) {
            stats.recentAnalyses.resize(5);
        }
        return stats;
    }
};

} // namespace plagiguard::service

