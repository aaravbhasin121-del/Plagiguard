#pragma once

#include "RollingHash.hpp"
#include "CustomHashTable.hpp"
#include <string>
#include <vector>
#include <sstream>

namespace plagiguard::dsa {

/**
 * @brief Represents a matched span between Document A and Document B.
 */
struct MatchSpan {
    size_t startA;
    size_t endA; // exclusive
    size_t startB;
    size_t endB; // exclusive
    size_t length;
    std::string text;
    bool verified;

    MatchSpan() : startA(0), endA(0), startB(0), endB(0), length(0), verified(false) {}

    MatchSpan(size_t sA, size_t eA, size_t sB, size_t eB, size_t len, const std::string& txt, bool v = true)
        : startA(sA), endA(eA), startB(sB), endB(eB), length(len), text(txt), verified(v) {}
};

/**
 * @brief RabinKarp - Pattern matching using polynomial rolling double hashes.
 *
 * Algorithm Design:
 * 1. Compute rolling hashes for all sliding windows of size K in Document A.
 * 2. Store Document A's hashes in CustomHashTable<RollingHash::HashPair, std::vector<size_t>>.
 * 3. Compute rolling hashes for Document B.
 * 4. For each hash in B that matches a hash in A, perform deep token verification
 *    to eliminate spurious hash collisions.
 * 5. Coalesce adjacent and overlapping matches into maximal contiguous passages.
 *
 * Complexity:
 * - Expected Time: O(N + M) where N = length(A), M = length(B)
 * - Worst Case Time: O(N * M) (if every hash collides and requires verification)
 * - Space Complexity: O(N) to index Document A's hashes
 */
class RabinKarp {
private:
    static bool verifyTokens(const std::vector<std::string>& tokensA, size_t startA,
                             const std::vector<std::string>& tokensB, size_t startB,
                             size_t len) {
        if (startA + len > tokensA.size() || startB + len > tokensB.size()) return false;
        for (size_t i = 0; i < len; ++i) {
            if (tokensA[startA + i] != tokensB[startB + i]) {
                return false;
            }
        }
        return true;
    }

    static std::string joinTokens(const std::vector<std::string>& tokens, size_t start, size_t len) {
        std::ostringstream oss;
        for (size_t i = 0; i < len && start + i < tokens.size(); ++i) {
            if (i > 0) oss << " ";
            oss << tokens[start + i];
        }
        return oss.str();
    }

public:
    static std::vector<MatchSpan> findMatches(
        const std::vector<std::string>& tokensA,
        const std::vector<std::string>& tokensB,
        size_t windowSize,
        RollingHash& rh) {

        std::vector<MatchSpan> rawMatches;
        if (tokensA.empty() || tokensB.empty()) return rawMatches;

        size_t k = windowSize;
        if (tokensA.size() < k || tokensB.size() < k) {
            // Check if one is a sub-slice or identical when shorter than window
            size_t minLen = std::min(tokensA.size(), tokensB.size());
            if (minLen > 0 && verifyTokens(tokensA, 0, tokensB, 0, minLen)) {
                rawMatches.emplace_back(0, minLen, 0, minLen, minLen,
                                       joinTokens(tokensA, 0, minLen), true);
            }
            return rawMatches;
        }

        rh.setWindowSize(k);

        // Map tokens to integer IDs for fast arithmetic
        std::vector<uint32_t> idsA = rh.mapTokensToIds(tokensA);
        std::vector<uint32_t> idsB = rh.mapTokensToIds(tokensB);

        std::vector<RollingHash::HashPair> hashesA = rh.computeAllHashes(idsA);
        std::vector<RollingHash::HashPair> hashesB = rh.computeAllHashes(idsB);

        // Index A's hashes using CustomHashTable
        CustomHashTable<RollingHash::HashPair, std::vector<size_t>> indexA(hashesA.size() * 2);
        for (size_t i = 0; i < hashesA.size(); ++i) {
            std::vector<size_t>* positions = indexA.find(hashesA[i]);
            if (positions != nullptr) {
                positions->push_back(i);
            } else {
                std::vector<size_t> vec = {i};
                indexA.put(hashesA[i], vec);
            }
        }

        // Search B's hashes against A's index
        for (size_t j = 0; j < hashesB.size(); ++j) {
            const std::vector<size_t>* positionsInA = indexA.find(hashesB[j]);
            if (positionsInA != nullptr) {
                for (size_t posA : *positionsInA) {
                    // Critical Academic Requirement: Verify original tokens to handle collision
                    if (verifyTokens(tokensA, posA, tokensB, j, k)) {
                        rawMatches.emplace_back(posA, posA + k, j, j + k, k,
                                               joinTokens(tokensA, posA, k), true);
                    }
                }
            }
        }

        return coalesceMatches(rawMatches, tokensA, tokensB);
    }

    /**
     * @brief Coalesces adjacent matching windows into extended maximal passages.
     */
    static std::vector<MatchSpan> coalesceMatches(
        const std::vector<MatchSpan>& raw,
        const std::vector<std::string>& tokensA,
        [[maybe_unused]] const std::vector<std::string>& tokensB) {

        if (raw.empty()) return raw;

        // Group and merge consecutive matches where posA and posB step forward together
        std::vector<MatchSpan> merged;
        MatchSpan current = raw[0];

        for (size_t i = 1; i < raw.size(); ++i) {
            const auto& next = raw[i];
            // If next immediately extends current
            if (next.startA <= current.endA && next.startB <= current.endB &&
                (next.startA - current.startA == next.startB - current.startB)) {
                current.endA = std::max(current.endA, next.endA);
                current.endB = std::max(current.endB, next.endB);
                current.length = current.endA - current.startA;
            } else {
                current.text = joinTokens(tokensA, current.startA, current.length);
                merged.push_back(current);
                current = next;
            }
        }
        current.text = joinTokens(tokensA, current.startA, current.length);
        merged.push_back(current);

        return merged;
    }
};

} // namespace plagiguard::dsa
