package io.github.poptracker.android;

import android.app.AlertDialog;
import android.content.Intent;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.text.InputType;
import android.util.Log;
import android.widget.EditText;
import android.widget.Toast;

import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Locale;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicReference;

public final class PopTrackerActivity extends SDLActivity {
    private static final String TAG = "PopTrackerAndroid";
    public static final String EXTRA_AP_HOST = "ap_host";
    public static final String EXTRA_AP_SLOT = "ap_slot";
    public static final String EXTRA_AP_PASSWORD = "ap_password";
    public static final String EXTRA_GAME = "game";
    private static final int REQUEST_IMPORT_PACK = 7001;
    private static final int REQUEST_OPEN_FILE = 7002;
    private static final long MAX_IMPORT_BYTES = 100L * 1024L * 1024L;
    private static final String ASSET_VERSION = "0.35.4";
    private static final String WORKSPACE_VIEW_PREFERENCES = "workspace_views";

    private final Object pickerSerialLock = new Object();
    private final Object pickerLock = new Object();
    private CountDownLatch pickerLatch;
    private String pickedFile;

    @Override
    protected String[] getLibraries() {
        return new String[]{"SDL2", "SDL2_image", "SDL2_ttf", "main"};
    }

    /** Converts a cold-start Android connection intent into PopTracker CLI arguments. */
    @Override
    protected String[] getArguments() {
        Intent intent = getIntent();
        String host = intent == null ? null : intent.getStringExtra(EXTRA_AP_HOST);
        String game = intent == null ? null : intent.getStringExtra(EXTRA_GAME);
        ArrayList<String> arguments = new ArrayList<>();

        if (game != null && !game.trim().isEmpty()) {
            arguments.add("--load-game");
            arguments.add(game.trim());
        }

        if (host != null && !host.trim().isEmpty()) {
            String slot = intent.getStringExtra(EXTRA_AP_SLOT);
            String password = intent.getStringExtra(EXTRA_AP_PASSWORD);
            arguments.add("--ap-host");
            arguments.add(host.trim());
            arguments.add("--ap-slot");
            arguments.add(slot == null || slot.isEmpty() ? "Player" : slot);
            arguments.add("--ap-password");
            arguments.add(password == null ? "" : password);
        }
        return arguments.toArray(new String[0]);
    }

    /** Delivers a new connection request when this single-instance activity is already running. */
    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        if (intent == null) {
            return;
        }
        String host = intent.getStringExtra(EXTRA_AP_HOST);
        String game = intent.getStringExtra(EXTRA_GAME);
        if ((host == null || host.trim().isEmpty())
                && (game == null || game.trim().isEmpty())) {
            return;
        }
        String slot = intent.getStringExtra(EXTRA_AP_SLOT);
        String password = intent.getStringExtra(EXTRA_AP_PASSWORD);
        nativeSetLaunchRequest(
                host == null ? "" : host.trim(),
                slot == null || slot.isEmpty() ? "Player" : slot,
                password == null ? "" : password,
                game == null ? "" : game.trim()
        );
    }

    private static native void nativeSetLaunchRequest(
            String host,
            String slot,
            String password,
            String game
    );

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mBrokenLibraries) {
            return;
        }

        try {
            prepareAssets();
        } catch (IOException error) {
            Log.e(TAG, "Unable to prepare assets", error);
            Toast.makeText(this, R.string.assets_failed, Toast.LENGTH_LONG).show();
        }
        applySafeAreaInsets();
        showTouchHelpOnce();
    }

    private void applySafeAreaInsets() {
        ViewCompat.setOnApplyWindowInsetsListener(mLayout, (view, windowInsets) -> {
            Insets safeArea = windowInsets.getInsets(
                    WindowInsetsCompat.Type.systemBars()
                            | WindowInsetsCompat.Type.displayCutout()
            );
            view.setPadding(safeArea.left, safeArea.top, safeArea.right, safeArea.bottom);
            return WindowInsetsCompat.CONSUMED;
        });
        ViewCompat.requestApplyInsets(mLayout);
    }

    private void showTouchHelpOnce() {
        if (!getPreferences(MODE_PRIVATE).getBoolean("touch_help_shown", false)) {
            Toast.makeText(this, R.string.touch_help, Toast.LENGTH_LONG).show();
            getPreferences(MODE_PRIVATE).edit().putBoolean("touch_help_shown", true).apply();
        }
    }

    /** Called from the native PopTracker toolbar through JNI. */
    public void launchPackPicker() {
        runOnUiThread(() -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("application/zip");
            intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{
                    "application/zip",
                    "application/x-zip-compressed",
                    "application/octet-stream"
            });
            startActivityForResult(intent, REQUEST_IMPORT_PACK);
        });
    }

    /** Persists viewer-only state without changing tracker packs or their saves. */
    public float[] loadWorkspaceViewState(String key) {
        android.content.SharedPreferences preferences =
                getSharedPreferences(WORKSPACE_VIEW_PREFERENCES, MODE_PRIVATE);
        String zoomKey = key + ".zoom";
        if (!preferences.contains(zoomKey)) {
            return null;
        }
        return new float[]{
                preferences.getFloat(zoomKey, 1.0f),
                preferences.getFloat(key + ".pan_x", 0.0f),
                preferences.getFloat(key + ".pan_y", 0.0f)
        };
    }

    public void saveWorkspaceViewState(
            String key,
            float zoom,
            float normalizedPanX,
            float normalizedPanY
    ) {
        getSharedPreferences(WORKSPACE_VIEW_PREFERENCES, MODE_PRIVATE)
                .edit()
                .putFloat(key + ".zoom", zoom)
                .putFloat(key + ".pan_x", normalizedPanX)
                .putFloat(key + ".pan_y", normalizedPanY)
                .apply();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Uri uri = resultCode == RESULT_OK && data != null ? data.getData() : null;

        if (requestCode == REQUEST_IMPORT_PACK) {
            if (uri == null) {
                return;
            }
            try {
                File imported = copyDocumentToCache(uri, true);
                SDLActivity.onNativeDropFile(imported.getAbsolutePath());
            } catch (IOException error) {
                Log.e(TAG, "Tracker ZIP import failed", error);
                Toast.makeText(this, R.string.import_failed, Toast.LENGTH_LONG).show();
            }
            return;
        }

        if (requestCode == REQUEST_OPEN_FILE) {
            synchronized (pickerLock) {
                pickedFile = null;
                if (uri != null) {
                    try {
                        pickedFile = copyDocumentToCache(uri, false).getAbsolutePath();
                    } catch (IOException error) {
                        Log.e(TAG, "Document import failed", error);
                    }
                }
                if (pickerLatch != null) {
                    pickerLatch.countDown();
                }
            }
        }
    }

    /** Called synchronously from PopTracker's SDL thread through JNI. */
    public int showMessageDialog(String title, String message, int buttons, int icon, int defaultResult) {
        CountDownLatch done = new CountDownLatch(1);
        AtomicReference<Integer> result = new AtomicReference<>(defaultResult);
        runOnUiThread(() -> {
            AlertDialog.Builder builder = new AlertDialog.Builder(this)
                    .setTitle(title)
                    .setMessage(message)
                    .setCancelable(false);

            if (buttons == 0) {
                builder.setPositiveButton(android.R.string.ok, (dialog, which) -> {
                    result.set(2);
                    done.countDown();
                });
            } else if (buttons == 1) {
                builder.setPositiveButton(android.R.string.ok, (dialog, which) -> {
                    result.set(2);
                    done.countDown();
                });
                builder.setNegativeButton(android.R.string.cancel, (dialog, which) -> {
                    result.set(-1);
                    done.countDown();
                });
            } else {
                builder.setPositiveButton(android.R.string.yes, (dialog, which) -> {
                    result.set(1);
                    done.countDown();
                });
                builder.setNegativeButton(android.R.string.no, (dialog, which) -> {
                    result.set(0);
                    done.countDown();
                });
                if (buttons == 3) {
                    builder.setNeutralButton(android.R.string.cancel, (dialog, which) -> {
                        result.set(-1);
                        done.countDown();
                    });
                }
            }
            builder.show();
        });
        await(done);
        return result.get();
    }

    /** Called synchronously from PopTracker's SDL thread through JNI. */
    public String showInputDialog(String title, String message, String defaultValue, boolean password) {
        CountDownLatch done = new CountDownLatch(1);
        AtomicReference<String> result = new AtomicReference<>();
        runOnUiThread(() -> {
            EditText input = new EditText(this);
            input.setSingleLine(true);
            input.setText(defaultValue);
            input.setSelection(input.length());
            if (password) {
                input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
            }
            int pad = Math.round(24 * getResources().getDisplayMetrics().density);
            input.setPadding(pad, 0, pad, 0);

            new AlertDialog.Builder(this)
                    .setTitle(title)
                    .setMessage(message)
                    .setView(input)
                    .setCancelable(false)
                    .setPositiveButton(android.R.string.ok, (dialog, which) -> {
                        result.set(input.getText().toString());
                        done.countDown();
                    })
                    .setNegativeButton(android.R.string.cancel, (dialog, which) -> done.countDown())
                    .show();
        });
        await(done);
        return result.get();
    }

    /** Imports a selected document to a normal path for native code. */
    public String openDocument(String mimeType) {
        synchronized (pickerSerialLock) {
            CountDownLatch activeLatch;
            synchronized (pickerLock) {
                pickedFile = null;
                pickerLatch = new CountDownLatch(1);
                activeLatch = pickerLatch;
            }
            runOnUiThread(() -> {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType(mimeType);
                startActivityForResult(intent, REQUEST_OPEN_FILE);
            });
            await(activeLatch);
            synchronized (pickerLock) {
                pickerLatch = null;
                return pickedFile;
            }
        }
    }

    private void prepareAssets() throws IOException {
        File destination = new File(getFilesDir(), "assets");
        File marker = new File(destination, ".android-assets-" + ASSET_VERSION);
        if (marker.isFile()) {
            return;
        }
        copyAssetTree(getAssets(), "", destination);
        if (!marker.createNewFile() && !marker.isFile()) {
            throw new IOException("Could not create asset version marker");
        }
    }

    private static void copyAssetTree(AssetManager assets, String source, File destination) throws IOException {
        String[] children = assets.list(source);
        if (children != null && children.length > 0) {
            if (!destination.isDirectory() && !destination.mkdirs()) {
                throw new IOException("Could not create " + destination);
            }
            for (String child : children) {
                String childSource = source.isEmpty() ? child : source + "/" + child;
                copyAssetTree(assets, childSource, new File(destination, child));
            }
            return;
        }

        File parent = destination.getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
            throw new IOException("Could not create " + parent);
        }
        try (InputStream input = assets.open(source);
             OutputStream output = new FileOutputStream(destination)) {
            copyWithLimit(input, output, Long.MAX_VALUE);
        }
    }

    private File copyDocumentToCache(Uri uri, boolean requireZip) throws IOException {
        String name = queryDisplayName(uri);
        name = name.replaceAll("[^A-Za-z0-9._ ()-]", "_");
        if (name.isEmpty()) {
            name = requireZip ? "tracker.zip" : "document";
        }
        if (requireZip && !name.toLowerCase(Locale.ROOT).endsWith(".zip")) {
            name += ".zip";
        }

        File imports = new File(getCacheDir(), "imports");
        if (!imports.isDirectory() && !imports.mkdirs()) {
            throw new IOException("Could not create import directory");
        }
        File outputFile = new File(imports, System.currentTimeMillis() + "-" + name);
        try (InputStream input = getContentResolver().openInputStream(uri);
             OutputStream output = new FileOutputStream(outputFile)) {
            if (input == null) {
                throw new IOException("Document provider returned no stream");
            }
            copyWithLimit(input, output, MAX_IMPORT_BYTES);
        } catch (IOException error) {
            //noinspection ResultOfMethodCallIgnored
            outputFile.delete();
            throw error;
        }
        return outputFile;
    }

    private String queryDisplayName(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (column >= 0) {
                    String value = cursor.getString(column);
                    if (value != null) {
                        return value;
                    }
                }
            }
        }
        String segment = uri.getLastPathSegment();
        return segment == null ? "document" : segment;
    }

    private static void copyWithLimit(InputStream input, OutputStream output, long limit) throws IOException {
        byte[] buffer = new byte[64 * 1024];
        long copied = 0;
        int read;
        while ((read = input.read(buffer)) >= 0) {
            copied += read;
            if (copied > limit) {
                throw new IOException("Document exceeds the 100 MiB import limit");
            }
            output.write(buffer, 0, read);
        }
    }

    private static void await(CountDownLatch latch) {
        boolean interrupted = false;
        while (true) {
            try {
                latch.await();
                break;
            } catch (InterruptedException ignored) {
                interrupted = true;
            }
        }
        if (interrupted) {
            Thread.currentThread().interrupt();
        }
    }
}
