package io.github.sansnope.marathonrecomp;

// Ported from UnleashedRecomp-Android 0.6.1 (org.libsdl.app.AppStorage).

import android.content.Context;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/** Paths shared by the launcher, document provider and native Android storage policy. */
final class AppStorage {
    private AppStorage() {}

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
        String selected = context.getSharedPreferences("game_storage", Context.MODE_PRIVATE)
            .getString("root", "");
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
        if (!root.isDirectory() && !root.mkdirs()) throw new IOException(context.getString(R.string.error_storage_create, root));
        File probe = File.createTempFile("write-probe-", ".tmp", root);
        if (!probe.delete()) throw new IOException(context.getString(R.string.error_storage_write, root));
    }

    static void selectStorage(Context context, Choice choice) throws IOException {
        validateRoot(context, choiceRoot(context, choice));
        android.content.SharedPreferences.Editor edit = context.getSharedPreferences("game_storage", Context.MODE_PRIVATE).edit();
        if (choice.root == null) edit.remove("root");
        else edit.putString("root", choice.root.getCanonicalPath());
        if (!edit.commit()) throw new LocalizedIOException("error_storage_selection", null);
        context.getContentResolver().notifyChange(android.provider.DocumentsContract.buildRootsUri(
            context.getPackageName() + ".documents"), null);
    }

    static File configFile(Context context) {
        return new File(activeGameRoot(context), ".config/MarathonRecomp/config.toml");
    }

    /** Where native paths.cpp keeps save data: <game root>/.config/MarathonRecomp/save. */
    static File saveDir(Context context) {
        return new File(activeGameRoot(context), ".config/MarathonRecomp/save");
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
