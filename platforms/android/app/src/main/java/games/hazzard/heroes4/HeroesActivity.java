package games.hazzard.heroes4;

import android.app.Dialog;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import org.libsdl.app.SDLActivity;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.concurrent.LinkedBlockingQueue;

// SDL's activity, loading the game: libmain.so (h4/CMakeLists.txt) with its SDL_main.
// The static h4* methods are the in-app installer's platform layer, called from native code
// (h4/src/install/platform_android.cpp) on the game's thread or the installer's worker threads,
// never on the UI thread: they block until done.
public class HeroesActivity extends SDLActivity {
    private static final int PICK_FILES = 0x4834;
    private static final int PICK_GAME_FOLDER = 0x4835;  // an installed game (h4PickFilesBegin(true))
    private static final String GOG_SUCCESS = "https://embed.gog.com/on_login_success";
    private static final LinkedBlockingQueue<String> loginResult = new LinkedBlockingQueue<>();
    private static final LinkedBlockingQueue<String[]> pickResult = new LinkedBlockingQueue<>();

    @Override
    protected void onCreate(android.os.Bundle b) {
        super.onCreate(b);
        SaveSync.start(this);  // saves kept in step with a picked (synced) folder
        // Landscape from the start: SDL asks for it only as the game's window is made, and a phone
        // held upright then turned while the window was being made, sometimes leaving the game
        // drawn for the upright screen and stretched into a thin band (issue #10). The installer
        // allows upright again with its own window (installer.cpp's orientation hint).
        setRequestedOrientation(android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
        // And should the game still miss a change of the screen's size (turned, a cutout's bar):
        // the surface's real size sent again shortly after every change of the layout (the same
        // size again changes nothing).
        if (mSurface != null)
            mSurface.addOnLayoutChangeListener((v, l, t, r, bo, ol, ot, or, ob) -> {
                v.postDelayed(HeroesActivity::resendSurfaceSize, 300);
                v.postDelayed(HeroesActivity::resendSurfaceSize, 1500);
            });
    }

    // What SDLSurface.surfaceChanged tells the game, again, with the surface's size now.
    static void resendSurfaceSize() {
        if (mSurface == null || mSingleton == null || mBrokenLibraries) return;
        android.view.SurfaceHolder holder = mSurface.getHolder();
        if (holder == null || holder.getSurface() == null || !holder.getSurface().isValid()) return;
        android.graphics.Rect f = holder.getSurfaceFrame();
        if (f.width() <= 0 || f.height() <= 0) return;
        android.view.Display d = mSingleton.getWindowManager().getDefaultDisplay();
        android.util.DisplayMetrics m = new android.util.DisplayMetrics();
        d.getRealMetrics(m);
        nativeSetScreenResolution(f.width(), f.height(), m.widthPixels, m.heightPixels, m.densityDpi / 160.0f,
                                  d.getRefreshRate());
        onNativeResize();
    }

    @Override
    protected void onPause() {
        super.onPause();
        SaveSync.soon();  // a pass as the app goes to the background
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "c++_shared", "SDL3", "main" };
    }

    // Arguments for SDL_main from the launching intent (testing: adb shell am start ... --es args
    // "--install-auto=files"), space-separated.
    @Override
    protected String[] getArguments() {
        String a = getIntent() != null ? getIntent().getStringExtra("args") : null;
        return a == null || a.trim().isEmpty() ? new String[0] : a.trim().split("\\s+");
    }

    // The app's words (h4/ui_text/<code>.json, ui_text.h): `english` in the language `lang`, its
    // {1}, {2}... filled in (menu_button_android.cpp).
    static native String h4Tr(String lang, String english, String[] args);
    // `english` in the game's language.
    static String tr(String english, String... args) {
        return h4Tr("", english, args);
    }

    // ---- What the settings (drawn into the game's frame: h4/src/menu_button_drawn.cpp) ask of the
    // system (h4/src/app_ui/native.h, through menu_button_android.cpp); called from the game's thread.

    // A web address in the browser.
    public static boolean h4OpenUrl(final String url) {
        final HeroesActivity a = self();
        if (a == null) return false;
        a.runOnUiThread(() -> {
            try {
                a.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
            } catch (Exception e) {
                // no browser
            }
        });
        return true;
    }

    // The Files app at a folder: the synced one (its label, SaveSync.syncedFolderLabel) or one of
    // the app's own.
    public static boolean h4OpenFolder(final String path) {
        final HeroesActivity a = self();
        if (a == null) return false;
        final String synced = SaveSync.syncedFolderLabel();
        if (synced != null && synced.equals(path)) {
            final Uri u = SaveSync.syncedFolderUri();  // asks the provider (not on the UI thread)
            a.runOnUiThread(() -> openUri(a, u));
            return true;
        }
        final java.io.File dir = new java.io.File(path);
        dir.mkdirs();
        final Uri u = ownFolder(a, dir);
        if (u == null) return false;
        a.runOnUiThread(() -> openUri(a, u));
        return true;
    }

    // The save sync (SaveSync): on; turned on (the system's folder picker: on once one is picked)
    // or off; a pass now; what it shows ("<folder>\n<status>", the folder "" when off); its folders.
    public static boolean h4SyncOn() {
        return SaveSync.on();
    }

    public static void h4SetSync(final boolean on) {
        final HeroesActivity a = self();
        if (!on) {
            SaveSync.stop();
            return;
        }
        if (a != null) a.runOnUiThread(() -> SaveSync.pick(a));
    }

    public static void h4SyncNow() {
        SaveSync.soon();
    }

    public static String h4SyncState() {
        return SaveSync.state();
    }

    public static String h4SavesFolder() {
        java.io.File f = SaveSync.savesFolder();
        return f == null ? "" : f.getPath();
    }

    public static String h4SyncedFolder() {
        String s = SaveSync.syncedFolderLabel();
        return s == null ? "" : s;
    }

    // The update's APK, downloaded by the system's DownloadManager into Downloads (its notification,
    // once done, installs it).
    public static boolean h4DownloadApk(String url, String tag) {
        final HeroesActivity a = self();
        if (a == null) return false;
        try {
            android.app.DownloadManager dm = (android.app.DownloadManager) a.getSystemService(DOWNLOAD_SERVICE);
            dm.enqueue(new android.app.DownloadManager.Request(Uri.parse(url))
                    .setTitle("ReHoMM4 " + tag)
                    .setMimeType("application/vnd.android.package-archive")
                    .setNotificationVisibility(android.app.DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED)
                    .setDestinationInExternalPublicDir(android.os.Environment.DIRECTORY_DOWNLOADS,
                            Uri.parse(url).getLastPathSegment()));  // the release's own name for it
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    // The game again (a new language: it reads its text and fonts at start; a pack not at hand is
    // downloaded then): RestartActivity.
    public static void h4Restart() {
        final HeroesActivity a = self();
        if (a == null) return;
        Intent i = new Intent(a, RestartActivity.class);
        i.putExtra("pid", android.os.Process.myPid());
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        a.startActivity(i);
    }


    private static java.io.File filesRoot(HeroesActivity a) {
        java.io.File ext = a.getExternalFilesDir(null);
        return ext != null ? ext : a.getFilesDir();
    }

    // The app's own folder `dir` in the Files app, through FilesProvider (Android 11+ lets no
    // file manager into Android/data); null when it is not below the app's files.
    private static android.net.Uri ownFolder(HeroesActivity a, java.io.File dir) {
        String id = FilesProvider.idOf(filesRoot(a), dir);
        return id == null ? null : android.provider.DocumentsContract.buildDocumentUri(FilesProvider.AUTHORITY, id);
    }

    // The Files app (DocumentsUI) at a folder document, else any app that opens folders; a note
    // when none does. True: opened.
    private static boolean openUri(HeroesActivity a, android.net.Uri uri) {
        String[] apps = {"com.google.android.documentsui", "com.android.documentsui", null};
        for (String app : uri == null ? new String[0] : apps) {
            android.content.Intent i = new android.content.Intent(android.content.Intent.ACTION_VIEW);
            i.setDataAndType(uri, android.provider.DocumentsContract.Document.MIME_TYPE_DIR);
            i.addFlags(android.content.Intent.FLAG_GRANT_READ_URI_PERMISSION
                    | android.content.Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            if (app != null) i.setPackage(app);
            try {
                a.startActivity(i);
                return true;
            } catch (Exception e) {
            }
        }
        String name = a.getString(R.string.app_name);
        android.widget.Toast.makeText(a, (CharSequence) tr("Could not open the folder. The Files app has it as \"{1}\".", name),
                android.widget.Toast.LENGTH_LONG).show();
        return false;
    }

    // The display cutout's (camera notch's) or rounded corners' side, a fraction of the screen's long side, for the
    // game to keep clear of it (main.cpp, as iOS's safe area): 0 when there is none, or before
    // Android 9, or while the view's insets are not known yet.
    public static float h4CutoutMargin() {
        final HeroesActivity a = self();
        if (a == null || android.os.Build.VERSION.SDK_INT < 28) return 0;
        final android.view.View v = a.getWindow().getDecorView();
        final android.view.WindowInsets in = v.getRootWindowInsets();
        if (in == null) return 0;
        final android.view.DisplayCutout c = in.getDisplayCutout();
        final int w = Math.max(v.getWidth(), v.getHeight());
        int side = 0;
        if (c != null)  // in portrait (not turned yet) the notch is at the top
            side = v.getWidth() >= v.getHeight() ? Math.max(c.getSafeInsetLeft(), c.getSafeInsetRight())
                                                 : Math.max(c.getSafeInsetTop(), c.getSafeInsetBottom());
        if (android.os.Build.VERSION.SDK_INT >= 31) {
            // Rounded screen corners (Android 12+): half the biggest radius, so the game's corner
            // buttons are clear of them (all of it would take too much; there only the frame's
            // edge is cut).
            int r = 0;
            for (int pos = 0; pos < 4; pos++) {
                final android.view.RoundedCorner rc = in.getRoundedCorner(pos);
                if (rc != null) r = Math.max(r, rc.getRadius());
            }
            side = Math.max(side, r / 2);
        }
        return w > 0 ? (float) side / w : 0;
    }

    private static HeroesActivity self() {
        return (HeroesActivity) mSingleton;
    }

    // HTTPS GET: the 3-digit status as text, then the body; null on a network error.
    public static byte[] h4Http(String url, String bearer, String range) {
        HttpURLConnection c = null;
        try {
            c = (HttpURLConnection) new URL(url).openConnection();
            c.setConnectTimeout(30000);
            c.setReadTimeout(60000);
            if (bearer != null && !bearer.isEmpty()) c.setRequestProperty("Authorization", "Bearer " + bearer);
            if (range != null && !range.isEmpty()) c.setRequestProperty("Range", "bytes=" + range);
            int code = c.getResponseCode();
            InputStream in = code >= 400 ? c.getErrorStream() : c.getInputStream();
            ByteArrayOutputStream out = new ByteArrayOutputStream(Math.max(c.getContentLength(), 0) + 3);
            out.write(String.format("%03d", code % 1000).getBytes());
            if (in != null) {
                byte[] buf = new byte[1 << 16];
                for (int n; (n = in.read(buf)) > 0; ) out.write(buf, 0, n);
                in.close();
            }
            return out.toByteArray();
        } catch (Exception e) {
            return null;
        } finally {
            if (c != null) c.disconnect();
        }
    }

    // GOG's login page in a full-screen web view over the game; returns the code GOG hands to its
    // login-success page, or "" if the player closed it. Nothing is kept: cookies are cleared.
    public static String h4GogLogin(final String url) {
        loginResult.clear();
        final HeroesActivity a = self();
        a.runOnUiThread(() -> {
            CookieManager.getInstance().removeAllCookies(null);
            final Dialog d = new Dialog(a, android.R.style.Theme_Black_NoTitleBar_Fullscreen);
            final WebView web = new WebView(a);
            web.setBackgroundColor(Color.WHITE);
            WebSettings s = web.getSettings();
            s.setJavaScriptEnabled(true);
            s.setDomStorageEnabled(true);
            // Google (a way to sign in to GOG) refuses web views that say so ("; wv").
            s.setUserAgentString(s.getUserAgentString().replace("; wv)", ")"));
            // GOG opens Google/Steam/... sign-in with window.open: shown over the page.
            s.setSupportMultipleWindows(true);
            s.setJavaScriptCanOpenWindowsAutomatically(true);
            final android.widget.FrameLayout frame = new android.widget.FrameLayout(a);
            final boolean[] sent = { false };
            final WebViewClient[] client = { null };
            web.setWebChromeClient(new android.webkit.WebChromeClient() {
                @Override
                public boolean onCreateWindow(WebView v, boolean dialog, boolean user, android.os.Message msg) {
                    WebView pop = new WebView(a);
                    WebSettings ps = pop.getSettings();
                    ps.setJavaScriptEnabled(true);
                    ps.setDomStorageEnabled(true);
                    ps.setUserAgentString(s.getUserAgentString());
                    ps.setSupportMultipleWindows(true);
                    ps.setJavaScriptCanOpenWindowsAutomatically(true);
                    CookieManager.getInstance().setAcceptThirdPartyCookies(pop, true);
                    pop.setWebViewClient(client[0]);
                    pop.setWebChromeClient(this);
                    frame.addView(pop, new ViewGroup.LayoutParams(
                            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                    ((WebView.WebViewTransport) msg.obj).setWebView(pop);
                    msg.sendToTarget();
                    return true;
                }

                @Override
                public void onCloseWindow(WebView w) {
                    frame.removeView(w);
                    w.destroy();
                }
            });
            CookieManager.getInstance().setAcceptThirdPartyCookies(web, true);
            web.setWebViewClient(client[0] = new WebViewClient() {
                @Override
                public boolean shouldOverrideUrlLoading(WebView v, WebResourceRequest r) {
                    return check(r.getUrl().toString());
                }

                @Override
                public void onPageStarted(WebView v, String u, android.graphics.Bitmap icon) {
                    check(u);
                }

                private boolean check(String u) {
                    if (!u.startsWith(GOG_SUCCESS)) return false;
                    String code = Uri.parse(u).getQueryParameter("code");
                    if (!sent[0]) {
                        sent[0] = true;
                        loginResult.offer(code == null ? "" : code);
                    }
                    d.dismiss();
                    return true;
                }
            });
            d.setOnDismissListener(x -> {
                if (!sent[0]) {
                    sent[0] = true;
                    loginResult.offer("");
                }
                web.destroy();
                CookieManager.getInstance().removeAllCookies(null);
            });
            // Back closes a popup first.
            d.setOnKeyListener((dlg, key, ev) -> {
                if (key != android.view.KeyEvent.KEYCODE_BACK || ev.getAction() != android.view.KeyEvent.ACTION_UP
                        || frame.getChildCount() < 2) return false;
                WebView top = (WebView) frame.getChildAt(frame.getChildCount() - 1);
                frame.removeView(top);
                top.destroy();
                return true;
            });
            frame.addView(web, new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            d.setContentView(frame, new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            d.show();
            web.loadUrl(url);
        });
        try {
            return loginResult.take();
        } catch (InterruptedException e) {
            return "";
        }
    }

    // The system file picker (several files): "path\tname" for each, the path a /proc/self/fd
    // link to a descriptor that stays open until native code closes it. Empty if cancelled.
    // The installer's file picker (install/platform_android.cpp), without blocking the game's
    // thread: it keeps handling SDL's pause/resume (its surface is replaced while the picker is
    // up) and drawing. Begin shows it; Poll returns null until it is closed, then the picked files.
    private static volatile boolean picking;

    // folder: one folder (an installed game): every file in it, "path\tits/path/inside".
    public static boolean h4PickFilesBegin(boolean folder) {
        final HeroesActivity a = self();
        if (a == null) return false;
        pickResult.clear();
        picking = true;
        a.runOnUiThread(() -> {
            Intent i;
            if (folder) {
                i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
            } else {
                i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                i.addCategory(Intent.CATEGORY_OPENABLE);
                i.setType("*/*");
                i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
            }
            try {
                a.startActivityForResult(i, folder ? PICK_GAME_FOLDER : PICK_FILES);
            } catch (Exception e) {
                picking = false;
                pickResult.offer(new String[0]);
            }
        });
        return true;
    }

    public static String[] h4PickFilesPoll() {
        return pickResult.poll();
    }

    // Some systems (MIUI) can fail to deliver the picker's result: back in the game with no
    // result means it was lost (the installer goes back to its menu instead of waiting forever).
    @Override
    protected void onResume() {
        super.onResume();
        if (picking && mLayout != null) {
            mLayout.postDelayed(() -> {
                if (picking && hasWindowFocus()) {
                    picking = false;
                    pickResult.offer(new String[0]);
                }
            }, 1500);
        }
    }

    // The files under a picked folder (a document tree): an open descriptor each, by its path inside.
    private void walkTree(Uri tree, String doc, String prefix, ArrayList<String> out) throws Exception {
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, doc);
        String[] cols = { DocumentsContract.Document.COLUMN_DOCUMENT_ID, DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                          DocumentsContract.Document.COLUMN_MIME_TYPE };
        ArrayList<String[]> found = new ArrayList<>();
        try (Cursor c = getContentResolver().query(children, cols, null, null, null)) {
            while (c != null && c.moveToNext()) found.add(new String[] { c.getString(0), c.getString(1), c.getString(2) });
        }
        for (String[] f : found) {
            if (f[1] == null || f[1].contains("/")) continue;
            if (DocumentsContract.Document.MIME_TYPE_DIR.equals(f[2])) {
                walkTree(tree, f[0], prefix + f[1] + "/", out);
                continue;
            }
            try {
                ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(
                        DocumentsContract.buildDocumentUriUsingTree(tree, f[0]), "r");
                if (pfd != null) out.add("/proc/self/fd/" + pfd.detachFd() + "\t" + prefix + f[1]);
            } catch (Exception e) {
                // unreadable: left out
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == SaveSync.PICK_FOLDER) {
            if (resultCode == RESULT_OK && data != null) SaveSync.picked(data.getData());
            return;
        }
        if (requestCode == PICK_GAME_FOLDER) {
            final Uri tree = resultCode == RESULT_OK && data != null ? data.getData() : null;
            if (tree == null) {
                picking = false;
                pickResult.offer(new String[0]);
                return;
            }
            // Walked on a thread (hundreds of files); the result is delivered (not lost: onResume).
            picking = false;
            new Thread(() -> {
                ArrayList<String> out = new ArrayList<>();
                try {
                    walkTree(tree, DocumentsContract.getTreeDocumentId(tree), "", out);
                } catch (Exception e) {
                    // unreadable: what was found (the installer says what is missing)
                }
                pickResult.offer(out.toArray(new String[0]));
            }).start();
            return;
        }
        if (requestCode != PICK_FILES) {
            super.onActivityResult(requestCode, resultCode, data);
            return;
        }
        picking = false;
        ArrayList<Uri> uris = new ArrayList<>();
        if (resultCode == RESULT_OK && data != null) {
            if (data.getClipData() != null) {
                for (int k = 0; k < data.getClipData().getItemCount(); k++)
                    uris.add(data.getClipData().getItemAt(k).getUri());
            } else if (data.getData() != null) {
                uris.add(data.getData());
            }
        }
        ArrayList<String> out = new ArrayList<>();
        for (Uri u : uris) {
            try {
                String name = u.getLastPathSegment();
                try (Cursor c = getContentResolver().query(u, new String[] { OpenableColumns.DISPLAY_NAME },
                        null, null, null)) {
                    if (c != null && c.moveToFirst()) name = c.getString(0);
                }
                ParcelFileDescriptor pfd = getContentResolver().openFileDescriptor(u, "r");
                if (pfd == null) continue;
                out.add("/proc/self/fd/" + pfd.detachFd() + "\t" + name);
            } catch (Exception e) {
                // unreadable: left out (the installer says what is missing)
            }
        }
        pickResult.offer(out.toArray(new String[0]));
    }
}
