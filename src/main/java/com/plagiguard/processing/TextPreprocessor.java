package com.plagiguard.processing;

import java.util.*;
import java.util.regex.*;

/**
 * Preprocesses extracted text for comparison.
 *
 * IMPORTANT: Normalization is done for COMPARISON purposes only.
 * Original text is always preserved separately for display and passage highlighting.
 *
 * Processing pipeline:
 * 1. Preserve original text
 * 2. Normalize whitespace
 * 3. Lowercase conversion
 * 4. Handle punctuation
 * 5. Tokenize
 * 6. Detect paragraph boundaries
 */
public class TextPreprocessor {

    // Common citation patterns: (Author, Year), [1], [Author 2020], etc.
    private static final Pattern CITATION_PATTERN = Pattern.compile(
        "\\(([A-Za-z]+(?:\\s+(?:et\\s+al\\.?|and|&)\\s+[A-Za-z]+)?(?:,\\s*\\d{4})?(?:,\\s*p\\.?\\s*\\d+)?)\\)" +
        "|\\[\\d+(?:[,–-]\\d+)*\\]" +
        "|\\[([A-Za-z]+(?:\\s+et\\s+al\\.?)?(?:,?\\s*\\d{4})?)\\]",
        Pattern.MULTILINE
    );

    // Bibliography section header patterns
    private static final Pattern BIBLIOGRAPHY_HEADER = Pattern.compile(
        "^\\s*(References|Bibliography|Works\\s+Cited|Sources|Literature\\s+Cited)\\s*$",
        Pattern.CASE_INSENSITIVE | Pattern.MULTILINE
    );

    // -----------------------------------------------------------------------
    // normalize
    // -----------------------------------------------------------------------

    /**
     * Normalize text for comparison (lowercase, normalized whitespace).
     * Does NOT remove stop words (to preserve positional accuracy).
     *
     * @param text raw extracted text
     * @return normalized text
     */
    public String normalize(String text) {
        if (text == null || text.isEmpty()) return "";

        // 1. Lowercase
        String result = text.toLowerCase(Locale.ROOT);

        // 2. Replace common ligatures / Unicode punctuation with ASCII equivalents
        result = result
            .replace('\u2018', '\'').replace('\u2019', '\'')   // curly single quotes
            .replace('\u201C', '"').replace('\u201D', '"')      // curly double quotes
            .replace('\u2013', '-').replace('\u2014', '-')      // en/em dash
            .replace('\u00A0', ' ')                             // non-breaking space
            .replace('\t', ' ');                                // tab -> space

        // 3. Remove punctuation except apostrophes inside words (contractions)
        //    Keep letters, digits, whitespace, and inner apostrophes
        result = result.replaceAll("[^a-z0-9'\\s]", " ");

        // 4. Remove apostrophes that are not surrounded by letters (stray ones)
        result = result.replaceAll("(?<![a-z])'|'(?![a-z])", " ");

        // 5. Collapse multiple whitespace (spaces, newlines) into single space
        result = result.replaceAll("\\s+", " ").strip();

        return result;
    }

    // -----------------------------------------------------------------------
    // tokenize
    // -----------------------------------------------------------------------

    /**
     * Tokenize normalized text into a word array.
     * Splits on whitespace, removes empty tokens.
     *
     * @param normalizedText preprocessed text
     * @return array of word tokens
     */
    public String[] tokenize(String normalizedText) {
        if (normalizedText == null || normalizedText.isBlank()) return new String[0];

        String[] raw = normalizedText.split("\\s+");
        List<String> tokens = new ArrayList<>(raw.length);
        for (String t : raw) {
            if (!t.isEmpty()) tokens.add(t);
        }
        return tokens.toArray(new String[0]);
    }

    // -----------------------------------------------------------------------
    // detectParagraphBoundaries
    // -----------------------------------------------------------------------

    /**
     * Detect paragraph boundaries in original text.
     * Paragraphs are separated by double newlines or single newlines followed by indent.
     *
     * @param tokens       token array (from normalized text)
     * @param originalText original raw text
     * @return list of int[]{startTokenIdx, endTokenIdx} for each paragraph
     */
    public List<int[]> detectParagraphBoundaries(String[] tokens, String originalText) {
        List<int[]> boundaries = new ArrayList<>();
        if (tokens == null || tokens.length == 0 || originalText == null || originalText.isEmpty()) {
            return boundaries;
        }

        // Split original text into paragraphs using double-newlines or newline+indent
        String[] paragraphs = originalText.split("\\n{2,}|\\n(?=[ \\t]+\\S)");

        // For each paragraph, figure out which token indices it covers by cumulative word count
        int tokenIndex = 0;
        for (String paragraph : paragraphs) {
            String trimmed = paragraph.strip();
            if (trimmed.isEmpty()) continue;

            // Normalize + tokenize this paragraph to know how many tokens it produces
            String normPara = normalize(trimmed);
            String[] paraTokens = tokenize(normPara);
            if (paraTokens.length == 0) continue;

            int startIdx = tokenIndex;
            int endIdx = Math.min(tokenIndex + paraTokens.length - 1, tokens.length - 1);
            boundaries.add(new int[]{startIdx, endIdx});
            tokenIndex += paraTokens.length;

            if (tokenIndex >= tokens.length) break;
        }

        // If paragraphs don't cover all tokens (e.g., single-newline document), add remainder
        if (tokenIndex < tokens.length && !boundaries.isEmpty()) {
            int[] last = boundaries.get(boundaries.size() - 1);
            last[1] = tokens.length - 1;
        } else if (boundaries.isEmpty() && tokens.length > 0) {
            boundaries.add(new int[]{0, tokens.length - 1});
        }

        return boundaries;
    }

    // -----------------------------------------------------------------------
    // getOriginalPassage
    // -----------------------------------------------------------------------

    /**
     * Get a substring of original text corresponding to token positions.
     * Used for highlighting matching passages.
     *
     * @param originalText full original text
     * @param tokens       all tokens (from normalized text)
     * @param startToken   start token index (inclusive)
     * @param endToken     end token index (inclusive)
     * @return substring of original text for those tokens (best-effort)
     */
    public String getOriginalPassage(String originalText, String[] tokens,
                                     int startToken, int endToken) {
        if (originalText == null || originalText.isEmpty()) return "";
        if (tokens == null || tokens.length == 0) return "";
        if (startToken < 0) startToken = 0;
        if (endToken >= tokens.length) endToken = tokens.length - 1;
        if (startToken > endToken) return "";

        // Build the token sequence we are looking for (use normalized form for searching)
        String startWord = tokens[startToken];
        String endWord   = tokens[endToken];

        // Search original text case-insensitively for the start word
        String lowerOriginal = originalText.toLowerCase(Locale.ROOT);

        // Find start character position by skipping `startToken` occurrences of words
        // We walk through the original text word-by-word counting matches
        int charPos = 0;
        int tokensCounted = 0;
        int charStart = -1;
        int charEnd = -1;

        // Simple approach: split original on whitespace preserving positions
        // We use a Matcher to find word boundaries
        Matcher wordMatcher = Pattern.compile("[\\S]+").matcher(originalText);
        int wordIndex = 0;
        while (wordMatcher.find()) {
            // Normalize this word for comparison
            String wordNorm = normalize(wordMatcher.group());
            String[] wordTokens = tokenize(wordNorm);
            if (wordTokens.length == 0) continue;

            if (tokensCounted == startToken) {
                charStart = wordMatcher.start();
            }
            tokensCounted += wordTokens.length;
            if (tokensCounted - 1 >= endToken) {
                charEnd = wordMatcher.end();
                break;
            }
        }

        if (charStart == -1) return "";
        if (charEnd == -1) charEnd = originalText.length();

        return originalText.substring(charStart, Math.min(charEnd, originalText.length())).strip();
    }

    // -----------------------------------------------------------------------
    // detectCitations
    // -----------------------------------------------------------------------

    /**
     * Detect citation/reference patterns in text.
     *
     * @param text original text
     * @return list of citation spans [start, end] char positions
     */
    public List<int[]> detectCitations(String text) {
        List<int[]> citations = new ArrayList<>();
        if (text == null || text.isEmpty()) return citations;

        Matcher m = CITATION_PATTERN.matcher(text);
        while (m.find()) {
            citations.add(new int[]{m.start(), m.end()});
        }
        return citations;
    }

    // -----------------------------------------------------------------------
    // isBibliographySection
    // -----------------------------------------------------------------------

    /**
     * Check if text appears to be a bibliography section.
     */
    public boolean isBibliographySection(String text) {
        if (text == null || text.isBlank()) return false;
        Matcher m = BIBLIOGRAPHY_HEADER.matcher(text);
        return m.find();
    }

    // -----------------------------------------------------------------------
    // computeTfIdf
    // -----------------------------------------------------------------------

    /**
     * Generate TF-IDF vector for a document's tokens.
     * Used in paraphrase analysis.
     *
     * @param tokens        document tokens
     * @param allDocTokens  all documents' token arrays (for IDF)
     * @return map of term to TF-IDF score
     */
    public Map<String, Double> computeTfIdf(String[] tokens, List<String[]> allDocTokens) {
        Map<String, Double> tfidf = new LinkedHashMap<>();
        if (tokens == null || tokens.length == 0) return tfidf;

        int totalTokens = tokens.length;
        int totalDocs = (allDocTokens != null) ? allDocTokens.size() : 1;

        // 1. Compute term frequency for this document
        Map<String, Integer> tf = new HashMap<>();
        for (String token : tokens) {
            tf.merge(token, 1, Integer::sum);
        }

        // 2. Compute IDF for each unique term
        for (Map.Entry<String, Integer> entry : tf.entrySet()) {
            String term = entry.getKey();
            double termFreq = (double) entry.getValue() / totalTokens;

            // Count documents containing this term
            int docsWithTerm = 0;
            if (allDocTokens != null) {
                for (String[] docTokens : allDocTokens) {
                    if (docTokens != null) {
                        for (String t : docTokens) {
                            if (term.equals(t)) { docsWithTerm++; break; }
                        }
                    }
                }
            } else {
                docsWithTerm = 1;
            }

            double idf = Math.log((double)(totalDocs + 1) / (docsWithTerm + 1)) + 1.0;
            tfidf.put(term, termFreq * idf);
        }

        return tfidf;
    }

    // -----------------------------------------------------------------------
    // cosineSimilarity
    // -----------------------------------------------------------------------

    /**
     * Compute cosine similarity between two TF-IDF vectors.
     *
     * @param vecA first TF-IDF map
     * @param vecB second TF-IDF map
     * @return cosine similarity in [0.0, 1.0]
     */
    public double cosineSimilarity(Map<String, Double> vecA, Map<String, Double> vecB) {
        if (vecA == null || vecB == null || vecA.isEmpty() || vecB.isEmpty()) return 0.0;

        // Dot product
        double dot = 0.0;
        for (Map.Entry<String, Double> entry : vecA.entrySet()) {
            Double bVal = vecB.get(entry.getKey());
            if (bVal != null) {
                dot += entry.getValue() * bVal;
            }
        }

        // Magnitudes
        double magA = 0.0;
        for (double v : vecA.values()) magA += v * v;
        magA = Math.sqrt(magA);

        double magB = 0.0;
        for (double v : vecB.values()) magB += v * v;
        magB = Math.sqrt(magB);

        if (magA == 0.0 || magB == 0.0) return 0.0;
        return dot / (magA * magB);
    }
}
