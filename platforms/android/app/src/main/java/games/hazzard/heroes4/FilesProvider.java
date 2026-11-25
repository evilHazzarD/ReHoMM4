package games.hazzard.heroes4;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.provider.DocumentsContract.Root;
import android.provider.DocumentsProvider;
import android.webkit.MimeTypeMap;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

// The app's files (getExternalFilesDir: game/, player/... with the saves) as a place of the
// system's Files app (DocumentsUI): Android 11+ lets no file manager into Android/data, so the
// settings' Open buttons (HeroesActivity.h4OpenFolder) open the folders here, and the player can
// copy saves and the game's files in and out. A document's id: "root", or "root/" + its path below
// the root (DocumentsUI takes no empty id).
public final class FilesProvider extends DocumentsProvider {
    static final String AUTHORITY = "games.hazzard.heroes4.files";
    private static final String ROOT = "files", TOP = "root";

    private static final String[] ROOT_COLUMNS = {Root.COLUMN_ROOT_ID, Root.COLUMN_FLAGS, Root.COLUMN_TITLE,
            Root.COLUMN_DOCUMENT_ID, Root.COLUMN_ICON, Root.COLUMN_AVAILABLE_BYTES};
    private static final String[] DOC_COLUMNS = {Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME,
            Document.COLUMN_MIME_TYPE, Document.COLUMN_FLAGS, Document.COLUMN_SIZE, Document.COLUMN_LAST_MODIFIED};

    private File base() {
        File ext = getContext().getExternalFilesDir(null);
        return ext != null ? ext : getContext().getFilesDir();
    }

    // The document id of a file below the root (null: outside it).
    static String idOf(File root, File f) {
        String r = root.getAbsolutePath(), p = f.getAbsolutePath();
        if (p.equals(r)) return TOP;
        return p.startsWith(r + "/") ? TOP + "/" + p.substring(r.length() + 1) : null;
    }

    private File fileOf(String id) throws FileNotFoundException {
        File root = base();
        if (id == null || !(id.equals(TOP) || id.startsWith(TOP + "/"))) throw new FileNotFoundException(id);
        File f = id.equals(TOP) ? root : new File(root, id.substring(TOP.length() + 1));
        try {
            String c = f.getCanonicalPath(), rc = root.getCanonicalPath();
            if (!c.equals(rc) && !c.startsWith(rc + "/")) throw new FileNotFoundException(id);
        } catch (IOException e) {
            throw new FileNotFoundException(id);
        }
        return f;
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor queryRoots(String[] projection) {
        MatrixCursor c = new MatrixCursor(projection != null ? projection : ROOT_COLUMNS);
        File root = base();
        MatrixCursor.RowBuilder r = c.newRow();
        r.add(Root.COLUMN_ROOT_ID, ROOT);
        r.add(Root.COLUMN_FLAGS, Root.FLAG_SUPPORTS_CREATE | Root.FLAG_SUPPORTS_IS_CHILD | Root.FLAG_LOCAL_ONLY);
        r.add(Root.COLUMN_TITLE, getContext().getString(R.string.app_name));
        r.add(Root.COLUMN_DOCUMENT_ID, TOP);
        r.add(Root.COLUMN_ICON, R.drawable.ic_launcher);
        r.add(Root.COLUMN_AVAILABLE_BYTES, root.getFreeSpace());
        return c;
    }

    private void row(MatrixCursor c, String id, File f) {
        int flags = 0;
        String mime;
        if (f.isDirectory()) {
            mime = Document.MIME_TYPE_DIR;
            if (f.canWrite()) flags |= Document.FLAG_DIR_SUPPORTS_CREATE;
        } else {
            mime = mimeOf(f.getName());
            if (f.canWrite()) flags |= Document.FLAG_SUPPORTS_WRITE;
        }
        if (!id.equals(TOP) && f.canWrite())
            flags |= Document.FLAG_SUPPORTS_DELETE | Document.FLAG_SUPPORTS_RENAME;
        MatrixCursor.RowBuilder r = c.newRow();
        r.add(Document.COLUMN_DOCUMENT_ID, id);
        r.add(Document.COLUMN_DISPLAY_NAME, id.equals(TOP) ? getContext().getString(R.string.app_name) : f.getName());
        r.add(Document.COLUMN_MIME_TYPE, mime);
        r.add(Document.COLUMN_FLAGS, flags);
        r.add(Document.COLUMN_SIZE, f.isDirectory() ? null : f.length());
        r.add(Document.COLUMN_LAST_MODIFIED, f.lastModified());
    }

    private static String mimeOf(String name) {
        int dot = name.lastIndexOf('.');
        if (dot >= 0) {
            String m = MimeTypeMap.getSingleton().getMimeTypeFromExtension(name.substring(dot + 1).toLowerCase());
            if (m != null) return m;
        }
        return "application/octet-stream";
    }

    private static String child(String parent, String name) {
        return parent + "/" + name;
    }

    @Override
    public Cursor queryDocument(String id, String[] projection) throws FileNotFoundException {
        MatrixCursor c = new MatrixCursor(projection != null ? projection : DOC_COLUMNS);
        File f = fileOf(id);
        if (!f.exists()) throw new FileNotFoundException(id);
        row(c, id, f);
        return c;
    }

    @Override
    public Cursor queryChildDocuments(String parent, String[] projection, String sortOrder)
            throws FileNotFoundException {
        MatrixCursor c = new MatrixCursor(projection != null ? projection : DOC_COLUMNS);
        File[] list = fileOf(parent).listFiles();
        if (list != null)
            for (File f : list) row(c, child(parent, f.getName()), f);
        return c;
    }

    @Override
    public boolean isChildDocument(String parent, String id) {
        return id.equals(parent) || id.startsWith(parent + "/");
    }

    // The ids from `parent` (null: the root) down to `id`: the Files app opens a folder by it.
    @Override
    public DocumentsContract.Path findDocumentPath(String parent, String id) throws FileNotFoundException {
        fileOf(id);
        String from = parent != null ? parent : TOP;
        if (!isChildDocument(from, id)) throw new FileNotFoundException(id);
        java.util.List<String> ids = new java.util.ArrayList<>();
        ids.add(from);
        for (int i = id.indexOf('/', from.length() + 1); ; i = id.indexOf('/', i + 1)) {
            if (id.length() <= from.length()) break;
            if (i < 0) {
                ids.add(id);
                break;
            }
            ids.add(id.substring(0, i));
        }
        return new DocumentsContract.Path(parent != null ? null : ROOT, ids);
    }

    @Override
    public ParcelFileDescriptor openDocument(String id, String mode, CancellationSignal signal)
            throws FileNotFoundException {
        return ParcelFileDescriptor.open(fileOf(id), ParcelFileDescriptor.parseMode(mode));
    }

    @Override
    public String createDocument(String parent, String mime, String name) throws FileNotFoundException {
        File dir = fileOf(parent);
        File f = new File(dir, name);
        for (int i = 1; f.exists(); i++) f = new File(dir, name + " (" + i + ")");
        try {
            boolean ok = Document.MIME_TYPE_DIR.equals(mime) ? f.mkdir() : f.createNewFile();
            if (!ok) throw new FileNotFoundException(name);
        } catch (IOException e) {
            throw new FileNotFoundException(name);
        }
        return child(parent, f.getName());
    }

    private static boolean deleteAll(File f) {
        File[] list = f.listFiles();
        if (list != null)
            for (File c : list) deleteAll(c);
        return f.delete();
    }

    @Override
    public void deleteDocument(String id) throws FileNotFoundException {
        if (id.equals(TOP) || !deleteAll(fileOf(id))) throw new FileNotFoundException(id);
    }

    @Override
    public String renameDocument(String id, String name) throws FileNotFoundException {
        File f = fileOf(id), to = new File(f.getParentFile(), name);
        if (id.equals(TOP) || name.contains("/") || to.exists() || !f.renameTo(to)) throw new FileNotFoundException(id);
        return child(id.substring(0, id.lastIndexOf('/')), name);
    }
}
