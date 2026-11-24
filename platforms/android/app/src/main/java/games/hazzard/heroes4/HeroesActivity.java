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

    // The settings button (⚙︎) over the main menu (h4/src/menu_button_android.cpp): shown or hidden.
    // A tap opens the settings: the languages (a new choice is written to savedPath, and a message
    // says it applies once the game is restarted: RestartActivity) and the checkbox that shows the
    // ☁️ button (save sync: SaveSync; left of ⚙︎), off at first. They are PopupWindows (their own windows,
    // drawn on their own): views added to the activity's layout over SDL's surface were not
    // redrawn after the activity turned to landscape.
    private static android.widget.PopupWindow langPopup;  // ⚙︎
    private static android.widget.Button langButton;
    private static String langNext;  // the language saved
    private static java.util.List<String[]> langList = new java.util.ArrayList<>();  // English first
    private static boolean menuShown;
    private static int cloudX, cloudY;

    private static android.widget.PopupWindow cloudPopup;  // ☁️: save sync (SaveSync)
    private static android.widget.PopupWindow kofiPopup;   // ☕: the author's Ko-fi page
    private static int kofiX;
    private static int gearX, gearH;

    private static String settingsPath, introPath, sidePanelsPath;

    private static boolean readFlag(String path) {
        try (java.io.FileInputStream f = new java.io.FileInputStream(path)) {
            return f.read() == '1';
        } catch (Exception e) {
            return false;
        }
    }

    // Back to the settings once a dialog opened from them is closed.
    private static void backToSettings(final HeroesActivity a, android.app.AlertDialog.Builder b) {
        b.setOnDismissListener(d -> showSettings(a, settingsPath));
    }

    // The ☁️ button shows while save sync is on (the settings' checkbox).
    private static boolean syncButtonShown(HeroesActivity a) {
        return SaveSync.on();
    }

    private static boolean settingsAfterPick;  // the checkbox opened the picker: the settings again after

    static void syncChanged(HeroesActivity a) {
        placeCloud(a);
    }

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

    // The menu's icon buttons (⚙︎, ☁️): a square with the icon bigger.
    private static android.widget.Button icon(HeroesActivity a, String text, int sp) {
        final float dp = a.getResources().getDisplayMetrics().density;
        android.widget.Button b = styled(a, text);
        b.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, sp);
        b.setPadding(0, 0, 0, 0);
        b.setGravity(android.view.Gravity.CENTER);
        final int side = Math.round(32 * dp);
        b.measure(android.view.View.MeasureSpec.makeMeasureSpec(side, android.view.View.MeasureSpec.EXACTLY),
                android.view.View.MeasureSpec.makeMeasureSpec(side, android.view.View.MeasureSpec.EXACTLY));
        return b;
    }

    private static android.widget.PopupWindow popup(android.widget.Button b) {
        android.widget.PopupWindow p = new android.widget.PopupWindow(b, b.getMeasuredWidth(), b.getMeasuredHeight(), false);
        p.setTouchable(true);
        p.setOutsideTouchable(false);
        p.setClippingEnabled(false);
        return p;
    }

    // The ☁️ button as the checkbox says (on the UI thread).
    private static void placeCloud(HeroesActivity a) {
        if (cloudPopup == null) return;
        final boolean sync = menuShown && syncButtonShown(a);
        if (sync) place(cloudPopup, cloudX, cloudY);
        else cloudPopup.dismiss();
    }

    // A checkbox's or radio button's box drawn `side` x `side` (its states passed on): smaller than
    // Android's and square (a LayerDrawable's size adds the box's own padding: stretched).
    // The settings' dropdowns (Spinner): their rows are made after compact(), so sized here. The
    // chosen one of the text's height (13sp, as the rest); the list's rows 32dp, the whole width of
    // the list, the name in full and its mark (20dp) at the right edge.
    static final class Dropdown extends android.widget.ArrayAdapter<String> {
        private final float dp;

        Dropdown(android.content.Context c, java.util.List<String> items, float dp) {
            super(c, android.R.layout.simple_spinner_item, items);
            this.dp = dp;
            setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        }

        @Override
        public android.view.View getView(int pos, android.view.View cv, android.view.ViewGroup parent) {
            android.widget.TextView t = (android.widget.TextView) super.getView(pos, cv, parent);
            t.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 13);
            t.setMinHeight(0);
            t.setMinimumHeight(0);
            t.setPadding(t.getPaddingLeft(), Math.round(2 * dp), t.getPaddingRight(), Math.round(2 * dp));
            return t;
        }

        @Override
        public android.view.View getDropDownView(int pos, android.view.View cv, android.view.ViewGroup parent) {
            android.widget.TextView t = (android.widget.TextView) super.getDropDownView(pos, cv, parent);
            t.setLayoutParams(new android.widget.AbsListView.LayoutParams(
                    android.view.ViewGroup.LayoutParams.MATCH_PARENT, android.view.ViewGroup.LayoutParams.WRAP_CONTENT));
            t.setSingleLine(true);
            t.setEllipsize(null);
            if (t instanceof android.widget.CheckedTextView) {
                android.widget.CheckedTextView c = (android.widget.CheckedTextView) t;
                android.graphics.drawable.Drawable m = c.getCheckMarkDrawable();
                if (m != null && !(m instanceof Square)) c.setCheckMarkDrawable(new Square(m, Math.round(20 * dp)));
            }
            t.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 14);
            t.setMinHeight(Math.round(32 * dp));
            t.setMinimumHeight(Math.round(32 * dp));
            t.setPadding(Math.round(14 * dp), Math.round(4 * dp), Math.round(14 * dp), Math.round(4 * dp));
            return t;
        }
    }

    static final class Square extends android.graphics.drawable.DrawableWrapper {
        private final int side;
        Square(android.graphics.drawable.Drawable d, int side) {
            super(d);
            this.side = side;
        }
        @Override public int getIntrinsicWidth() { return side; }
        @Override public int getIntrinsicHeight() { return side; }
        @Override public boolean getPadding(android.graphics.Rect padding) {
            padding.set(0, 0, 0, 0);
            return false;
        }
    }

    // The settings' dialogs on a phone held sideways (about 375dp high): smaller text and controls than
    // Android's (48dp a row), the content in a ScrollView, a short title and a wider window.
    private static void compact(android.view.View v, float dp) {
        if (v instanceof android.view.ViewGroup) {
            android.view.ViewGroup g = (android.view.ViewGroup) v;
            for (int i = 0; i < g.getChildCount(); i++) compact(g.getChildAt(i), dp);
        }
        if (v instanceof android.widget.Spinner) {
            v.setMinimumHeight(0);
            v.setPadding(v.getPaddingLeft(), 0, v.getPaddingRight(), 0);
        }
        if (!(v instanceof android.widget.TextView)) return;
        android.widget.TextView t = (android.widget.TextView) v;
        t.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 13);
        t.setMinHeight(0);
        t.setMinimumHeight(0);
        if (v instanceof android.widget.CompoundButton) {  // checkbox, radio button: its box 20dp
            android.widget.CompoundButton cb = (android.widget.CompoundButton) v;
            android.graphics.drawable.Drawable box = cb.getButtonDrawable();
            if (box != null && !(box instanceof Square)) cb.setButtonDrawable(new Square(box, Math.round(20 * dp)));
            t.setPadding(Math.round(6 * dp), Math.round(2 * dp), t.getPaddingRight(), Math.round(2 * dp));
        } else if (v instanceof android.widget.Button) {
            // Android's own background keeps a button 48dp high: a flat one of the text's height.
            android.graphics.drawable.GradientDrawable bg = new android.graphics.drawable.GradientDrawable();
            bg.setColor(Color.rgb(224, 224, 224));
            bg.setCornerRadius(4 * dp);
            android.content.res.ColorStateList ripple =
                    android.content.res.ColorStateList.valueOf(Color.argb(60, 0, 0, 0));
            // Focused (a gamepad moves the focus): a gold frame.
            android.graphics.drawable.GradientDrawable focused = new android.graphics.drawable.GradientDrawable();
            focused.setColor(Color.rgb(224, 224, 224));
            focused.setCornerRadius(4 * dp);
            focused.setStroke(Math.round(3 * dp), Color.rgb(214, 172, 86));
            android.graphics.drawable.StateListDrawable states = new android.graphics.drawable.StateListDrawable();
            states.addState(new int[] {android.R.attr.state_focused}, focused);
            states.addState(new int[] {}, bg);
            t.setBackground(new android.graphics.drawable.RippleDrawable(ripple, states, null));
            t.setTextColor(Color.rgb(30, 30, 30));
            t.setStateListAnimator(null);
            t.setMinWidth(0);
            t.setMinimumWidth(0);
            t.setPadding(Math.round(14 * dp), Math.round(6 * dp), Math.round(14 * dp), Math.round(6 * dp));
        }
    }

    // The scroll bar shown all the time while there is more to scroll to (it fades out by default:
    // nothing told the player the settings go on below).
    private static void alwaysBar(final android.widget.ScrollView sv) {
        sv.setScrollbarFadingEnabled(false);
        sv.setScrollBarStyle(android.view.View.SCROLLBARS_INSIDE_INSET);
        sv.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, or, ob) ->
                sv.setVerticalScrollBarEnabled(sv.canScrollVertically(1) || sv.canScrollVertically(-1)));
    }

    private static android.app.AlertDialog.Builder compactDialog(HeroesActivity a, String title, android.view.View content) {
        final float dp = a.getResources().getDisplayMetrics().density;
        compact(content, dp);
        android.view.View body = content;
        if (!(content instanceof android.widget.ScrollView)) {
            android.widget.ScrollView sv = new android.widget.ScrollView(a);
            sv.addView(content);
            body = sv;
        }
        alwaysBar((android.widget.ScrollView) body);
        android.widget.TextView t = new android.widget.TextView(a);
        t.setText(title);
        t.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 16);
        t.setTypeface(null, android.graphics.Typeface.BOLD);
        t.setPadding(Math.round(20 * dp), Math.round(12 * dp), Math.round(20 * dp), Math.round(4 * dp));
        return new android.app.AlertDialog.Builder(a).setCustomTitle(t).setView(body);
    }

    // After show(): the window wide (most of a landscape screen), its Close button small.
    private static android.app.AlertDialog shown(HeroesActivity a, android.app.AlertDialog d) {
        final float dp = a.getResources().getDisplayMetrics().density;
        final android.util.DisplayMetrics m = a.getResources().getDisplayMetrics();
        if (d.getWindow() != null)
            d.getWindow().setLayout(Math.min(m.widthPixels * 9 / 10, Math.round(640 * dp)),
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT);
        android.widget.Button close = d.getButton(android.content.DialogInterface.BUTTON_POSITIVE);
        if (close != null) compact(close, dp);
        return d;
    }

    private static native String h4GameFolder();  // menu_button_android.cpp
    private static native String h4FilterLabel(boolean uk);  // the scaling filter (F10's)
    private static native String h4RendererLabel(boolean uk);  // GPU or CPU (renderer.h)
    private static native void h4NextRenderer();
    private static native void h4NextFilter();

    // A folder in the settings: its title (bold) with Open at the right (open: null, no button),
    // the path under them (small, selectable).
    private static void folderRow(HeroesActivity a, android.widget.LinearLayout box, String title, String path,
                                  float dp, Runnable open) {
        android.widget.LinearLayout row = new android.widget.LinearLayout(a);
        row.setOrientation(android.widget.LinearLayout.HORIZONTAL);
        row.setGravity(android.view.Gravity.CENTER_VERTICAL);
        row.setPadding(0, Math.round(6 * dp), 0, 0);
        android.widget.TextView t = new android.widget.TextView(a);
        t.setText(title);
        t.setTypeface(null, android.graphics.Typeface.BOLD);
        row.addView(t, new android.widget.LinearLayout.LayoutParams(0,
                android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        if (open != null) {
            android.widget.Button b = new android.widget.Button(a);
            b.setAllCaps(false);
            b.setText("uk".equals(langNext) ? "Відкрити" : "Open");
            b.setOnClickListener(v -> open.run());
            row.addView(b);
        }
        box.addView(row);
        android.widget.TextView p = new android.widget.TextView(a);
        p.setText(path);
        p.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, 12);
        p.setTextIsSelectable(true);
        p.setPadding(0, 0, 0, Math.round(4 * dp));
        box.addView(p);
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
        android.widget.Toast.makeText(a, (CharSequence) ("uk".equals(langNext)
                ? "Не вдалося відкрити теку. У застосунку «Файли» вона є як «" + name + "»."
                : "Could not open the folder. The Files app has it as \"" + name + "\"."),
                android.widget.Toast.LENGTH_LONG).show();
        return false;
    }

    // How to set the save sync up on Android; OK: back to the settings.
    private static void syncHelp(final HeroesActivity a) {
        final boolean uk = "uk".equals(langNext);
        String text = uk
                ? "Збереження синхронізуються з текою, яку ви виберете один раз (у ній гра створить теку ReHOMM4): "
                + "файл за файлом, новіша копія перемагає, нічого не видаляється.\n\n"
                + "Щоб збереження були й на інших пристроях, синхронізуйте цю теку будь-яким застосунком:\n\n"
                + "• Syncthing (Syncthing-Fork з Google Play або F-Droid): додайте спільну теку, наприклад "
                + "Documents/ReHOMM4, і поділіться нею з іншими пристроями. ReHoMM4 на Linux і Steam Deck "
                + "синхронізується з ~/ReHOMM4, на Mac — з iCloud Drive/ReHOMM4: там і вкажіть цю спільну теку.\n\n"
                + "• Хмара (Google Drive, Dropbox, OneDrive…): FolderSync, Round Sync (rclone) або Autosync "
                + "двосторонньо синхронізують теку з хмарою. ReHoMM4 на Linux і Mac бере ту саму хмару через "
                + "rclone (рядок rclone=<remote>:ReHOMM4 у його sync.conf).\n\n"
                + "• Nextcloud: увімкніть синхронізацію теки в його застосунку.\n\n"
                + "Далі поставте галочку «Синхронізація збережень» і виберіть цю теку. Кнопка \u2601\uFE0F у меню "
                + "показує стан і синхронізує одразу."
                : "The saves are kept in step with a folder you pick once (the game makes ReHOMM4 in it): file by "
                + "file, the newer copy wins, nothing is deleted.\n\n"
                + "To have them on your other devices too, sync that folder with any app:\n\n"
                + "• Syncthing (Syncthing-Fork from Google Play or F-Droid): add a shared folder, say "
                + "Documents/ReHOMM4, and share it with your other devices. ReHoMM4 on Linux and the Steam Deck "
                + "syncs ~/ReHOMM4, on the Mac iCloud Drive/ReHOMM4: share it into that folder there.\n\n"
                + "• A cloud (Google Drive, Dropbox, OneDrive…): FolderSync, Round Sync (rclone) or Autosync "
                + "keep the folder in two-way sync with it. ReHoMM4 on Linux and the Mac takes the same cloud "
                + "through rclone (the line rclone=<remote>:ReHOMM4 in its sync.conf).\n\n"
                + "• Nextcloud: turn the folder's sync on in its app.\n\n"
                + "Then tick \"Sync saved games\" and pick that folder. The \u2601\uFE0F button in the menu "
                + "shows the state and syncs at once.";
        android.widget.TextView t = new android.widget.TextView(a);
        t.setText(text);
        t.setTextIsSelectable(true);
        final float dp = a.getResources().getDisplayMetrics().density;
        t.setPadding(Math.round(20 * dp), Math.round(8 * dp), Math.round(20 * dp), 0);
        android.app.AlertDialog.Builder b = compactDialog(a,
                uk ? "Як налаштувати синхронізацію" : "Setting the sync up", t)
                .setPositiveButton("OK", null);
        backToSettings(a, b);
        shown(a, b.show());
    }

    // The settings dialog: the languages (savedPath "": none), the ☁️ checkbox, Close.
    private static void showSettings(final HeroesActivity a, final String savedPath) {
        final boolean uk = "uk".equals(langNext);
        final float dp = a.getResources().getDisplayMetrics().density;
        final int pad = Math.round(20 * dp);
        android.widget.LinearLayout box = new android.widget.LinearLayout(a);
        box.setOrientation(android.widget.LinearLayout.VERTICAL);
        box.setPadding(pad, Math.round(8 * dp), pad, 0);
        final android.app.AlertDialog[] dialog = new android.app.AlertDialog[1];
        final java.util.List<String[]> items = langList;
        // At the top: "ReHoMM4 v<version>" and the update check right of it.
        android.widget.Button upd = new android.widget.Button(a);
        upd.setAllCaps(false);
        upd.setText(uk ? "Перевірити оновлення" : "Check for updates");
        upd.setOnClickListener(v -> {
            if (dialog[0] != null) dialog[0].dismiss();
            checkForUpdates(a);
        });
        android.widget.LinearLayout last = new android.widget.LinearLayout(a);
        last.setOrientation(android.widget.LinearLayout.HORIZONTAL);
        last.setGravity(android.view.Gravity.CENTER_VERTICAL);
        android.widget.TextView ver = new android.widget.TextView(a);
        String version = "";
        try {
            version = a.getPackageManager().getPackageInfo(a.getPackageName(), 0).versionName;
        } catch (Exception e) {
        }
        ver.setText("ReHoMM4 v" + version);
        ver.setTypeface(null, android.graphics.Typeface.BOLD);
        last.addView(ver, new android.widget.LinearLayout.LayoutParams(0,
                android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
        last.setPadding(0, 0, 0, Math.round(8 * dp));
        last.addView(upd);
        box.addView(last);
        if (savedPath != null && !savedPath.isEmpty() && !items.isEmpty()) {
            // The language: its title at the left, a dropdown of the languages at the right.
            android.widget.LinearLayout row = new android.widget.LinearLayout(a);
            row.setOrientation(android.widget.LinearLayout.HORIZONTAL);
            row.setGravity(android.view.Gravity.CENTER_VERTICAL);
            android.widget.TextView t = new android.widget.TextView(a);
            t.setText(uk ? "Мова гри" : "Game language");
            t.setTypeface(null, android.graphics.Typeface.BOLD);
            row.addView(t, new android.widget.LinearLayout.LayoutParams(0,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            java.util.List<String> names = new java.util.ArrayList<>();
            int chosen = 0;
            for (int i = 0; i < items.size(); i++) {
                names.add(items.get(i)[1]);
                if (items.get(i)[0].equals(langNext)) chosen = i;
            }
            android.widget.Spinner sp = new android.widget.Spinner(a, android.widget.Spinner.MODE_DROPDOWN);
            android.widget.ArrayAdapter<String> ad = new Dropdown(a, names, dp);
            sp.setDropDownWidth(Math.round(220 * dp));
            sp.setAdapter(ad);
            sp.setSelection(chosen, false);
            sp.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(android.widget.AdapterView<?> parent, android.view.View view, int pos, long id) {
                    final String[] next = items.get(pos);
                    if (next[0].equals(langNext)) return;
                    try (java.io.FileOutputStream f = new java.io.FileOutputStream(savedPath)) {
                        f.write(next[0].getBytes());
                    } catch (Exception e) {
                        return;
                    }
                    langNext = next[0];
                    if (dialog[0] != null) dialog[0].dismiss();
                    // The game again, with it (it reads its text and fonts at start): RestartActivity.
                    android.content.Intent i = new android.content.Intent(a, RestartActivity.class);
                    i.putExtra("pid", android.os.Process.myPid());
                    i.addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK);
                    a.startActivity(i);
                }

                @Override
                public void onNothingSelected(android.widget.AdapterView<?> parent) {
                }
            });
            row.addView(sp);
            row.setPadding(0, Math.round(4 * dp), 0, Math.round(4 * dp));
            box.addView(row);
        } else if (items.size() == 1 && items.get(0).length > 1) {
            // Installed from GOG in that language (menu_button_android.cpp): shown, nothing to choose.
            android.widget.LinearLayout row = new android.widget.LinearLayout(a);
            row.setOrientation(android.widget.LinearLayout.HORIZONTAL);
            row.setGravity(android.view.Gravity.CENTER_VERTICAL);
            android.widget.TextView t = new android.widget.TextView(a);
            t.setText(uk ? "Мова гри" : "Game language");
            t.setTypeface(null, android.graphics.Typeface.BOLD);
            row.addView(t, new android.widget.LinearLayout.LayoutParams(0,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            android.widget.TextView name = new android.widget.TextView(a);
            name.setText(items.get(0)[1]);
            row.addView(name);
            row.setPadding(0, Math.round(8 * dp), 0, Math.round(8 * dp));
            box.addView(row);
        }
        android.widget.CheckBox c = new android.widget.CheckBox(a);
        c.setText(uk ? "Синхронізація збережень (кнопка \u2601\uFE0F у меню)"
                     : "Sync saved games (the \u2601\uFE0F button in the menu)");
        c.setChecked(syncButtonShown(a));
        c.setPadding(c.getPaddingLeft(), Math.round(8 * dp), c.getPaddingRight(), Math.round(8 * dp));
        c.setOnCheckedChangeListener((v, on) -> {
            if (on && !SaveSync.on()) {  // on once a folder is picked
                if (dialog[0] != null) dialog[0].dismiss();
                settingsAfterPick = true;
                SaveSync.pick(a);
                return;
            }
            if (!on) SaveSync.stop();
            placeCloud(a);
        });
        {  // the checkbox with a small "?" at its right: how to set the sync up (syncHelp)
            android.widget.LinearLayout row = new android.widget.LinearLayout(a);
            row.setOrientation(android.widget.LinearLayout.HORIZONTAL);
            row.setGravity(android.view.Gravity.CENTER_VERTICAL);
            row.addView(c, new android.widget.LinearLayout.LayoutParams(0,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            android.widget.Button q = new android.widget.Button(a);
            q.setAllCaps(false);
            q.setText("?");
            q.setOnClickListener(v -> {
                if (dialog[0] != null) dialog[0].dismiss();
                syncHelp(a);
            });
            row.addView(q);
            box.addView(row);
        }
        if (introPath != null && !introPath.isEmpty()) {
            android.widget.CheckBox ci = new android.widget.CheckBox(a);
            ci.setText(uk ? "Пропускати вступні відео (з наступного запуску)" : "Skip the intro movies (from the next start)");
            ci.setChecked(readFlag(introPath));
            ci.setPadding(ci.getPaddingLeft(), Math.round(8 * dp), ci.getPaddingRight(), Math.round(8 * dp));
            ci.setOnCheckedChangeListener((v, on) -> {
                try (java.io.FileOutputStream f = new java.io.FileOutputStream(introPath)) {
                    f.write(on ? '1' : '0');
                } catch (Exception e) {
                }
            });
            box.addView(ci);
        }
        try {  // the scaling filter: each tap the next one (shown over the game behind the dialog)
            final android.widget.Button scaling = new android.widget.Button(a);
            scaling.setAllCaps(false);
            scaling.setText(h4FilterLabel(uk));
            scaling.setOnClickListener(v -> {
                h4NextFilter();
                scaling.setText(h4FilterLabel(uk));
            });
            box.addView(scaling);
        } catch (UnsatisfiedLinkError e) {
        }
        try {  // who draws the frame: each tap the next choice (Auto, GPU, CPU), from the next start
            final android.widget.Button renderer = new android.widget.Button(a);
            renderer.setAllCaps(false);
            renderer.setText(h4RendererLabel(uk));
            renderer.setOnClickListener(v -> {
                h4NextRenderer();
                renderer.setText(h4RendererLabel(uk));
            });
            box.addView(renderer);
        } catch (UnsatisfiedLinkError e) {
        }
        if (sidePanelsPath != null && !sidePanelsPath.isEmpty()) {
            android.widget.CheckBox sides = new android.widget.CheckBox(a);
            sides.setText(uk ? "Чорні бічні поля (з наступного запуску)"
                            : "Black side panels (from the next start)");
            try (java.io.BufferedReader reader = new java.io.BufferedReader(new java.io.FileReader(sidePanelsPath))) {
                String mode = reader.readLine();
                sides.setChecked(mode != null && mode.trim().equals("black"));
            } catch (java.io.IOException ignored) {
            }
            sides.setOnClickListener(v -> {
                try (java.io.FileWriter writer = new java.io.FileWriter(sidePanelsPath)) {
                    writer.write(sides.isChecked() ? "black\n" : "wings\n");
                } catch (java.io.IOException e) {
                    sides.setChecked(!sides.isChecked());
                    android.widget.Toast.makeText(a, uk ? "Не вдалося зберегти налаштування"
                            : "Could not save the setting", android.widget.Toast.LENGTH_SHORT).show();
                }
            });
            box.addView(sides);
        }
        {  // the folders, as on the Mac: the title, the path under it (selectable, to copy)
            java.io.File saves = SaveSync.savesFolder();
            String game = "";
            try {
                game = h4GameFolder();
            } catch (UnsatisfiedLinkError e) {
            }
            Runnable close = () -> {
                if (dialog[0] != null) dialog[0].dismiss();
            };
            // The saves: the synced folder while sync is on (the one the player keeps), else here.
            final String synced = SaveSync.syncedFolderLabel();
            if (synced != null) {
                folderRow(a, box, uk ? "Збереження (синхронізуються)" : "Saves (synced)", synced, dp, () -> {
                    close.run();
                    new Thread(() -> {
                        final android.net.Uri u = SaveSync.syncedFolderUri();  // asks the provider
                        a.runOnUiThread(() -> openUri(a, u));
                    }).start();
                });
            } else if (saves != null) {
                final android.net.Uri u = ownFolder(a, saves);
                folderRow(a, box, uk ? "Збереження" : "Saves", saves.getPath(), dp, u == null ? null : () -> {
                    saves.mkdirs();
                    if (openUri(a, u)) close.run();
                });
            }
            if (game != null && !game.isEmpty()) {
                final java.io.File g = new java.io.File(game);
                final android.net.Uri u = ownFolder(a, g);
                folderRow(a, box, uk ? "Тека гри" : "Game folder", game, dp, u == null ? null : () -> {
                    if (openUri(a, u)) close.run();
                });
            }
        }
        {  // the gamepad's dialog (showGamepad)
            android.widget.LinearLayout row = new android.widget.LinearLayout(a);
            row.setOrientation(android.widget.LinearLayout.HORIZONTAL);
            row.setGravity(android.view.Gravity.CENTER_VERTICAL);
            android.widget.TextView t = new android.widget.TextView(a);
            t.setText(uk ? "Геймпад" : "Gamepad");
            t.setTypeface(null, android.graphics.Typeface.BOLD);
            row.addView(t, new android.widget.LinearLayout.LayoutParams(0,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            android.widget.Button set = new android.widget.Button(a);
            set.setAllCaps(false);
            set.setText(uk ? "Налаштувати" : "Set up");
            set.setOnClickListener(v -> {
                if (dialog[0] != null) dialog[0].dismiss();
                showGamepad(a);
            });
            row.setPadding(0, Math.round(6 * dp), 0, Math.round(6 * dp));
            row.addView(set);
            box.addView(row);
        }
        dialog[0] = shown(a, compactDialog(a, uk ? "Налаштування" : "Settings", box)
                .setPositiveButton(uk ? "Закрити" : "Close", null)
                .show());
    }

    // The gamepad's settings (h4/src/pad_config.h, through the natives below): support on or off, the
    // sticks' speeds (sliders), what each button does (a dropdown each), the preset (a dropdown) at
    // the bottom left and Reset at the right; changes apply at once. Close: back to the settings.
    // A gamepad moves Android's focus through it (the D-pad or the stick; A presses, B closes).
    private static native String h4PadState(boolean uk);
    private static native void h4PadSet(String key, float value);

    private static void showGamepad(final HeroesActivity a) {
        final boolean uk = "uk".equals(langNext);
        final float dp = a.getResources().getDisplayMetrics().density;
        final int pad = Math.round(20 * dp);
        String state;
        try {
            state = h4PadState(uk);
        } catch (UnsatisfiedLinkError e) {
            return;
        }
        String[] style = null, pointer = null, scroll = null, actions = null;
        final java.util.List<String[]> buttons = new java.util.ArrayList<>();
        for (String line : state.split("\n")) {
            String[] f = line.split("\t", -1);
            switch (f[0]) {
                case "style": style = f; break;
                case "pointer": pointer = f; break;
                case "scroll": scroll = f; break;
                case "actions": actions = f; break;
                case "button": buttons.add(f); break;
                default: break;
            }
        }
        if (style == null || pointer == null || scroll == null || actions == null) return;
        final String[] actionNames = java.util.Arrays.copyOfRange(actions, 1, actions.length);
        android.widget.ScrollView scrollView = new android.widget.ScrollView(a);
        android.widget.LinearLayout box = new android.widget.LinearLayout(a);
        box.setOrientation(android.widget.LinearLayout.VERTICAL);
        box.setPadding(pad, Math.round(8 * dp), pad, 0);
        scrollView.addView(box);
        final android.app.AlertDialog[] dialog = new android.app.AlertDialog[1];
        // A slider: its title, the slider and its value (%) on one row.
        final String[][] sliders = {pointer, scroll};
        final String[] titles = uk ? new String[] {"Швидкість курсора (лівий стік)", "Швидкість прокрутки мапи (правий стік)"}
                                   : new String[] {"Pointer speed (left stick)", "Map scrolling speed (right stick)"};
        for (int k = 0; k < 2; k++) {
            final String key = k == 0 ? "pointer" : "scroll";
            final float value = Float.parseFloat(sliders[k][1]), lo = Float.parseFloat(sliders[k][2]),
                        hi = Float.parseFloat(sliders[k][3]);
            android.widget.TextView t = new android.widget.TextView(a);
            t.setText(titles[k]);
            t.setTypeface(null, android.graphics.Typeface.BOLD);
            t.setPadding(0, Math.round(8 * dp), 0, 0);
            box.addView(t);
            android.widget.LinearLayout row = new android.widget.LinearLayout(a);
            row.setOrientation(android.widget.LinearLayout.HORIZONTAL);
            row.setGravity(android.view.Gravity.CENTER_VERTICAL);
            android.widget.SeekBar bar = new android.widget.SeekBar(a);
            bar.setMax(100);
            bar.setProgress(Math.round((value - lo) / (hi - lo) * 100));
            final android.widget.TextView pct = new android.widget.TextView(a);
            pct.setText(Math.round(value * 100) + "%");
            pct.setMinWidth(Math.round(48 * dp));
            pct.setGravity(android.view.Gravity.END);
            bar.setOnSeekBarChangeListener(new android.widget.SeekBar.OnSeekBarChangeListener() {
                @Override public void onProgressChanged(android.widget.SeekBar s, int p, boolean user) {
                    final float v = lo + p / 100f * (hi - lo);
                    pct.setText(Math.round(v * 100) + "%");
                    if (user) h4PadSet(key, v);
                }
                @Override public void onStartTrackingTouch(android.widget.SeekBar s) { }
                @Override public void onStopTrackingTouch(android.widget.SeekBar s) { }
            });
            row.addView(bar, new android.widget.LinearLayout.LayoutParams(0,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            row.addView(pct);
            box.addView(row);
        }
        android.widget.TextView bt = new android.widget.TextView(a);
        bt.setText(uk ? "Кнопки" : "Buttons");
        bt.setTypeface(null, android.graphics.Typeface.BOLD);
        bt.setPadding(0, Math.round(10 * dp), 0, Math.round(4 * dp));
        box.addView(bt);
        // A dropdown that calls `picked` only for a choice made (not for the one set at first).
        final java.util.function.BiFunction<String[], Integer, android.widget.Spinner> dropdown = (items, at) -> {
            android.widget.Spinner sp = new android.widget.Spinner(a, android.widget.Spinner.MODE_DROPDOWN);
            android.widget.ArrayAdapter<String> ad = new Dropdown(a, java.util.Arrays.asList(items), dp);
            sp.setAdapter(ad);
            sp.setSelection(at, false);
            return sp;
        };
        for (int i = 0; i < buttons.size(); i++) {
            final int index = i;
            final String[] b = buttons.get(i);
            android.widget.LinearLayout row = new android.widget.LinearLayout(a);
            row.setOrientation(android.widget.LinearLayout.HORIZONTAL);
            row.setGravity(android.view.Gravity.CENTER_VERTICAL);
            android.widget.TextView name = new android.widget.TextView(a);
            name.setText(b[1]);
            name.setTypeface(null, android.graphics.Typeface.BOLD);
            row.addView(name, new android.widget.LinearLayout.LayoutParams(Math.round(100 * dp),
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT));
            android.widget.Spinner sp = dropdown.apply(actionNames, Integer.parseInt(b[2]));
            sp.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
                @Override public void onItemSelected(android.widget.AdapterView<?> p, android.view.View v, int pos, long id) {
                    h4PadSet("button" + index, pos);
                }
                @Override public void onNothingSelected(android.widget.AdapterView<?> p) { }
            });
            row.addView(sp, new android.widget.LinearLayout.LayoutParams(0,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            box.addView(row);
        }
        {  // at the bottom: the preset (a dropdown) at the left, Reset at the right
            android.widget.LinearLayout row = new android.widget.LinearLayout(a);
            row.setOrientation(android.widget.LinearLayout.HORIZONTAL);
            row.setGravity(android.view.Gravity.CENTER_VERTICAL);
            row.setPadding(0, Math.round(10 * dp), 0, Math.round(8 * dp));
            android.widget.TextView t = new android.widget.TextView(a);
            t.setText(uk ? "Пресет" : "Preset");
            t.setTypeface(null, android.graphics.Typeface.BOLD);
            row.addView(t);
            android.widget.Spinner sp = dropdown.apply(java.util.Arrays.copyOfRange(style, 2, style.length),
                    Integer.parseInt(style[1]));
            sp.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
                @Override public void onItemSelected(android.widget.AdapterView<?> p, android.view.View v, int pos, long id) {
                    h4PadSet("preset", pos);
                    if (dialog[0] != null) {  // again, with its layout and names
                        dialog[0].setOnDismissListener(d -> showGamepad(a));
                        dialog[0].dismiss();
                    }
                }
                @Override public void onNothingSelected(android.widget.AdapterView<?> p) { }
            });
            row.addView(sp, new android.widget.LinearLayout.LayoutParams(0,
                    android.view.ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            android.widget.Button reset = new android.widget.Button(a);
            reset.setAllCaps(false);
            reset.setText(uk ? "Скинути до стандартних" : "Reset to the defaults");
            reset.setOnClickListener(v -> {
                h4PadSet("reset", 0);
                if (dialog[0] != null) {
                    dialog[0].setOnDismissListener(d -> showGamepad(a));
                    dialog[0].dismiss();
                }
            });
            row.addView(reset);
            box.addView(row);
        }
        android.app.AlertDialog.Builder b = compactDialog(a,
                (uk ? "Геймпад — " : "Gamepad — ") + style[2 + Integer.parseInt(style[1])], scrollView)
                .setPositiveButton(uk ? "Закрити" : "Close", null);
        backToSettings(a, b);
        dialog[0] = shown(a, b.show());
    }

    // A gamepad's click at (x, y) (pixels of SDL's view; h4/src/menu_button_android.cpp): whether it is
    // on ⚙︎, ☕ or ☁️ (shown), and with `click` that button's tap. Called from the game's thread.
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

    public static boolean h4PadClick(final float x, final float y, final boolean click) {
        final HeroesActivity a = self();
        if (a == null || !menuShown) return false;
        final android.widget.PopupWindow[] all = {langPopup, kofiPopup, cloudPopup};
        for (final android.widget.PopupWindow p : all) {
            if (p == null || !p.isShowing() || p.getContentView() == null) continue;
            // Its place on screen against SDL's view's (the popups are windows of their own).
            final android.view.View v = p.getContentView();
            final int[] screen = new int[2], layoutScreen = new int[2];
            v.getLocationOnScreen(screen);
            if (mLayout != null) mLayout.getLocationOnScreen(layoutScreen);
            final float vx = screen[0] - layoutScreen[0], vy = screen[1] - layoutScreen[1];
            if (x >= vx && x < vx + v.getWidth() && y >= vy && y < vy + v.getHeight()) {
                if (click) a.runOnUiThread(v::performClick);
                return true;
            }
        }
        return false;
    }

    // Updates: the latest release on GitHub against the app's version; a newer one's APK downloaded
    // by the system's DownloadManager into Downloads (its notification, once done, installs it).
    private static final String RELEASES = "https://api.github.com/repos/evilHazzarD/ReHoMM4/releases/latest";
    private static final String APK = "-android_arm64.apk";  // reHoMM4-v<version>-android_arm64.apk (0.2: HeroesIV-<version>-…)

    private static boolean newer(String a, String b) {  // "0.10" > "0.9" ("v" in front ignored)
        String[] x = a.replaceFirst("^v", "").split("\\."), y = b.replaceFirst("^v", "").split("\\.");
        for (int i = 0; i < Math.max(x.length, y.length); i++) {
            long p = 0, q = 0;
            try { if (i < x.length) p = Long.parseLong(x[i].trim()); } catch (NumberFormatException e) { }
            try { if (i < y.length) q = Long.parseLong(y[i].trim()); } catch (NumberFormatException e) { }
            if (p != q) return p > q;
        }
        return false;
    }

    private static void checkForUpdates(final HeroesActivity a) {
        final boolean uk = "uk".equals(langNext);
        final android.app.AlertDialog wait = new android.app.AlertDialog.Builder(a)
                .setTitle(uk ? "Оновлення" : "Updates")
                .setMessage(uk ? "Перевіряю GitHub\u2026" : "Checking GitHub\u2026")
                .setNegativeButton(uk ? "Скасувати" : "Cancel", (d, w) -> showSettings(a, settingsPath))
                .show();
        new Thread(() -> {
            String tag = null, page = null, apk = null, error = null;
            byte[] r = h4Http(RELEASES, null, null);
            try {
                if (r == null || r.length < 3) throw new Exception(uk ? "немає мережі" : "no network");
                String status = new String(r, 0, 3);
                if (!status.equals("200")) throw new Exception("HTTP " + status);
                org.json.JSONObject rel = new org.json.JSONObject(new String(r, 3, r.length - 3, "UTF-8"));
                tag = rel.getString("tag_name");
                page = rel.optString("html_url", "https://github.com/evilHazzarD/ReHoMM4/releases");
                org.json.JSONArray assets = rel.optJSONArray("assets");
                for (int i = 0; assets != null && i < assets.length(); i++)
                    if ((assets.getJSONObject(i).optString("name").startsWith("reHoMM4-")
                            || assets.getJSONObject(i).optString("name").startsWith("HeroesIV-"))
                            && assets.getJSONObject(i).optString("name").endsWith(APK))
                        apk = assets.getJSONObject(i).optString("browser_download_url");
            } catch (Exception e) {
                error = e.getMessage();
            }
            final String fTag = tag, fPage = page, fApk = apk, fError = error;
            a.runOnUiThread(() -> {
                if (!wait.isShowing()) return;  // cancelled
                wait.dismiss();
                String have = "0";
                try {
                    have = a.getPackageManager().getPackageInfo(a.getPackageName(), 0).versionName;
                } catch (Exception e) {
                }
                android.app.AlertDialog.Builder b = new android.app.AlertDialog.Builder(a);
                if (fTag == null) {
                    b.setTitle(uk ? "Не вдалося перевірити" : "Could not check").setMessage(String.valueOf(fError))
                            .setPositiveButton("OK", null);
                } else if (!newer(fTag, have)) {
                    b.setTitle(uk ? "Оновлень немає" : "Up to date")
                            .setMessage((uk ? "У вас остання версія: " : "You have the latest version: ") + have + ".")
                            .setPositiveButton("OK", null);
                } else {
                    b.setTitle(uk ? "Є нова версія" : "A new version")
                            .setMessage("ReHoMM4 " + fTag + (uk ? " (у вас " : " (you have ") + have + ")")
                            .setNeutralButton(uk ? "Що нового" : "What's new", (d, w) -> a.startActivity(
                                    new Intent(Intent.ACTION_VIEW, Uri.parse(fPage))))
                            .setNegativeButton(uk ? "Пізніше" : "Later", null);
                    if (fApk != null)
                        b.setPositiveButton(uk ? "Завантажити" : "Download", (d, w) -> download(a, fApk, fTag, uk));
                }
                if (fTag == null || !newer(fTag, have)) backToSettings(a, b);  // OK: the settings again
                else b.setNegativeButton(uk ? "Пізніше" : "Later", (d, w) -> showSettings(a, settingsPath));
                b.show();
            });
        }, "h4-update-check").start();
    }

    private static void download(HeroesActivity a, String url, String tag, boolean uk) {
        try {
            android.app.DownloadManager dm = (android.app.DownloadManager) a.getSystemService(DOWNLOAD_SERVICE);
            android.app.DownloadManager.Request r = new android.app.DownloadManager.Request(Uri.parse(url))
                    .setTitle("ReHoMM4 " + tag)
                    .setMimeType("application/vnd.android.package-archive")
                    .setNotificationVisibility(android.app.DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED)
                    .setDestinationInExternalPublicDir(android.os.Environment.DIRECTORY_DOWNLOADS,
                            Uri.parse(url).getLastPathSegment());  // the release's own name for it
            dm.enqueue(r);
            new android.app.AlertDialog.Builder(a)
                    .setTitle(uk ? "Завантаження" : "Downloading")
                    .setMessage(uk ? "APK завантажується в «Завантаження». Коли завершиться, натисніть на сповіщення, "
                                    + "щоб встановити (Android попросить дозволу встановлювати з цього джерела)."
                                  : "The APK is downloading into Downloads. When it is done, tap its notification to "
                                    + "install it (Android asks to allow installing from that source).")
                    .setPositiveButton("OK", (d, w) -> showSettings(a, settingsPath))
                    .show();
        } catch (Exception e) {
            new android.app.AlertDialog.Builder(a)
                    .setTitle(uk ? "Не вдалося завантажити" : "Download failed")
                    .setMessage(String.valueOf(e.getMessage()))
                    .setPositiveButton("OK", null)
                    .show();
        }
    }

    // (x, y): the ⚙︎ button's top-right corner (the ☁️ one and the title go left of it; lx unused), in pixels of
    // SDL's view;
    // savedPath "": no languages in the settings (no translation). languages: a line per language
    // the app offers, English first: "code<TAB>name<TAB>restart note" (h4/src/language.h).
    public static void h4LanguageButton(final boolean show, final float x, final float y, final float lx,
            final String lang, final String savedPath, final String languages, final String skipPath,
            final String sidesPath) {
        final HeroesActivity a = self();
        if (a == null) return;
        a.runOnUiThread(() -> {
            menuShown = show;
            if (!show) {
                if (langPopup != null) langPopup.dismiss();
                if (cloudPopup != null) cloudPopup.dismiss();
                if (kofiPopup != null) kofiPopup.dismiss();
                return;
            }
            if (mLayout == null || mLayout.getWindowToken() == null) return;
            if (langNext == null) langNext = lang;
            final java.util.List<String[]> list = new java.util.ArrayList<>();
            for (String line : languages.split("\n"))
                if (!line.isEmpty()) list.add(line.split("\t", -1));
            langList = list;
            if (cloudPopup == null) {
                android.widget.Button c = icon(a, "\u2601\uFE0F", 14);
                c.setOnClickListener(v -> SaveSync.showDialog(a, "uk".equals(langNext)));
                cloudPopup = popup(c);
            }
            if (kofiPopup == null) {
                android.widget.Button k = icon(a, "\u2615", 16);
                k.setContentDescription("uk".equals(langNext) ? "Підтримати автора на Ko-fi" : "Support the author on Ko-fi");
                k.setOnClickListener(v -> {
                    try {
                        a.startActivity(new Intent(Intent.ACTION_VIEW, android.net.Uri.parse("https://ko-fi.com/hazzard")));
                    } catch (Exception e) {
                        // no browser
                    }
                });
                kofiPopup = popup(k);
            }
            if (langPopup == null) {
                android.widget.Button b = icon(a, "\u2699\uFE0E", 20);
                settingsPath = savedPath;
                introPath = skipPath;
                sidePanelsPath = sidesPath;
                b.setOnClickListener(v -> showSettings(a, savedPath));
                langButton = b;
                langPopup = popup(b);
            }
            final int w = langButton.getMeasuredWidth();
            final int px = Math.max(4, Math.min(Math.round(x - w), mLayout.getWidth() - w - 4));
            place(langPopup, px, Math.max(4, Math.round(y)));
            final float dp = a.getResources().getDisplayMetrics().density;
            kofiX = Math.max(4, px - kofiPopup.getWidth() - Math.round(6 * dp));  // ☕ left of ⚙︎
            place(kofiPopup, kofiX, Math.max(4, Math.round(y)));
            cloudX = Math.max(4, kofiX - cloudPopup.getWidth() - Math.round(6 * dp));  // ☁️ left of ☕
            cloudY = Math.max(4, Math.round(y));
            gearX = px;
            gearH = langButton.getMeasuredHeight();
            placeCloud(a);
        });
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
            placeCloud(this);
            if (settingsAfterPick) showSettings(this, settingsPath);
            settingsAfterPick = false;
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
