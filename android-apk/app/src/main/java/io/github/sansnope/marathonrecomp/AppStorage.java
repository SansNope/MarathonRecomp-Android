package io.github.sansnope.marathonrecomp;

// Ported from UnleashedRecomp-Android 0.6.1 (org.libsdl.app.AppStorage).

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/** Paths shared by the launcher, document provider and native Android storage policy. */
final class AppStorage {
    private static final String SHARED_SAVE_PATH = "shared_save_path";
    private static volatile boolean gameStorageLocked;

    private AppStorage() {}

    /** Native paths stay fixed until the game process exits. */
    static boolean isGameStorageLocked() {
        return gameStorageLocked;
    }

    static synchronized void lockGameStorage() {
        gameStorageLocked = true;
    }

    private static SharedPreferences storagePreferences(Context context) {
        return context.getSharedPreferences("game_storage", Context.MODE_PRIVATE);
    }

    /**
     * App-specific dir under Android/media. Unlike Android/data, on-device file
     * managers can browse it on Android 11+, so it serves as a PC-less fallback
     * for game files and driver imports. Null when external storage is unavailable.
     */
    static File mediaBase(Context context) {
        File[] dirs = context.getExternalMediaDirs();
        return (dirs != null && dirs.length > 0) ? dirs[0] : null;
    }

    /** Native GetDataRoot queries this through MarathonActivity.getGameStoragePath(). */
    static File activeGameRoot(Context context) {
        String selected = storagePreferences(context).getString("root", "");
        return selected.isEmpty() ? automaticGameRoot(context) : new File(selected);
    }

    private static File automaticGameRoot(Context context) {
        File internal = new File(context.getFilesDir(), "MarathonRecomp");
        if (new File(internal, "game").isDirectory()) {
            return internal;
        }

        File externalBase = context.getExternalFilesDir(null);
        File external = externalBase != null ? new File(externalBase, "MarathonRecomp") : null;
        if (external != null && new File(external, "game").isDirectory()) {
            return external;
        }

        File media = mediaBase(context);
        if (media != null) {
            File mediaRoot = new File(media, "MarathonRecomp");
            if (new File(mediaRoot, "game").isDirectory()) {
                return mediaRoot;
            }
        }

        return external != null ? external : internal;
    }

    static final class Choice {
        final String label;
        final File root; // null means the legacy automatic policy
        Choice(String label, File root) { this.label = label; this.root = root; }
    }

    static List<Choice> storageChoices(Context context) {
        List<Choice> choices = new ArrayList<>();
        choices.add(new Choice(context.getString(R.string.storage_auto), null));
        choices.add(new Choice(context.getString(R.string.storage_internal),
            new File(context.getFilesDir(), "MarathonRecomp")));
        addChoices(context, choices, context.getExternalFilesDirs(null), false);
        addChoices(context, choices, context.getExternalMediaDirs(), true);
        return choices;
    }

    private static void addChoices(Context context, List<Choice> choices, File[] bases, boolean media) {
        if (bases == null) return;
        StorageManager manager = context.getSystemService(StorageManager.class);
        for (File base : bases) {
            if (base == null || !Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(base))) continue;
            File root = new File(base, "MarathonRecomp");
            StorageVolume volume = manager != null ? manager.getStorageVolume(base) : null;
            String volumeName = volume != null ? volume.getDescription(context) : base.getPath();
            String label = context.getString(media ? R.string.storage_media : R.string.storage_app, volumeName);
            choices.add(new Choice(label, root));
        }
    }

    static File choiceRoot(Context context, Choice choice) {
        return choice.root != null ? choice.root : automaticGameRoot(context);
    }

    static void validateRoot(Context context, File root) throws IOException {
        File internal = new File(context.getFilesDir(), "MarathonRecomp");
        if (!root.getCanonicalFile().equals(internal.getCanonicalFile())) {
            try {
                if (!Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(root))) {
                    throw new IOException(context.getString(R.string.storage_unavailable, root));
                }
            } catch (IllegalArgumentException exception) {
                throw new IOException(context.getString(R.string.storage_unavailable, root), exception);
            }
        }
        ensureWritableDirectory(context, root);
    }

    private static void ensureWritableDirectory(Context context, File directory) throws IOException {
        if (!directory.isDirectory() && !directory.mkdirs() && !directory.isDirectory()) {
            throw new IOException(context.getString(R.string.error_storage_create, directory));
        }
        probeWritableDirectory(context, directory);
    }

    private static void probeWritableDirectory(Context context, File directory) throws IOException {
        File probe = File.createTempFile(".write-probe-", ".tmp", directory);
        if (!probe.delete()) throw new IOException(context.getString(R.string.error_storage_write, directory));
    }

    static synchronized void selectStorage(Context context, Choice choice) throws IOException {
        if (gameStorageLocked) throw new LocalizedIOException("storage_restart", null);
        validateRoot(context, choiceRoot(context, choice));
        SharedPreferences.Editor edit = storagePreferences(context).edit();
        if (choice.root == null) edit.remove("root");
        else edit.putString("root", choice.root.getCanonicalPath());
        if (!edit.commit()) throw new LocalizedIOException("error_storage_selection", null);
        notifyRootsChanged(context);
    }

    static File configFile(Context context) {
        return new File(activeGameRoot(context), ".config/MarathonRecomp/config.toml");
    }

    static File installationSaveDir(Context context) {
        return new File(activeGameRoot(context), ".config/MarathonRecomp/save");
    }

    static File primarySharedSaveDir(Context context) {
        File media = mediaBase(context);
        return media != null ? new File(media, "saves") : null;
    }

    static boolean usesSharedSaves(Context context) {
        return storagePreferences(context).contains(SHARED_SAVE_PATH);
    }

    /** Base save directory for the document provider and the game. */
    static File saveDir(Context context) {
        String selected = storagePreferences(context).getString(SHARED_SAVE_PATH, null);
        return selected != null ? new File(selected) : installationSaveDir(context);
    }

    static void validateActiveSaveDir(Context context) throws IOException {
        if (usesSharedSaves(context)) validateSaveDir(context, saveDir(context), true, false);
    }

    static void validateSaveDir(Context context, File directory, boolean shared, boolean create) throws IOException {
        try {
            if (shared && !Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(directory))) {
                throw new IOException();
            }
            if (create) ensureWritableDirectory(context, directory);
            else probeWritableDirectory(context, directory);
        } catch (IOException | IllegalArgumentException | SecurityException exception) {
            throw new IOException(context.getString(R.string.save_storage_unavailable, directory), exception);
        }
    }

    static synchronized void selectSaveStorage(Context context, File directory, boolean shared) throws IOException {
        if (gameStorageLocked) throw new LocalizedIOException("storage_restart", null);
        validateSaveDir(context, directory, shared, false);
        SharedPreferences preferences = storagePreferences(context);
        String previous = preferences.getString(SHARED_SAVE_PATH, null);
        SharedPreferences.Editor edit = preferences.edit();
        if (shared) edit.putString(SHARED_SAVE_PATH, directory.getCanonicalPath());
        else edit.remove(SHARED_SAVE_PATH);
        if (!edit.commit()) {
            // A failed commit still updates Android's in-memory preference map.
            preferences.edit().putString(SHARED_SAVE_PATH, previous).apply();
            throw new LocalizedIOException("error_storage_selection", null);
        }
        notifyRootsChanged(context);
    }

    private static void notifyRootsChanged(Context context) {
        context.getContentResolver().notifyChange(android.provider.DocumentsContract.buildRootsUri(
            context.getPackageName() + ".documents"), null);
    }

    static File transferRoot(Context context) {
        File external = context.getExternalFilesDir(null);
        return external != null ? external : context.getFilesDir();
    }

    /** Primary import folder; SAF imports and marker files go here. */
    static File driverImportDir(Context context) {
        return new File(transferRoot(context), "driver_import");
    }

    /** All folders the native side scans for dropped drivers, primary first. */
    static File[] driverImportDirs(Context context) {
        File media = mediaBase(context);
        return media != null
            ? new File[] { driverImportDir(context), new File(media, "driver_import") }
            : new File[] { driverImportDir(context) };
    }
}
