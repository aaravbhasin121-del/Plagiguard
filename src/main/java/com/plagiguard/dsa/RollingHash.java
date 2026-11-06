package com.plagiguard.dsa;

/**
 * RollingHash - Polynomial rolling hash for use in the Rabin-Karp string-matching algorithm.
 *
 * <p>Uses <em>double hashing</em> (two independent polynomial hashes) to dramatically reduce
 * the probability of spurious collisions (false positives) when comparing windows of tokens.
 *
 * <p><b>Hash formula for a window of size w starting at index s:</b>
 * <pre>
 *   H(s) = tokens[s]*BASE^(w-1) + tokens[s+1]*BASE^(w-2) + ... + tokens[s+w-1]*BASE^0  (mod MOD)
 * </pre>
 *
 * <p><b>Rolling (sliding) step:</b>
 * <pre>
 *   H(s+1) = (H(s) - tokens[s]*BASE^(w-1)) * BASE + tokens[s+w]  (mod MOD)
 * </pre>
 *
 * <p><b>Time Complexity:</b>
 * <ul>
 *   <li>{@code computeInitialHash} – O(w) where w = window size</li>
 *   <li>{@code rollHash}          – O(1) per slide</li>
 *   <li>{@code computeAllHashes} – O(n) total for n tokens</li>
 * </ul>
 *
 * <p><b>Space Complexity:</b> O(n) for the hash array
 */
public class RollingHash {

    // -----------------------------------------------------------------------
    // Constants
    // -----------------------------------------------------------------------

    /** Polynomial base (prime). A small prime works well for word/character tokens. */
    public static final long BASE = 31L;

    /** First large prime modulus (reduces hash to a safe long range). */
    public static final long MOD1 = 1_000_000_007L;

    /** Second large prime modulus for double hashing (reduces false-positive rate). */
    public static final long MOD2 = 998_244_353L;

    // -----------------------------------------------------------------------
    // Fields
    // -----------------------------------------------------------------------

    /** Number of tokens in each hash window. */
    private final int windowSize;

    /**
     * Maps each unique token string to a stable small integer ID used as the
     * polynomial coefficient. Stable IDs are critical for consistency: the same
     * token must produce the same coefficient across all documents.
     */
    private final CustomHashTable<String, Integer> wordToId;

    /** Monotonically increasing counter used to assign new token IDs. Starts at 1 (not 0). */
    private int idCounter;

    // -----------------------------------------------------------------------
    // Constructor
    // -----------------------------------------------------------------------

    /**
     * Creates a RollingHash with the given window size.
     *
     * @param windowSize number of tokens per hash window (must be &gt; 0)
     */
    public RollingHash(int windowSize) {
        if (windowSize <= 0) {
            throw new IllegalArgumentException("Window size must be positive: " + windowSize);
        }
        this.windowSize = windowSize;
        this.wordToId = new CustomHashTable<>();
        this.idCounter = 1; // IDs start at 1 so the zero token is distinguishable
    }

    // -----------------------------------------------------------------------
    // Token ID Management
    // -----------------------------------------------------------------------

    /**
     * Returns the stable integer ID assigned to {@code token}, creating a new one
     * if this token has not been seen before.
     *
     * <p>Using integer IDs rather than raw strings as hash coefficients keeps the
     * arithmetic simple and avoids hashing character sequences repeatedly.
     *
     * <p><b>Time Complexity:</b> O(1) amortised (hash table lookup/insert)
     *
     * @param token the token string (e.g. a word or character)
     * @return the stable integer ID &ge; 1
     */
    public int getOrAssignId(String token) {
        if (token == null) {
            token = ""; // Treat null as empty string
        }
        Integer existing = wordToId.get(token);
        if (existing != null) {
            return existing;
        }
        int id = idCounter++;
        wordToId.put(token, id);
        return id;
    }

    /**
     * Converts an array of token strings into their corresponding integer IDs,
     * assigning new IDs for any tokens not yet encountered.
     *
     * <p><b>Time Complexity:</b> O(n) where n = tokens.length
     *
     * @param tokens array of token strings
     * @return array of integer IDs in the same order
     */
    public int[] tokenizeToIds(String[] tokens) {
        int[] ids = new int[tokens.length];
        for (int i = 0; i < tokens.length; i++) {
            ids[i] = getOrAssignId(tokens[i]);
        }
        return ids;
    }

    // -----------------------------------------------------------------------
    // Hash Computation
    // -----------------------------------------------------------------------

    /**
     * Computes the initial polynomial hash for the window {@code [start, start+windowSize)}
     * using the given modulus.
     *
     * <p>Formula: {@code H = ids[start]*BASE^(w-1) + ... + ids[start+w-1]  (mod mod)}
     *
     * <p><b>Time Complexity:</b> O(w)
     *
     * @param ids   integer ID array (length &ge; start + windowSize)
     * @param start starting index of the window
     * @param mod   the prime modulus to use
     * @return the hash value in [0, mod)
     */
    public long computeInitialHash(int[] ids, int start, long mod) {
        long hash = 0L;
        for (int i = 0; i < windowSize; i++) {
            hash = (hash * BASE + ids[start + i]) % mod;
        }
        return hash;
    }

    /**
     * Slides the hash window by one position, removing the contribution of the
     * {@code outgoing} token and adding the {@code incoming} token.
     *
     * <p>Formula:
     * <pre>
     *   newHash = ((currentHash - outgoing * BASE^(w-1)) * BASE + incoming)  mod  mod
     * </pre>
     *
     * <p><b>Time Complexity:</b> O(1)
     *
     * @param currentHash the hash of the current window
     * @param outgoing    the token ID leaving the window (leftmost)
     * @param incoming    the token ID entering the window (rightmost)
     * @param windowSize  the window size (for computing the high-order power)
     * @param mod         the prime modulus
     * @return the hash of the new window
     */
    public long rollHash(long currentHash, int outgoing, int incoming, int windowSize, long mod) {
        long highPow = getPow(windowSize - 1, BASE, mod);
        long newHash = (currentHash - (long) outgoing * highPow % mod + mod) % mod;
        newHash = (newHash * BASE + incoming) % mod;
        return newHash;
    }

    /**
     * Computes double-hashes for every window of size {@code windowSize} over the given
     * token array. Each window yields a {@code long[2]} containing hashes under MOD1 and MOD2.
     *
     * <p>The first call computes the initial window hash in O(w), then each subsequent
     * window is computed by rolling in O(1), giving O(n) overall.
     *
     * <p><b>Time Complexity:</b> O(n) where n = tokens.length
     *
     * @param tokens array of token strings
     * @return a {@code long[numWindows][2]} array; index [i] contains {hash1, hash2} for window i
     */
    public long[][] computeAllHashes(String[] tokens) {
        int n = tokens.length;
        if (n < windowSize) {
            return new long[0][2]; // Not enough tokens for even one window
        }

        int numWindows = n - windowSize + 1;
        long[][] hashes = new long[numWindows][2];

        // Convert tokens to IDs once
        int[] ids = tokenizeToIds(tokens);

        // Compute hash for the first window
        hashes[0][0] = computeInitialHash(ids, 0, MOD1);
        hashes[0][1] = computeInitialHash(ids, 0, MOD2);

        // Roll the window across the rest of the token stream
        for (int i = 1; i < numWindows; i++) {
            hashes[i][0] = rollHash(hashes[i - 1][0], ids[i - 1], ids[i + windowSize - 1], windowSize, MOD1);
            hashes[i][1] = rollHash(hashes[i - 1][1], ids[i - 1], ids[i + windowSize - 1], windowSize, MOD2);
        }

        return hashes;
    }

    /**
     * Computes modular exponentiation: {@code base^exp mod mod} using repeated squaring.
     *
     * <p><b>Time Complexity:</b> O(log exp)
     *
     * @param exp  the exponent (&ge; 0)
     * @param base the base
     * @param mod  the modulus
     * @return {@code base^exp mod mod}
     */
    public long getPow(int exp, long base, long mod) {
        long result = 1L;
        base %= mod;
        while (exp > 0) {
            if ((exp & 1) == 1) {
                result = result * base % mod;
            }
            base = base * base % mod;
            exp >>= 1;
        }
        return result;
    }

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /**
     * Returns the configured window size.
     *
     * @return the window size
     */
    public int getWindowSize() {
        return windowSize;
    }

    /**
     * Returns the number of unique tokens that have been assigned IDs so far.
     *
     * @return vocabulary size
     */
    public int getVocabularySize() {
        return wordToId.size();
    }

    /**
     * Returns the token-to-ID mapping table (read-only intent; do not mutate externally).
     *
     * @return the internal {@link CustomHashTable}
     */
    public CustomHashTable<String, Integer> getWordToId() {
        return wordToId;
    }

    @Override
    public String toString() {
        return String.format("RollingHash{windowSize=%d, vocabulary=%d}", windowSize, getVocabularySize());
    }
}
