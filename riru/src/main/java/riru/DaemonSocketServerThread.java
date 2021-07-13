package riru;

import android.net.Credentials;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.util.Log;

import java.io.EOFException;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import rikka.io.LittleEndianDataInputStream;
import rikka.io.LittleEndianDataOutputStream;
import riru.core.BuildConfig;

import static riru.Daemon.TAG;

public class DaemonSocketServerThread extends Thread {

    private static final int ACTION_READ_FILE = 4;
    private static final int ACTION_READ_DIR = 5;

    // used by riru itself only, could be removed in the future
    private static final int ACTION_WRITE_STATUS = 2;
    private static final int ACTION_READ_NATIVE_BRIDGE = 3;
    private static final int ACTION_READ_MAGISK_TMPFS_PATH = 6;

    private static final Executor EXECUTOR = Executors.newFixedThreadPool(Runtime.getRuntime().availableProcessors() / 4 + 1);

    private LocalServerSocket serverSocket;
    private CountDownLatch countDownLatch;

    private void handleReadOriginalNativeBridge(LittleEndianDataOutputStream out) throws IOException {
        String s = DaemonUtils.getOriginalNativeBridge();
        out.writeInt(s.length());
        out.write(s.getBytes());
    }

    private void handleMagiskTmpfsPath(LittleEndianDataOutputStream out) throws IOException {
        String s = DaemonUtils.getMagiskTmpfsPath();
        out.writeInt(s.length());
        out.write(s.getBytes());
    }

    private void writeToFile(File parent, String name, String content) {
        if (!parent.exists()) {
            //noinspection ResultOfMethodCallIgnored
            parent.mkdirs();
        }
        File file = new File(parent, name);

        try (FileOutputStream out = new FileOutputStream(file)) {
            out.write(content.getBytes());
        } catch (IOException ignored) {
        }
    }

    private void handleWriteStatus(LittleEndianDataInputStream in, LittleEndianDataOutputStream out) throws IOException {
        boolean is64Bit = in.readBoolean();
        int count = in.readInt();

        File parent = new File("/dev/riru" + (is64Bit ? "64" : "") + "_" + DaemonUtils.getDevRandom());
        File modules = new File(parent, "modules");

        writeToFile(parent, "api", Integer.toString(BuildConfig.RIRU_API_VERSION));
        writeToFile(parent, "version", Integer.toString(BuildConfig.RIRU_VERSION_CODE));
        writeToFile(parent, "version_name", BuildConfig.RIRU_VERSION_NAME);

        byte[] buffer = new byte[256];
        int size;
        String id;
        int apiVersion;
        int version;
        String versionName;
        boolean supportHide;

        for (int i = 0; i < count; i++) {
            size = in.readInt();
            in.readFully(buffer, 0, size);
            id = new String(buffer, 0, size);
            Arrays.fill(buffer, (byte) 0);

            apiVersion = in.readInt();
            version = in.readInt();

            size = in.readInt();
            in.readFully(buffer, 0, size);
            versionName = new String(buffer, 0, size);
            Arrays.fill(buffer, (byte) 0);

            supportHide = in.readBoolean();

            File module = new File(modules, id);

            writeToFile(module, "hide", Boolean.toString(supportHide));
            writeToFile(module, "api", Integer.toString(apiVersion));
            writeToFile(module, "version", Integer.toString(version));
            writeToFile(module, "version_name", versionName);
        }
    }

    private void handleReadFile(LittleEndianDataInputStream in, LittleEndianDataOutputStream out) throws IOException {
        int path_size = in.readInt();
        byte[] path_bytes = new byte[path_size];
        in.readFully(path_bytes);
        String path = new String(path_bytes);

        Log.i(TAG, "Read file " + path);

        FileDescriptor fd;
        try {
            fd = Os.open(path, OsConstants.O_RDONLY, 0);
            out.writeInt(0);
        } catch (ErrnoException e) {
            Log.w(TAG, "Open file " + path + " failed with " + e.errno, e);
            out.writeInt(e.errno);
            return;
        }

        int file_size;
        try {
            file_size = (int) Os.fstat(fd).st_size;
        } catch (ErrnoException e) {
            file_size = -1;
        }
        out.writeInt(file_size);

        byte[] buffer = new byte[8192];
        try (InputStream fin = new FileInputStream(fd)) {
            if (file_size > 0) {
                int bytes_remaining = file_size;

                do {
                    int count = fin.read(buffer, 0, 8192);
                    if (count == -1) {
                        return;
                    } else {
                        out.write(buffer, 0, count);
                        bytes_remaining -= count;
                    }
                } while (bytes_remaining > 0);
            } else if (file_size == -1) {
                int count;
                while ((count = fin.read(buffer, 0, 8192)) > 0) {
                    out.write(buffer, 0, count);
                }
            }
        }
    }

    private void handleReadDir(LittleEndianDataInputStream in, LittleEndianDataOutputStream out) throws IOException {
        int path_size = in.readInt();
        byte[] path_bytes = new byte[path_size];
        in.readFully(path_bytes);
        String path = new String(path_bytes);

        Log.i(TAG, "Read dir " + path);

        FileDescriptor fd;
        try {
            fd = Os.open(path, OsConstants.O_RDONLY | 0x00200000 /*O_DIRECTORY*/, 0);
            out.writeInt(0);
        } catch (ErrnoException e) {
            Log.w(TAG, "Open file " + path + " failed with " + e.errno, e);
            out.writeInt(e.errno);
            return;
        }
        try {
            Os.close(fd);
        } catch (ErrnoException ignored) {
        }

        File[] files = new File(path).listFiles();
        int index = 0;
        int size = files != null ? files.length : -1;
        while (true) {
            boolean continue_read = in.readBoolean();
            if (!continue_read) {
                break;
            }
            if (files == null || index >= size) {
                out.writeInt(-1);
                break;
            } else {
                out.writeInt(0);
            }

            File f = files[index];
            index++;

            boolean isFile = f.isFile();
            boolean isDir = f.isDirectory();
            if (isFile)
                out.writeByte(/*DT_REG*/ 8);
            else if (isDir)
                out.writeByte(/*DT_DIR*/ 4);
            else
                out.writeByte(/*DT_UNKNOWN*/0);

            byte[] name = Arrays.copyOf(f.getName().getBytes(StandardCharsets.UTF_8), 256);
            name[255] = 0; // null-terminated
            out.write(name);
        }
    }

    private void handleAction(LittleEndianDataInputStream in, LittleEndianDataOutputStream out, int action) throws IOException {
        Log.i(TAG, "Action " + action);

        switch (action) {
            case ACTION_READ_NATIVE_BRIDGE: {
                Log.i(TAG, "Action: read original native bridge");
                handleReadOriginalNativeBridge(out);
                break;
            }
            case ACTION_READ_MAGISK_TMPFS_PATH: {
                Log.i(TAG, "Action: read Magisk tmpfs path");
                handleMagiskTmpfsPath(out);
                break;
            }
            case ACTION_WRITE_STATUS: {
                Log.i(TAG, "Action: write status");
                handleWriteStatus(in, out);
                break;
            }
            case ACTION_READ_FILE: {
                Log.i(TAG, "Action: read file");
                handleReadFile(in, out);
                break;
            }
            case ACTION_READ_DIR: {
                Log.i(TAG, "Action: read dir");
                handleReadDir(in, out);
                break;
            }
            default:
                break;
        }

        Log.i(TAG, "Handle action " + action + " finished");
    }

    private void handleSocket(LocalSocket socket) throws IOException {
        int action;
        boolean first = true;

        try (LittleEndianDataInputStream in = new LittleEndianDataInputStream(socket.getInputStream());
             LittleEndianDataOutputStream out = new LittleEndianDataOutputStream(socket.getOutputStream())) {

            while (true) {
                try {
                    action = in.readInt();
                } catch (EOFException e) {
                    if (first) {
                        Log.e(TAG, "Failed to read next action", e);
                    } else {
                        Log.i(TAG, "No next action, exiting...");
                    }
                    return;
                }

                handleAction(in, out, action);
                first = false;
            }
        }
    }

    private void startServer() throws IOException {
        serverSocket = new LocalServerSocket("rirud");

        while (true) {
            Log.d(TAG, "Accept");
            LocalSocket socket;
            try {
                socket = serverSocket.accept();
            } catch (IOException e) {
                Log.w(TAG, "Accept failed, server is closed ?", e);
                return;
            }

            Credentials credentials = socket.getPeerCredentials();
            if (credentials.getUid() != 0) {
                socket.close();
                Log.w(TAG, "Accept from non-root (" +
                        "uid=" + credentials.getUid() + ", " +
                        "pid=" + credentials.getPid() + ")");
                continue;
            }
            Log.d(TAG, "Accepted " +
                    "uid=" + credentials.getUid() + " " +
                    "pid=" + credentials.getPid());

            EXECUTOR.execute(() -> {
                try {
                    handleSocket(socket);
                } catch (Throwable e) {
                    Log.w(TAG, "Handle socket", e);
                } finally {
                    try {
                        socket.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            });
        }
    }

    public void stopServer() {
        Log.i(TAG, "Stop server");

        if (serverSocket != null) {
            try {
                Os.shutdown(serverSocket.getFileDescriptor(), OsConstants.SHUT_RD);
                serverSocket.close();
                Log.i(TAG, "Server stopped");
            } catch (IOException | ErrnoException e) {
                Log.w(TAG, "Close server socket", e);
            }
            serverSocket = null;
        } else {
            Log.w(TAG, "Server socket is null while stopping server");
        }
    }

    public void restartServer() {
        Log.i(TAG, "Restart server");

        if (serverSocket != null) {
            Log.w(TAG, "Socket is not null while restarting server");
            return;
        }

        if (countDownLatch != null) {
            countDownLatch.countDown();
        }
    }

    @Override
    public void run() {
        //noinspection InfiniteLoopStatement
        while (true) {
            Log.d(TAG, "Start server");

            try {
                startServer();
            } catch (Throwable e) {
                Log.w(TAG, "Start server", e);
            }

            countDownLatch = new CountDownLatch(1);
            Log.i(TAG, "Waiting for restart server...");

            try {
                countDownLatch.await();
                Log.i(TAG, "Restart server received");
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
