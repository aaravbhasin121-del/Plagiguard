#pragma once

#include "ExactMatchDetector.hpp"
#include "PartialMatchDetector.hpp"
#include "DuplicateDetector.hpp"
#include "ParaphraseAnalyzer.hpp"
#include "dsa/NGramGenerator.hpp"
#include "dsa/RollingHash.hpp"
#include "dsa/Similarity.hpp"
#include "model/Document.hpp"
#include "model/SimilarityResult.hpp"
#include "processing/TextPreprocessor.hpp"
#include <chrono>

namespace plagiguard::detector {

/**
 * @brief PlagiarismDetector - Core orchestration engine for document comparison.
 *
 * Pipeline:
 * 1. Generate word-level N-grams and shingles.
 * 2. Execute Rabin-Karp rolling double-hash search with token verification.
 * 3. Calculate Jaccard similarity and Directional overlap metrics.
 * 4. Generate paragraph-level similarity heatmap.
 * 5. Extract citations and mark matching passages.
 */
class PlagiarismDetector {
private:
    size_t ngramWindow;
    size_t minPassageLength;

public:
    explicit PlagiarismDetector(size_t window = 5, size_t minLength = 6)
        : ngramWindow(window), minPassageLength(minLength) {}

    void setNgramWindow(size_t window) {
        ngramWindow = window > 0 ? window : 5;
    }

    size_t getNgramWindow() const {
        return ngramWindow;
    }

    model::SimilarityResult compare(const model::Document& inputDocA, const model::Document& inputDocB) {
        auto startTime = std::chrono::high_resolution_clock::now();
        model::SimilarityResult result;

        model::Document docA = inputDocA;
        model::Document docB = inputDocB;

        if (docA.tokens.empty() && !docA.originalContent.empty()) {
            auto detailedA = processing::TextPreprocessor::tokenizeDetailed(docA.originalContent);
            docA.tokens.reserve(detailedA.size());
            docA.tokenCharOffsets.reserve(detailedA.size());
            docA.tokenParagraphs.reserve(detailedA.size());
            for (const auto& t : detailedA) {
                docA.tokens.push_back(t.text);
                docA.tokenCharOffsets.push_back({t.charStart, t.charEnd});
                docA.tokenParagraphs.push_back(t.paragraphIndex);
            }
            docA.paragraphs = processing::TextPreprocessor::extractParagraphs(docA.originalContent);
            docA.wordCount = docA.tokens.size();
        }

        if (docB.tokens.empty() && !docB.originalContent.empty()) {
            auto detailedB = processing::TextPreprocessor::tokenizeDetailed(docB.originalContent);
            docB.tokens.reserve(detailedB.size());
            docB.tokenCharOffsets.reserve(detailedB.size());
            docB.tokenParagraphs.reserve(detailedB.size());
            for (const auto& t : detailedB) {
                docB.tokens.push_back(t.text);
                docB.tokenCharOffsets.push_back({t.charStart, t.charEnd});
                docB.tokenParagraphs.push_back(t.paragraphIndex);
            }
            docB.paragraphs = processing::TextPreprocessor::extractParagraphs(docB.originalContent);
            docB.wordCount = docB.tokens.size();
        }

        if (docA.paragraphs.empty() && !docA.originalContent.empty()) {
            docA.paragraphs = processing::TextPreprocessor::extractParagraphs(docA.originalContent);
        }
        if (docB.paragraphs.empty() && !docB.originalContent.empty()) {
            docB.paragraphs = processing::TextPreprocessor::extractParagraphs(docB.originalContent);
        }

        result.docAId = docA.id;
        result.docAName = docA.name;
        result.docBId = docB.id;
        result.docBName = docB.name;

        // 1. Shingle Generation
        dsa::NGramGenerator generator(ngramWindow);
        std::vector<std::string> shinglesA = generator.generateShingles(docA.tokens);
        std::vector<std::string> shinglesB = generator.generateShingles(docB.tokens);

        result.totalNgramsA = shinglesA.size();
        result.totalNgramsB = shinglesB.size();

        // 2. Exact Mathematical Similarity Calculation (Jaccard + Directional)
        auto score = dsa::SimilarityMetrics::compute(shinglesA, shinglesB);
        result.jaccardScore = score.jaccard;
        result.directionalScoreAtoB = score.directionalAtoB;
        result.directionalScoreBtoA = score.directionalBtoA;
        result.matchingNgramCount = score.intersectionCount;
        result.uniqueNgramsA = score.uniqueA;
        result.uniqueNgramsB = score.uniqueB;

        // 3. Rabin-Karp Passage Detection with Deep Token Verification
        dsa::RollingHash rh(ngramWindow);
        ExactMatchDetector exactDetector(minPassageLength);
        result.matchingPassages = exactDetector.detect(docA, docB, ngramWindow, rh);
        result.matchingPassageCount = result.matchingPassages.size();

        // 4. Compute Paragraph-Level Heatmap for Document A
        result.paragraphHeatmapA.resize(docA.paragraphs.size(), 0.0);
        if (!docA.paragraphs.empty() && !result.matchingPassages.empty()) {
            for (size_t p = 0; p < docA.paragraphs.size(); ++p) {
                size_t pMatchedTokens = 0;
                size_t pTotalTokens = 0;

                // Count tokens in this paragraph
                for (size_t t = 0; t < docA.tokens.size(); ++t) {
                    if (t < docA.tokenParagraphs.size() && docA.tokenParagraphs[t] == static_cast<int>(p)) {
                        pTotalTokens++;
                        // Check if token falls within any matching passage
                        for (const auto& match : result.matchingPassages) {
                            if (t >= match.sourceStartToken && t < match.sourceEndToken) {
                                pMatchedTokens++;
                                break;
                            }
                        }
                    }
                }

                if (pTotalTokens > 0) {
                    result.paragraphHeatmapA[p] = (static_cast<double>(pMatchedTokens) / static_cast<double>(pTotalTokens)) * 100.0;
                }
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.processingTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        return result;
    }
};

} // namespace plagiguard::detector

