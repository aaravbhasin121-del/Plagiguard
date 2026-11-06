package com.plagiguard.processing;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.*;

/**
 * Generates unique fingerprints for documents using MinHash-like technique.
 *
 * Two identical documents will always produce the same fingerprint.
 * Used to detect duplicate document uploads and previously seen documents.
 *
 * Fingerprint = SHA-256 of normalized content + rolling hash of content
 */
public class DocumentFingerprint {

    /**
     * Large prime used in polynomial rolling hash computation.
     */
    private static final long ROLLING_BASE  = 31L;
    private static final long ROLLING_MOD   = 1_000_000_007L;

    /**
     * Prime multipliers for MinHash universal hash family.
     * a_i and b_i for h_i(x) = ((a_i * x + b_i) % LARGE_PRIME) % numBuckets
     */
    private static final long MINHASH_PRIME = 2_147_483_647L; // Mersenne prime 2^31 - 1
    private static final int  MINHASH_A_SEED = 0x9E3779B9;
    private static final int  MINHASH_B_SEED = 0x6C62272E;

    // -----------------------------------------------------------------------
    // generate
    // -----------------------------------------------------------------------

    /**
     * Generate a fingerprint string for a document.
     *
     * The fingerprint combines:
     *  - SHA-256 of the normalized content (for exact-match detection)
     *  - a polynomial rolling hash of the content (extra uniqueness layer)
     *
     * @param normalizedContent normalized document text
     * @return hex fingerprint string (64-char SHA-256 + 16-char rolling hash)
     */
    public String generate(String normalizedContent) {
        if (normalizedContent == null || normalizedContent.isEmpty()) {
            return "empty_document_" + System.nanoTime();
        }

        String sha = sha256(normalizedContent);
        long rollingHash = computeRollingHash(normalizedContent);
        String rollingHex = Long.toHexString(rollingHash);

        return sha + "_" + rollingHex;
    }

    // -----------------------------------------------------------------------
    // isIdentical
    // -----------------------------------------------------------------------

    /**
     * Check if two fingerprints indicate identical documents.
     *
     * @param fp1 first fingerprint
     * @param fp2 second fingerprint
     * @return {@code true} if the fingerprints are equal (and neither is null)
     */
    public boolean isIdentical(String fp1, String fp2) {
        if (fp1 == null || fp2 == null) return false;
        return fp1.equals(fp2);
    }

    // -----------------------------------------------------------------------
    // sha256 (private)
    // -----------------------------------------------------------------------

    /**
     * Compute SHA-256 hash of content and return as lowercase hex string.
     *
     * @param content input string
     * @return 64-character hex string
     */
    private String sha256(String content) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] hashBytes = digest.digest(content.getBytes(StandardCharsets.UTF_8));
            StringBuilder hex = new StringBuilder(hashBytes.length * 2);
            for (byte b : hashBytes) {
                hex.append(String.format("%02x", b));
            }
            return hex.toString();
        } catch (NoSuchAlgorithmException e) {
            // SHA-256 is mandated by the Java spec — this should never happen
            throw new RuntimeException("SHA-256 algorithm not available", e);
        }
    }

    // -----------------------------------------------------------------------
    // computeRollingHash (private)
    // -----------------------------------------------------------------------

    /**
     * Polynomial rolling hash of the full content string.
     *
     * hash = sum( char[i] * BASE^i ) mod MOD
     */
    private long computeRollingHash(String content) {
        long hash = 0L;
        long power = 1L;
        for (int i = 0; i < content.length(); i++) {
            hash = (hash + (content.charAt(i) * power) % ROLLING_MOD) % ROLLING_MOD;
            power = (power * ROLLING_BASE) % ROLLING_MOD;
        }
        return hash;
    }

    // -----------------------------------------------------------------------
    // generateMinHashSignature
    // -----------------------------------------------------------------------

    /**
     * Generate MinHash-style signature for approximate similarity.
     * Uses multiple hash functions on the n-gram set.
     *
     * Hash function family:
     *   h_i(x) = ((a_i * hashCode(x) + b_i) mod MINHASH_PRIME)
     *
     * The signature for each hash function i is the minimum h_i value
     * across all n-grams in the set.
     *
     * @param ngrams           set of n-gram strings
     * @param numHashFunctions number of hash functions (e.g., 100)
     * @return int[] signature of length numHashFunctions
     */
    public int[] generateMinHashSignature(Set<String> ngrams, int numHashFunctions) {
        int[] signature = new int[numHashFunctions];
        Arrays.fill(signature, Integer.MAX_VALUE);

        if (ngrams == null || ngrams.isEmpty()) return signature;

        // Pre-compute a_i and b_i for each hash function deterministically
        long[] aCoeffs = new long[numHashFunctions];
        long[] bCoeffs = new long[numHashFunctions];
        Random rng = new Random(MINHASH_A_SEED ^ MINHASH_B_SEED);
        for (int i = 0; i < numHashFunctions; i++) {
            // Ensure a != 0
            aCoeffs[i] = (Math.abs(rng.nextLong()) % (MINHASH_PRIME - 1)) + 1;
            bCoeffs[i] = Math.abs(rng.nextLong()) % MINHASH_PRIME;
        }

        for (String ngram : ngrams) {
            long x = (long) ngram.hashCode() & 0xFFFFFFFFL; // unsigned 32-bit hash
            for (int i = 0; i < numHashFunctions; i++) {
                int hashed = (int) ((aCoeffs[i] * x + bCoeffs[i]) % MINHASH_PRIME);
                if (hashed < signature[i]) {
                    signature[i] = hashed;
                }
            }
        }

        return signature;
    }

    // -----------------------------------------------------------------------
    // estimateJaccard
    // -----------------------------------------------------------------------

    /**
     * Estimate Jaccard similarity from two MinHash signatures.
     *
     * similarity ≈ count(sig1[i] == sig2[i]) / numHashFunctions
     *
     * @param sig1 MinHash signature for document A
     * @param sig2 MinHash signature for document B
     * @return estimated Jaccard similarity in [0.0, 1.0]
     */
    public double estimateJaccard(int[] sig1, int[] sig2) {
        if (sig1 == null || sig2 == null) return 0.0;
        int len = Math.min(sig1.length, sig2.length);
        if (len == 0) return 0.0;

        int matches = 0;
        for (int i = 0; i < len; i++) {
            if (sig1[i] == sig2[i]) matches++;
        }
        return (double) matches / len;
    }
}
