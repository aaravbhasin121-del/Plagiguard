#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace plagiguard::dsa {

/**
 * @brief Difference change types for version comparison.
 */
enum class DiffType {
    UNCHANGED,
    ADDED,
    REMOVED
};

struct DiffEntry {
    DiffType type;
    std::string text;
    size_t indexA;
    size_t indexB;
};

struct VersionDiffResult {
    size_t lcsLength;
    double similarityRatio; // (2.0 * lcs) / (lenA + lenB)
    size_t additions;
    size_t deletions;
    size_t unchanged;
    std::vector<DiffEntry> diffs;
};

/**
 * @brief LCS - Longest Common Subsequence dynamic programming for version comparison.
 *
 * Recurrence relation:
 *   L[i][j] = L[i-1][j-1] + 1                     if A[i-1] == B[j-1]
 *   L[i][j] = max(L[i-1][j], L[i][j-1])          if A[i-1] != B[j-1]
 *
 * Traceback constructs diff blocks:
 * - ADDED: Present in Version B, absent in Version A
 * - REMOVED: Present in Version A, absent in Version B
 * - UNCHANGED: Part of the common subsequence
 *
 * Complexity:
 * - Time: O(M * N)
 * - Space: O(M * N)
 */
class LCS {
public:
    static size_t computeLength(const std::vector<std::string>& a, const std::vector<std::string>& b) {
        size_t m = a.size();
        size_t n = b.size();

        if (m == 0 || n == 0) return 0;

        std::vector<size_t> prev(n + 1, 0);
        std::vector<size_t> curr(n + 1, 0);

        for (size_t i = 1; i <= m; ++i) {
            for (size_t j = 1; j <= n; ++j) {
                if (a[i - 1] == b[j - 1]) {
                    curr[j] = prev[j - 1] + 1;
                } else {
                    curr[j] = std::max(prev[j], curr[j - 1]);
                }
            }
            prev = curr;
        }

        return prev[n];
    }

    static VersionDiffResult computeDiff(const std::vector<std::string>& a, const std::vector<std::string>& b) {
        size_t m = a.size();
        size_t n = b.size();

        VersionDiffResult result{0, 0.0, 0, 0, 0, {}};

        std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1, 0));

        for (size_t i = 1; i <= m; ++i) {
            for (size_t j = 1; j <= n; ++j) {
                if (a[i - 1] == b[j - 1]) {
                    dp[i][j] = dp[i - 1][j - 1] + 1;
                } else {
                    dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
                }
            }
        }

        result.lcsLength = dp[m][n];
        size_t totalTokens = m + n;
        result.similarityRatio = totalTokens == 0 ? 1.0 : (2.0 * static_cast<double>(result.lcsLength)) / static_cast<double>(totalTokens);

        // Backtrack to build diff
        int i = static_cast<int>(m);
        int j = static_cast<int>(n);

        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
                result.diffs.push_back({DiffType::UNCHANGED, a[i - 1], static_cast<size_t>(i - 1), static_cast<size_t>(j - 1)});
                result.unchanged++;
                i--;
                j--;
            } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
                result.diffs.push_back({DiffType::ADDED, b[j - 1], static_cast<size_t>(i), static_cast<size_t>(j - 1)});
                result.additions++;
                j--;
            } else if (i > 0 && (j == 0 || dp[i][j - 1] < dp[i - 1][j])) {
                result.diffs.push_back({DiffType::REMOVED, a[i - 1], static_cast<size_t>(i - 1), static_cast<size_t>(j)});
                result.deletions++;
                i--;
            }
        }

        std::reverse(result.diffs.begin(), result.diffs.end());
        return result;
    }
};

} // namespace plagiguard::dsa

