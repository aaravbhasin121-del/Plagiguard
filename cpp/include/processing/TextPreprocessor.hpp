#pragma once

#include "dsa/CustomHashTable.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <regex>
#include <cmath>

namespace plagiguard::processing {

struct TokenInfo {
    std::string text;
    size_t charStart;
    size_t charEnd;
    int paragraphIndex;
};

/**
 * @brief TextPreprocessor - Normalizes, tokenizes, and segments document text.
 *
 * Core Principles:
 * - Non-destructive normalization: Original text is preserved untouched.
 * - Exact position mapping: Every token retains its [charStart, charEnd) byte offsets
 *   in the original document so UI highlighting never drifts.
 * - Citation and Reference recognition: Separates cited quotes from uncited similarity.
 */
class TextPreprocessor {
public:
    static std::string normalize(const std::string& input) {
        std::string result;
        result.reserve(input.size());
        bool inSpace = false;

        for (char c : input) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (std::isspace(uc)) {
                if (!inSpace && !result.empty()) {
                    result.push_back(' ');
                    inSpace = true;
                }
            } else {
                result.push_back(static_cast<char>(std::tolower(uc)));
                inSpace = false;
            }
        }

        while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
            result.pop_back();
        }
        return result;
    }

    static std::vector<TokenInfo> tokenizeDetailed(const std::string& originalText) {
        std::vector<TokenInfo> tokens;
        size_t len = originalText.size();
        size_t i = 0;
        int currentParagraph = 0;

        while (i < len) {
            // Count paragraph breaks (two or more consecutive newlines)
            if (originalText[i] == '\n') {
                size_t nlCount = 0;
                while (i < len && (originalText[i] == '\n' || originalText[i] == '\r')) {
                    if (originalText[i] == '\n') nlCount++;
                    i++;
                }
                if (nlCount >= 2) {
                    currentParagraph++;
                }
                continue;
            }

            // Skip whitespace
            if (std::isspace(static_cast<unsigned char>(originalText[i]))) {
                i++;
                continue;
            }

            // Word token start
            size_t start = i;
            while (i < len && !std::isspace(static_cast<unsigned char>(originalText[i]))) {
                // If punctuation attaches to word, split if necessary or include alphanumeric
                i++;
            }
            size_t end = i;

            // Trim leading/trailing punctuation for normalized word comparison
            size_t wordStart = start;
            size_t wordEnd = end;

            while (wordStart < wordEnd && (std::ispunct(static_cast<unsigned char>(originalText[wordStart])) &&
                                          originalText[wordStart] != '-' && originalText[wordStart] != '_')) {
                wordStart++;
            }
            while (wordEnd > wordStart && (std::ispunct(static_cast<unsigned char>(originalText[wordEnd - 1])) &&
                                          originalText[wordEnd - 1] != '-' && originalText[wordEnd - 1] != '_')) {
                wordEnd--;
            }

            if (wordStart < wordEnd) {
                std::string word = originalText.substr(wordStart, wordEnd - wordStart);
                std::string normWord;
                normWord.reserve(word.size());
                for (char c : word) {
                    normWord.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
                }
                tokens.push_back({normWord, wordStart, wordEnd, currentParagraph});
            }
        }

        return tokens;
    }

    static std::vector<std::string> extractParagraphs(const std::string& text) {
        std::vector<std::string> paragraphs;
        std::istringstream stream(text);
        std::string line;
        std::string current;

        while (std::getline(stream, line)) {
            // Check if line is empty (paragraph delimiter)
            bool isEmpty = true;
            for (char c : line) {
                if (!std::isspace(static_cast<unsigned char>(c))) {
                    isEmpty = false;
                    break;
                }
            }

            if (isEmpty) {
                if (!current.empty()) {
                    paragraphs.push_back(current);
                    current.clear();
                }
            } else {
                if (!current.empty()) current += " ";
                current += line;
            }
        }
        if (!current.empty()) {
            paragraphs.push_back(current);
        }
        if (paragraphs.empty() && !text.empty()) {
            paragraphs.push_back(text);
        }
        return paragraphs;
    }

    /**
     * @brief Detects academic citations such as [1], [1, 2], (Smith et al., 2021).
     */
    static std::vector<std::pair<size_t, size_t>> detectCitations(const std::string& text) {
        std::vector<std::pair<size_t, size_t>> spans;
        // Bracketed numeric citations [1], [12, 14]
        std::regex bracketRegex(R"(\[\s*\d+(?:\s*,\s*\d+)*\s*\])");
        // Author-year citations (Author, 2020)
        std::regex authorYearRegex(R"(\([A-Za-z]+(?:\s+et\s+al\.)?,\s*\d{4}\))");

        auto findMatches = [&](const std::regex& re) {
            auto words_begin = std::sregex_iterator(text.begin(), text.end(), re);
            auto words_end = std::sregex_iterator();
            for (std::sregex_iterator it = words_begin; it != words_end; ++it) {
                std::smatch match = *it;
                spans.push_back({static_cast<size_t>(match.position()),
                                 static_cast<size_t>(match.position() + match.length())});
            }
        };

        findMatches(bracketRegex);
        findMatches(authorYearRegex);
        return spans;
    }

    /**
     * @brief Identifies whether a text block represents a Bibliography/References section.
     */
    static bool isBibliography(const std::string& text) {
        std::string lower = normalize(text);
        if (lower.find("references") != std::string::npos ||
            lower.find("bibliography") != std::string::npos ||
            lower.find("works cited") != std::string::npos) {
            return true;
        }
        return false;
    }

    /**
     * @brief Computes TF-IDF vector for a document against a collection.
     */
    static dsa::CustomHashTable<std::string, double> computeTfIdf(
        const std::vector<std::string>& docTokens,
        const std::vector<std::vector<std::string>>& allDocTokens) {

        dsa::CustomHashTable<std::string, double> tfIdf(docTokens.size() * 2 + 16);
        if (docTokens.empty()) return tfIdf;

        // 1. Term Frequency (TF)
        dsa::CustomHashTable<std::string, size_t> tf(docTokens.size() * 2 + 16);
        for (const auto& tok : docTokens) {
            size_t c = 0;
            tf.get(tok, c);
            tf.put(tok, c + 1);
        }

        size_t totalTokens = docTokens.size();
        size_t totalDocs = allDocTokens.empty() ? 1 : allDocTokens.size();

        // 2. Compute TF * IDF
        for (const auto& term : tf.keys()) {
            size_t count = 0;
            tf.get(term, count);
            double termFreq = static_cast<double>(count) / static_cast<double>(totalTokens);

            // Document Frequency (DF): in how many docs does term appear
            size_t docFreq = 0;
            for (const auto& otherDoc : allDocTokens) {
                for (const auto& otherTok : otherDoc) {
                    if (otherTok == term) {
                        docFreq++;
                        break;
                    }
                }
            }
            if (docFreq == 0) docFreq = 1;

            double idf = std::log((1.0 + static_cast<double>(totalDocs)) / (1.0 + static_cast<double>(docFreq))) + 1.0;
            tfIdf.put(term, termFreq * idf);
        }

        return tfIdf;
    }
};

} // namespace plagiguard::processing

