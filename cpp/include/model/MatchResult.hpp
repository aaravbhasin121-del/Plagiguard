#pragma once

#include <string>

namespace plagiguard::model {

enum class MatchType {
    EXACT,
    PARTIAL,
    CITATION,
    PARAPHRASE
};

/**
 * @brief Represents a matched passage identified between two documents.
 */
struct MatchResult {
    int id;
    int resultId;
    int sourceDocId;
    std::string sourceDocName;
    int targetDocId;
    std::string targetDocName;
    size_t sourceStartToken;
    size_t sourceEndToken;
    size_t targetStartToken;
    size_t targetEndToken;
    size_t sourceCharStart;
    size_t sourceCharEnd;
    size_t targetCharStart;
    size_t targetCharEnd;
    std::string matchingText;
    size_t matchLength;
    MatchType type;
    double similarityContribution;

    MatchResult()
        : id(0), resultId(0), sourceDocId(0), targetDocId(0),
          sourceStartToken(0), sourceEndToken(0),
          targetStartToken(0), targetEndToken(0),
          sourceCharStart(0), sourceCharEnd(0),
          targetCharStart(0), targetCharEnd(0),
          matchLength(0), type(MatchType::EXACT), similarityContribution(0.0) {}
};

} // namespace plagiguard::model

