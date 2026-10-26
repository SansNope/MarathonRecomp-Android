package io.github.sansnope.marathonrecomp;

import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.IOException;

/**
 * Game activity. SDL3 is linked statically into libmain.so, so only that library is
 * loaded and its SDL_main (the game's main()) is run by SDL3's unmodified SDLActivity.
 * The launcher-related hooks below were part of UnleashedRecomp-Android's SDLActivity.
 */
public class MarathonActivity extends SDLActivity {
    private static final String TAG = "MarathonActivity";

    /** Launcher -> game: pass --skip-logos to main(). */
    static final String EXTRA_SKIP_LOGOS = "io.github.sansnope.marathonrecomp.SKIP_LOGOS";

    private static volatile boolean gameStorageLocked;

    /** True once native code resolved its storage root; it stays fixed for this process. */
    static boolean isGameStorageLocked() {
        return gameStorageLocked;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestHighestRefreshRate();
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
        gameStorageLocked = true;
        File root = AppStorage.activeGameRoot(this);
        try {
            AppStorage.validateRoot(this, root);
            return root.getCanonicalPath();
        } catch (IOException exception) {
            throw new IllegalStateException(exception.getMessage(), exception);
        }
    }

    /** Native messages follow Android's language. */
    public String getLocalizedMessage(String resourceName, String detail) {
        int resource = getResources().getIdentifier(resourceName, "string", getPackageName());
        return resource == 0 ? "" : getString(resource, detail);
    }

    // Without an explicit mode, high refresh rate panels may stay at 60 Hz for the game window.
    private void requestHighestRefreshRate() {
        try {
            Display display = getWindowManager().getDefaultDisplay();
            Display.Mode current = display.getMode();
            Display.Mode best = current;
            for (Display.Mode mode : display.getSupportedModes()) {
                if (mode.getPhysicalWidth() == current.getPhysicalWidth() &&
                    mode.getPhysicalHeight() == current.getPhysicalHeight() &&
                    mode.getRefreshRate() > best.getRefreshRate()) {
                    best = mode;
                }
            }
            WindowManager.LayoutParams params = getWindow().getAttributes();
            params.preferredDisplayModeId = best.getModeId();
            getWindow().setAttributes(params);
            Log.v(TAG, "Preferred display mode " + best.getModeId() + ": " +
                best.getPhysicalWidth() + "x" + best.getPhysicalHeight() + " @ " + best.getRefreshRate() + " Hz");
        } catch (Exception exception) {
            Log.w(TAG, "Could not request the highest refresh rate", exception);
        }
    }
}
