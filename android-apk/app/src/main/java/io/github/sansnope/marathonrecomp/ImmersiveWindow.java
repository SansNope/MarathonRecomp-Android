package io.github.sansnope.marathonrecomp;

// Ported from UnleashedRecomp-Android 0.6.1 (org.libsdl.app.ImmersiveWindow).

import android.app.Activity;
import android.view.DisplayCutout;
import android.view.View;

/**
 * Full-screen presentation for the launcher and mod screens, matching the game: the
 * system bars are hidden (sticky immersive) and the window extends under the display
 * cutout (theme: windowLayoutInDisplayCutoutMode=shortEdges). The content view is
 * padded by the cutout's safe insets so no text sits under the camera hole, while the
 * window background still fills the cutout area instead of leaving a black bar.
 */
final class ImmersiveWindow {
    private ImmersiveWindow() {
    }

    /** Call after setContentView() and again from onWindowFocusChanged(true). */
    static void apply(Activity activity) {
        hideSystemBars(activity);

        View content = activity.findViewById(android.R.id.content);
        if (content == null) return;
        content.setOnApplyWindowInsetsListener((view, insets) -> {
            DisplayCutout cutout = insets.getDisplayCutout();
            if (cutout != null) {
                view.setPadding(cutout.getSafeInsetLeft(), cutout.getSafeInsetTop(),
                    cutout.getSafeInsetRight(), cutout.getSafeInsetBottom());
            } else {
                view.setPadding(0, 0, 0, 0);
            }
            return insets;
        });
        content.requestApplyInsets();
    }

    /**
     * Only hides the system bars; for screens that pad their own panes by the cutout
     * insets so each pane's background reaches the screen edge.
     */
    @SuppressWarnings("deprecation")
    static void hideSystemBars(Activity activity) {
        View decor = activity.getWindow().getDecorView();
        decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_FULLSCREEN
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }
}
