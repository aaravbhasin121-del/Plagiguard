package com.plagiguard.dsa;

import java.util.ArrayList;
import java.util.List;

/**
 * RabinKarp - Rabin-Karp rolling-hash string-matching algorithm adapted for token streams.
 *
 * <p>The classic Rabin-Karp algorithm finds all occurrences of a pattern in a text in
 * <b>O(n + m) expected time</b> by comparing hash values instead of individual characters.
 * Only when hashes match is a character-by-character (or token-by-token) comparison made
 * to confirm the match and rule out hash collisions.
 *
 * <p>PlagiGuard extends the algorithm to work on <em>token arrays</em> (words) rather than
 * character strings, enabling semantic-level plagiarism detection that is robust to minor
 * whitespace and punctuation differences.
 *
 * <p><b>Time Complexity:</b>
 * <ul>
 *   <li>{@code search}:             O(n + m) expected, O(n * m) worst case (pathological collisions)</li>
 *   <li>{@code searchAllPatterns}:  O(n + P*m) expected where P = pattern count</li>
 *   <li>{@code findMatchingNGrams}: O(n + m) expected</li>
 * </ul>
 *
 * <p><b>Space Complexity:</b> O(n + m) for the hash arrays
 */
public class RabinKarp {

    // -----------------------------------------------------------------------
    // Inner Classes
    // -----------------------------------------------------------------------

    /**
     * Represents a single position where document A and document B share identical
     * token sequences.
     */
    public static class MatchPosition {
        /** Zero-based start index in document A's token array. */
        public final int posA;
        /** Zero-based start index in document B's token array. */
        public final int posB;
        /** Number of tokens in the matched segment. */
        public final int length;
        /** The matched token sequence joined into a readable string. */
        public final String matchedText;

        /**
         * Constructs a MatchPosition.
         *
         * @param posA        start position in document A
         * @param posB        start position in document B
         * @param length      length of the match in tokens
         * @param matchedText human-readable form of the matched sequence
         */
        public MatchPosition(int posA, int posB, int length, String matchedText) {
            this.posA = posA;
            this.posB = posB;
            this.length = length;
            this.matchedText = matchedText;
        }

        @Override
        public String toString() {
            return String.format("MatchPosition{docA[%d], docB[%d], len=%d, text='%s'}",
                    posA, posB, length, matchedText);
        }
    }

    // -----------------------------------------------------------------------
    // Core Algorithm
    // -----------------------------------------------------------------------

    /**
     * Searches for all occurrences of {@code patternTokens} within {@code textTokens}
     * using the Rabin-Karp rolling-hash algorithm.
     *
     * <p>Steps:
     * <ol>
     *   <li>Compute double-hash of the pattern window.</li>
     *   <li>Roll a window of the same size over the text, updating the double-hash in O(1).</li>
     *   <li>On a hash match, verify by direct token comparison to eliminate false positives.</li>
     * </ol>
     *
     * <p><b>Time Complexity:</b> O(n + m) expected; O(n*m) worst case
     *
     * @param textTokens    the text document tokens (haystack)
     * @param patternTokens the pattern tokens to search for (needle)
     * @param rh            the {@link RollingHash} instance (used for token ID assignment)
     * @return a {@link List} of zero-based start indices in {@code textTokens} where the
     *         pattern was found
     */
    public List<Integer> search(String[] textTokens, String[] patternTokens, RollingHash rh) {
        List<Integer> matches = new ArrayList<>();

        int n = textTokens.length;
        int m = patternTokens.length;

        if (m == 0 || n < m) {
            return matches; // Pattern cannot be found
        }

        // Convert both to stable integer IDs using the shared RollingHash vocabulary
        int[] textIds = rh.tokenizeToIds(textTokens);
        int[] patternIds = rh.tokenizeToIds(patternTokens);

        // Compute pattern hash (both mods)
        long patHash1 = rh.computeInitialHash(patternIds, 0, RollingHash.MOD1);
        long patHash2 = rh.computeInitialHash(patternIds, 0, RollingHash.MOD2);

        // Compute initial window hash for text
        long winHash1 = rh.computeInitialHash(textIds, 0, RollingHash.MOD1);
        long winHash2 = rh.computeInitialHash(textIds, 0, RollingHash.MOD2);

        for (int i = 0; i <= n - m; i++) {
            // Hash comparison (O(1))
            if (winHash1 == patHash1 && winHash2 == patHash2) {
                // Verify token-by-token to rule out collision (spurious hash match)
                if (tokensMatch(textTokens, i, patternTokens)) {
                    matches.add(i);
                }
            }

            // Roll the window (skip for last iteration)
            if (i < n - m) {
                winHash1 = rh.rollHash(winHash1, textIds[i], textIds[i + m], m, RollingHash.MOD1);
                winHash2 = rh.rollHash(winHash2, textIds[i], textIds[i + m], m, RollingHash.MOD2);
            }
        }

        return matches;
    }

    /**
     * Searches for every pattern in {@code patterns} within {@code textTokens} in a single pass.
     *
     * <p>Each pattern is looked up by its double-hash fingerprint. The text is scanned once;
     * all pattern hashes are pre-stored in a {@link CustomHashTable} for O(1) lookup per window.
     *
     * <p><b>Time Complexity:</b> O(n + P*m) expected where P = number of patterns, m = max pattern length
     *
     * @param textTokens the text document tokens
     * @param patterns   list of patterns to search for simultaneously
     * @param rh         the {@link RollingHash} instance
     * @return a {@link CustomHashTable} mapping each pattern (as joined string) to the list of
     *         positions where it was found in the text
     */
    public CustomHashTable<String, List<Integer>> searchAllPatterns(
            String[] textTokens, List<String[]> patterns, RollingHash rh) {

        CustomHashTable<String, List<Integer>> results = new CustomHashTable<>();
        if (patterns == null || patterns.isEmpty() || textTokens.length == 0) {
            return results;
        }

        // Group patterns by their length for efficient windowing
        // For simplicity, process each pattern independently (optimal for varied-length patterns)
        for (String[] pattern : patterns) {
            String key = String.join(" ", pattern);
            List<Integer> positions = search(textTokens, pattern, rh);
            results.put(key, positions);
        }

        return results;
    }

    /**
     * Finds all pairs of positions (posA, posB) where document A and document B share
     * a common token sequence of exactly {@code ngramSize} tokens.
     *
     * <p>Algorithm:
     * <ol>
     *   <li>Compute double-hashes for all n-gram windows in both documents.</li>
     *   <li>Store document B's hashes in a {@link CustomHashTable} keyed by (hash1, hash2).</li>
     *   <li>For every window in document A, look up its hashes in the table.</li>
     *   <li>For each hash match, verify token equality to eliminate false positives.</li>
     * </ol>
     *
     * <p><b>Time Complexity:</b> O(n + m) expected where n = |docA|, m = |docB|
     *
     * @param docATokens the first document's tokens
     * @param docBTokens the second document's tokens
     * @param ngramSize  the window / n-gram size
     * @param rh         the {@link RollingHash} instance
     * @return a list of {@link MatchPosition} objects describing each shared segment
     */
    public List<MatchPosition> findMatchingNGrams(
            String[] docATokens, String[] docBTokens, int ngramSize, RollingHash rh) {

        List<MatchPosition> matches = new ArrayList<>();

        if (docATokens.length < ngramSize || docBTokens.length < ngramSize) {
            return matches; // One or both documents too short
        }

        // Compute hashes for document B and store in a hash table: (h1,h2) -> List of positions in B
        long[][] hashesB = rh.computeAllHashes(docBTokens);
        // Key: "h1:h2" string (compact composite key)
        CustomHashTable<String, List<Integer>> bHashTable = new CustomHashTable<>();

        for (int i = 0; i < hashesB.length; i++) {
            String key = hashKey(hashesB[i][0], hashesB[i][1]);
            List<Integer> positions = bHashTable.get(key);
            if (positions == null) {
                positions = new ArrayList<>();
                bHashTable.put(key, positions);
            }
            positions.add(i);
        }

        // Slide over document A and probe document B's hash table
        long[][] hashesA = rh.computeAllHashes(docATokens);

        for (int i = 0; i < hashesA.length; i++) {
            String key = hashKey(hashesA[i][0], hashesA[i][1]);
            List<Integer> bPositions = bHashTable.get(key);

            if (bPositions == null) {
                continue; // No hash match – skip
            }

            // Verify each candidate in B token-by-token
            for (int j : bPositions) {
                if (tokensMatch(docATokens, i, docBTokens, j, ngramSize)) {
                    // Build matched text for display
                    String matchedText = buildMatchText(docATokens, i, ngramSize);
                    matches.add(new MatchPosition(i, j, ngramSize, matchedText));
                }
            }
        }

        return matches;
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /**
     * Verifies that the tokens in {@code text} starting at {@code textStart} exactly match
     * all tokens in {@code pattern}.
     *
     * <p><b>Time Complexity:</b> O(m)
     *
     * @param text      the text token array
     * @param textStart the start index in {@code text}
     * @param pattern   the pattern token array
     * @return {@code true} if all tokens match
     */
    private boolean tokensMatch(String[] text, int textStart, String[] pattern) {
        for (int i = 0; i < pattern.length; i++) {
            if (!text[textStart + i].equals(pattern[i])) {
                return false;
            }
        }
        return true;
    }

    /**
     * Verifies that a window of {@code length} tokens starting at {@code startA} in array {@code a}
     * equals the same-length window starting at {@code startB} in array {@code b}.
     *
     * <p><b>Time Complexity:</b> O(length)
     *
     * @param a       first token array
     * @param startA  start index in {@code a}
     * @param b       second token array
     * @param startB  start index in {@code b}
     * @param length  number of tokens to compare
     * @return {@code true} if the windows are identical
     */
    private boolean tokensMatch(String[] a, int startA, String[] b, int startB, int length) {
        for (int i = 0; i < length; i++) {
            if (!a[startA + i].equals(b[startB + i])) {
                return false;
            }
        }
        return true;
    }

    /**
     * Creates a composite string key from two hash values for use in a {@link CustomHashTable}.
     *
     * @param h1 first hash value
     * @param h2 second hash value
     * @return the key string {@code "h1:h2"}
     */
    private String hashKey(long h1, long h2) {
        return h1 + ":" + h2;
    }

    /**
     * Joins {@code length} tokens from {@code tokens} starting at {@code start} into a
     * human-readable space-separated string.
     *
     * @param tokens the token array
     * @param start  start index
     * @param length number of tokens to join
     * @return the joined string
     */
    private String buildMatchText(String[] tokens, int start, int length) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < length; i++) {
            if (i > 0) sb.append(' ');
            sb.append(tokens[start + i]);
        }
        return sb.toString();
    }
}
