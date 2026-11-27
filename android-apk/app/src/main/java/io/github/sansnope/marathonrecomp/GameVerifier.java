package io.github.sansnope.marathonrecomp;

import android.content.Context;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * Verifies installed game and DLC files against upstream MarathonRecomp XXH3-64 hash tables
 * packed in assets/game_hashes.bin. Uses streaming 1 MiB reads so large movie/archive files
 * never spike heap memory on Android.
 */
final class GameVerifier {
    private static final int MAGIC = 0x4D524831; // "MRH1"
    private static final int MAX_REPORTED_ISSUES = 20;

    interface Progress {
        boolean isCancelled();
        void onProgress(int checkedFiles, int totalFiles, long checkedBytes, long totalBytes);
    }

    static final class Result {
        final boolean cancelled;
        final int verifiedFiles;
        final int totalFiles;
        final long verifiedBytes;
        final int dlcVerifiedCount;
        final boolean usedDecryptedExecutable;
        final int missingCount;
        final int mismatchCount;
        final List<String> missingPaths = new ArrayList<>();
        final List<String> mismatchPaths = new ArrayList<>();

        Result(boolean cancelled, int verifiedFiles, int totalFiles, long verifiedBytes,
               int dlcVerifiedCount, boolean usedDecryptedExecutable,
               int missingCount, int mismatchCount,
               List<String> missing, List<String> mismatch) {
            this.cancelled = cancelled;
            this.verifiedFiles = verifiedFiles;
            this.totalFiles = totalFiles;
            this.verifiedBytes = verifiedBytes;
            this.dlcVerifiedCount = dlcVerifiedCount;
            this.usedDecryptedExecutable = usedDecryptedExecutable;
            this.missingCount = missingCount;
            this.mismatchCount = mismatchCount;
            this.missingPaths.addAll(missing);
            this.mismatchPaths.addAll(mismatch);
        }

        boolean isClean() {
            return !cancelled && missingCount == 0 && mismatchCount == 0;
        }
    }

    private static final class Entry {
        final String displayPath;
        final File file;
        final long[] hashes;

        Entry(String displayPath, File file, long[] hashes) {
            this.displayPath = displayPath;
            this.file = file;
            this.hashes = hashes;
        }
    }

    static Result verify(Context context, File gameRoot, Progress progress) throws IOException {
        List<Entry> plan = new ArrayList<>(5020);
        int dlcCount = 0;
        boolean usedDecrypted = false;
        File decryptedBin = new File(gameRoot, "decrypted/default.bin");
        boolean hasDecryptedBin = decryptedBin.isFile() && decryptedBin.length() > 0;

        try (InputStream raw = context.getAssets().open("game_hashes.bin");
             DataInputStream in = new DataInputStream(new BufferedInputStream(raw, 64 * 1024))) {
            if (in.readInt() != MAGIC) {
                throw new IOException("Invalid game_hashes.bin header");
            }
            int packCount = in.readUnsignedShort();
            for (int p = 0; p < packCount; p++) {
                int subLen = in.readUnsignedShort();
                boolean required = in.readUnsignedByte() != 0;
                byte[] subBytes = new byte[subLen];
                in.readFully(subBytes);
                String subDir = new String(subBytes, StandardCharsets.UTF_8);
                int fileCount = in.readInt();

                File packRoot = new File(gameRoot, subDir);
                boolean checkPack = required || new File(packRoot, "download.arc").isFile();
                if (!required && checkPack) {
                    dlcCount++;
                }

                for (int i = 0; i < fileCount; i++) {
                    int relLen = in.readUnsignedShort();
                    int hashCount = in.readUnsignedShort();
                    byte[] relBytes = new byte[relLen];
                    in.readFully(relBytes);
                    long[] hashes = new long[hashCount];
                    for (int h = 0; h < hashCount; h++) {
                        hashes[h] = in.readLong();
                    }
                    if (!checkPack) continue;

                    String relPath = new String(relBytes, StandardCharsets.UTF_8);
                    File target = new File(packRoot, relPath);
                    if (required && "default.xex".equals(relPath) && !target.isFile() && hasDecryptedBin) {
                        usedDecrypted = true;
                        continue;
                    }
                    plan.add(new Entry(subDir + "/" + relPath, target, hashes));
                }
            }
        }

        long totalBytes = 0;
        for (Entry entry : plan) {
            if (progress.isCancelled()) {
                return new Result(true, 0, plan.size(), 0, dlcCount, usedDecrypted,
                    0, 0, new ArrayList<>(), new ArrayList<>());
            }
            if (entry.file.isFile()) {
                totalBytes += entry.file.length();
            }
        }

        byte[] ioBuffer = new byte[1024 * 1024];
        Xxh3 hasher = new Xxh3();
        long checkedBytes = 0;
        long bytesSinceReport = 0;
        int checkedFiles = 0;
        int missingCount = 0;
        int mismatchCount = 0;
        List<String> missing = new ArrayList<>();
        List<String> mismatch = new ArrayList<>();

        progress.onProgress(0, plan.size(), 0, totalBytes);

        for (Entry entry : plan) {
            if (progress.isCancelled()) {
                return new Result(true, checkedFiles, plan.size(), checkedBytes, dlcCount, usedDecrypted,
                    missingCount, mismatchCount, missing, mismatch);
            }

            if (!entry.file.isFile()) {
                missingCount++;
                if (missing.size() < MAX_REPORTED_ISSUES) {
                    missing.add(entry.displayPath);
                }
                checkedFiles++;
                if (checkedFiles % 25 == 0) {
                    progress.onProgress(checkedFiles, plan.size(), checkedBytes, totalBytes);
                }
                continue;
            }

            hasher.reset();
            try (FileInputStream fis = new FileInputStream(entry.file)) {
                int n;
                while ((n = fis.read(ioBuffer)) > 0) {
                    if (progress.isCancelled()) {
                        return new Result(true, checkedFiles, plan.size(), checkedBytes, dlcCount, usedDecrypted,
                            missingCount, mismatchCount, missing, mismatch);
                    }
                    hasher.update(ioBuffer, 0, n);
                    checkedBytes += n;
                    bytesSinceReport += n;
                    if (bytesSinceReport >= 16L * 1024L * 1024L) {
                        bytesSinceReport = 0;
                        progress.onProgress(checkedFiles, plan.size(), checkedBytes, totalBytes);
                    }
                }
            }

            long actual = hasher.digest();
            boolean matched = false;
            for (long expected : entry.hashes) {
                if (actual == expected) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                mismatchCount++;
                if (mismatch.size() < MAX_REPORTED_ISSUES) {
                    mismatch.add(entry.displayPath);
                }
            }

            checkedFiles++;
            if (checkedFiles % 25 == 0) {
                bytesSinceReport = 0;
                progress.onProgress(checkedFiles, plan.size(), checkedBytes, totalBytes);
            }
        }

        progress.onProgress(checkedFiles, plan.size(), checkedBytes, totalBytes);
        return new Result(false, checkedFiles, plan.size(), checkedBytes, dlcCount, usedDecrypted,
            missingCount, mismatchCount, missing, mismatch);
    }
}
