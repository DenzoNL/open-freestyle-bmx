package org.openfreestylebmx.app;

import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.CountDownLatch;

import org.libsdl.app.SDLActivity;

/*
 * SDL's activity with our libraries and command line (docs/PORTING.md "X6 step 1 (Android arm64)"). SDL loads
 * libSDL3.so and libmain.so and calls libmain.so's SDL_main (src/native/main.c) on its own thread; that changes
 * into the game folder under the app's external files folder (src/native/android.c).
 *
 * The first start without that folder (docs/RELEASE-PLAN.md D2-A): android.c calls pickGameFolder on SDL's
 * thread, which asks for the player's game folder (the Storage Access Framework's folder picker, no storage
 * permission needed), checks it for Scene.Txt and copies it into the app's files folder once.
 */
public class BmxActivity extends SDLActivity {
    private static final int kPickRequest = 0x4f46; /* "OF": apart from SDL's own request codes */

    private static CountDownLatch sDone;
    private static boolean sResult;
    private static String sTarget;

    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL3", "main"};
    }

    @Override
    protected String[] getArguments() {
        return new String[] {"Scene.Txt"};
    }

    /*
     * Called from SDL's thread (android.c) when <files>/game has no Scene.Txt: blocks until the player has picked
     * a folder with the game and it is copied to `target`, or has given up (false).
     */
    public static boolean pickGameFolder(String target) {
        final BmxActivity activity = (BmxActivity) mSingleton;

        if (activity == null) {
            return false;
        }
        sTarget = target;
        sResult = false;
        sDone = new CountDownLatch(1);
        activity.runOnUiThread(() -> activity.ask(null));
        try {
            sDone.await();
        } catch (InterruptedException e) {
            return false;
        }
        return sResult;
    }

    /* The explanation (with a problem from the last try, if any), then the picker; Cancel ends the app. */
    private void ask(String problem) {
        String text = "Open Freestyle BMX needs the files of the PC game.\n\n"
                + "Copy the installed game folder (the one with Scene.Txt and the Assets folder) to this device, "
                + "then choose it in the next screen. It is copied into the app once (about 250 MB).";
        if (problem != null) {
            text = problem + "\n\n" + text;
        }
        new AlertDialog.Builder(this)
                .setTitle("Choose the game folder")
                .setMessage(text)
                .setCancelable(false)
                .setPositiveButton("Choose folder", (d, w) -> {
                    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                    startActivityForResult(intent, kPickRequest);
                })
                .setNegativeButton("Quit", (d, w) -> finishPick(false))
                .show();
    }

    private static void finishPick(boolean ok) {
        sResult = ok;
        if (sDone != null) {
            sDone.countDown();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode != kPickRequest) {
            super.onActivityResult(requestCode, resultCode, data);
            return;
        }
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            ask(null);
            return;
        }
        final Uri tree = data.getData();
        final AlertDialog progress = new AlertDialog.Builder(this)
                .setTitle("Copying the game files")
                .setMessage("Starting...")
                .setCancelable(false)
                .show();
        new Thread(() -> {
            String problem = null;
            try {
                copyGame(tree, progress);
            } catch (Exception e) {
                problem = e.getMessage() != null ? e.getMessage() : e.toString();
            }
            final String failed = problem;
            runOnUiThread(() -> {
                progress.dismiss();
                if (failed == null) {
                    finishPick(true);
                } else {
                    ask(failed);
                }
            });
        }).start();
    }

    /* The picked folder's children: {document id, display name, mime type}. */
    private java.util.List<String[]> children(ContentResolver resolver, Uri tree, String parentId) {
        java.util.List<String[]> out = new java.util.ArrayList<>();
        Uri uri = DocumentsContract.buildChildDocumentsUriUsingTree(tree, parentId);
        String[] columns = {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                DocumentsContract.Document.COLUMN_DISPLAY_NAME, DocumentsContract.Document.COLUMN_MIME_TYPE};
        try (Cursor c = resolver.query(uri, columns, null, null, null)) {
            while (c != null && c.moveToNext()) {
                out.add(new String[] {c.getString(0), c.getString(1), c.getString(2)});
            }
        }
        return out;
    }

    private int mCopied;

    private void copyGame(Uri tree, AlertDialog progress) throws Exception {
        ContentResolver resolver = getContentResolver();
        String rootId = DocumentsContract.getTreeDocumentId(tree);
        boolean hasScene = false;

        for (String[] child : children(resolver, tree, rootId)) {
            hasScene |= child[1] != null && child[1].equalsIgnoreCase("Scene.Txt");
        }
        if (!hasScene) {
            throw new Exception("That folder does not contain the game (no Scene.Txt in it).");
        }
        File target = new File(sTarget);
        File part = new File(sTarget + ".part");
        deleteTree(part);
        mCopied = 0;
        copyFolder(resolver, tree, rootId, part, progress);
        deleteTree(target);
        if (!part.renameTo(target)) {
            throw new Exception("Could not finish the copy (rename to " + target + ").");
        }
    }

    private void copyFolder(ContentResolver resolver, Uri tree, String id, File dir, AlertDialog progress)
            throws Exception {
        if (!dir.isDirectory() && !dir.mkdirs()) {
            throw new Exception("Cannot create " + dir);
        }
        for (String[] child : children(resolver, tree, id)) {
            String name = child[1];
            if (DocumentsContract.Document.MIME_TYPE_DIR.equals(child[2])) {
                copyFolder(resolver, tree, child[0], new File(dir, name), progress);
                continue;
            }
            if (name.toLowerCase(java.util.Locale.ROOT).endsWith(".exe")) {
                continue; /* the original program: nothing reads it */
            }
            Uri file = DocumentsContract.buildDocumentUriUsingTree(tree, child[0]);
            try (InputStream in = resolver.openInputStream(file);
                    OutputStream out = new FileOutputStream(new File(dir, name))) {
                byte[] buffer = new byte[1 << 16];
                int n;
                while ((n = in.read(buffer)) > 0) {
                    out.write(buffer, 0, n);
                }
            }
            final int count = ++mCopied;
            final String shown = name;
            runOnUiThread(() -> progress.setMessage(count + " files copied\n" + shown));
        }
    }

    private static void deleteTree(File f) {
        File[] list = f.listFiles();
        if (list != null) {
            for (File c : list) {
                deleteTree(c);
            }
        }
        f.delete();
    }
}
