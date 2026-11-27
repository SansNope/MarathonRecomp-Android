package io.github.sansnope.marathonrecomp;

/**
 * Pure Java streaming and one-shot implementation of unseeded XXH3_64bits (xxHash 0.8.x),
 * matching MarathonRecomp/install/installer.cpp for game and DLC file integrity checks.
 */
final class Xxh3 {
    private static final long PRIME32_1 = 0x9E3779B1L;
    private static final long PRIME32_2 = 0x85EBCA77L;
    private static final long PRIME32_3 = 0xC2B2AE3DL;

    private static final long PRIME64_1 = 0x9E3779B185EBCA87L;
    private static final long PRIME64_2 = 0xC2B2AE3D27D4EB4FL;
    private static final long PRIME64_3 = 0x165667B19E3779F9L;
    private static final long PRIME64_4 = 0x85EBCA77C2B2AE63L;
    private static final long PRIME64_5 = 0x27D4EB2F165667C5L;

    private static final long PRIME_MX1 = 0x165667919E3779F9L;
    private static final long PRIME_MX2 = 0x9FB21C651E98DF25L;

    private static final int STRIPE_LEN = 64;
    private static final int SECRET_CONSUME_RATE = 8;
    private static final int SECRET_DEFAULT_SIZE = 192;
    private static final int SECRET_LIMIT = SECRET_DEFAULT_SIZE - STRIPE_LEN; // 128
    private static final int STRIPES_PER_BLOCK = SECRET_LIMIT / SECRET_CONSUME_RATE; // 16
    private static final int INTERNAL_BUFFER_SIZE = 256;
    private static final int INTERNAL_BUFFER_STRIPES = INTERNAL_BUFFER_SIZE / STRIPE_LEN; // 4
    private static final int MIDSIZE_MAX = 240;
    private static final int MIDSIZE_STARTOFFSET = 3;
    private static final int MIDSIZE_LASTOFFSET = 17;
    private static final int SECRET_MIN_SIZE = 136;
    private static final int SECRET_LASTACC_START = 7;
    private static final int SECRET_MERGEACCS_START = 11;

    private static final byte[] SECRET = {
        (byte) 0xb8, (byte) 0xfe, (byte) 0x6c, (byte) 0x39, (byte) 0x23, (byte) 0xa4, (byte) 0x4b, (byte) 0xbe,
        (byte) 0x7c, (byte) 0x01, (byte) 0x81, (byte) 0x2c, (byte) 0xf7, (byte) 0x21, (byte) 0xad, (byte) 0x1c,
        (byte) 0xde, (byte) 0xd4, (byte) 0x6d, (byte) 0xe9, (byte) 0x83, (byte) 0x90, (byte) 0x97, (byte) 0xdb,
        (byte) 0x72, (byte) 0x40, (byte) 0xa4, (byte) 0xa4, (byte) 0xb7, (byte) 0xb3, (byte) 0x67, (byte) 0x1f,
        (byte) 0xcb, (byte) 0x79, (byte) 0xe6, (byte) 0x4e, (byte) 0xcc, (byte) 0xc0, (byte) 0xe5, (byte) 0x78,
        (byte) 0x82, (byte) 0x5a, (byte) 0xd0, (byte) 0x7d, (byte) 0xcc, (byte) 0xff, (byte) 0x72, (byte) 0x21,
        (byte) 0xb8, (byte) 0x08, (byte) 0x46, (byte) 0x74, (byte) 0xf7, (byte) 0x43, (byte) 0x24, (byte) 0x8e,
        (byte) 0xe0, (byte) 0x35, (byte) 0x90, (byte) 0xe6, (byte) 0x81, (byte) 0x3a, (byte) 0x26, (byte) 0x4c,
        (byte) 0x3c, (byte) 0x28, (byte) 0x52, (byte) 0xbb, (byte) 0x91, (byte) 0xc3, (byte) 0x00, (byte) 0xcb,
        (byte) 0x88, (byte) 0xd0, (byte) 0x65, (byte) 0x8b, (byte) 0x1b, (byte) 0x53, (byte) 0x2e, (byte) 0xa3,
        (byte) 0x71, (byte) 0x64, (byte) 0x48, (byte) 0x97, (byte) 0xa2, (byte) 0x0d, (byte) 0xf9, (byte) 0x4e,
        (byte) 0x38, (byte) 0x19, (byte) 0xef, (byte) 0x46, (byte) 0xa9, (byte) 0xde, (byte) 0xac, (byte) 0xd8,
        (byte) 0xa8, (byte) 0xfa, (byte) 0x76, (byte) 0x3f, (byte) 0xe3, (byte) 0x9c, (byte) 0x34, (byte) 0x3f,
        (byte) 0xf9, (byte) 0xdc, (byte) 0xbb, (byte) 0xc7, (byte) 0xc7, (byte) 0x0b, (byte) 0x4f, (byte) 0x1d,
        (byte) 0x8a, (byte) 0x51, (byte) 0xe0, (byte) 0x4b, (byte) 0xcd, (byte) 0xb4, (byte) 0x59, (byte) 0x31,
        (byte) 0xc8, (byte) 0x9f, (byte) 0x7e, (byte) 0xc9, (byte) 0xd9, (byte) 0x78, (byte) 0x73, (byte) 0x64,
        (byte) 0xea, (byte) 0xc5, (byte) 0xac, (byte) 0x83, (byte) 0x34, (byte) 0xd3, (byte) 0xeb, (byte) 0xc3,
        (byte) 0xc5, (byte) 0x81, (byte) 0xa0, (byte) 0xff, (byte) 0xfa, (byte) 0x13, (byte) 0x63, (byte) 0xeb,
        (byte) 0x17, (byte) 0x0d, (byte) 0xdd, (byte) 0x51, (byte) 0xb7, (byte) 0xf0, (byte) 0xda, (byte) 0x49,
        (byte) 0xd3, (byte) 0x16, (byte) 0x55, (byte) 0x26, (byte) 0x29, (byte) 0xd4, (byte) 0x68, (byte) 0x9e,
        (byte) 0x2b, (byte) 0x16, (byte) 0xbe, (byte) 0x58, (byte) 0x7d, (byte) 0x47, (byte) 0xa1, (byte) 0xfc,
        (byte) 0x8f, (byte) 0xf8, (byte) 0xb8, (byte) 0xd1, (byte) 0x7a, (byte) 0xd0, (byte) 0x31, (byte) 0xce,
        (byte) 0x45, (byte) 0xcb, (byte) 0x3a, (byte) 0x8f, (byte) 0x95, (byte) 0x16, (byte) 0x04, (byte) 0x28,
        (byte) 0xaf, (byte) 0xd7, (byte) 0xfb, (byte) 0xca, (byte) 0xbb, (byte) 0x4b, (byte) 0x40, (byte) 0x7e
    };

    private final long[] acc = new long[8];
    private final long[] digestAcc = new long[8];
    private final byte[] buffer = new byte[INTERNAL_BUFFER_SIZE];
    private final byte[] lastStripe = new byte[STRIPE_LEN];
    private long totalLen;
    private int bufferedSize;
    private int stripesSoFar;

    Xxh3() {
        reset();
    }

    void reset() {
        acc[0] = PRIME32_3;
        acc[1] = PRIME64_1;
        acc[2] = PRIME64_2;
        acc[3] = PRIME64_3;
        acc[4] = PRIME64_4;
        acc[5] = PRIME32_2;
        acc[6] = PRIME64_5;
        acc[7] = PRIME32_1;
        totalLen = 0;
        bufferedSize = 0;
        stripesSoFar = 0;
    }

    void update(byte[] input, int offset, int length) {
        if (length <= 0) return;
        totalLen += length;

        if (length <= INTERNAL_BUFFER_SIZE - bufferedSize) {
            System.arraycopy(input, offset, buffer, bufferedSize, length);
            bufferedSize += length;
            return;
        }

        int pos = offset;
        final int end = offset + length;

        if (bufferedSize > 0) {
            int loadSize = INTERNAL_BUFFER_SIZE - bufferedSize;
            System.arraycopy(input, pos, buffer, bufferedSize, loadSize);
            pos += loadSize;
            stripesSoFar = consumeStripes(acc, stripesSoFar, buffer, 0, INTERNAL_BUFFER_STRIPES);
            bufferedSize = 0;
        }

        if (end - pos > INTERNAL_BUFFER_SIZE) {
            int nbStripes = (end - 1 - pos) / STRIPE_LEN;
            stripesSoFar = consumeStripes(acc, stripesSoFar, input, pos, nbStripes);
            pos += nbStripes * STRIPE_LEN;
            System.arraycopy(input, pos - STRIPE_LEN, buffer, INTERNAL_BUFFER_SIZE - STRIPE_LEN, STRIPE_LEN);
        }

        int rem = end - pos;
        System.arraycopy(input, pos, buffer, 0, rem);
        bufferedSize = rem;
    }

    long digest() {
        if (totalLen > MIDSIZE_MAX) {
            System.arraycopy(acc, 0, digestAcc, 0, 8);
            if (bufferedSize >= STRIPE_LEN) {
                int nbStripes = (bufferedSize - 1) / STRIPE_LEN;
                consumeStripes(digestAcc, stripesSoFar, buffer, 0, nbStripes);
                accumulate512(digestAcc, buffer, bufferedSize - STRIPE_LEN, SECRET_LIMIT - SECRET_LASTACC_START);
            } else {
                int catchupSize = STRIPE_LEN - bufferedSize;
                System.arraycopy(buffer, INTERNAL_BUFFER_SIZE - catchupSize, lastStripe, 0, catchupSize);
                System.arraycopy(buffer, 0, lastStripe, catchupSize, bufferedSize);
                accumulate512(digestAcc, lastStripe, 0, SECRET_LIMIT - SECRET_LASTACC_START);
            }
            return mergeAccs(digestAcc, SECRET_MERGEACCS_START, totalLen * PRIME64_1);
        }
        return hashShortOrMid(buffer, 0, (int) totalLen);
    }

    static long hash64(byte[] input, int offset, int length) {
        if (length <= MIDSIZE_MAX) {
            return hashShortOrMid(input, offset, length);
        }
        long[] a = {
            PRIME32_3, PRIME64_1, PRIME64_2, PRIME64_3,
            PRIME64_4, PRIME32_2, PRIME64_5, PRIME32_1
        };
        int blockLen = STRIPE_LEN * STRIPES_PER_BLOCK;
        int nbBlocks = (length - 1) / blockLen;
        for (int n = 0; n < nbBlocks; n++) {
            accumulate(a, input, offset + n * blockLen, 0, STRIPES_PER_BLOCK);
            scrambleAcc(a, SECRET_LIMIT);
        }
        int nbStripes = ((length - 1) - (blockLen * nbBlocks)) / STRIPE_LEN;
        accumulate(a, input, offset + nbBlocks * blockLen, 0, nbStripes);
        accumulate512(a, input, offset + length - STRIPE_LEN, SECRET_LIMIT - SECRET_LASTACC_START);
        return mergeAccs(a, SECRET_MERGEACCS_START, (long) length * PRIME64_1);
    }

    private static long hashShortOrMid(byte[] input, int off, int len) {
        if (len <= 16) {
            if (len > 8) {
                long bitflip1 = readLE64(SECRET, 24) ^ readLE64(SECRET, 32);
                long bitflip2 = readLE64(SECRET, 40) ^ readLE64(SECRET, 48);
                long inputLo = readLE64(input, off) ^ bitflip1;
                long inputHi = readLE64(input, off + len - 8) ^ bitflip2;
                long h = (long) len + Long.reverseBytes(inputLo) + inputHi + mul128Fold64(inputLo, inputHi);
                return xxh3Avalanche(h);
            }
            if (len >= 4) {
                long input1 = readLE32(input, off);
                long input2 = readLE32(input, off + len - 4);
                long bitflip = readLE64(SECRET, 8) ^ readLE64(SECRET, 16);
                long input64 = input2 + (input1 << 32);
                return rrmxmx(input64 ^ bitflip, len);
            }
            if (len > 0) {
                int c1 = input[off] & 0xFF;
                int c2 = input[off + (len >>> 1)] & 0xFF;
                int c3 = input[off + len - 1] & 0xFF;
                long combined = (((long) c1 << 16) | ((long) c2 << 24) | ((long) c3) | ((long) len << 8)) & 0xFFFFFFFFL;
                long bitflip = readLE32(SECRET, 0) ^ readLE32(SECRET, 4);
                return xxh64Avalanche(combined ^ bitflip);
            }
            return xxh64Avalanche(readLE64(SECRET, 56) ^ readLE64(SECRET, 64));
        }

        if (len <= 128) {
            long h = (long) len * PRIME64_1;
            if (len > 32) {
                if (len > 64) {
                    if (len > 96) {
                        h += mix16B(input, off + 48, 96);
                        h += mix16B(input, off + len - 64, 112);
                    }
                    h += mix16B(input, off + 32, 64);
                    h += mix16B(input, off + len - 48, 80);
                }
                h += mix16B(input, off + 16, 32);
                h += mix16B(input, off + len - 32, 48);
            }
            h += mix16B(input, off, 0);
            h += mix16B(input, off + len - 16, 16);
            return xxh3Avalanche(h);
        }

        long h = (long) len * PRIME64_1;
        int nbRounds = len / 16;
        for (int i = 0; i < 8; i++) {
            h += mix16B(input, off + 16 * i, 16 * i);
        }
        long hEnd = mix16B(input, off + len - 16, SECRET_MIN_SIZE - MIDSIZE_LASTOFFSET);
        h = xxh3Avalanche(h);
        for (int i = 8; i < nbRounds; i++) {
            hEnd += mix16B(input, off + 16 * i, 16 * (i - 8) + MIDSIZE_STARTOFFSET);
        }
        return xxh3Avalanche(h + hEnd);
    }

    private static int consumeStripes(long[] a, int stripesSoFar, byte[] input, int inOff, int nbStripes) {
        int secretOff = stripesSoFar * SECRET_CONSUME_RATE;
        if (nbStripes >= STRIPES_PER_BLOCK - stripesSoFar) {
            int thisIter = STRIPES_PER_BLOCK - stripesSoFar;
            do {
                accumulate(a, input, inOff, secretOff, thisIter);
                scrambleAcc(a, SECRET_LIMIT);
                inOff += thisIter * STRIPE_LEN;
                nbStripes -= thisIter;
                thisIter = STRIPES_PER_BLOCK;
                secretOff = 0;
            } while (nbStripes >= STRIPES_PER_BLOCK);
            stripesSoFar = 0;
        }
        if (nbStripes > 0) {
            accumulate(a, input, inOff, secretOff, nbStripes);
            stripesSoFar += nbStripes;
        }
        return stripesSoFar;
    }

    private static void accumulate(long[] a, byte[] input, int inOff, int secretOff, int nbStripes) {
        for (int n = 0; n < nbStripes; n++) {
            accumulate512(a, input, inOff + n * STRIPE_LEN, secretOff + n * SECRET_CONSUME_RATE);
        }
    }

    private static void accumulate512(long[] a, byte[] input, int inOff, int secretOff) {
        for (int lane = 0; lane < 8; lane++) {
            long dataVal = readLE64(input, inOff + lane * 8);
            long dataKey = dataVal ^ readLE64(SECRET, secretOff + lane * 8);
            a[lane ^ 1] += dataVal;
            a[lane] += (dataKey & 0xFFFFFFFFL) * (dataKey >>> 32);
        }
    }

    private static void scrambleAcc(long[] a, int secretOff) {
        for (int lane = 0; lane < 8; lane++) {
            long key64 = readLE64(SECRET, secretOff + lane * 8);
            long acc64 = a[lane];
            acc64 ^= acc64 >>> 47;
            acc64 ^= key64;
            acc64 *= PRIME32_1;
            a[lane] = acc64;
        }
    }

    private static long mergeAccs(long[] a, int secretOff, long start) {
        long result = start;
        for (int i = 0; i < 4; i++) {
            result += mul128Fold64(
                a[2 * i] ^ readLE64(SECRET, secretOff + 16 * i),
                a[2 * i + 1] ^ readLE64(SECRET, secretOff + 16 * i + 8));
        }
        return xxh3Avalanche(result);
    }

    private static long mix16B(byte[] input, int inOff, int secretOff) {
        long lo = readLE64(input, inOff);
        long hi = readLE64(input, inOff + 8);
        return mul128Fold64(lo ^ readLE64(SECRET, secretOff), hi ^ readLE64(SECRET, secretOff + 8));
    }

    private static long mul128Fold64(long lhs, long rhs) {
        long loLo = (lhs & 0xFFFFFFFFL) * (rhs & 0xFFFFFFFFL);
        long hiLo = (lhs >>> 32) * (rhs & 0xFFFFFFFFL);
        long loHi = (lhs & 0xFFFFFFFFL) * (rhs >>> 32);
        long hiHi = (lhs >>> 32) * (rhs >>> 32);
        long cross = (loLo >>> 32) + (hiLo & 0xFFFFFFFFL) + loHi;
        long upper = (hiLo >>> 32) + (cross >>> 32) + hiHi;
        long lower = (cross << 32) | (loLo & 0xFFFFFFFFL);
        return lower ^ upper;
    }

    private static long xxh3Avalanche(long h) {
        h ^= h >>> 37;
        h *= PRIME_MX1;
        h ^= h >>> 32;
        return h;
    }

    private static long xxh64Avalanche(long h) {
        h ^= h >>> 33;
        h *= PRIME64_2;
        h ^= h >>> 29;
        h *= PRIME64_3;
        h ^= h >>> 32;
        return h;
    }

    private static long rrmxmx(long h, long len) {
        h ^= Long.rotateLeft(h, 49) ^ Long.rotateLeft(h, 24);
        h *= PRIME_MX2;
        h ^= (h >>> 35) + len;
        h *= PRIME_MX2;
        return h ^ (h >>> 28);
    }

    private static long readLE32(byte[] b, int o) {
        return (b[o] & 0xFFL)
            | ((b[o + 1] & 0xFFL) << 8)
            | ((b[o + 2] & 0xFFL) << 16)
            | ((b[o + 3] & 0xFFL) << 24);
    }

    private static long readLE64(byte[] b, int o) {
        return readLE32(b, o) | (readLE32(b, o + 4) << 32);
    }
}
