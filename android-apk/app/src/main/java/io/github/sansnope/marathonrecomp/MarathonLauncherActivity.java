package io.github.sansnope.marathonrecomp;

// Ported from UnleashedRecomp-Android 0.6.1 (org.libsdl.app.LauncherActivity). Changes for
// Marathon Recompiled: Sonic 2006 install layout and DLC packs, touch settings moved here
// (the in-game options menu has no Android entries yet), skip-logos passed as an argument.
// Not ported: GitHub update checks (no auto-update in this app), the Unleashed mod
// manager, audio presets, ISO/package staging and the validation/GFXReconstruct toggles.

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ClipData;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.InsetDrawable;
import android.graphics.drawable.RippleDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.function.Consumer;

/** Lightweight pre-flight screen. It deliberately does not load SDL or Vulkan. */
public final class MarathonLauncherActivity extends Activity {
    private static final int REQUEST_DRIVER = 1001;
    private static final int REQUEST_GAME_ZIP = 1002;
    private static final int REQUEST_GAME_TREE = 1003;
    private static final int DRIVER_EXPERIMENTAL_A725 = 4;
    private static final int DRIVER_ADRENO_8XX = 5;
    private static final int DRIVER_IMPORTED = 6;
    private static final int RENDER_MODE_SYSMEM = 2;
    // Must match the CONFIG_DEFINE_ENUM_TEMPLATEs in MarathonRecomp/user/config.cpp.
    private static final String[] DRIVER_VALUES = {
        "Auto", "System", "Bundled", "Vauzi710", "ExperimentalA725", "Adreno8xx", "Imported"
    };
    private static final String[] RENDER_MODE_VALUES = {"Auto", "GMEM", "Sysmem"};
    private static final String[] TOUCH_POLICY_VALUES = {"Auto", "Always On", "Off"};
    private static final String[] TOUCH_CAMERA_VALUES = {"Touch Area", "Right Stick", "Off"};
    private static final String[] TOUCH_STICK_VALUES = {"Analog", "D-Pad"};
    // Must match the DLC directories in MarathonRecomp/install/installer.cpp.
    private static final String[] DLC_DIRECTORIES = {
        "Additional Episode - Sonic Boss Attack", "Additional Episode - Shadow Boss Attack",
        "Additional Episode - Silver Boss Attack", "Additional Episode - Team Attack Amigo",
        "Additional Mission Pack - Sonic Very Hard", "Additional Mission Pack - Shadow Very Hard",
        "Additional Mission Pack - Silver Very Hard"
    };
    private static final String DLC_VALIDATION_FILE = "download.arc";

    // Settings sections, in rail order. The index is remembered across launches.
    private static final int SECTION_FILES = 0;
    private static final int SECTION_GRAPHICS = 1;
    private static final int SECTION_CONTROLS = 2;
    private static final int SECTION_ADVANCED = 3;

    // Deep blue palette in the spirit of the 2006 menus (see LauncherStyle); the window
    // background is in LauncherTheme. Panes are translucent over LauncherStyle.Backdrop.
    private static final int COLOR_PANE = 0x8C030816;
    private static final int COLOR_RAIL = 0xB3030816;
    private static final int COLOR_DIVIDER = 0xFF1F3F75;
    private static final int COLOR_FRAME = 0xFF5F8FD0;
    private static final int COLOR_BUTTON_TOP = 0xE6173466;
    private static final int COLOR_BUTTON_BOTTOM = 0xE60A1A3C;
    private static final int COLOR_PLAY_TOP = 0xFF3D9BFF;
    private static final int COLOR_PLAY_BOTTOM = 0xFF0B47C9;
    private static final int COLOR_PLAY_FRAME = 0xFFC8E8FF;
    private static final int COLOR_SELECTED_TOP = 0xCC2C74E0;
    private static final int COLOR_SELECTED_BOTTOM = 0xCC0F3888;
    private static final int COLOR_TEXT = 0xFFF2F6FF;
    private static final int COLOR_TEXT_SECONDARY = 0xFFA9BCDD;
    private static final int COLOR_TEXT_MUTED = 0xFF7088B0;
    private static final int COLOR_ACCENT = 0xFF5CC8FF;
    private static final int COLOR_GLOW = 0xCC3D9BFF;
    private static final int COLOR_SUCCESS = 0xFF74EBA8;
    private static final int COLOR_SUCCESS_BG = 0xCC0C3A2C;
    private static final int COLOR_DANGER = 0xFFFF8F80;
    private static final int COLOR_DANGER_BG = 0xCC45161B;
    private static final int COLOR_NEUTRAL_BG = 0xCC16294D;

    // Headings and the Play button use a slanted condensed face, like the 2006 menus.
    private static final Typeface FACE_HEADING = Typeface.create("sans-serif-condensed", Typeface.BOLD_ITALIC);

    private TextView installStatus;
    private TextView driverStatus;
    private TextView diagnosticsStatus;
    private Button playButton;
    private Spinner driverSpinner;
    private Spinner renderSpinner;
    private Spinner touchPolicySpinner;
    private Spinner touchCameraSpinner;
    private Spinner touchStickSpinner;
    private CompoundButton skipIntro;
    private SharedPreferences prefs;
    private InstallState lastInstallState;
    private TextView storageStatus;
    private boolean fileOperation;

    // Home pane.
    private LinearLayout homeContent;
    private LinearLayout statusChips;
    private TextView installSummary;
    private TextView driverSummary;

    // Settings pane.
    private ScrollView sectionScroll;
    private LinearLayout sectionContent;
    private final List<View> sections = new ArrayList<>();
    private final List<TextView> railItems = new ArrayList<>();

    private static final class InstallState {
        final boolean ready;
        final String message;
        final int dlcCount;
        InstallState(boolean ready, String message) {
            this(ready, message, 0);
        }
        InstallState(boolean ready, String message, int dlcCount) {
            this.ready = ready;
            this.message = message;
            this.dlcCount = dlcCount;
        }
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        prefs = getPreferences(MODE_PRIVATE);
        setContentView(buildPage());
        ImmersiveWindow.hideSystemBars(this);
        loadSettings();
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshStatuses();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) ImmersiveWindow.hideSystemBars(this);
    }

    /**
     * Landscape two-pane launcher: the home pane (status, Play, quick actions) stays
     * visible, and the settings pane shows one section at a time next to a section rail.
     * Every widget is created up front, so loadSettings()/saveSettings() never see a
     * missing view whichever section is selected.
     */
    private View buildPage() {
        LinearLayout root = row();
        root.setGravity(Gravity.NO_GRAVITY);
        root.setBaselineAligned(false);
        root.setBackground(new LauncherStyle.Backdrop());
        root.addView(buildHomePane(), new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 38f));
        View seam = new View(this);
        seam.setBackground(LauncherStyle.fadeLine(COLOR_GLOW, false));
        root.addView(seam, new LinearLayout.LayoutParams(dp(2), ViewGroup.LayoutParams.MATCH_PARENT));
        root.addView(buildSettingsPane(), new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 62f));

        // The window extends under the display cutout. Pad the outer panes instead of the
        // root so each pane's own background reaches the screen edge around the camera.
        root.setOnApplyWindowInsetsListener((view, insets) -> {
            android.view.DisplayCutout cutout = insets.getDisplayCutout();
            int left = cutout != null ? cutout.getSafeInsetLeft() : 0;
            int top = cutout != null ? cutout.getSafeInsetTop() : 0;
            int right = cutout != null ? cutout.getSafeInsetRight() : 0;
            int bottom = cutout != null ? cutout.getSafeInsetBottom() : 0;
            homeContent.setPadding(dp(20) + left, dp(18) + top, dp(20), dp(14) + bottom);
            sectionContent.setPadding(dp(18), dp(14) + top, dp(18) + right, dp(18) + bottom);
            return insets;
        });
        return root;
    }

    private View buildHomePane() {
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setBackgroundColor(COLOR_PANE);
        LinearLayout pane = column();
        homeContent = pane;
        pane.setPadding(dp(20), dp(18), dp(20), dp(14));
        scroll.addView(pane, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView title = heading(getString(R.string.launcher_title), 26);
        title.setShadowLayer(dp(8), 0, 0, COLOR_GLOW);
        pane.addView(title);
        View titleLine = new View(this);
        titleLine.setBackground(LauncherStyle.fadeLine(COLOR_ACCENT, true));
        LinearLayout.LayoutParams titleLineParams = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(2));
        titleLineParams.topMargin = dp(2);
        titleLineParams.bottomMargin = dp(4);
        pane.addView(titleLine, titleLineParams);
        TextView version = text("v" + currentVersion(), 13, false);
        version.setTextColor(COLOR_TEXT_MUTED);
        pane.addView(version);

        statusChips = row();
        statusChips.setPadding(0, dp(12), 0, 0);
        pane.addView(statusChips);

        installSummary = text("", 13, false);
        installSummary.setPadding(0, dp(8), 0, 0);
        installSummary.setMaxLines(4);
        installSummary.setEllipsize(TextUtils.TruncateAt.END);
        installSummary.setOnClickListener(view -> selectSection(SECTION_FILES));
        pane.addView(installSummary);

        driverSummary = summaryText(dp(10));
        driverSummary.setOnClickListener(view -> selectSection(SECTION_GRAPHICS));
        pane.addView(driverSummary);

        pane.addView(new View(this), new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f));

        playButton = primaryButton(R.string.launcher_play, view -> launchGame(false));
        LinearLayout.LayoutParams playParams = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(60));
        playParams.topMargin = dp(14);
        pane.addView(playButton, playParams);

        LinearLayout quick = row();
        quick.setPadding(0, dp(8), 0, 0);
        quick.addView(button(R.string.launcher_controls, view -> launchLayoutEditor()), spaced(0, 4));
        quick.addView(button(R.string.launcher_quick_files, view -> openFiles("game")), spaced(4, 4));
        quick.addView(button(R.string.launcher_info, view -> showInfo()), spaced(4, 0));
        pane.addView(quick);

        TextView exit = text(getString(R.string.launcher_exit), 13, false);
        exit.setTextColor(COLOR_TEXT_MUTED);
        exit.setGravity(Gravity.CENTER);
        exit.setPadding(0, dp(10), 0, dp(2));
        exit.setOnClickListener(view -> exitApp());
        pane.addView(exit, matchWrap());
        return scroll;
    }

    private View buildSettingsPane() {
        LinearLayout pane = row();
        pane.setGravity(Gravity.NO_GRAVITY);
        pane.setBaselineAligned(false);

        ScrollView railScroll = new ScrollView(this);
        railScroll.setBackgroundColor(COLOR_RAIL);
        LinearLayout rail = column();
        rail.setPadding(dp(8), dp(12), dp(8), dp(12));
        railScroll.addView(rail);
        pane.addView(railScroll, new LinearLayout.LayoutParams(dp(136), ViewGroup.LayoutParams.MATCH_PARENT));
        View railSeam = new View(this);
        railSeam.setBackground(LauncherStyle.fadeLine(COLOR_DIVIDER, false));
        pane.addView(railSeam, new LinearLayout.LayoutParams(dp(1), ViewGroup.LayoutParams.MATCH_PARENT));

        sectionScroll = new ScrollView(this);
        LinearLayout content = column();
        sectionContent = content;
        content.setPadding(dp(18), dp(14), dp(18), dp(18));
        sectionScroll.addView(content);
        pane.addView(sectionScroll, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1f));

        addSection(rail, content, R.string.launcher_game_files, R.string.launcher_game_files, this::buildFilesSection);
        addSection(rail, content, R.string.launcher_section_graphics, R.string.launcher_graphics, this::buildGraphicsSection);
        addSection(rail, content, R.string.launcher_controls, R.string.launcher_controls, this::buildControlsSection);
        addSection(rail, content, R.string.launcher_section_advanced, R.string.launcher_debug, this::buildAdvancedSection);
        selectSection(prefs.getInt("launcher_section", SECTION_FILES));
        return pane;
    }

    private void buildFilesSection(LinearLayout section) {
        installStatus = statusText();
        section.addView(installStatus);
        buttonGrid(section,
            button(R.string.launcher_install_game, view -> chooseInstallSource()),
            button(R.string.launcher_open_files, view -> openFiles("game")),
            button(R.string.launcher_recheck, view -> refreshStatuses()),
            button(R.string.launcher_open_saves, view -> openFiles("save")));
        storageStatus = statusText();
        storageStatus.setPadding(0, dp(16), 0, 0);
        section.addView(storageStatus);
        buttonGrid(section, button(R.string.storage_choose, view -> chooseStorage()));
    }

    private void buildGraphicsSection(LinearLayout section) {
        driverStatus = statusText();
        section.addView(driverStatus);
        driverSpinner = spinner(R.array.driver_labels);
        settingRow(section, R.string.launcher_driver, driverSpinner);
        renderSpinner = spinner(R.array.render_mode_labels);
        settingRow(section, R.string.launcher_render_mode, renderSpinner);
        driverSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                applyDriverPresetToLauncher();
                updateSummaries();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                applyDriverPresetToLauncher();
            }
        });
        buttonGrid(section,
            button(R.string.launcher_import_driver, view -> chooseDriver()),
            button(R.string.launcher_driver_folder, view -> openFiles("transfer")));
    }

    private void buildControlsSection(LinearLayout section) {
        touchPolicySpinner = spinner(R.array.touch_policy_labels);
        settingRow(section, R.string.launcher_touch_policy, touchPolicySpinner);
        touchCameraSpinner = spinner(R.array.touch_camera_labels);
        settingRow(section, R.string.launcher_touch_camera, touchCameraSpinner);
        touchStickSpinner = spinner(R.array.touch_stick_labels);
        settingRow(section, R.string.launcher_touch_stick, touchStickSpinner);
        buttonGrid(section, button(R.string.launcher_edit_layout, view -> launchLayoutEditor()));
    }

    private void buildAdvancedSection(LinearLayout section) {
        skipIntro = switchRow(section, R.string.launcher_skip_intro);
        diagnosticsStatus = statusText();
        diagnosticsStatus.setPadding(0, dp(12), 0, 0);
        section.addView(diagnosticsStatus);
        buttonGrid(section, button(R.string.launcher_open_logs, view -> openFiles("transfer")));
    }

    private void addSection(LinearLayout rail, LinearLayout content, int railLabelId, int titleId,
            Consumer<LinearLayout> builder) {
        final int index = sections.size();
        LinearLayout section = column();
        TextView title = heading(getString(titleId), 19);
        title.setTextColor(COLOR_ACCENT);
        section.addView(title);
        View titleLine = new View(this);
        titleLine.setBackground(LauncherStyle.fadeLine(COLOR_GLOW, true));
        LinearLayout.LayoutParams titleLineParams = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(2));
        titleLineParams.topMargin = dp(2);
        titleLineParams.bottomMargin = dp(10);
        section.addView(titleLine, titleLineParams);
        builder.accept(section);
        content.addView(section, matchWrap());
        sections.add(section);

        TextView item = text(getString(railLabelId), 14, false);
        item.setPadding(dp(12), dp(10), dp(12), dp(10));
        item.setSingleLine(true);
        item.setEllipsize(TextUtils.TruncateAt.END);
        item.setOnClickListener(view -> selectSection(index));
        LinearLayout.LayoutParams itemParams = matchWrap();
        itemParams.bottomMargin = dp(2);
        rail.addView(item, itemParams);
        railItems.add(item);
    }

    private void selectSection(int index) {
        if (index < 0 || index >= sections.size()) index = SECTION_FILES;
        for (int i = 0; i < sections.size(); i++) {
            boolean selected = i == index;
            sections.get(i).setVisibility(selected ? View.VISIBLE : View.GONE);
            TextView item = railItems.get(i);
            item.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_SECONDARY);
            item.setTypeface(selected ? FACE_HEADING : Typeface.DEFAULT);
            item.setBackground(pressable(selected
                ? new LauncherStyle.Panel(COLOR_SELECTED_TOP, COLOR_SELECTED_BOTTOM, COLOR_ACCENT, dp(10), dp(1), true)
                : new LauncherStyle.Panel(0, 0, 0, dp(10), 0, false)));
        }
        if (sectionScroll != null) sectionScroll.scrollTo(0, 0);
        prefs.edit().putInt("launcher_section", index).apply();
    }

    private void updateSummaries() {
        if (driverSummary == null || driverSpinner == null) return;
        driverSummary.setText(getString(R.string.launcher_summary_driver, driverSpinner.getSelectedItem()));
    }

    private String currentVersion() {
        try {
            return getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (Exception exception) {
            return "?";
        }
    }

    /** Credits, icon attribution and the unofficial-port notice (launcher_info_text, per language). */
    private void showInfo() {
        LinearLayout content = column();
        content.setPadding(dp(22), dp(18), dp(22), dp(6));

        TextView title = heading(getString(R.string.launcher_info_title), 20);
        title.setTextColor(COLOR_ACCENT);
        content.addView(title);
        View titleLine = new View(this);
        titleLine.setBackground(LauncherStyle.fadeLine(COLOR_GLOW, true));
        LinearLayout.LayoutParams titleLineParams = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(2));
        titleLineParams.topMargin = dp(2);
        titleLineParams.bottomMargin = dp(12);
        content.addView(titleLine, titleLineParams);

        TextView body = text("", 15, false);
        body.setLineSpacing(0, 1.15f);
        // Auto-linking applies on setText, so the mask is set first.
        body.setAutoLinkMask(Linkify.WEB_URLS);
        body.setLinkTextColor(COLOR_ACCENT);
        body.setText(R.string.launcher_info_text);
        body.setMovementMethod(LinkMovementMethod.getInstance());
        content.addView(body, matchWrap());

        TextView version = text("Marathon Recompiled v" + currentVersion(), 13, false);
        version.setTextColor(COLOR_TEXT_MUTED);
        version.setPadding(0, dp(14), 0, 0);
        content.addView(version);

        ScrollView scroll = new ScrollView(this);
        scroll.addView(content);

        AlertDialog dialog = new AlertDialog.Builder(this)
            .setView(scroll)
            .setPositiveButton(android.R.string.ok, null)
            .create();
        dialog.show();
        if (dialog.getWindow() != null) {
            dialog.getWindow().setBackgroundDrawable(new InsetDrawable(
                new LauncherStyle.Panel(0xF20B2459, 0xF2040B1F, COLOR_FRAME, dp(16), dp(1), true), dp(16)));
        }
    }

    private void exitApp() {
        // Back/Home keep a started game alive in this process, and SDL cannot boot a
        // second session in it, so leaving with a running game means ending the process.
        if (!MarathonActivity.isGameStorageLocked()) {
            finishAndRemoveTask();
            return;
        }
        new AlertDialog.Builder(this)
            .setMessage(R.string.launcher_exit_running)
            .setNegativeButton(android.R.string.cancel, null)
            .setPositiveButton(R.string.launcher_exit, (dialog, which) -> {
                finishAndRemoveTask();
                android.os.Process.killProcess(android.os.Process.myPid());
            })
            .show();
    }

    private void refreshStatuses() {
        File currentRoot = AppStorage.activeGameRoot(this);
        storageStatus.setText(getString(R.string.storage_current, currentRoot));
        lastInstallState = inspectInstallation();
        installStatus.setText(lastInstallState.message);
        installStatus.setTextColor(lastInstallState.ready ? COLOR_SUCCESS : COLOR_DANGER);
        playButton.setEnabled(lastInstallState.ready);
        playButton.setAlpha(lastInstallState.ready ? 1f : 0.45f);

        statusChips.removeAllViews();
        if (lastInstallState.ready) {
            statusChips.addView(chip(getString(R.string.launcher_chip_game_ready), COLOR_SUCCESS, COLOR_SUCCESS_BG));
            boolean allDlc = lastInstallState.dlcCount == DLC_DIRECTORIES.length;
            statusChips.addView(chip(getString(R.string.launcher_chip_dlc, lastInstallState.dlcCount,
                DLC_DIRECTORIES.length), allDlc ? COLOR_SUCCESS : COLOR_TEXT_SECONDARY,
                allDlc ? COLOR_SUCCESS_BG : COLOR_NEUTRAL_BG));
        } else {
            statusChips.addView(chip(getString(R.string.launcher_chip_game_missing), COLOR_DANGER, COLOR_DANGER_BG));
        }
        // The home pane repeats the full explanation only when something needs action.
        installSummary.setVisibility(lastInstallState.ready ? View.GONE : View.VISIBLE);
        installSummary.setText(installStatus.getText());
        installSummary.setTextColor(COLOR_DANGER);
        updateSummaries();

        File installedMarker = new File(getFilesDir(), "turnip/last_imported_driver.txt");
        File recoveryMarker = new File(getFilesDir(), "vulkan_startup_state.txt");
        int pending = 0;
        for (File importDir : AppStorage.driverImportDirs(this)) {
            pending += countDriverPackages(importDir);
        }
        String imported = readFirstLine(installedMarker);
        if (recoveryMarker.isFile()) {
            driverStatus.setText(R.string.launcher_driver_recovery);
            driverStatus.setTextColor(COLOR_DANGER);
        } else if (pending > 0) {
            driverStatus.setText(getString(R.string.launcher_driver_pending, pending));
            driverStatus.setTextColor(COLOR_TEXT_SECONDARY);
        } else if (!imported.isEmpty()) {
            driverStatus.setText(getString(R.string.launcher_driver_installed, imported));
            driverStatus.setTextColor(COLOR_TEXT_SECONDARY);
        } else {
            driverStatus.setText(R.string.launcher_driver_builtin);
            driverStatus.setTextColor(COLOR_TEXT_SECONDARY);
        }

        File log = new File(AppStorage.transferRoot(this), "log.txt");
        diagnosticsStatus.setText(log.isFile()
            ? getString(R.string.launcher_log_found, formatBytes(log.length()))
            : getString(R.string.launcher_log_missing));
    }

    private InstallState inspectInstallation() {
        File root = AppStorage.activeGameRoot(this);
        try {
            AppStorage.validateRoot(this, root);
        } catch (IOException exception) {
            return new InstallState(false, FileErrorMessages.describe(this, exception));
        }

        // Same rule as Installer::checkGameInstall(): a decrypted executable produced by the
        // desktop installer, or the original game/default.xex.
        File executable = new File(root, "game/default.xex");
        File decrypted = new File(root, "decrypted/default.bin");
        boolean installed = (executable.isFile() && executable.length() > 0) ||
            (decrypted.isFile() && decrypted.length() > 0);
        if (!installed) {
            File misplaced = findFile(root, "default.xex", 3);
            if (misplaced != null && !misplaced.equals(executable)) {
                return new InstallState(false, getString(R.string.error_game_nested,
                    relativePath(root, misplaced), root));
            }
            return new InstallState(false, getString(R.string.error_game_missing, root));
        }

        File archives = new File(root, "game/xenon/archives");
        if (executable.isFile() && !archives.isDirectory()) {
            return new InstallState(false, getString(R.string.error_game_partial,
                "\n• game/xenon/archives", root));
        }

        int dlc = 0;
        for (String directory : DLC_DIRECTORIES) {
            if (new File(root, "dlc/" + directory + "/" + DLC_VALIDATION_FILE).isFile()) dlc++;
        }
        return new InstallState(true, dlc == DLC_DIRECTORIES.length
            ? getString(R.string.game_ready_all_dlc)
            : getString(R.string.game_ready_missing_dlc, dlc, DLC_DIRECTORIES.length), dlc);
    }

    private void loadSettings() {
        Map<String, String> config = readConfig(AppStorage.configFile(this));
        String configuredDriver = config.get("Video.VulkanDriver");
        // UnleashedRecomp 0.5.3 shipped a separate "ExperimentalA8xx" profile; it is superseded by Adreno8xx.
        if (configuredDriver != null && configuredDriver.replace("\"", "").trim().equalsIgnoreCase("ExperimentalA8xx"))
            configuredDriver = "Adreno8xx";
        select(driverSpinner, configuredDriver, DRIVER_VALUES);
        select(renderSpinner, config.get("Video.RenderMode"), RENDER_MODE_VALUES);
        select(touchPolicySpinner, config.get("Input.TouchControls"), TOUCH_POLICY_VALUES);
        select(touchCameraSpinner, config.get("Input.TouchCamera"), TOUCH_CAMERA_VALUES);
        select(touchStickSpinner, config.get("Input.TouchStickMode"), TOUCH_STICK_VALUES);
        applyDriverPresetToLauncher();
        skipIntro.setChecked(prefs.getBoolean("skip_logos", false));
    }

    private boolean saveSettings() {
        applyDriverPresetToLauncher();
        LinkedHashMap<String, String> values = new LinkedHashMap<>();
        values.put("Video.VulkanDriver", quote(DRIVER_VALUES[driverSpinner.getSelectedItemPosition()]));
        values.put("Video.RenderMode", quote(RENDER_MODE_VALUES[renderSpinner.getSelectedItemPosition()]));
        values.put("Input.TouchControls", quote(TOUCH_POLICY_VALUES[touchPolicySpinner.getSelectedItemPosition()]));
        values.put("Input.TouchCamera", quote(TOUCH_CAMERA_VALUES[touchCameraSpinner.getSelectedItemPosition()]));
        values.put("Input.TouchStickMode", quote(TOUCH_STICK_VALUES[touchStickSpinner.getSelectedItemPosition()]));
        prefs.edit().putBoolean("skip_logos", skipIntro.isChecked()).apply();
        try {
            AppStorage.validateRoot(this, AppStorage.activeGameRoot(this));
            patchConfig(AppStorage.configFile(this), values);
            return true;
        } catch (IOException exception) {
            showError(getString(R.string.error_settings_save, FileErrorMessages.describe(this, exception)));
            return false;
        }
    }

    private void launchGame(boolean editControls) {
        if (fileOperation) { showError(getString(R.string.storage_busy)); return; }
        if (!saveSettings()) return;
        InstallState current = inspectInstallation();
        if (!current.ready) {
            showError(current.message);
            refreshStatuses();
            return;
        }
        // Play clears a marker left by an editor request that never reached the game,
        // so it cannot open the editor instead of the game on the next cold launch.
        try {
            setMarker(new File(AppStorage.activeGameRoot(this), "touch_layout_edit.txt"), editControls, "1\n");
        } catch (IOException exception) {
            if (editControls) {
                showError(getString(R.string.error_layout_editor, FileErrorMessages.describe(this, exception)));
                return;
            }
        }
        Intent game = new Intent(this, MarathonActivity.class);
        game.putExtra(MarathonActivity.EXTRA_SKIP_LOGOS, skipIntro.isChecked());
        startActivity(game);
    }

    private void launchLayoutEditor() {
        InstallState current = inspectInstallation();
        if (!current.ready) {
            showError(getString(R.string.error_layout_requires_game) + "\n\n" + current.message);
            return;
        }
        launchGame(true);
    }

    private void chooseDriver() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {
            "application/octet-stream", "application/zip", "application/x-zip-compressed"
        });
        startActivityForResult(intent, REQUEST_DRIVER);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null) return;
        switch (requestCode) {
            case REQUEST_GAME_ZIP:
                startInstall(data.getData(), true);
                return;
            case REQUEST_GAME_TREE:
                startInstall(data.getData(), false);
                return;
            case REQUEST_DRIVER:
                break;
            default:
                return;
        }
        Uri uri = data.getData();
        String name = queryDisplayName(uri);
        String lower = name.toLowerCase(Locale.ROOT);
        if (!lower.endsWith(".so") && !lower.endsWith(".zip")) {
            showError(getString(R.string.error_driver_extension));
            return;
        }
        File directory = AppStorage.driverImportDir(this);
        directory.mkdirs();
        File target = uniqueFile(directory, safeName(name));
        try (InputStream input = getContentResolver().openInputStream(uri);
             FileOutputStream output = new FileOutputStream(target)) {
            if (input == null) throw new LocalizedIOException("error_io_read", uri);
            byte[] buffer = new byte[64 * 1024];
            int count;
            long total = 0;
            while ((count = input.read(buffer)) >= 0) {
                output.write(buffer, 0, count);
                total += count;
            }
            if (total == 0) throw new LocalizedIOException("error_file_empty", null);
            driverSpinner.setSelection(DRIVER_IMPORTED);
            Toast.makeText(this, getString(R.string.driver_imported, target.getName()), Toast.LENGTH_LONG).show();
            refreshStatuses();
        } catch (IOException exception) {
            target.delete();
            showError(getString(R.string.error_driver_copy, FileErrorMessages.describe(this, exception)));
        }
    }

    // ------------------------------------------------------------------
    // Game files installer: copy from a user-picked ZIP archive or folder
    // into the game root, locating the content root automatically.
    // ------------------------------------------------------------------

    private void chooseInstallSource() {
        if (fileOperation || MarathonActivity.isGameStorageLocked()) {
            showError(getString(R.string.storage_restart));
            return;
        }
        String[] items = new String[] {
            getString(R.string.install_source_zip), getString(R.string.install_source_folder)
        };
        new AlertDialog.Builder(this)
            .setTitle(R.string.launcher_install_game)
            .setItems(items, (dialog, which) -> {
                if (which == 0) {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    intent.setType("*/*");
                    intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[] {
                        "application/zip", "application/x-zip-compressed", "application/octet-stream" });
                    startActivityForResult(intent, REQUEST_GAME_ZIP);
                } else {
                    startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), REQUEST_GAME_TREE);
                }
            })
            .show();
    }

    /** One copyable file inside the picked source, addressed by a relative path. */
    private static final class SourceEntry {
        final String path;     // normalized, '/'-separated, no leading slash
        final Uri document;    // tree sources only; null for zip entries
        SourceEntry(String path, Uri document) {
            this.path = path;
            this.document = document;
        }
    }

    private static final class InstallProgress {
        volatile boolean cancelled;
        TextView label;
        AlertDialog dialog;
        int files;
        long bytes;
    }

    private void startInstall(Uri source, boolean isZip) {
        fileOperation = true;
        InstallProgress progress = new InstallProgress();
        progress.label = text(getString(R.string.install_scanning), 15, false);
        progress.label.setPadding(dp(20), dp(16), dp(20), dp(16));
        progress.dialog = new AlertDialog.Builder(this)
            .setTitle(R.string.install_progress_title)
            .setView(progress.label)
            .setCancelable(false)
            .setNegativeButton(R.string.install_cancel, (dialog, which) -> progress.cancelled = true)
            .show();

        new Thread(() -> {
            try {
                List<SourceEntry> entries = isZip ? listZipEntries(source) : listTreeEntries(source);
                Map<SourceEntry, File> plan = planGameInstall(entries);
                if (plan.isEmpty()) {
                    finishInstall(progress, getString(R.string.error_install_no_game), false);
                    return;
                }

                if (isZip) {
                    copyZipEntries(source, plan, progress);
                } else {
                    copyTreeEntries(plan, progress);
                }

                finishInstall(progress, progress.cancelled
                    ? getString(R.string.install_cancelled)
                    : getString(R.string.install_done_game), !progress.cancelled);
            } catch (Exception exception) {
                String reason = FileErrorMessages.describe(this, exception);
                finishInstall(progress, getString(R.string.error_install_failed, reason), false);
            }
        }, "installer").start();
    }

    private void finishInstall(InstallProgress progress, String message, boolean success) {
        runOnUiThread(() -> {
            fileOperation = false;
            progress.dialog.dismiss();
            new AlertDialog.Builder(this)
                .setTitle(success ? R.string.install_progress_title : R.string.error_title)
                .setMessage(message)
                .setPositiveButton(android.R.string.ok, null)
                .show();
            refreshStatuses();
        });
    }

    private void publishProgress(InstallProgress progress) {
        String status = getString(R.string.install_progress_status, progress.files, formatBytes(progress.bytes));
        runOnUiThread(() -> progress.label.setText(status));
    }

    private void chooseStorage() {
        if (fileOperation) { showError(getString(R.string.storage_busy)); return; }
        if (MarathonActivity.isGameStorageLocked()) { showError(getString(R.string.storage_restart)); return; }
        List<AppStorage.Choice> choices = AppStorage.storageChoices(this);
        String[] labels = new String[choices.size()];
        for (int index = 0; index < choices.size(); index++) {
            AppStorage.Choice choice = choices.get(index);
            labels[index] = choice.label + "\n" + AppStorage.choiceRoot(this, choice);
        }
        new AlertDialog.Builder(this).setTitle(R.string.storage_choose)
            .setItems(labels, (dialog, index) -> confirmStorage(choices.get(index)))
            .setNegativeButton(android.R.string.cancel, null).show();
    }

    private void confirmStorage(AppStorage.Choice choice) {
        File oldRoot = AppStorage.activeGameRoot(this);
        File newRoot = AppStorage.choiceRoot(this, choice);
        try {
            AppStorage.validateRoot(this, newRoot);
            if (oldRoot.getCanonicalFile().equals(newRoot.getCanonicalFile())) {
                useStorage(choice);
                return;
            }
        } catch (IOException exception) { showError(FileErrorMessages.describe(this, exception)); return; }
        AlertDialog.Builder dialog = new AlertDialog.Builder(this)
            .setTitle(choice.label)
            .setMessage(getString(R.string.storage_confirm, newRoot))
            .setNegativeButton(android.R.string.cancel, null)
            .setPositiveButton(R.string.storage_use, (unused, which) -> useStorage(choice));
        File[] files = newRoot.listFiles();
        if (oldRoot.isDirectory() && files != null && files.length == 0) {
            dialog.setNeutralButton(R.string.storage_copy, (unused, which) -> copyStorage(oldRoot, newRoot, choice));
        }
        dialog.show();
    }

    private void useStorage(AppStorage.Choice choice) {
        try {
            AppStorage.selectStorage(this, choice);
            loadSettings();
            refreshStatuses();
        } catch (IOException exception) { showError(FileErrorMessages.describe(this, exception)); }
    }

    private void copyStorage(File oldRoot, File newRoot, AppStorage.Choice choice) {
        if (!saveSettings()) return;
        fileOperation = true;
        InstallProgress progress = new InstallProgress();
        progress.label = text(getString(R.string.storage_copying), 15, false);
        progress.label.setPadding(dp(20), dp(16), dp(20), dp(16));
        progress.dialog = new AlertDialog.Builder(this).setTitle(R.string.storage_copy)
            .setView(progress.label).setCancelable(false)
            .setNegativeButton(R.string.install_cancel, (dialog, which) -> progress.cancelled = true).show();
        new Thread(() -> {
            String error = null;
            try {
                StorageMigration.copy(oldRoot, newRoot, new StorageMigration.Progress() {
                    long lastUpdate;
                    public boolean cancelled() { return progress.cancelled; }
                    public void copied(long bytes) {
                        progress.bytes += bytes;
                        if (progress.bytes - lastUpdate >= 16 * 1024 * 1024) {
                            lastUpdate = progress.bytes;
                            runOnUiThread(() -> progress.label.setText(getString(R.string.storage_copy_progress,
                                formatBytes(progress.bytes))));
                        }
                    }
                });
                AppStorage.selectStorage(this, choice);
            } catch (Exception exception) {
                error = FileErrorMessages.describe(this, exception);
            }
            String result = error;
            runOnUiThread(() -> {
                fileOperation = false;
                progress.dialog.dismiss();
                if (result != null) showError(getString(R.string.error_install_failed, result));
                else {
                    loadSettings();
                    new AlertDialog.Builder(this).setMessage(R.string.storage_copied)
                        .setPositiveButton(android.R.string.ok, null).show();
                }
                refreshStatuses();
            });
        }, "storage-copy").start();
    }

    /** Normalizes a zip/tree relative path; returns null when it must be skipped. */
    private static String normalizeRelativePath(String raw) {
        String path = raw.replace('\\', '/');
        while (path.startsWith("/")) path = path.substring(1);
        if (path.isEmpty() || path.endsWith("/")) return null;
        for (String segment : path.split("/")) {
            if (segment.isEmpty() || segment.equals(".") || segment.equals("..")) return null;
        }
        return path;
    }

    private List<SourceEntry> listZipEntries(Uri source) throws IOException {
        List<SourceEntry> entries = new ArrayList<>();
        try (java.util.zip.ZipInputStream zip = new java.util.zip.ZipInputStream(
                openSourceStream(source))) {
            java.util.zip.ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (entry.isDirectory()) continue;
                String path = normalizeRelativePath(entry.getName());
                if (path != null) entries.add(new SourceEntry(path, null));
            }
        }
        return entries;
    }

    private List<SourceEntry> listTreeEntries(Uri treeUri) {
        List<SourceEntry> entries = new ArrayList<>();
        listTreeChildren(treeUri, DocumentsContract.getTreeDocumentId(treeUri), "", entries, 0);
        return entries;
    }

    private void listTreeChildren(Uri treeUri, String documentId, String prefix,
            List<SourceEntry> out, int depth) {
        if (depth > 12) return;
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, documentId);
        try (android.database.Cursor cursor = getContentResolver().query(children, new String[] {
                DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                DocumentsContract.Document.COLUMN_MIME_TYPE }, null, null, null)) {
            if (cursor == null) return;
            while (cursor.moveToNext()) {
                String childId = cursor.getString(0);
                String name = cursor.getString(1);
                String mime = cursor.getString(2);
                if (name == null || childId == null) continue;
                if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) {
                    listTreeChildren(treeUri, childId, prefix + name + "/", out, depth + 1);
                } else {
                    String path = normalizeRelativePath(prefix + name);
                    if (path != null) {
                        out.add(new SourceEntry(path,
                            DocumentsContract.buildDocumentUriUsingTree(treeUri, childId)));
                    }
                }
            }
        }
    }

    /**
     * Everything under the folder that holds game/default.xex goes into the game root
     * (game/, optional dlc/). A picked source that is itself the extracted game folder
     * (default.xex next to xenon/) is installed as game/.
     */
    private Map<SourceEntry, File> planGameInstall(List<SourceEntry> entries) {
        String marker = "game/default.xex";
        String prefix = null;
        for (SourceEntry entry : entries) {
            if (entry.path.equals(marker) || entry.path.endsWith("/" + marker)) {
                String candidate = entry.path.substring(0, entry.path.length() - marker.length());
                if (prefix == null || candidate.length() < prefix.length()) prefix = candidate;
            }
        }
        Map<SourceEntry, File> plan = new LinkedHashMap<>();
        File root = AppStorage.activeGameRoot(this);
        if (prefix != null) {
            for (SourceEntry entry : entries) {
                if (entry.path.startsWith(prefix)) {
                    plan.put(entry, new File(root, entry.path.substring(prefix.length())));
                }
            }
            return plan;
        }

        String gamePrefix = null;
        for (SourceEntry entry : entries) {
            if (!entry.path.equals("default.xex") && !entry.path.endsWith("/default.xex")) continue;
            String candidate = entry.path.substring(0, entry.path.length() - "default.xex".length());
            boolean hasArchives = false;
            for (SourceEntry other : entries) {
                if (other.path.startsWith(candidate + "xenon/archives/")) { hasArchives = true; break; }
            }
            if (hasArchives && (gamePrefix == null || candidate.length() < gamePrefix.length())) gamePrefix = candidate;
        }
        if (gamePrefix == null) return plan;
        for (SourceEntry entry : entries) {
            if (entry.path.startsWith(gamePrefix)) {
                plan.put(entry, new File(root, "game/" + entry.path.substring(gamePrefix.length())));
            }
        }
        return plan;
    }

    private InputStream openSourceStream(Uri uri) throws IOException {
        InputStream input = getContentResolver().openInputStream(uri);
        if (input == null) throw new LocalizedIOException("error_io_read", uri);
        return input;
    }

    private void copyZipEntries(Uri source, Map<SourceEntry, File> plan, InstallProgress progress)
            throws IOException {
        Map<String, File> byPath = new LinkedHashMap<>();
        for (Map.Entry<SourceEntry, File> item : plan.entrySet()) {
            byPath.put(item.getKey().path, item.getValue());
        }
        try (java.util.zip.ZipInputStream zip = new java.util.zip.ZipInputStream(
                openSourceStream(source))) {
            java.util.zip.ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null && !progress.cancelled) {
                if (entry.isDirectory()) continue;
                String path = normalizeRelativePath(entry.getName());
                File destination = path != null ? byPath.get(path) : null;
                if (destination == null) continue;
                copyStreamToFile(zip, destination, progress);
            }
        }
    }

    private void copyTreeEntries(Map<SourceEntry, File> plan, InstallProgress progress)
            throws IOException {
        for (Map.Entry<SourceEntry, File> item : plan.entrySet()) {
            if (progress.cancelled) return;
            try (InputStream input = openSourceStream(item.getKey().document)) {
                copyStreamToFile(input, item.getValue(), progress);
            }
        }
    }

    private void copyStreamToFile(InputStream input, File destination, InstallProgress progress)
            throws IOException {
        File parent = destination.getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
            throw new LocalizedIOException("error_io_create", parent);
        }
        try (FileOutputStream output = new FileOutputStream(destination)) {
            byte[] buffer = new byte[256 * 1024];
            int count;
            long sinceUpdate = 0;
            while ((count = input.read(buffer)) >= 0) {
                if (progress.cancelled) return;
                output.write(buffer, 0, count);
                progress.bytes += count;
                sinceUpdate += count;
                if (sinceUpdate >= 16 * 1024 * 1024) {
                    sinceUpdate = 0;
                    publishProgress(progress);
                }
            }
        }
        progress.files++;
        if (progress.files % 25 == 0) publishProgress(progress);
    }

    private void openFiles(String rootId) {
        try {
            Uri directory = DocumentsContract.buildRootUri(getPackageName() + ".documents", rootId);
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setDataAndType(directory, "vnd.android.document/root");
            intent.setClipData(ClipData.newRawUri("Marathon Recompiled files", directory));
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION |
                Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
            startActivity(intent);
        } catch (Exception exception) {
            showError(getString(R.string.error_open_files));
        }
    }

    private Spinner spinner(int arrayId) {
        Spinner spinner = new Spinner(this);
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this, arrayId,
            android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        return spinner;
    }

    /** One compact "label - control" line with a hairline below it. */
    private void settingRow(LinearLayout section, int labelId, View control) {
        LinearLayout line = row();
        line.setPadding(0, dp(4), 0, dp(4));
        TextView label = text(getString(labelId), 14, false);
        line.addView(label, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 2f));
        line.addView(control, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 3f));
        section.addView(line, matchWrap());
        section.addView(divider(), new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(1)));
    }

    private CompoundButton switchRow(LinearLayout section, int stringId) {
        Switch toggle = new Switch(this);
        toggle.setText(stringId);
        toggle.setTextSize(14);
        toggle.setTextColor(COLOR_TEXT);
        toggle.setPadding(0, dp(10), 0, dp(10));
        section.addView(toggle, matchWrap());
        section.addView(divider(), new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(1)));
        return toggle;
    }

    /** Lays out secondary actions two per row below the section's settings. */
    private void buttonGrid(LinearLayout section, Button... buttons) {
        for (int i = 0; i < buttons.length; i += 2) {
            LinearLayout line = row();
            line.setPadding(0, dp(8), 0, 0);
            if (i + 1 < buttons.length) {
                line.addView(buttons[i], spaced(0, 4));
                line.addView(buttons[i + 1], spaced(4, 0));
            } else {
                line.addView(buttons[i], spaced(0, 0));
            }
            section.addView(line, matchWrap());
        }
    }

    private void applyDriverPresetToLauncher() {
        if (driverSpinner == null || renderSpinner == null) return;
        int driver = driverSpinner.getSelectedItemPosition();
        boolean sysmemPreset = driver == DRIVER_EXPERIMENTAL_A725 || driver == DRIVER_ADRENO_8XX;
        if (sysmemPreset) renderSpinner.setSelection(RENDER_MODE_SYSMEM);
        renderSpinner.setEnabled(!sysmemPreset);
    }

    private TextView statusText() {
        TextView status = text("", 14, false);
        status.setTextColor(COLOR_TEXT_SECONDARY);
        status.setPadding(0, 0, 0, dp(8));
        return status;
    }

    private TextView summaryText(int topPadding) {
        TextView summary = text("", 13, false);
        summary.setTextColor(COLOR_TEXT_SECONDARY);
        summary.setPadding(0, topPadding, 0, 0);
        summary.setSingleLine(true);
        summary.setEllipsize(TextUtils.TruncateAt.END);
        return summary;
    }

    private TextView chip(String value, int textColor, int background) {
        TextView chip = text(value, 12, true);
        chip.setTextColor(textColor);
        chip.setBackground(new LauncherStyle.Panel(background, background, textColor & 0x80FFFFFF, dp(6), dp(1), false));
        chip.setPadding(dp(10), dp(3), dp(10), dp(3));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        params.rightMargin = dp(6);
        chip.setLayoutParams(params);
        return chip;
    }

    private Button button(int stringId, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(stringId);
        button.setAllCaps(false);
        button.setTextSize(14);
        button.setTextColor(COLOR_TEXT);
        button.setStateListAnimator(null);
        button.setBackground(pressable(new LauncherStyle.Panel(COLOR_BUTTON_TOP, COLOR_BUTTON_BOTTOM, COLOR_FRAME, dp(10), dp(1), true)));
        button.setMinHeight(dp(42));
        button.setMinimumHeight(dp(42));
        button.setPadding(dp(10), dp(6), dp(10), dp(6));
        button.setOnClickListener(listener);
        return button;
    }

    private Button primaryButton(int stringId, View.OnClickListener listener) {
        Button button = button(stringId, listener);
        button.setTextSize(22);
        button.setTextColor(COLOR_TEXT);
        button.setAllCaps(true);
        button.setLetterSpacing(0.06f);
        button.setTypeface(FACE_HEADING);
        button.setShadowLayer(dp(4), 0, dp(1), 0xCC041030);
        button.setBackground(pressable(new LauncherStyle.Panel(COLOR_PLAY_TOP, COLOR_PLAY_BOTTOM, COLOR_PLAY_FRAME, dp(16), dp(2), true)));
        return button;
    }

    private TextView text(String value, int size, boolean bold) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(size);
        view.setTextColor(COLOR_TEXT);
        if (bold) view.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        return view;
    }

    /** Slanted, upper-case heading in the launcher's display face. */
    private TextView heading(String value, int size) {
        TextView view = text(value, size, false);
        view.setTypeface(FACE_HEADING);
        view.setAllCaps(true);
        view.setLetterSpacing(0.04f);
        return view;
    }

    /** Hairline below a setting row, fading out to the right. */
    private View divider() {
        View line = new View(this);
        line.setBackground(LauncherStyle.fadeLine(COLOR_DIVIDER, true));
        return line;
    }

    private Drawable pressable(LauncherStyle.Panel base) {
        return new RippleDrawable(ColorStateList.valueOf(0x405CC8FF), base, base.mask());
    }

    private LinearLayout column() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        return layout;
    }

    private LinearLayout row() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.HORIZONTAL);
        layout.setGravity(Gravity.CENTER_VERTICAL);
        return layout;
    }

    private LinearLayout.LayoutParams matchWrap() {
        return new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT);
    }

    /** Equal-width slot in a row with the given horizontal margins (dp). */
    private LinearLayout.LayoutParams spaced(int leftDp, int rightDp) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
        params.leftMargin = dp(leftDp);
        params.rightMargin = dp(rightDp);
        return params;
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private void showError(String message) {
        new AlertDialog.Builder(this).setTitle(R.string.error_title).setMessage(message)
            .setPositiveButton(android.R.string.ok, null).show();
    }

    private static String quote(String value) { return "\"" + value + "\""; }

    private static void select(Spinner spinner, String value, String[] options) {
        if (value == null) return;
        value = value.replace("\"", "").trim();
        for (int i = 0; i < options.length; i++) {
            if (options[i].equalsIgnoreCase(value)) {
                spinner.setSelection(i);
                return;
            }
        }
    }

    private static Map<String, String> readConfig(File file) {
        Map<String, String> result = new LinkedHashMap<>();
        if (!file.isFile()) return result;
        String section = "";
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(
            new FileInputStream(file), StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                String trimmed = line.trim();
                if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                    section = trimmed.substring(1, trimmed.length() - 1).trim();
                } else if (!trimmed.startsWith("#") && !trimmed.isEmpty()) {
                    int equals = trimmed.indexOf('=');
                    if (equals > 0) result.put(section + "." + trimmed.substring(0, equals).trim(),
                        trimmed.substring(equals + 1).trim());
                }
            }
        } catch (IOException ignored) {}
        return result;
    }

    private static void patchConfig(File file, LinkedHashMap<String, String> changes) throws IOException {
        List<String> lines = file.isFile()
            ? Files.readAllLines(file.toPath(), StandardCharsets.UTF_8)
            : new ArrayList<>();
        Map<String, Boolean> written = new LinkedHashMap<>();
        for (String key : changes.keySet()) written.put(key, false);
        String section = "";
        for (int i = 0; i < lines.size(); i++) {
            String trimmed = lines.get(i).trim();
            if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                section = trimmed.substring(1, trimmed.length() - 1).trim();
                continue;
            }
            int equals = trimmed.indexOf('=');
            if (equals <= 0 || trimmed.startsWith("#")) continue;
            String full = section + "." + trimmed.substring(0, equals).trim();
            if (changes.containsKey(full)) {
                lines.set(i, trimmed.substring(0, equals).trim() + " = " + changes.get(full));
                written.put(full, true);
            }
        }
        for (String full : changes.keySet()) {
            if (written.get(full)) continue;
            int dot = full.indexOf('.');
            String wantedSection = full.substring(0, dot);
            String name = full.substring(dot + 1);
            int insertAt = lines.size();
            boolean found = false;
            for (int i = 0; i < lines.size(); i++) {
                String trimmed = lines.get(i).trim();
                if (trimmed.equals("[" + wantedSection + "]")) {
                    found = true;
                    insertAt = i + 1;
                    while (insertAt < lines.size() && !lines.get(insertAt).trim().startsWith("[")) insertAt++;
                    break;
                }
            }
            if (!found) {
                if (!lines.isEmpty() && !lines.get(lines.size() - 1).isEmpty()) lines.add("");
                lines.add("[" + wantedSection + "]");
                insertAt = lines.size();
            }
            lines.add(insertAt, name + " = " + changes.get(full));
        }
        File parent = file.getParentFile();
        if (!parent.isDirectory() && !parent.mkdirs()) throw new LocalizedIOException("error_io_create", parent);
        File temporary = new File(parent, file.getName() + ".tmp");
        try (OutputStreamWriter writer = new OutputStreamWriter(new FileOutputStream(temporary), StandardCharsets.UTF_8)) {
            for (String line : lines) writer.write(line + "\n");
        }
        try {
            Files.move(temporary.toPath(), file.toPath(), StandardCopyOption.REPLACE_EXISTING,
                StandardCopyOption.ATOMIC_MOVE);
        } catch (IOException ignored) {
            Files.move(temporary.toPath(), file.toPath(), StandardCopyOption.REPLACE_EXISTING);
        }
    }

    private static void setMarker(File file, boolean enabled, String contents) throws IOException {
        if (!enabled) {
            Files.deleteIfExists(file.toPath());
            return;
        }
        File parent = file.getParentFile();
        if (!parent.isDirectory() && !parent.mkdirs()) throw new LocalizedIOException("error_io_create", parent);
        try (OutputStreamWriter writer = new OutputStreamWriter(new FileOutputStream(file), StandardCharsets.UTF_8)) {
            writer.write(contents);
        }
    }

    private String queryDisplayName(Uri uri) {
        try (android.database.Cursor cursor = getContentResolver().query(uri,
            new String[] {android.provider.OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                String name = cursor.getString(0);
                if (name != null && !name.trim().isEmpty()) return name;
            }
        } catch (Exception ignored) {}
        return "vulkan_driver.so";
    }

    private static String safeName(String name) {
        return name.replaceAll("[^A-Za-z0-9._-]", "_");
    }

    private static File uniqueFile(File directory, String name) {
        File result = new File(directory, name);
        if (!result.exists()) return result;
        int dot = name.lastIndexOf('.');
        String base = dot > 0 ? name.substring(0, dot) : name;
        String extension = dot > 0 ? name.substring(dot) : "";
        for (int i = 2; ; i++) {
            result = new File(directory, base + "_" + i + extension);
            if (!result.exists()) return result;
        }
    }

    private static int countDriverPackages(File directory) {
        File[] files = directory.listFiles(file -> file.isFile() &&
            (file.getName().toLowerCase(Locale.ROOT).endsWith(".so") ||
             file.getName().toLowerCase(Locale.ROOT).endsWith(".zip")));
        return files == null ? 0 : files.length;
    }

    private static String readFirstLine(File file) {
        if (!file.isFile()) return "";
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(
            new FileInputStream(file), StandardCharsets.UTF_8))) {
            String line = reader.readLine();
            return line == null ? "" : line.trim();
        } catch (IOException ignored) { return ""; }
    }

    private static File findFile(File root, String name, int depth) {
        if (depth < 0) return null;
        File[] children = root.listFiles();
        if (children == null) return null;
        for (File child : children) {
            if (child.isFile() && child.getName().equalsIgnoreCase(name)) return child;
        }
        for (File child : children) {
            if (child.isDirectory()) {
                File found = findFile(child, name, depth - 1);
                if (found != null) return found;
            }
        }
        return null;
    }

    private static String relativePath(File root, File file) {
        try { return root.toPath().relativize(file.toPath()).toString(); }
        catch (Exception ignored) { return file.toString(); }
    }

    private static String formatBytes(long bytes) {
        if (bytes < 1024) return bytes + " B";
        if (bytes < 1024 * 1024) return (bytes / 1024) + " KiB";
        return String.format(Locale.ROOT, "%.1f MiB", bytes / (1024.0 * 1024.0));
    }
}
