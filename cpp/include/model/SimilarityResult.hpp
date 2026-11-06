#pragma once

#include "MatchResult.hpp"
#include <string>
#include <vector>

namespace plagiguard::model {

/**
 * @brief Complete similarity result between two documents.
 */
struct SimilarityResult {
    int id;
    int analysisId;
    int docAId;
    std::string docAName;
    int docBId;
    std::string docBName;
    double jaccardScore;        // [0.0 - 100.0]
    double directionalScoreAtoB; // [0.0 - 100.0]
    double directionalScoreBtoA; // [0.0 - 100.0]
    size_t matchingNgramCount;
    size_t uniqueNgramsA;
    size_t uniqueNgramsB;
    size_t totalNgramsA;
    size_t totalNgramsB;
    size_t matchingPassageCount;
    long long processingTimeMs;
    std::vector<MatchResult> matchingPassages;
    std::vector<double> paragraphHeatmapA; // Overlap ratio per paragraph of Doc A

    SimilarityResult()
        : id(0), analysisId(0), docAId(0), docBId(0),
          jaccardScore(0.0), directionalScoreAtoB(0.0), directionalScoreBtoA(0.0),
          matchingNgramCount(0), uniqueNgramsA(0), uniqueNgramsB(0),
          totalNgramsA(0), totalNgramsB(0), matchingPassageCount(0),
          processingTimeMs(0) {}

    std::string getInterpretation() const {
        if (jaccardScore >= 70.0) {
            return "High Overlap: Substantial portions of textual content are shared.";
        } else if (jaccardScore >= 40.0) {
            return "Moderate Overlap: Noticeable shared phrasing and passages detected.";
        } else if (jaccardScore >= 15.0) {
            return "Low Overlap: Minor shared terminology or standard idioms detected.";
        } else {
            return "Minimal Overlap: Little to no significant textual similarity detected.";
        }
    }
};

} // namespace plagiguard::model

