package games.hazzard.heroes4;

import android.app.Dialog;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
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
    private static final String GOG_SUCCESS = "https://embed.gog.com/on_login_success";
    private static final LinkedBlockingQueue<String> loginResult = new LinkedBlockingQueue<>();
    private static final LinkedBlockingQueue<String[]> pickResult = new LinkedBlockingQueue<>();

    @Override
    protected void onCreate(android.os.Bundle b) {
        super.onCreate(b);
        SaveSync.start(this);  // saves kept in step with a picked (synced) folder
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

    // The language button over the main menu (h4/src/menu_button_android.cpp): shown or hidden; a
    // new choice is written to savedPath and the game relaunched in it (RestartActivity). It is a
    // PopupWindow (its own window, drawn on its own): views added to the activity's layout over
    // SDL's surface were not redrawn after the activity turned to landscape.
    private static android.widget.PopupWindow langPopup;
    private static android.widget.Button langButton;
    private static String langNext;  // the language saved: shown on the button
    private static java.util.List<String[]> langList = new java.util.ArrayList<>();  // English first

    private static android.widget.PopupWindow cloudPopup;  // ☁️: save sync (SaveSync)

    private static android.widget.Button styled(HeroesActivity a, String text) {
        final float dp = a.getResources().getDisplayMetrics().density;
        android.widget.Button b = new android.widget.Button(a);
        b.setAllCaps(false);
        b.setText(text);
        b.setTextColor(Color.rgb(250, 235, 200));
        b.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 12);
        android.graphics.drawable.GradientDrawable bg = new android.graphics.drawable.GradientDrawable();
        bg.setColor(Color.argb(217, 26, 18, 8));
        bg.setCornerRadius(10 * dp);
        bg.setStroke(Math.round(1.5f * dp), Color.rgb(214, 172, 86));
        b.setBackground(bg);
        b.setPadding(Math.round(12 * dp), Math.round(4 * dp), Math.round(12 * dp), Math.round(4 * dp));
        b.setMinHeight(0);
        b.setMinimumHeight(0);
        b.setMinWidth(0);
        b.setMinimumWidth(0);
        b.measure(android.view.View.MeasureSpec.UNSPECIFIED, android.view.View.MeasureSpec.UNSPECIFIED);
        return b;
    }

    private static void place(android.widget.PopupWindow p, int px, int py) {
        if (p.isShowing()) p.update(px, py, -1, -1);
        else p.showAtLocation(mLayout, android.view.Gravity.TOP | android.view.Gravity.START, px, py);
    }

    // (x, y): the language button's top centre, lx the ☁️ one's left, in pixels of SDL's view;
    // savedPath "": no language button (no translation). languages: a line per language the app
    // offers, English first: "code<TAB>name<TAB>restart note" (h4/src/language.h).
    public static void h4LanguageButton(final boolean show, final float x, final float y, final float lx,
            final String lang, final String savedPath, final String languages) {
        final HeroesActivity a = self();
        if (a == null) return;
        a.runOnUiThread(() -> {
            if (!show) {
                if (langPopup != null) langPopup.dismiss();
                if (cloudPopup != null) cloudPopup.dismiss();
                return;
            }
            if (mLayout == null || mLayout.getWindowToken() == null) return;
            if (cloudPopup == null) {
                android.widget.Button c = styled(a, "\u2601\uFE0F");
                c.setOnClickListener(v -> SaveSync.showDialog(a, "uk".equals(langNext != null ? langNext : lang)));
                cloudPopup = new android.widget.PopupWindow(c, c.getMeasuredWidth(), c.getMeasuredHeight(), false);
                cloudPopup.setTouchable(true);
                cloudPopup.setClippingEnabled(false);
            }
            place(cloudPopup, Math.max(4, Math.round(lx)), Math.max(4, Math.round(y)));
            if (savedPath == null || savedPath.isEmpty()) return;
            final float dp = a.getResources().getDisplayMetrics().density;
            final java.util.List<String[]> list = new java.util.ArrayList<>();
            for (String line : languages.split("\n"))
                if (!line.isEmpty()) list.add(line.split("\t", -1));
            langList = list;
            if (langPopup == null) {
                android.widget.Button b = new android.widget.Button(a);
                b.setAllCaps(false);
                b.setText("\uD83C\uDF10 " + language(list, lang)[1]);
                b.setTextColor(Color.rgb(250, 235, 200));
                b.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 12);
                android.graphics.drawable.GradientDrawable bg = new android.graphics.drawable.GradientDrawable();
                bg.setColor(Color.argb(217, 26, 18, 8));
                bg.setCornerRadius(10 * dp);
                bg.setStroke(Math.round(1.5f * dp), Color.rgb(214, 172, 86));
                b.setBackground(bg);
                b.setPadding(Math.round(12 * dp), Math.round(4 * dp), Math.round(12 * dp), Math.round(4 * dp));
                b.setMinHeight(0);
                b.setMinimumHeight(0);
                langNext = lang;
                // A tap opens the list of languages; the one picked is saved, and a message
                // says it applies once the game is restarted.
                b.setOnClickListener(v -> {
                    final java.util.List<String[]> items = langList;
                    final String[] names = new String[items.size()];
                    for (int i = 0; i < names.length; i++)
                        names[i] = (items.get(i)[0].equals(langNext) ? "\u2713 " : "") + items.get(i)[1];
                    new android.app.AlertDialog.Builder(a)
                            .setItems(names, (d, which) -> {
                                final String[] next = items.get(which);
                                if (next[0].equals(langNext)) return;
                                try (java.io.FileOutputStream f = new java.io.FileOutputStream(savedPath)) {
                                    f.write(next[0].getBytes());
                                } catch (Exception e) {
                                    return;
                                }
                                langNext = next[0];
                                b.setText("\uD83C\uDF10 " + next[1]);
                                b.measure(android.view.View.MeasureSpec.UNSPECIFIED, android.view.View.MeasureSpec.UNSPECIFIED);
                                langPopup.update(b.getMeasuredWidth(), b.getMeasuredHeight());
                                new android.app.AlertDialog.Builder(a)
                                        .setTitle(next[1])
                                        .setMessage(next.length > 2 ? next[2] : "")
                                        .setPositiveButton("OK", null)
                                        .show();
                            })
                            .setNegativeButton("uk".equals(langNext) ? "Скасувати" : "Cancel", null)
                            .show();
                });
                b.measure(android.view.View.MeasureSpec.UNSPECIFIED, android.view.View.MeasureSpec.UNSPECIFIED);
                langButton = b;
                langPopup = new android.widget.PopupWindow(b, b.getMeasuredWidth(), b.getMeasuredHeight(), false);
                langPopup.setTouchable(true);
                langPopup.setOutsideTouchable(false);
                langPopup.setClippingEnabled(false);
            }
            langButton.measure(android.view.View.MeasureSpec.UNSPECIFIED, android.view.View.MeasureSpec.UNSPECIFIED);
            final int w = langButton.getMeasuredWidth();
            final int px = Math.max(4, Math.min(Math.round(x - w / 2f), mLayout.getWidth() - w - 4));
            place(langPopup, px, Math.max(4, Math.round(y)));
        });
    }

    // The entry of code in the language list (English, the first, if it is not there).
    private static String[] language(java.util.List<String[]> list, String code) {
        for (String[] l : list)
            if (l[0].equals(code)) return l;
        return list.isEmpty() ? new String[] {"en", "English", ""} : list.get(0);
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

    public static boolean h4PickFilesBegin() {
        final HeroesActivity a = self();
        if (a == null) return false;
        pickResult.clear();
        picking = true;
        a.runOnUiThread(() -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
            try {
                a.startActivityForResult(i, PICK_FILES);
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

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == SaveSync.PICK_FOLDER) {
            if (resultCode == RESULT_OK && data != null) SaveSync.picked(data.getData());
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
