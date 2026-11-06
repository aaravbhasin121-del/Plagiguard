#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <algorithm>

namespace plagiguard::processing {

/**
 * @brief DocumentFingerprint - Generates unique signatures for documents.
 *
 * Implements:
 * 1. 64-bit Deterministic FNV-1a / Murmur-inspired rolling fingerprint.
 * 2. MinHash signature matrix: An array of K minimum hash values under different
 *    hash functions h_i(x) = (a_i * x + b_i) mod P.
 * 3. Fast Jaccard estimation: J(A, B) ≈ count(sigA[i] == sigB[i]) / K.
 *
 * Guarantees that identical documents produce identical fingerprints.
 */
class DocumentFingerprint {
public:
    static constexpr size_t NUM_HASH_FUNCTIONS = 100;
    static constexpr uint64_t LARGE_PRIME = 4294967311ULL; // 2^32 + 15

    static std::string generateFingerprint(const std::string& normalizedText) {
        // 64-bit FNV-1a hash
        uint64_t hash = 14695981039346656037ULL;
        for (char c : normalizedText) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }

        // Secondary hash to form 128-bit hex digest
        uint64_t hash2 = 0xcbf29ce484222325ULL;
        for (auto it = normalizedText.rbegin(); it != normalizedText.rend(); ++it) {
            hash2 ^= static_cast<uint64_t>(*it);
            hash2 *= 0x100000001b3ULL;
        }

        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(16) << hash
            << std::setw(16) << hash2;
        return oss.str();
    }

    /**
     * @brief Generates MinHash signature vector of size K.
     */
    static std::vector<uint32_t> generateMinHash(const std::vector<std::string>& shingles) {
        std::vector<uint32_t> signature(NUM_HASH_FUNCTIONS, UINT32_MAX);
        if (shingles.empty()) return signature;

        // Hash coefficients: a_i, b_i
        static const std::vector<std::pair<uint64_t, uint64_t>> hashCoeffs = []() {
            std::vector<std::pair<uint64_t, uint64_t>> coeffs;
            coeffs.reserve(NUM_HASH_FUNCTIONS);
            uint64_t a = 31ULL;
            uint64_t b = 17ULL;
            for (size_t i = 0; i < NUM_HASH_FUNCTIONS; ++i) {
                coeffs.push_back({a, b});
                a = (a * 6364136223846793005ULL + 1442695040888963407ULL) % 2147483647ULL;
                b = (b * 6364136223846793005ULL + 1442695040888963407ULL) % 2147483647ULL;
                if (a % 2 == 0) a += 1;
            }
            return coeffs;
        }();

        for (const auto& shingle : shingles) {
            // Hash shingle to 32-bit integer x
            uint32_t x = 2166136261U;
            for (char c : shingle) {
                x ^= static_cast<uint32_t>(c);
                x *= 16777619U;
            }

            for (size_t i = 0; i < NUM_HASH_FUNCTIONS; ++i) {
                uint32_t h = static_cast<uint32_t>((hashCoeffs[i].first * x + hashCoeffs[i].second) % LARGE_PRIME);
                if (h < signature[i]) {
                    signature[i] = h;
                }
            }
        }

        return signature;
    }

    /**
     * @brief Estimates Jaccard similarity from MinHash signatures in O(K) time.
     */
    static double estimateJaccard(const std::vector<uint32_t>& sigA, const std::vector<uint32_t>& sigB) {
        if (sigA.size() != sigB.size() || sigA.empty()) return 0.0;
        size_t matches = 0;
        for (size_t i = 0; i < sigA.size(); ++i) {
            if (sigA[i] == sigB[i] && sigA[i] != UINT32_MAX) {
                matches++;
            }
        }
        return (static_cast<double>(matches) / static_cast<double>(sigA.size())) * 100.0;
    }
};

} // namespace plagiguard::processing

