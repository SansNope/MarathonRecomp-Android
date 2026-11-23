package io.github.sansnope.marathonrecomp;

// Ported from UnleashedRecomp-Android 0.6.1 (org.libsdl.app.StorageMigration).

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.nio.file.attribute.BasicFileAttributes;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.UUID;

/** Copy and verify an installation or its saves before switching folders. */
final class StorageMigration {
    private static final String[] SAVE_FILES = {"SonicNextSaveData.bin", "SonicNextAchievementData.bin"};

    interface Progress {
        boolean cancelled();
        void copied(long bytes);
    }

    static void copy(File source, File target, Progress progress) throws IOException {
        source = source.getCanonicalFile();
        target = target.getCanonicalFile();
        if (source.equals(target) || source.getPath().startsWith(target.getPath() + File.separator) ||
                target.getPath().startsWith(source.getPath() + File.separator)) {
            throw new LocalizedIOException("error_copy_separate", null);
        }
        File[] existing = target.listFiles();
        if (existing == null || existing.length != 0) throw new LocalizedIOException("error_copy_empty", null);
        File temporary = new File(target.getParentFile(), ".storage-copy-" + UUID.randomUUID());
        try {
            copyDirectory(source, temporary, source.getPath(), target.getPath(), progress);
            checkCancelled(progress);
            // target is empty and no game is running; publication stays on one volume.
            Files.delete(target.toPath());
            Files.move(temporary.toPath(), target.toPath());
        } finally {
            deleteTree(temporary);
        }
    }

    static boolean hasSaves(File directory) throws IOException {
        boolean result = false;
        for (String name : SAVE_FILES) {
            result |= isSaveFile(new File(directory, name));
        }
        return result;
    }

    static void copySaves(File source, File target) throws IOException {
        // Inspect both destinations before starting. Matching copies are allowed so an
        // interrupted transfer can be retried without replacing either save file.
        for (String name : SAVE_FILES) {
            File original = new File(source, name);
            isSaveFile(original);
            matchesExistingSave(original, new File(target, name));
        }
        for (String name : SAVE_FILES) {
            File original = new File(source, name);
            File destination = new File(target, name);
            if (!isSaveFile(original) || matchesExistingSave(original, destination)) continue;

            // Stage beside the folder so a sync app watching it only sees finished saves.
            Path temporary = Files.createTempFile(target.getParentFile().toPath(), ".marathon-save-", ".tmp");
            try {
                copyFile(original, temporary.toFile(), null);
                if (!matchesExistingSave(original, destination)) {
                    Files.move(temporary, destination.toPath());
                }
            } finally {
                Files.deleteIfExists(temporary);
            }
        }
    }

    private static boolean isSaveFile(File file) throws IOException {
        try {
            if (Files.readAttributes(file.toPath(), BasicFileAttributes.class, LinkOption.NOFOLLOW_LINKS)
                    .isRegularFile()) return true;
            throw new LocalizedIOException("save_storage_invalid_file", file);
        } catch (NoSuchFileException exception) {
            return false;
        }
    }

    private static boolean matchesExistingSave(File original, File destination) throws IOException {
        if (!isSaveFile(destination)) return false;
        if (isSaveFile(original) && Files.size(original.toPath()) == Files.size(destination.toPath())) {
            try (InputStream source = new BufferedInputStream(Files.newInputStream(original.toPath()));
                 InputStream target = new BufferedInputStream(Files.newInputStream(destination.toPath()))) {
                int value;
                while ((value = source.read()) != -1) {
                    if (value != target.read()) break;
                }
                if (value == -1 && target.read() == -1) return true;
            }
        }
        throw new LocalizedIOException("save_storage_conflict", destination);
    }

    private static void copyDirectory(File source, File target, String oldRoot, String newRoot,
            Progress progress) throws IOException {
        checkCancelled(progress);
        if (Files.isSymbolicLink(source.toPath())) throw new LocalizedIOException("error_copy_link", source);
        if (!target.mkdirs() && !target.isDirectory()) throw new LocalizedIOException("error_io_create", target);
        File[] children = source.listFiles();
        if (children == null) throw new LocalizedIOException("error_io_read", source);
        for (File child : children) {
            checkCancelled(progress);
            File destination = new File(target, child.getName());
            if (Files.isSymbolicLink(child.toPath())) throw new LocalizedIOException("error_copy_link", child);
            if (child.isDirectory()) copyDirectory(child, destination, oldRoot, newRoot, progress);
            else {
                copyFile(child, destination, progress);
                // These INIs may contain absolute mod/database/save paths. Rebase only
                // paths belonging to the old root; external mod folders remain valid.
                if (child.getName().toLowerCase(java.util.Locale.ROOT).endsWith(".ini") && child.length() <= 4 * 1024 * 1024) {
                    String content = new String(Files.readAllBytes(destination.toPath()), StandardCharsets.UTF_8);
                    String rebased = content.replace(oldRoot + File.separator, newRoot + File.separator);
                    if (!content.equals(rebased)) Files.write(destination.toPath(), rebased.getBytes(StandardCharsets.UTF_8));
                }
            }
        }
    }

    private static void copyFile(File source, File target, Progress progress) throws IOException {
        long length = source.length();
        long modified = source.lastModified();
        MessageDigest written = digest();
        try (FileInputStream input = new FileInputStream(source);
             FileOutputStream output = new FileOutputStream(target)) {
            byte[] buffer = new byte[256 * 1024];
            int count;
            while ((count = input.read(buffer)) != -1) {
                checkCancelled(progress);
                output.write(buffer, 0, count);
                written.update(buffer, 0, count);
                if (progress != null) progress.copied(count);
            }
            output.getFD().sync();
        }
        MessageDigest readBack = digest();
        try (FileInputStream input = new FileInputStream(target)) {
            byte[] buffer = new byte[256 * 1024];
            int count;
            while ((count = input.read(buffer)) != -1) {
                checkCancelled(progress);
                readBack.update(buffer, 0, count);
            }
        }
        if (source.length() != length || source.lastModified() != modified || target.length() != length ||
                !Arrays.equals(written.digest(), readBack.digest())) throw new LocalizedIOException("error_copy_verify", source);
    }

    private static MessageDigest digest() {
        try { return MessageDigest.getInstance("SHA-256"); }
        catch (NoSuchAlgorithmException exception) { throw new AssertionError(exception); }
    }

    private static void checkCancelled(Progress progress) throws IOException {
        if (progress != null && progress.cancelled()) throw new LocalizedIOException("error_copy_cancelled", null);
    }

    private static void deleteTree(File file) throws IOException {
        File[] children = file.listFiles();
        if (children != null) for (File child : children) deleteTree(child);
        Files.deleteIfExists(file.toPath());
    }
}
