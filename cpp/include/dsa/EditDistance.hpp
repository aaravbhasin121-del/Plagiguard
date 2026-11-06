#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace plagiguard::dsa {

/**
 * @brief Operation types for sequence alignment.
 */
enum class EditOpType {
    MATCH,
    INSERT,
    DELETE,
    REPLACE
};

struct EditOp {
    EditOpType type;
    std::string tokenA;
    std::string tokenB;
    size_t indexA;
    size_t indexB;
};

struct AlignmentResult {
    size_t distance;
    double similarity; // 1.0 - (distance / max(lenA, lenB))
    std::vector<EditOp> operations;
};

/**
 * @brief EditDistance - Dynamic programming Levenshtein distance and sequence alignment.
 *
 * Recurrence relation:
 *   dp[i][j] = min {
 *     dp[i-1][j] + 1                     (Deletion)
 *     dp[i][j-1] + 1                     (Insertion)
 *     dp[i-1][j-1] + (A[i-1]==B[j-1]? 0 : 1) (Match / Replace)
 *   }
 *
 * Complexity:
 * - Time: O(M * N)
 * - Space: O(M * N) for alignment traceback, O(min(M, N)) for scalar distance
 */
class EditDistance {
public:
    static size_t computeDistance(const std::vector<std::string>& a, const std::vector<std::string>& b) {
        size_t m = a.size();
        size_t n = b.size();

        if (m == 0) return n;
        if (n == 0) return m;

        std::vector<size_t> prev(n + 1);
        std::vector<size_t> curr(n + 1);

        for (size_t j = 0; j <= n; ++j) prev[j] = j;

        for (size_t i = 1; i <= m; ++i) {
            curr[0] = i;
            for (size_t j = 1; j <= n; ++j) {
                if (a[i - 1] == b[j - 1]) {
                    curr[j] = prev[j - 1];
                } else {
                    curr[j] = 1 + std::min({prev[j], curr[j - 1], prev[j - 1]});
                }
            }
            prev = curr;
        }

        return prev[n];
    }

    static AlignmentResult align(const std::vector<std::string>& a, const std::vector<std::string>& b) {
        size_t m = a.size();
        size_t n = b.size();

        AlignmentResult result{0, 1.0, {}};

        if (m == 0 && n == 0) return result;

        // 2D DP Table for backtracking
        std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1, 0));

        for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
        for (size_t j = 0; j <= n; ++j) dp[0][j] = j;

        for (size_t i = 1; i <= m; ++i) {
            for (size_t j = 1; j <= n; ++j) {
                if (a[i - 1] == b[j - 1]) {
                    dp[i][j] = dp[i - 1][j - 1];
                } else {
                    dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
                }
            }
        }

        result.distance = dp[m][n];
        size_t maxLen = std::max(m, n);
        result.similarity = maxLen == 0 ? 1.0 : (1.0 - static_cast<double>(result.distance) / static_cast<double>(maxLen));

        // Backtracking to reconstruct optimal alignment
        int i = static_cast<int>(m);
        int j = static_cast<int>(n);

        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
                result.operations.push_back({EditOpType::MATCH, a[i - 1], b[j - 1], static_cast<size_t>(i - 1), static_cast<size_t>(j - 1)});
                i--;
                j--;
            } else if (i > 0 && j > 0 && dp[i][j] == dp[i - 1][j - 1] + 1) {
                result.operations.push_back({EditOpType::REPLACE, a[i - 1], b[j - 1], static_cast<size_t>(i - 1), static_cast<size_t>(j - 1)});
                i--;
                j--;
            } else if (i > 0 && dp[i][j] == dp[i - 1][j] + 1) {
                result.operations.push_back({EditOpType::DELETE, a[i - 1], "", static_cast<size_t>(i - 1), static_cast<size_t>(j)});
                i--;
            } else {
                result.operations.push_back({EditOpType::INSERT, "", b[j - 1], static_cast<size_t>(i), static_cast<size_t>(j - 1)});
                j--;
            }
        }

        std::reverse(result.operations.begin(), result.operations.end());
        return result;
    }

    static size_t computeCharDistance(const std::string& a, const std::string& b) {
        std::vector<std::string> vA, vB;
        vA.reserve(a.size());
        vB.reserve(b.size());
        for (char c : a) vA.push_back(std::string(1, c));
        for (char c : b) vB.push_back(std::string(1, c));
        return computeDistance(vA, vB);
    }
};

} // namespace plagiguard::dsa

