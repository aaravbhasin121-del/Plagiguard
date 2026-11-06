package com.plagiguard.dsa;

import java.util.ArrayList;
import java.util.List;

/**
 * NGramGenerator - Generates n-grams (contiguous subsequences of tokens) from a token stream.
 *
 * <p>N-grams are the backbone of PlagiGuard's fingerprinting pipeline. A document is tokenised
 * into words, and overlapping windows of {@code n} consecutive words are extracted. These
 * windows are then hashed (via {@link RollingHash}) and stored in the {@link InvertedIndex}
 * for similarity lookup.
 *
 * <p><b>Example:</b> tokens = ["the", "quick", "brown", "fox"], n = 3
 * <pre>
 *   NGram 0: ["the",   "quick", "brown"]  positions 0..2
 *   NGram 1: ["quick", "brown", "fox"  ]  positions 1..3
 * </pre>
 *
 * <p><b>Time Complexity:</b> O(n * k) where n = token count, k = n-gram size (for join)
 * <p><b>Space Complexity:</b> O(n * k) for all generated n-gram strings
 */
public class NGramGenerator {

    /** Default n-gram size used when no explicit size is provided. */
    public static final int DEFAULT_N = 5;

    // -----------------------------------------------------------------------
    // Inner Classes
    // -----------------------------------------------------------------------

    /**
     * Holds an n-gram together with the position information needed to locate
     * it in the original document.
     */
    public static class NGramEntry {
        /** The individual tokens comprising this n-gram. */
        public final String[] tokens;
        /** Zero-based index of the first token in the source array. */
        public final int startPos;
        /** Zero-based index of the last token in the source array (inclusive). */
        public final int endPos;
        /** The tokens joined by a single space for convenient string operations. */
        public final String joined;

        /**
         * Constructs a new NGramEntry.
         *
         * @param tokens   the n-gram token array
         * @param startPos start index in the source token array
         * @param endPos   end index (inclusive) in the source token array
         */
        public NGramEntry(String[] tokens, int startPos, int endPos) {
            this.tokens = tokens;
            this.startPos = startPos;
            this.endPos = endPos;
            this.joined = String.join(" ", tokens);
        }

        @Override
        public String toString() {
            return String.format("NGramEntry{pos=%d..%d, text='%s'}", startPos, endPos, joined);
        }
    }

    // -----------------------------------------------------------------------
    // Core Generation Methods
    // -----------------------------------------------------------------------

    /**
     * Generates all overlapping n-grams from the given token array as arrays of tokens.
     *
     * <p>If the token array is shorter than {@code n}, a single n-gram containing all
     * available tokens is returned (the document is too short to form full n-grams, but
     * discarding it entirely would lose information).
     *
     * <p><b>Time Complexity:</b> O(n * k) – n windows, each copied in O(k)
     *
     * @param tokens the source token array (must not be {@code null})
     * @param n      the n-gram size (must be &gt; 0)
     * @return a {@link List} of token arrays, one per n-gram window
     */
    public List<String[]> generateNGrams(String[] tokens, int n) {
        validateInputs(tokens, n);
        List<String[]> result = new ArrayList<>();

        if (tokens.length == 0) {
            return result; // Empty document – return empty list
        }

        if (tokens.length < n) {
            // Document shorter than n – treat entire document as one n-gram
            String[] shortNGram = new String[tokens.length];
            System.arraycopy(tokens, 0, shortNGram, 0, tokens.length);
            result.add(shortNGram);
            return result;
        }

        int numNGrams = tokens.length - n + 1;
        for (int i = 0; i < numNGrams; i++) {
            String[] ngram = new String[n];
            System.arraycopy(tokens, i, ngram, 0, n);
            result.add(ngram);
        }
        return result;
    }

    /**
     * Generates all overlapping n-grams as space-joined strings.
     *
     * <p>This is a convenience wrapper over {@link #generateNGrams(String[], int)} that
     * joins each n-gram token array into a single string, making it ready for hashing
     * or display.
     *
     * <p><b>Time Complexity:</b> O(n * k)
     *
     * @param tokens the source token array
     * @param n      the n-gram size
     * @return a {@link List} of joined n-gram strings
     */
    public List<String> generateNGramStrings(String[] tokens, int n) {
        List<String[]> ngrams = generateNGrams(tokens, n);
        List<String> result = new ArrayList<>(ngrams.size());
        for (String[] ngram : ngrams) {
            result.add(String.join(" ", ngram));
        }
        return result;
    }

    /**
     * Generates all overlapping n-grams together with their position metadata.
     *
     * <p>Position information is needed by {@link RabinKarp} to report exactly <em>where</em>
     * in the original document a matching segment was found.
     *
     * <p><b>Time Complexity:</b> O(n * k)
     *
     * @param tokens the source token array
     * @param n      the n-gram size
     * @return a {@link List} of {@link NGramEntry} objects, each with start/end positions
     */
    public List<NGramEntry> generateWithPositions(String[] tokens, int n) {
        validateInputs(tokens, n);
        List<NGramEntry> result = new ArrayList<>();

        if (tokens.length == 0) {
            return result;
        }

        if (tokens.length < n) {
            // Document too short – produce one entry spanning the whole document
            String[] shortNGram = new String[tokens.length];
            System.arraycopy(tokens, 0, shortNGram, 0, tokens.length);
            result.add(new NGramEntry(shortNGram, 0, tokens.length - 1));
            return result;
        }

        int numNGrams = tokens.length - n + 1;
        for (int i = 0; i < numNGrams; i++) {
            String[] ngram = new String[n];
            System.arraycopy(tokens, i, ngram, 0, n);
            result.add(new NGramEntry(ngram, i, i + n - 1));
        }
        return result;
    }

    /**
     * Convenience method: splits a normalised text string on whitespace and generates
     * n-grams with position information.
     *
     * <p>The text is expected to already be normalised (lower-cased, punctuation removed,
     * etc.) before being passed here. If the text is blank the result is an empty list.
     *
     * <p><b>Time Complexity:</b> O(n * k)
     *
     * @param normalizedText the pre-normalised document text
     * @param n              the n-gram size
     * @return a {@link List} of {@link NGramEntry} objects
     */
    public List<NGramEntry> generateFromText(String normalizedText, int n) {
        if (normalizedText == null || normalizedText.isBlank()) {
            return new ArrayList<>();
        }
        String[] tokens = normalizedText.trim().split("\\s+");
        return generateWithPositions(tokens, n);
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /**
     * Generates n-grams using the {@link #DEFAULT_N} window size.
     *
     * @param tokens the source token array
     * @return list of n-gram token arrays
     */
    public List<String[]> generateNGrams(String[] tokens) {
        return generateNGrams(tokens, DEFAULT_N);
    }

    /**
     * Generates joined n-gram strings using the {@link #DEFAULT_N} window size.
     *
     * @param tokens the source token array
     * @return list of joined n-gram strings
     */
    public List<String> generateNGramStrings(String[] tokens) {
        return generateNGramStrings(tokens, DEFAULT_N);
    }

    /**
     * Validates that the inputs are non-null and {@code n} is positive.
     *
     * @param tokens the token array
     * @param n      the n-gram size
     * @throws IllegalArgumentException if {@code tokens} is null or {@code n &le; 0}
     */
    private void validateInputs(String[] tokens, int n) {
        if (tokens == null) {
            throw new IllegalArgumentException("Token array must not be null");
        }
        if (n <= 0) {
            throw new IllegalArgumentException("N-gram size must be positive: " + n);
        }
    }
}
