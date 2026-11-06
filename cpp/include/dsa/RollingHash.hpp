#pragma once

#include "CustomHashTable.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace plagiguard::dsa {

/**
 * @brief RollingHash - Implements polynomial rolling double-hash for Rabin-Karp.
 *
 * For a sequence of tokens [x_0, x_1, ..., x_{k-1}]:
 *   H = (x_0 * B^(k-1) + x_1 * B^(k-2) + ... + x_{k-1}) mod P
 *
 * Window Slide (outgoing x_0, incoming x_k):
 *   H_next = ((H - x_0 * B^(k-1)) * B + x_k) mod P
 *
 * Double Hashing:
 * - Modulus 1: 1,000,000,007 (Prime)
 * - Modulus 2: 998,244,353 (Prime)
 * - Base: 31 (Prime)
 *
 * Double hashing drastically reduces collision probability to near 1 / (MOD1 * MOD2) ~ 10^-18.
 *
 * Time Complexity:
 * - Initial window hash: O(k) where k is window size
 * - Window slide:        O(1)
 * Space Complexity:      O(V) where V is unique vocabulary size for token mapping
 */
class RollingHash {
public:
    static constexpr uint64_t BASE = 31ULL;
    static constexpr uint64_t MOD1 = 1000000007ULL;
    static constexpr uint64_t MOD2 = 998244353ULL;

    struct HashPair {
        uint64_t h1;
        uint64_t h2;

        bool operator==(const HashPair& other) const {
            return h1 == other.h1 && h2 == other.h2;
        }

        bool operator<(const HashPair& other) const {
            if (h1 != other.h1) return h1 < other.h1;
            return h2 < other.h2;
        }
    };

private:
    size_t windowSize;
    uint64_t basePow1;
    uint64_t basePow2;
    CustomHashTable<std::string, uint32_t> tokenDict;
    uint32_t nextTokenId;

    static uint64_t modPow(uint64_t base, size_t exp, uint64_t mod) {
        uint64_t res = 1;
        base %= mod;
        while (exp > 0) {
            if (exp % 2 == 1) res = (res * base) % mod;
            base = (base * base) % mod;
            exp /= 2;
        }
        return res;
    }

public:
    explicit RollingHash(size_t window = 5)
        : windowSize(window), nextTokenId(1) {
        setWindowSize(window);
    }

    void setWindowSize(size_t window) {
        windowSize = window > 0 ? window : 1;
        basePow1 = modPow(BASE, windowSize - 1, MOD1);
        basePow2 = modPow(BASE, windowSize - 1, MOD2);
    }

    size_t getWindowSize() const {
        return windowSize;
    }

    uint32_t getOrAssignTokenId(const std::string& token) {
        uint32_t id = 0;
        if (tokenDict.get(token, id)) {
            return id;
        }
        id = nextTokenId++;
        tokenDict.put(token, id);
        return id;
    }

    std::vector<uint32_t> mapTokensToIds(const std::vector<std::string>& tokens) {
        std::vector<uint32_t> ids;
        ids.reserve(tokens.size());
        for (const auto& tok : tokens) {
            ids.push_back(getOrAssignTokenId(tok));
        }
        return ids;
    }

    HashPair computeInitialHash(const std::vector<uint32_t>& ids, size_t start = 0) const {
        uint64_t h1 = 0;
        uint64_t h2 = 0;
        size_t end = start + windowSize;
        if (end > ids.size()) end = ids.size();

        for (size_t i = start; i < end; ++i) {
            h1 = (h1 * BASE + ids[i]) % MOD1;
            h2 = (h2 * BASE + ids[i]) % MOD2;
        }
        return {h1, h2};
    }

    HashPair roll(const HashPair& current, uint32_t outgoingId, uint32_t incomingId) const {
        // H_new = ((H - outgoing * B^(k-1)) * B + incoming) mod P
        // Using unsigned arithmetic: add mod before subtracting to avoid underflow
        uint64_t sub1 = (outgoingId * basePow1) % MOD1;
        uint64_t h1 = (current.h1 + MOD1 - sub1) % MOD1;
        h1 = (h1 * BASE + incomingId) % MOD1;

        uint64_t sub2 = (outgoingId * basePow2) % MOD2;
        uint64_t h2 = (current.h2 + MOD2 - sub2) % MOD2;
        h2 = (h2 * BASE + incomingId) % MOD2;

        return {h1, h2};
    }

    std::vector<HashPair> computeAllHashes(const std::vector<uint32_t>& ids) const {
        std::vector<HashPair> hashes;
        if (ids.size() < windowSize) {
            if (!ids.empty()) {
                hashes.push_back(computeInitialHash(ids, 0));
            }
            return hashes;
        }

        hashes.reserve(ids.size() - windowSize + 1);
        HashPair curr = computeInitialHash(ids, 0);
        hashes.push_back(curr);

        for (size_t i = 1; i <= ids.size() - windowSize; ++i) {
            uint32_t outgoing = ids[i - 1];
            uint32_t incoming = ids[i + windowSize - 1];
            curr = roll(curr, outgoing, incoming);
            hashes.push_back(curr);
        }

        return hashes;
    }
};

} // namespace plagiguard::dsa

namespace std {
template <>
struct hash<plagiguard::dsa::RollingHash::HashPair> {
    size_t operator()(const plagiguard::dsa::RollingHash::HashPair& p) const noexcept {
        return static_cast<size_t>(p.h1 ^ (p.h2 << 1));
    }
};
} // namespace std

