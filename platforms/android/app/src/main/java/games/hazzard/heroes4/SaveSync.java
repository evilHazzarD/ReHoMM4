package games.hazzard.heroes4;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Locale;
import java.util.Map;

// The saved games kept in step with a folder the player picks once in Android's folder picker
// (Storage Access Framework): a folder that Syncthing (Syncthing-Fork), Round Sync (rclone),
// Nextcloud, FolderSync... carry to the player's other devices. As on iOS and the Mac
// (h4/src/save_sync_apple.mm, save_sync.cpp): ReHOMM4 inside the picked folder (the picked one
// itself when it is called ReHOMM4), made when missing; file by file the newer copy wins, one
// side's file is copied over, nothing is deleted, a save changed in the last 3 s waits.
// Passes: at start, every 20 s, when the app goes to the background, on "Sync now".
//
// A document provider sets a written file's time itself (now): after a save is sent, the local
// copy takes the provider's time, so the two agree and it is not sent back.
final class SaveSync {
    static final int PICK_FOLDER = 0x4835;
    private static final String PREFS = "save_sync", KEY = "tree";
    private static final String FOLDER = "ReHOMM4";

    private static Context app;
    private static File install;  // <data>/data/h4/install, where the game writes its "games" folder
    private static volatile Uri tree;
    private static volatile String status = "";
    private static final Object passLock = new Object();
    private static final Object wake = new Object();

    static void start(Activity a) {
        if (app != null) return;
        app = a.getApplicationContext();
        File ext = a.getExternalFilesDir(null);  // SDL_GetAndroidExternalStoragePath (main.cpp)
        if (ext == null) ext = a.getFilesDir();
        install = new File(ext, "player/data/h4/install");
        String t = prefs().getString(KEY, null);
        if (t != null) tree = Uri.parse(t);
        Thread th = new Thread(() -> {
            for (;;) {
                pass();
                synchronized (wake) {
                    try {
                        wake.wait(20000);
                    } catch (InterruptedException e) {
                        return;
                    }
                }
            }
        }, "h4-save-sync");
        th.setDaemon(true);
        th.start();
    }

    static void soon() {
        synchronized (wake) {
            wake.notifyAll();
        }
    }

    private static SharedPreferences prefs() {
        return app.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
    }

    // The picker's result (HeroesActivity.onActivityResult).
    static void picked(Uri u) {
        if (u == null || app == null) return;
        try {
            app.getContentResolver().takePersistableUriPermission(u,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        } catch (Exception e) {
            return;
        }
        prefs().edit().putString(KEY, u.toString()).apply();
        tree = u;
        soon();
    }

    static void stop() {
        Uri t = tree;
        tree = null;
        status = "";
        prefs().edit().remove(KEY).apply();
        if (t != null)
            try {
                app.getContentResolver().releasePersistableUriPermission(t,
                        Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            } catch (Exception e) {
                // already gone
            }
    }

    private static final class Doc {
        String id;
        long time;
    }

    private static String name(Uri t, String id) {
        try (Cursor c = app.getContentResolver().query(DocumentsContract.buildDocumentUriUsingTree(t, id),
                new String[] { Document.COLUMN_DISPLAY_NAME }, null, null, null)) {
            return c != null && c.moveToFirst() ? c.getString(0) : null;
        } catch (Exception e) {
            return null;
        }
    }

    // Children of a folder document: name -> (id, time); directories under "/" + name.
    private static Map<String, Doc> children(Uri t, String parentId) {
        Map<String, Doc> out = new HashMap<>();
        Uri q = DocumentsContract.buildChildDocumentsUriUsingTree(t, parentId);
        try (Cursor c = app.getContentResolver().query(q, new String[] { Document.COLUMN_DOCUMENT_ID,
                Document.COLUMN_DISPLAY_NAME, Document.COLUMN_MIME_TYPE, Document.COLUMN_LAST_MODIFIED },
                null, null, null)) {
            while (c != null && c.moveToNext()) {
                String n = c.getString(1);
                if (n == null || n.startsWith(".")) continue;
                Doc d = new Doc();
                d.id = c.getString(0);
                d.time = c.isNull(3) ? 0 : c.getLong(3);
                out.put(Document.MIME_TYPE_DIR.equals(c.getString(2)) ? "/" + n : n, d);
            }
        }
        return out;
    }

    // The ReHOMM4 folder's document id in the picked tree (made when missing).
    private static String folderId(Uri t) throws Exception {
        String root = DocumentsContract.getTreeDocumentId(t);
        if (FOLDER.equalsIgnoreCase(name(t, root))) return root;
        for (Map.Entry<String, Doc> e : children(t, root).entrySet())
            if (e.getKey().equalsIgnoreCase("/" + FOLDER)) return e.getValue().id;
        Uri made = DocumentsContract.createDocument(app.getContentResolver(),
                DocumentsContract.buildDocumentUriUsingTree(t, root), Document.MIME_TYPE_DIR, FOLDER);
        if (made == null) throw new Exception("cannot make " + FOLDER);
        return DocumentsContract.getDocumentId(made);
    }

    private static void copy(InputStream in, OutputStream out) throws Exception {
        byte[] b = new byte[65536];
        for (int n; (n = in.read(b)) > 0;) out.write(b, 0, n);
    }

    static String pass() {
        synchronized (passLock) {
            Uri t = tree;
            if (t == null) return "";
            String s;
            try {
                s = passIn(t);
            } catch (Exception e) {
                s = "the folder cannot be read (" + e.getMessage() + ")";
            }
            if (tree == t) status = s;
            return s;
        }
    }

    private static String passIn(Uri t) throws Exception {
        ContentResolver cr = app.getContentResolver();
        String fid = folderId(t);
        Map<String, Doc> there = new HashMap<>();
        for (Map.Entry<String, Doc> e : children(t, fid).entrySet())
            if (!e.getKey().startsWith("/")) there.put(e.getKey(), e.getValue());
        // Here: games/ (as the game spelled it) and the saves beside it; the newer one by name.
        File games = new File(install, "games");
        File[] top = install.listFiles();
        if (top != null)
            for (File f : top)
                if (f.isDirectory() && f.getName().equalsIgnoreCase("games")) games = f;
        games.mkdirs();
        Map<String, File> here = new HashMap<>();
        File[] g = games.listFiles();
        if (g != null)
            for (File f : g)
                if (f.isFile() && !f.getName().startsWith(".")) here.put(f.getName(), f);
        if (top != null)
            for (File f : top)
                if (f.isFile() && f.getName().toLowerCase(Locale.ROOT).endsWith(".h4s")) {
                    File o = here.get(f.getName());
                    if (o == null || f.lastModified() > o.lastModified()) here.put(f.getName(), f);
                }
        HashSet<String> names = new HashSet<>(there.keySet());
        names.addAll(here.keySet());
        int up = 0, down = 0, failed = 0;
        long now = System.currentTimeMillis();
        for (String n : names) {
            Doc r = there.get(n);
            File l = here.get(n);
            long diff = (r != null && l != null) ? l.lastModified() - r.time : 0;
            try {
                if (r != null && (l == null || diff < -2000)) {  // down, into games/
                    File to = l != null && l.getParentFile().equals(games) ? l : new File(games, n);
                    File tmp = new File(games, ".sync-" + n);
                    try (InputStream in = cr.openInputStream(DocumentsContract.buildDocumentUriUsingTree(t, r.id));
                            OutputStream out = new FileOutputStream(tmp)) {
                        copy(in, out);
                    }
                    if (r.time > 0) tmp.setLastModified(r.time);
                    if (!tmp.renameTo(to)) throw new Exception("rename");
                    down++;
                } else if (l != null && (r == null || diff > 2000) && now - l.lastModified() > 3000) {  // up
                    Uri doc = r != null ? DocumentsContract.buildDocumentUriUsingTree(t, r.id)
                                        : DocumentsContract.createDocument(cr,
                                                DocumentsContract.buildDocumentUriUsingTree(t, fid),
                                                "application/octet-stream", n);
                    if (doc == null) throw new Exception("create");
                    try (InputStream in = new FileInputStream(l); OutputStream out = cr.openOutputStream(doc, "wt")) {
                        copy(in, out);
                    }
                    // The provider's time for it, taken by the local copy (see the top).
                    try (Cursor c = cr.query(doc, new String[] { Document.COLUMN_LAST_MODIFIED }, null, null, null)) {
                        if (c != null && c.moveToFirst() && !c.isNull(0) && c.getLong(0) > 0)
                            l.setLastModified(c.getLong(0));
                    }
                    up++;
                }
            } catch (Exception e) {
                failed++;
            }
        }
        String s = new SimpleDateFormat("HH:mm", Locale.ROOT).format(new Date()) + ": " + names.size()
                + " save(s) in " + FOLDER + ", " + up + " sent, " + down + " received"
                + (failed > 0 ? ", " + failed + " failed" : "");
        if (up + down + failed > 0) android.util.Log.i("h4", "save sync " + s);
        return s;
    }

    private static String folderLabel() {
        Uri t = tree;
        if (t == null) return "";
        String root = DocumentsContract.getTreeDocumentId(t);
        String n = name(t, root);
        if (n == null) n = root;
        return FOLDER.equalsIgnoreCase(n) ? n : n + "/" + FOLDER;
    }

    // The ☁️ button's dialog.
    static void showDialog(Activity a, boolean uk) {
        new Thread(() -> {
            final String folder = folderLabel();  // a provider query: off the UI thread
            a.runOnUiThread(() -> dialog(a, uk, folder));
        }).start();
    }

    private static void dialog(Activity a, boolean uk, String folder) {
        AlertDialog.Builder b = new AlertDialog.Builder(a).setTitle(uk ? "Синхронізація збережень" : "Save sync");
        if (folder.isEmpty()) {
            b.setMessage(uk ? "Оберіть папку, яку синхронізує Syncthing, Round Sync (rclone), Nextcloud чи інша "
                            + "програма: гра створить у ній папку ReHOMM4 і копіюватиме збереження туди й звідти."
                            : "Pick a folder that Syncthing, Round Sync (rclone), Nextcloud or another app keeps in "
                            + "sync: the game makes a ReHOMM4 folder in it and copies saved games there and back.");
        } else {
            b.setMessage((uk ? "Папка: " : "Folder: ") + folder + "\n" + status);
            b.setNeutralButton(uk ? "Синхронізувати зараз" : "Sync now", (d, w) -> soon());
            b.setNegativeButton(uk ? "Припинити" : "Stop syncing", (d, w) -> stop());
        }
        b.setPositiveButton(folder.isEmpty() ? (uk ? "Обрати папку…" : "Choose a folder…")
                                             : (uk ? "Інша папка…" : "Another folder…"), (d, w) -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
            i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                    | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
            try {
                a.startActivityForResult(i, PICK_FOLDER);
            } catch (Exception e) {
                // no picker
            }
        });
        b.show();
    }
}
