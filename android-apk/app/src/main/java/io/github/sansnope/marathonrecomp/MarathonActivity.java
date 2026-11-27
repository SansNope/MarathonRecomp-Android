package io.github.sansnope.marathonrecomp;

import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

/**
 * Game activity. SDL3 is linked statically into libmain.so, so only that library is
 * loaded and its SDL_main (the game's main()) is run by SDL3's unmodified SDLActivity.
 * The launcher-related hooks below were part of UnleashedRecomp-Android's SDLActivity.
 */
public class MarathonActivity extends SDLActivity {
    private static final String TAG = "MarathonActivity";

    /** Launcher -> game: pass --skip-logos to main(). */
    static final String EXTRA_SKIP_LOGOS = "io.github.sansnope.marathonrecomp.SKIP_LOGOS";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        applyPreferredRefreshRate(readConfiguredTargetFps());
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "main" };
    }

    @Override
    protected String[] getArguments() {
        if (getIntent() != null && getIntent().getBooleanExtra(EXTRA_SKIP_LOGOS, false)) {
            return new String[] { "--skip-logos" };
        }
        return new String[0];
    }

    /** Called by native storage_android.cpp so every component uses the same root. */
    public String getGameStoragePath() {
        AppStorage.lockGameStorage();
        File root = AppStorage.activeGameRoot(this);
        try {
            AppStorage.validateRoot(this, root);
            return root.getCanonicalPath();
        } catch (IOException exception) {
            throw new IllegalStateException(exception.getMessage(), exception);
        }
    }

    /** Called once by native code to freeze the save path for this process. */
    public String getSaveStoragePath() {
        AppStorage.lockGameStorage();
        File directory = AppStorage.saveDir(this);
        try {
            AppStorage.validateActiveSaveDir(this);
            return directory.getCanonicalPath();
        } catch (IOException exception) {
            throw new IllegalStateException(exception.getMessage(), exception);
        }
    }

    /** Native messages follow Android's language. */
    public String getLocalizedMessage(String resourceName, String detail) {
        int resource = getResources().getIdentifier(resourceName, "string", getPackageName());
        return resource == 0 ? "" : getString(resource, detail);
    }

    /** Called from native UpdateAndroidFrameRateVote when the confirmed FPS cap changes. */
    public void setPreferredRefreshRate(final float targetFps) {
        runOnUiThread(() -> applyPreferredRefreshRate(targetFps));
    }

    private float readConfiguredTargetFps() {
        File config = AppStorage.configFile(this);
        if (!config.isFile()) return 60.0f;
        String section = "";
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(new FileInputStream(config), StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                String trimmed = line.trim();
                if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                    section = trimmed.substring(1, trimmed.length() - 1).trim();
                    continue;
                }
                if (!"Video".equals(section) || trimmed.startsWith("#")) continue;
                int equals = trimmed.indexOf('=');
                if (equals <= 0) continue;
                String key = trimmed.substring(0, equals).trim();
                if ("FPS".equals(key)) {
                    int value = Integer.parseInt(trimmed.substring(equals + 1).trim());
                    return (value >= 15 && value < 241) ? (float) value : 240.0f;
                }
            }
        } catch (Exception ignored) {
            // Fall back to standard 60 Hz until native Config::Load votes explicitly.
        }
        return 60.0f;
    }

    // Match the display panel mode to the active FPS limit: keep ~60 Hz when capped at 60 FPS
    // to save power and avoid non-integer judder, and unlock high refresh rate modes above 60 FPS.
    private void applyPreferredRefreshRate(float targetFps) {
        try {
            Display display = getWindowManager().getDefaultDisplay();
            Display.Mode current = display.getMode();
            Display.Mode best = current;
            if (targetFps <= 60.5f) {
                float bestDelta = Float.MAX_VALUE;
                for (Display.Mode mode : display.getSupportedModes()) {
                    if (mode.getPhysicalWidth() != current.getPhysicalWidth() ||
                        mode.getPhysicalHeight() != current.getPhysicalHeight()) {
                        continue;
                    }
                    float delta = Math.abs(mode.getRefreshRate() - targetFps);
                    if (delta < bestDelta) {
                        bestDelta = delta;
                        best = mode;
                    }
                }
            } else {
                float maxRate = 0.0f;
                Display.Mode highest = current;
                Display.Mode smallestAboveTarget = null;
                for (Display.Mode mode : display.getSupportedModes()) {
                    if (mode.getPhysicalWidth() != current.getPhysicalWidth() ||
                        mode.getPhysicalHeight() != current.getPhysicalHeight()) {
                        continue;
                    }
                    float rate = mode.getRefreshRate();
                    if (rate > maxRate) {
                        maxRate = rate;
                        highest = mode;
                    }
                    if (rate + 0.5f >= targetFps &&
                        (smallestAboveTarget == null || rate < smallestAboveTarget.getRefreshRate())) {
                        smallestAboveTarget = mode;
                    }
                }
                best = smallestAboveTarget != null ? smallestAboveTarget : highest;
            }
            WindowManager.LayoutParams params = getWindow().getAttributes();
            if (params.preferredDisplayModeId != best.getModeId()) {
                params.preferredDisplayModeId = best.getModeId();
                getWindow().setAttributes(params);
                Log.v(TAG, "Preferred display mode " + best.getModeId() + ": " +
                    best.getPhysicalWidth() + "x" + best.getPhysicalHeight() + " @ " +
                    best.getRefreshRate() + " Hz (target " + targetFps + " FPS)");
            }
        } catch (Exception exception) {
            Log.w(TAG, "Could not apply preferred refresh rate", exception);
        }
    }
}
