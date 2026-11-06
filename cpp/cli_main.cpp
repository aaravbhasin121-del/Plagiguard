#include "database/DatabaseManager.hpp"
#include "service/DocumentService.hpp"
#include "service/AnalysisService.hpp"
#include "service/ReportService.hpp"
#include "dsa/MaxHeap.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>

using namespace plagiguard;

void printUsage() {
    std::cout << "PlagiGuard CLI — Intelligent Academic Plagiarism Detection\n";
    std::cout << "Usage:\n";
    std::cout << "  ./plagiguard-cli compare <docA.txt> <docB.txt> [ngram_window]\n";
    std::cout << "  ./plagiguard-cli version <oldVersion.txt> <newVersion.txt>\n";
    std::cout << "  ./plagiguard-cli benchmark\n";
    std::cout << "  ./plagiguard-cli report <analysis_id> [output.html]\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    try {
        database::DatabaseManager::getInstance().initialize("plagiguard.db");
    } catch (const std::exception& e) {
        std::cerr << "Database warning: " << e.what() << "\n";
    }

    service::DocumentService docService;
    service::AnalysisService analysisService;

    if (command == "compare") {
        if (argc < 4) {
            std::cerr << "Error: provide two document paths.\n";
            return 1;
        }
        std::string fileA = argv[2];
        std::string fileB = argv[3];
        size_t window = (argc >= 5) ? std::stoul(argv[4]) : 5;

        auto resA = docService.processDocument(fileA);
        auto resB = docService.processDocument(fileB);

        model::Document docA = resA.isDuplicate ? resA.duplicateOf : resA.document;
        model::Document docB = resB.isDuplicate ? resB.duplicateOf : resB.document;

        if (docA.tokens.empty()) {
            auto detailedA = processing::TextPreprocessor::tokenizeDetailed(docA.originalContent);
            for (const auto& t : detailedA) {
                docA.tokens.push_back(t.text);
                docA.tokenCharOffsets.push_back({t.charStart, t.charEnd});
                docA.tokenParagraphs.push_back(t.paragraphIndex);
            }
            docA.paragraphs = processing::TextPreprocessor::extractParagraphs(docA.originalContent);
        }
        if (docB.tokens.empty()) {
            auto detailedB = processing::TextPreprocessor::tokenizeDetailed(docB.originalContent);
            for (const auto& t : detailedB) {
                docB.tokens.push_back(t.text);
                docB.tokenCharOffsets.push_back({t.charStart, t.charEnd});
                docB.tokenParagraphs.push_back(t.paragraphIndex);
            }
            docB.paragraphs = processing::TextPreprocessor::extractParagraphs(docB.originalContent);
        }

        std::cout << "\n=======================================================\n";
        std::cout << "  PLAGIGUARD COMPARISON: " << docA.name << " vs " << docB.name << "\n";
        std::cout << "=======================================================\n";

        auto analysis = analysisService.analyzeTwoDocuments(docA, docB, window, [](int pct, const std::string& status) {
            std::cout << "  [" << std::setw(3) << pct << "%] " << status << "\n";
        });

        const auto& sim = analysis.results[0];
        std::cout << "\n--- SIMILARITY METRICS ---\n";
        std::cout << "Jaccard Similarity:      " << std::fixed << std::setprecision(2) << sim.jaccardScore << "%\n";
        std::cout << "Directional Overlap:     " << sim.directionalScoreAtoB << "%\n";
        std::cout << "Matching N-grams:        " << sim.matchingNgramCount << "\n";
        std::cout << "Unique Shingles (A / B): " << sim.uniqueNgramsA << " / " << sim.uniqueNgramsB << "\n";
        std::cout << "Matching Passages:       " << sim.matchingPassageCount << "\n";
        std::cout << "Processing Time:         " << sim.processingTimeMs << " ms\n";
        std::cout << "Interpretation:          " << sim.getInterpretation() << "\n\n";

        if (!sim.matchingPassages.empty()) {
            std::cout << "--- MATCHING PASSAGES (FIRST 5) ---\n";
            for (size_t i = 0; i < sim.matchingPassages.size() && i < 5; ++i) {
                const auto& m = sim.matchingPassages[i];
                std::cout << "[" << (i + 1) << "] (" << m.matchLength << " words, tokens " << m.sourceStartToken << "-" << m.sourceEndToken << "):\n";
                std::cout << "    \"" << m.matchingText << "\"\n\n";
            }
        }
    } else if (command == "version") {
        if (argc < 4) {
            std::cerr << "Error: provide two document paths for version diff.\n";
            return 1;
        }
        auto resA = docService.processDocument(argv[2]);
        auto resB = docService.processDocument(argv[3]);
        model::Document docA = resA.isDuplicate ? resA.duplicateOf : resA.document;
        model::Document docB = resB.isDuplicate ? resB.duplicateOf : resB.document;

        if (docA.tokens.empty()) {
            auto detailedA = processing::TextPreprocessor::tokenizeDetailed(docA.originalContent);
            for (const auto& t : detailedA) docA.tokens.push_back(t.text);
        }
        if (docB.tokens.empty()) {
            auto detailedB = processing::TextPreprocessor::tokenizeDetailed(docB.originalContent);
            for (const auto& t : detailedB) docB.tokens.push_back(t.text);
        }

        std::cout << "\n=======================================================\n";
        std::cout << "  PLAGIGUARD VERSION DIFF (LCS + EDIT DISTANCE)\n";
        std::cout << "=======================================================\n";

        auto diff = analysisService.compareVersions(docA, docB);
        auto align = analysisService.alignDocuments(docA, docB);

        std::cout << "LCS Length:          " << diff.lcsLength << " tokens\n";
        std::cout << "Similarity Ratio:    " << std::fixed << std::setprecision(2) << (diff.similarityRatio * 100.0) << "%\n";
        std::cout << "Additions:           +" << diff.additions << " tokens\n";
        std::cout << "Deletions:           -" << diff.deletions << " tokens\n";
        std::cout << "Unchanged:           " << diff.unchanged << " tokens\n";
        std::cout << "Edit Distance:       " << align.distance << " operations\n\n";
    } else if (command == "report") {
        if (argc < 3) {
            std::cerr << "Usage: ./plagiguard-cli report <analysis_id> [output_file.pdf/html]\n";
            return 1;
        }
        int id = std::stoi(argv[2]);
        std::string outPath = (argc >= 4) ? argv[3] : ("reports/report_" + std::to_string(id) + ".pdf");
        auto history = analysisService.getHistory();
        bool found = false;
        for (const auto& a : history) {
            if (a.id == id) {
                if (outPath.ends_with(".html")) {
                    service::ReportService::saveReport(a, outPath);
                    std::cout << "[Report] Saved HTML report to " << outPath << "\n";
                } else {
                    service::ReportService::generatePdfReport(a, outPath);
                    std::cout << "[Report] Saved official PDF report to " << outPath << "\n";
                }
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "Error: Analysis ID " << id << " not found in database.\n";
            return 1;
        }
    } else if (command == "benchmark") {
        std::cout << "\n=======================================================\n";
        std::cout << "  PLAGIGUARD ALGORITHMIC BENCHMARK SUITE\n";
        std::cout << "=======================================================\n";

        // Synthetic large document benchmark
        std::vector<std::string> largeTokens;
        for (int i = 0; i < 50000; ++i) {
            largeTokens.push_back("term_" + std::to_string(i % 500));
        }

        std::cout << "[Benchmark] Testing CustomHashTable (100,000 operations)...\n";
        auto t0 = std::chrono::high_resolution_clock::now();
        dsa::CustomHashTable<std::string, int> benchTable(1024);
        for (int i = 0; i < 100000; ++i) {
            benchTable.put("key_" + std::to_string(i), i);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        std::cout << "  -> Inserted 100,000 keys in: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
                  << " ms (Load factor: " << benchTable.getLoadFactor() << ")\n";

        std::cout << "[Benchmark] Testing Polynomial Rolling Hash on 50,000 tokens...\n";
        dsa::RollingHash rh(5);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto hashes = rh.computeAllHashes(rh.mapTokensToIds(largeTokens));
        auto t3 = std::chrono::high_resolution_clock::now();
        std::cout << "  -> Computed " << hashes.size() << " rolling hashes in: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count() << " ms\n";

        std::cout << "[Benchmark] Testing MaxHeap (50,000 priority entries)...\n";
        auto t4 = std::chrono::high_resolution_clock::now();
        dsa::MaxHeap heap(1024);
        for (int i = 0; i < 50000; ++i) {
            heap.insert(dsa::HeapEntry(i, "docA", i + 1, "docB", (i * 37) % 100));
        }
        auto top10 = heap.getTopK(10);
        auto t5 = std::chrono::high_resolution_clock::now();
        std::cout << "  -> MaxHeap top-10 extracted in: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(t5 - t4).count() << " ms (Top score: "
                  << top10[0].similarityScore << "%)\n";

        std::cout << "\nAll benchmarks executed with 100% manual C++ data structures.\n";
    } else {
        printUsage();
    }

    return 0;
}
