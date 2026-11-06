#pragma once

#include "dsa/CustomHashTable.hpp"
#include "dsa/Similarity.hpp"
#include "model/Document.hpp"
#include "processing/TextPreprocessor.hpp"
#include <string>
#include <vector>

namespace plagiguard::detector {

struct ParaphraseCandidate {
    std::string sentenceA;
    std::string sentenceB;
    double cosineSimilarity; // [0.0 - 1.0]
    size_t indexA;
    size_t indexB;
};

/**
 * @brief ParaphraseAnalyzer - Computes statistical term-weight similarity between sentences.
 *
 * Notice:
 * This module identifies candidate sentence pairs with high TF-IDF cosine similarity
 * despite different word order. It does not claim full semantic neural comprehension.
 */
class ParaphraseAnalyzer {
private:
    double similarityThreshold;

public:
    explicit ParaphraseAnalyzer(double threshold = 0.65)
        : similarityThreshold(threshold) {}

    std::vector<ParaphraseCandidate> analyze(
        const model::Document& docA,
        const model::Document& docB) {

        std::vector<ParaphraseCandidate> candidates;
        if (docA.paragraphs.empty() || docB.paragraphs.empty()) return candidates;

        std::vector<std::vector<std::string>> allDocs = {docA.tokens, docB.tokens};

        for (size_t i = 0; i < docA.paragraphs.size(); ++i) {
            std::vector<std::string> pATokens;
            auto detailedA = processing::TextPreprocessor::tokenizeDetailed(docA.paragraphs[i]);
            for (const auto& t : detailedA) pATokens.push_back(t.text);

            auto tfIdfA = processing::TextPreprocessor::computeTfIdf(pATokens, allDocs);

            for (size_t j = 0; j < docB.paragraphs.size(); ++j) {
                std::vector<std::string> pBTokens;
                auto detailedB = processing::TextPreprocessor::tokenizeDetailed(docB.paragraphs[j]);
                for (const auto& t : detailedB) pBTokens.push_back(t.text);

                auto tfIdfB = processing::TextPreprocessor::computeTfIdf(pBTokens, allDocs);
                double cosine = dsa::SimilarityMetrics::computeCosine(tfIdfA, tfIdfB);

                if (cosine >= similarityThreshold) {
                    candidates.push_back({docA.paragraphs[i], docB.paragraphs[j], cosine, i, j});
                }
            }
        }

        return candidates;
    }
};

} // namespace plagiguard::detector

