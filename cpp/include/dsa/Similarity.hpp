#pragma once

#include "CustomHashTable.hpp"
#include <string>
#include <vector>
#include <cmath>

namespace plagiguard::dsa {

/**
 * @brief SimilarityMetrics - Implements mathematical similarity scores.
 *
 * Implements:
 * 1. Jaccard Similarity:
 *    J(A, B) = (|A ∩ B| / |A ∪ B|) * 100
 *    Symmetric coefficient measuring the proportion of shared shingles across both documents.
 *
 * 2. Directional Overlap:
 *    O(A, B) = (|A ∩ B| / |A|) * 100
 *    Asymmetric coefficient measuring what percentage of Document A is contained in Document B.
 *
 * Academic Ethics Notice:
 * These coefficients measure textual similarity only. They do not constitute legal or
 * definitive proof of plagiarism, which requires human academic evaluation.
 */
class SimilarityMetrics {
public:
    struct SimilarityScore {
        double jaccard;          // [0.0 - 100.0]
        double directionalAtoB;  // [0.0 - 100.0]
        double directionalBtoA;  // [0.0 - 100.0]
        size_t intersectionCount;
        size_t unionCount;
        size_t uniqueA;
        size_t uniqueB;
    };

    /**
     * @brief Computes similarity metrics using CustomHashTable for set operations.
     */
    static SimilarityScore compute(const std::vector<std::string>& shinglesA,
                                   const std::vector<std::string>& shinglesB) {
        SimilarityScore score{0.0, 0.0, 0.0, 0, 0, 0, 0};

        if (shinglesA.empty() && shinglesB.empty()) {
            return score; // Both empty -> 0.0 similarity, safe division
        }

        // Build unique set A using CustomHashTable
        CustomHashTable<std::string, bool> setA(shinglesA.size() * 2 + 16);
        for (const auto& s : shinglesA) {
            setA.put(s, true);
        }

        // Build unique set B using CustomHashTable
        CustomHashTable<std::string, bool> setB(shinglesB.size() * 2 + 16);
        for (const auto& s : shinglesB) {
            setB.put(s, true);
        }

        score.uniqueA = setA.size();
        score.uniqueB = setB.size();

        if (score.uniqueA == 0 || score.uniqueB == 0) {
            score.unionCount = score.uniqueA + score.uniqueB;
            return score;
        }

        // Calculate intersection: shingles present in both A and B
        std::vector<std::string> keysA = setA.keys();
        size_t intersection = 0;
        for (const auto& key : keysA) {
            if (setB.contains(key)) {
                intersection++;
            }
        }

        score.intersectionCount = intersection;
        // |A ∪ B| = |A| + |B| - |A ∩ B|
        score.unionCount = score.uniqueA + score.uniqueB - intersection;

        if (score.unionCount > 0) {
            score.jaccard = (static_cast<double>(intersection) / static_cast<double>(score.unionCount)) * 100.0;
        }

        if (score.uniqueA > 0) {
            score.directionalAtoB = (static_cast<double>(intersection) / static_cast<double>(score.uniqueA)) * 100.0;
        }

        if (score.uniqueB > 0) {
            score.directionalBtoA = (static_cast<double>(intersection) / static_cast<double>(score.uniqueB)) * 100.0;
        }

        return score;
    }

    /**
     * @brief Computes cosine similarity between two TF-IDF term vectors.
     */
    static double computeCosine(const CustomHashTable<std::string, double>& vecA,
                                const CustomHashTable<std::string, double>& vecB) {
        double dotProduct = 0.0;
        double normA = 0.0;
        double normB = 0.0;

        for (const auto& key : vecA.keys()) {
            double valA = 0.0;
            vecA.get(key, valA);
            normA += valA * valA;

            double valB = 0.0;
            if (vecB.get(key, valB)) {
                dotProduct += valA * valB;
            }
        }

        for (const auto& key : vecB.keys()) {
            double valB = 0.0;
            vecB.get(key, valB);
            normB += valB * valB;
        }

        if (normA <= 1e-9 || normB <= 1e-9) return 0.0;
        return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
    }
};

} // namespace plagiguard::dsa

