#pragma once

#include <string>
#include <vector>
#include <sstream>

namespace plagiguard::dsa {

/**
 * @brief Represents a single N-gram with position and paragraph metadata.
 */
struct NGramEntry {
    std::vector<std::string> tokens;
    std::string text;
    size_t startTokenIndex;
    size_t endTokenIndex; // exclusive
    size_t charStart;
    size_t charEnd;
    int paragraphIndex;

    NGramEntry()
        : startTokenIndex(0), endTokenIndex(0), charStart(0), charEnd(0), paragraphIndex(0) {}

    NGramEntry(const std::vector<std::string>& toks,
               size_t startTok, size_t endTok,
               size_t cStart = 0, size_t cEnd = 0,
               int pIdx = 0)
        : tokens(toks), startTokenIndex(startTok), endTokenIndex(endTok),
          charStart(cStart), charEnd(cEnd), paragraphIndex(pIdx) {
        std::ostringstream oss;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (i > 0) oss << " ";
            oss << tokens[i];
        }
        text = oss.str();
    }
};

/**
 * @brief NGramGenerator - Generates contiguous sequences of N word tokens.
 *
 * Requirements:
 * - Configurable window size (default 5)
 * - Preserves original token and character positions
 * - Maps to paragraph indices
 * - Handles documents shorter than N safely
 */
class NGramGenerator {
private:
    size_t n;

public:
    explicit NGramGenerator(size_t windowSize = 5)
        : n(windowSize > 0 ? windowSize : 5) {}

    void setN(size_t windowSize) {
        n = windowSize > 0 ? windowSize : 1;
    }

    size_t getN() const {
        return n;
    }

    std::vector<NGramEntry> generate(const std::vector<std::string>& tokens,
                                     const std::vector<std::pair<size_t, size_t>>& charOffsets = {},
                                     const std::vector<int>& tokenParagraphs = {}) const {
        std::vector<NGramEntry> result;
        if (tokens.empty()) return result;

        if (tokens.size() < n) {
            // Document shorter than window: produce single partial N-gram
            std::vector<std::string> slice = tokens;
            size_t cStart = charOffsets.empty() ? 0 : charOffsets.front().first;
            size_t cEnd = charOffsets.empty() ? 0 : charOffsets.back().second;
            int pIdx = tokenParagraphs.empty() ? 0 : tokenParagraphs.front();
            result.emplace_back(slice, 0, tokens.size(), cStart, cEnd, pIdx);
            return result;
        }

        result.reserve(tokens.size() - n + 1);
        for (size_t i = 0; i <= tokens.size() - n; ++i) {
            std::vector<std::string> slice(tokens.begin() + i, tokens.begin() + i + n);
            size_t cStart = (charOffsets.size() > i) ? charOffsets[i].first : 0;
            size_t cEnd = (charOffsets.size() >= i + n) ? charOffsets[i + n - 1].second : 0;
            int pIdx = (tokenParagraphs.size() > i) ? tokenParagraphs[i] : 0;
            result.emplace_back(slice, i, i + n, cStart, cEnd, pIdx);
        }

        return result;
    }

    std::vector<std::string> generateShingles(const std::vector<std::string>& tokens) const {
        std::vector<std::string> shingles;
        if (tokens.empty()) return shingles;

        if (tokens.size() < n) {
            std::ostringstream oss;
            for (size_t i = 0; i < tokens.size(); ++i) {
                if (i > 0) oss << " ";
                oss << tokens[i];
            }
            shingles.push_back(oss.str());
            return shingles;
        }

        shingles.reserve(tokens.size() - n + 1);
        for (size_t i = 0; i <= tokens.size() - n; ++i) {
            std::ostringstream oss;
            for (size_t j = 0; j < n; ++j) {
                if (j > 0) oss << " ";
                oss << tokens[i + j];
            }
            shingles.push_back(oss.str());
        }

        return shingles;
    }
};

} // namespace plagiguard::dsa

