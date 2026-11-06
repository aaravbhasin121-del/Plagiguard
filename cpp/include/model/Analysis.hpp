#pragma once

#include "Document.hpp"
#include "SimilarityResult.hpp"
#include <string>
#include <vector>

namespace plagiguard::model {

enum class AnalysisType {
    TWO_DOCUMENT,
    BATCH,
    LIBRARY_COMPARE,
    VERSION_COMPARE
};

/**
 * @brief Represents an overall analysis session.
 */
struct Analysis {
    int id;
    AnalysisType type;
    std::string typeString;
    std::string analysisDate;
    size_t documentCount;
    long long processingTimeMs;
    std::string status; // PENDING, RUNNING, COMPLETED, FAILED
    std::string errorMessage;
    std::vector<Document> documents;
    std::vector<SimilarityResult> results;

    Analysis()
        : id(0), type(AnalysisType::TWO_DOCUMENT),
          typeString("TWO_DOCUMENT"), documentCount(0),
          processingTimeMs(0), status("PENDING") {}

    double getAverageSimilarity() const {
        if (results.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& r : results) {
            sum += r.jaccardScore;
        }
        return sum / static_cast<double>(results.size());
    }

    size_t getTotalMatchingPassages() const {
        size_t total = 0;
        for (const auto& r : results) {
            total += r.matchingPassageCount;
        }
        return total;
    }
};

} // namespace plagiguard::model

