#pragma once

#include "dsa/RabinKarp.hpp"
#include "dsa/RollingHash.hpp"
#include "model/Document.hpp"
#include "model/MatchResult.hpp"
#include <vector>

namespace plagiguard::detector {

/**
 * @brief ExactMatchDetector - Identifies identical text passages of length >= minMatchLength.
 */
class ExactMatchDetector {
private:
    size_t minMatchLength; // in tokens

public:
    explicit ExactMatchDetector(size_t minLength = 6)
        : minMatchLength(minLength) {}

    std::vector<model::MatchResult> detect(
        const model::Document& docA,
        const model::Document& docB,
        size_t windowSize,
        dsa::RollingHash& rh) {

        std::vector<model::MatchResult> results;
        std::vector<dsa::MatchSpan> spans = dsa::RabinKarp::findMatches(
            docA.tokens, docB.tokens, windowSize, rh);

        for (const auto& span : spans) {
            if (span.length < minMatchLength) continue;

            model::MatchResult match;
            match.sourceDocId = docA.id;
            match.sourceDocName = docA.name;
            match.targetDocId = docB.id;
            match.targetDocName = docB.name;
            match.sourceStartToken = span.startA;
            match.sourceEndToken = span.endA;
            match.targetStartToken = span.startB;
            match.targetEndToken = span.endB;

            // Compute exact character coordinates
            if (span.startA < docA.tokenCharOffsets.size()) {
                match.sourceCharStart = docA.tokenCharOffsets[span.startA].first;
            }
            if (span.endA > 0 && span.endA - 1 < docA.tokenCharOffsets.size()) {
                match.sourceCharEnd = docA.tokenCharOffsets[span.endA - 1].second;
            }

            if (span.startB < docB.tokenCharOffsets.size()) {
                match.targetCharStart = docB.tokenCharOffsets[span.startB].first;
            }
            if (span.endB > 0 && span.endB - 1 < docB.tokenCharOffsets.size()) {
                match.targetCharEnd = docB.tokenCharOffsets[span.endB - 1].second;
            }

            match.matchingText = span.text;
            match.matchLength = span.length;
            match.type = model::MatchType::EXACT;
            match.similarityContribution = (static_cast<double>(span.length) / static_cast<double>(docA.tokens.size() > 0 ? docA.tokens.size() : 1)) * 100.0;

            results.push_back(match);
        }

        return results;
    }
};

} // namespace plagiguard::detector

