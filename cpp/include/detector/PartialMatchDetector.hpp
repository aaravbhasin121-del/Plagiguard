#pragma once

#include "dsa/NGramGenerator.hpp"
#include "dsa/CustomHashTable.hpp"
#include "model/Document.hpp"
#include "model/MatchResult.hpp"
#include <vector>

namespace plagiguard::detector {

/**
 * @brief PartialMatchDetector - Detects shared N-grams and scattered matching phrases.
 */
class PartialMatchDetector {
private:
    size_t ngramSize;

public:
    explicit PartialMatchDetector(size_t n = 4)
        : ngramSize(n) {}

    std::vector<model::MatchResult> detect(
        const model::Document& docA,
        const model::Document& docB) {

        std::vector<model::MatchResult> results;
        dsa::NGramGenerator generator(ngramSize);

        std::vector<dsa::NGramEntry> ngramsA = generator.generate(docA.tokens, docA.tokenCharOffsets, docA.tokenParagraphs);
        std::vector<dsa::NGramEntry> ngramsB = generator.generate(docB.tokens, docB.tokenCharOffsets, docB.tokenParagraphs);

        if (ngramsA.empty() || ngramsB.empty()) return results;

        // Index B's n-grams using CustomHashTable
        dsa::CustomHashTable<std::string, std::vector<size_t>> indexB(ngramsB.size() * 2 + 16);
        for (size_t i = 0; i < ngramsB.size(); ++i) {
            std::vector<size_t>* pos = indexB.find(ngramsB[i].text);
            if (pos != nullptr) {
                pos->push_back(i);
            } else {
                std::vector<size_t> list = {i};
                indexB.put(ngramsB[i].text, list);
            }
        }

        // Compare A's n-grams against B
        for (size_t i = 0; i < ngramsA.size(); ++i) {
            const std::vector<size_t>* matchesInB = indexB.find(ngramsA[i].text);
            if (matchesInB != nullptr) {
                for (size_t posB : *matchesInB) {
                    model::MatchResult match;
                    match.sourceDocId = docA.id;
                    match.sourceDocName = docA.name;
                    match.targetDocId = docB.id;
                    match.targetDocName = docB.name;
                    match.sourceStartToken = ngramsA[i].startTokenIndex;
                    match.sourceEndToken = ngramsA[i].endTokenIndex;
                    match.targetStartToken = ngramsB[posB].startTokenIndex;
                    match.targetEndToken = ngramsB[posB].endTokenIndex;
                    match.sourceCharStart = ngramsA[i].charStart;
                    match.sourceCharEnd = ngramsA[i].charEnd;
                    match.targetCharStart = ngramsB[posB].charStart;
                    match.targetCharEnd = ngramsB[posB].charEnd;
                    match.matchingText = ngramsA[i].text;
                    match.matchLength = ngramSize;
                    match.type = model::MatchType::PARTIAL;
                    match.similarityContribution = (static_cast<double>(ngramSize) / static_cast<double>(docA.tokens.size() > 0 ? docA.tokens.size() : 1)) * 100.0;
                    results.push_back(match);
                }
            }
        }

        return results;
    }
};

} // namespace plagiguard::detector

