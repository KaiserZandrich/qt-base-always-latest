// Copyright (C) 2016 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Objects;
import java.util.concurrent.Semaphore;

import android.app.Activity;
import android.app.Service;
import android.content.Context;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;
import android.content.UriPermission;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.system.Os;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.View;
import android.view.InputDevice;
import android.view.Display;
import android.hardware.display.DisplayManager;
import android.database.Cursor;
import android.provider.DocumentsContract;

import java.lang.reflect.Method;
import java.security.KeyStore;
import java.security.cert.X509Certificate;
import java.util.Iterator;
import java.util.List;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import android.util.Size;
import android.util.DisplayMetrics;
import android.view.WindowManager;
import android.view.WindowMetrics;
import android.graphics.Rect;

public class QtNative
{
    private static Activity m_activity = null;
    private static boolean m_activityPaused = false;
    private static Service m_service = null;
    private static QtActivityDelegate m_activityDelegate = null;
    private static QtServiceDelegate m_serviceDelegate = null;
    public static Object m_mainActivityMutex = new Object(); // mutex used to synchronize runnable operations

    public static final String QtTAG = "Qt JAVA"; // string used for Log.x
    private static ArrayList<Runnable> m_lostActions = new ArrayList<Runnable>(); // a list containing all actions which could not be performed (e.g. the main activity is destroyed, etc.)
    private static boolean m_started = false;
    private static int m_displayMetricsScreenWidthPixels = 0;
    private static int m_displayMetricsScreenHeightPixels = 0;
    private static int m_displayMetricsAvailableLeftPixels = 0;
    private static int m_displayMetricsAvailableTopPixels = 0;
    private static int m_displayMetricsAvailableWidthPixels = 0;
    private static int m_displayMetricsAvailableHeightPixels = 0;
    private static float m_displayMetricsRefreshRate = 60;
    private static double m_displayMetricsXDpi = .0;
    private static double m_displayMetricsYDpi = .0;
    private static double m_displayMetricsScaledDensity = 1.0;
    private static double m_displayMetricsDensity = 1.0;
    private static final int m_moveThreshold = 0;
    private static Method m_checkSelfPermissionMethod = null;

    public static QtThread m_qtThread = new QtThread();

    private static final String INVALID_OR_NULL_URI_ERROR_MESSAGE = "Received invalid/null Uri";

    private static final Runnable runPendingCppRunnablesRunnable = new Runnable() {
        @Override
        public void run() {
            runPendingCppRunnables();
        }
    };

    public static boolean isStarted()
    {
        boolean hasActivity = m_activity != null && m_activityDelegate != null;
        boolean hasService = m_service != null && m_serviceDelegate != null;
        return m_started && (hasActivity || hasService);
    }

    private static ClassLoader m_classLoader = null;
    public static ClassLoader classLoader()
    {
        return m_classLoader;
    }

    public static void setClassLoader(ClassLoader classLoader)
    {
            m_classLoader = classLoader;
    }

    public static Activity activity()
    {
        synchronized (m_mainActivityMutex) {
            return m_activity;
        }
    }

    public static Service service()
    {
        synchronized (m_mainActivityMutex) {
            return m_service;
        }
    }


    public static QtActivityDelegate activityDelegate()
    {
        synchronized (m_mainActivityMutex) {
            return m_activityDelegate;
        }
    }

    public static QtServiceDelegate serviceDelegate()
    {
        synchronized (m_mainActivityMutex) {
            return m_serviceDelegate;
        }
    }

    public static String[] getStringArray(String joinedString)
    {
        return joinedString.split(",");
    }

    private static String getCurrentMethodNameLog()
    {
        return new Exception().getStackTrace()[1].getMethodName() + ": ";
    }

    private static Uri getUriWithValidPermission(Context context, String uri, String openMode)
    {
        Uri parsedUri;
        try {
            parsedUri = Uri.parse(uri);
        } catch (NullPointerException e) {
            e.printStackTrace();
            return null;
        }

        try {
            String scheme = parsedUri.getScheme();

            // We only want to check permissions for content Uris
            if (scheme.compareTo("content") != 0)
                return parsedUri;

            List<UriPermission> permissions = context.getContentResolver().getPersistedUriPermissions();
            String uriStr = parsedUri.getPath();

            for (int i = 0; i < permissions.size(); ++i) {
                Uri iterUri = permissions.get(i).getUri();
                boolean isRequestPermission = permissions.get(i).isReadPermission();

                if (!openMode.equals("r"))
                   isRequestPermission = permissions.get(i).isWritePermission();

                if (iterUri.getPath().equals(uriStr) && isRequestPermission)
                    return iterUri;
            }

            // if we only have transient permissions on uri all the above will fail,
            // but we will be able to read the file anyway, so continue with uri here anyway
            // and check for SecurityExceptions later
            return parsedUri;
        } catch (SecurityException e) {
            Log.e(QtTAG, getCurrentMethodNameLog() + e.toString());
            return parsedUri;
        }
    }

    public static boolean openURL(Context context, String url, String mime)
    {
        final Uri uri = getUriWithValidPermission(context, url, "r");
        if (uri == null) {
            Log.e(QtTAG, getCurrentMethodNameLog() + INVALID_OR_NULL_URI_ERROR_MESSAGE);
            return false;
        }

        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            if (!mime.isEmpty())
                intent.setDataAndType(uri, mime);

            activity().startActivity(intent);

            return true;
        } catch (Exception e) {
            Log.e(QtTAG, getCurrentMethodNameLog() + e.toString());
            return false;
        }
    }

    static QtThread getQtThread() {
        return m_qtThread;
    }

    public static void setActivity(Activity qtMainActivity, QtActivityDelegate qtActivityDelegate)
    {
        synchronized (m_mainActivityMutex) {
            m_activity = qtMainActivity;
            m_activityDelegate = qtActivityDelegate;
        }
    }

    public static void setService(Service qtMainService, QtServiceDelegate qtServiceDelegate)
    {
        synchronized (m_mainActivityMutex) {
            m_service = qtMainService;
            m_serviceDelegate = qtServiceDelegate;
        }
    }

    public static void setApplicationState(int state)
    {
        synchronized (m_mainActivityMutex) {
            switch (state) {
                case QtConstants.ApplicationState.ApplicationActive:
                    m_activityPaused = false;
                    Iterator<Runnable> itr = m_lostActions.iterator();
                    while (itr.hasNext())
                        runAction(itr.next());
                    m_lostActions.clear();
                    break;
                default:
                    m_activityPaused = true;
                    break;
            }
        }
        updateApplicationState(state);
    }

    static void runAction(Runnable action)
    {
        synchronized (m_mainActivityMutex) {
            final Looper mainLooper = Looper.getMainLooper();
            final Handler handler = new Handler(mainLooper);
            final boolean active = (m_activity != null && !m_activityPaused) || m_service != null;
            if (!active || mainLooper == null || !handler.post(action))
                m_lostActions.add(action);
        }
    }

    private static void runPendingCppRunnablesOnAndroidThread()
    {
        synchronized (m_mainActivityMutex) {
            if (m_activity != null) {
                if (!m_activityPaused)
                    m_activity.runOnUiThread(runPendingCppRunnablesRunnable);
                else
                    runAction(runPendingCppRunnablesRunnable);
            } else {
                final Looper mainLooper = Looper.getMainLooper();
                final Thread looperThread = mainLooper.getThread();
                if (looperThread.equals(Thread.currentThread())) {
                    runPendingCppRunnablesRunnable.run();
                } else {
                    final Handler handler = new Handler(mainLooper);
                    handler.post(runPendingCppRunnablesRunnable);
                }
            }
        }
    }

    private static void setViewVisibility(final View view, final boolean visible)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                view.setVisibility(visible ? View.VISIBLE : View.GONE);
            }
        });
    }

    public static Display getDisplay(int displayId)
    {
        Context context = getContext();
        DisplayManager displayManager =
                (DisplayManager)context.getSystemService(Context.DISPLAY_SERVICE);
        if (displayManager != null) {
            return displayManager.getDisplay(displayId);
        }
        return null;
    }

    public static List<Display> getAvailableDisplays()
    {
        Context context = getContext();
        DisplayManager displayManager =
                (DisplayManager)context.getSystemService(Context.DISPLAY_SERVICE);
        if (displayManager != null) {
            Display[] displays = displayManager.getDisplays();
            return Arrays.asList(displays);
        }
        return new ArrayList<Display>();
    }

    public static Size getDisplaySize(Context displayContext, Display display)
    {
        if (Build.VERSION.SDK_INT < 31) {
            DisplayMetrics realMetrics = new DisplayMetrics();
            display.getRealMetrics(realMetrics);
            return new Size(realMetrics.widthPixels, realMetrics.heightPixels);
        }

        Context windowsContext = displayContext.createWindowContext(
                                                WindowManager.LayoutParams.TYPE_APPLICATION, null);
        WindowManager displayMgr =
                        (WindowManager) windowsContext.getSystemService(Context.WINDOW_SERVICE);
        WindowMetrics windowsMetrics = displayMgr.getCurrentWindowMetrics();
        Rect bounds = windowsMetrics.getBounds();
        return new Size(bounds.width(), bounds.height());
    }

    public static boolean startApplication(ArrayList<String> params, String mainLib)
    {
        final boolean[] res = new boolean[1];
        synchronized (m_mainActivityMutex) {
            String paramsStr = String.join("\t", params);
            final String qtParams = mainLib + "\t" + paramsStr;
            m_qtThread.run(new Runnable() {
                @Override
                public void run() {
                    res[0] = startQtAndroidPlugin(qtParams);
                    setDisplayMetrics(
                            m_displayMetricsScreenWidthPixels, m_displayMetricsScreenHeightPixels,
                            m_displayMetricsAvailableLeftPixels, m_displayMetricsAvailableTopPixels,
                            m_displayMetricsAvailableWidthPixels,
                            m_displayMetricsAvailableHeightPixels, m_displayMetricsXDpi,
                            m_displayMetricsYDpi, m_displayMetricsScaledDensity,
                            m_displayMetricsDensity, m_displayMetricsRefreshRate);
                }
            });
            m_qtThread.post(new Runnable() {
                @Override
                public void run() {
                    startQtApplication();
                }
            });
            waitForServiceSetup();
            m_started = true;
        }
        return res[0];
    }

    public static void setApplicationDisplayMetrics(int screenWidthPixels, int screenHeightPixels,
                                                    int availableLeftPixels, int availableTopPixels,
                                                    int availableWidthPixels,
                                                    int availableHeightPixels, double XDpi,
                                                    double YDpi, double scaledDensity,
                                                    double density, float refreshRate)
    {
        /* Fix buggy dpi report */
        if (XDpi < android.util.DisplayMetrics.DENSITY_LOW)
            XDpi = android.util.DisplayMetrics.DENSITY_LOW;
        if (YDpi < android.util.DisplayMetrics.DENSITY_LOW)
            YDpi = android.util.DisplayMetrics.DENSITY_LOW;

        synchronized (m_mainActivityMutex) {
            if (m_started) {
                setDisplayMetrics(screenWidthPixels, screenHeightPixels, availableLeftPixels,
                                  availableTopPixels, availableWidthPixels, availableHeightPixels,
                                  XDpi, YDpi, scaledDensity, density, refreshRate);
            } else {
                m_displayMetricsScreenWidthPixels = screenWidthPixels;
                m_displayMetricsScreenHeightPixels = screenHeightPixels;
                m_displayMetricsAvailableLeftPixels = availableLeftPixels;
                m_displayMetricsAvailableTopPixels = availableTopPixels;
                m_displayMetricsAvailableWidthPixels = availableWidthPixels;
                m_displayMetricsAvailableHeightPixels = availableHeightPixels;
                m_displayMetricsXDpi = XDpi;
                m_displayMetricsYDpi = YDpi;
                m_displayMetricsScaledDensity = scaledDensity;
                m_displayMetricsDensity = density;
                m_displayMetricsRefreshRate = refreshRate;
            }
        }
    }

    // application methods
    public static native boolean startQtAndroidPlugin(String params);
    public static native void startQtApplication();
    public static native void waitForServiceSetup();
    public static native void quitQtCoreApplication();
    public static native void quitQtAndroidPlugin();
    public static native void terminateQt();
    public static native boolean updateNativeActivity();
    // application methods

    public static void quitApp()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                quitQtAndroidPlugin();
                if (m_activity != null)
                     m_activity.finish();
                 if (m_service != null)
                     m_service.stopSelf();

                 m_started = false;
            }
        });
    }

    public static Context getContext() {
        if (m_activity != null)
            return m_activity;
        return m_service;
    }

    public static int checkSelfPermission(String permission)
    {
        int perm = PackageManager.PERMISSION_DENIED;
        synchronized (m_mainActivityMutex) {
            Context context = getContext();
            PackageManager pm = context.getPackageManager();
            perm = pm.checkPermission(permission, context.getPackageName());
        }

        return perm;
    }

    // TODO get rid of the delegation from QtNative, call directly the Activity in c++
    private static void setSystemUiVisibility(final int systemUiVisibility)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.setSystemUiVisibility(systemUiVisibility);
                }
                updateWindow();
            }
        });
    }

    private static void notifyAccessibilityLocationChange(final int viewId)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyAccessibilityLocationChange(viewId);
                }
            }
        });
    }

    private static void notifyObjectHide(final int viewId, final int parentId)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyObjectHide(viewId, parentId);
                }
            }
        });
    }

    private static void notifyObjectFocus(final int viewId)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyObjectFocus(viewId);
                }
            }
        });
    }

    private static void notifyValueChanged(int viewId, String value)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyValueChanged(viewId, value);
                }
            }
        });
    }

    private static void notifyScrolledEvent(final int viewId)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null) {
                    m_activityDelegate.notifyScrolledEvent(viewId);
                }
            }
        });
    }

    public static void notifyQtAndroidPluginRunning(final boolean running)
    {
        if (m_activityDelegate != null)
            m_activityDelegate.notifyQtAndroidPluginRunning(running);
    }



    private static void openContextMenu(final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.openContextMenu(x, y, w, h);
            }
        });
    }

    private static void closeContextMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.closeContextMenu();
            }
        });
    }

    private static void resetOptionsMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.resetOptionsMenu();
            }
        });
    }

    private static void openOptionsMenu()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activity != null)
                    m_activity.openOptionsMenu();
            }
        });
    }

    private static byte[][] getSSLCertificates()
    {
        ArrayList<byte[]> certificateList = new ArrayList<byte[]>();

        try {
            TrustManagerFactory factory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            factory.init((KeyStore) null);

            for (TrustManager manager : factory.getTrustManagers()) {
                if (manager instanceof X509TrustManager) {
                    X509TrustManager trustManager = (X509TrustManager) manager;

                    for (X509Certificate certificate : trustManager.getAcceptedIssuers()) {
                        byte buffer[] = certificate.getEncoded();
                        certificateList.add(buffer);
                    }
                }
            }
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to get certificates", e);
        }

        byte[][] certificateArray = new byte[certificateList.size()][];
        certificateArray = certificateList.toArray(certificateArray);
        return certificateArray;
    }

    private static void createSurface(final int id, final boolean onTop, final int x, final int y, final int w, final int h, final int imageDepth)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.createSurface(id, onTop, x, y, w, h, imageDepth);
            }
        });
    }

    private static void insertNativeView(final int id, final View view, final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.insertNativeView(id, view, x, y, w, h);
            }
        });
    }

    private static void setSurfaceGeometry(final int id, final int x, final int y, final int w, final int h)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.setSurfaceGeometry(id, x, y, w, h);
            }
        });
    }

    private static void bringChildToFront(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.bringChildToFront(id);
            }
        });
    }

    private static void bringChildToBack(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.bringChildToBack(id);
            }
        });
    }

    private static void destroySurface(final int id)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.destroySurface(id);
            }
        });
    }

    private static void initializeAccessibility()
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.initializeAccessibility();
            }
        });
    }

    private static void hideSplashScreen(final int duration)
    {
        runAction(new Runnable() {
            @Override
            public void run() {
                if (m_activityDelegate != null)
                    m_activityDelegate.hideSplashScreen(duration);
            }
        });
    }

    private static String[] listAssetContent(android.content.res.AssetManager asset, String path) {
        String [] list;
        ArrayList<String> res = new ArrayList<String>();
        try {
            list = asset.list(path);
            if (list.length > 0) {
                for (String file : list) {
                    try {
                        String[] isDir = asset.list(path.length() > 0 ? path + "/" + file : file);
                        if (isDir != null && isDir.length > 0)
                            file += "/";
                        res.add(file);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return res.toArray(new String[res.size()]);
    }

    // screen methods
    public static native void setDisplayMetrics(int screenWidthPixels, int screenHeightPixels,
                                                int availableLeftPixels, int availableTopPixels,
                                                int availableWidthPixels, int availableHeightPixels,
                                                double XDpi, double YDpi, double scaledDensity,
                                                double density, float refreshRate);
    public static native void handleOrientationChanged(int newRotation, int nativeOrientation);
    public static native void handleRefreshRateChanged(float refreshRate);
    public static native void handleScreenAdded(int displayId);
    public static native void handleScreenChanged(int displayId);
    public static native void handleScreenRemoved(int displayId);
    // screen methods
    public static native void handleUiDarkModeChanged(int newUiMode);

    // surface methods
    public static native void setSurface(int id, Object surface, int w, int h);
    // surface methods

    // window methods
    public static native void updateWindow();
    // window methods

    // application methods
    public static native void updateApplicationState(int state);

    // menu methods
    public static native boolean onPrepareOptionsMenu(Menu menu);
    public static native boolean onOptionsItemSelected(int itemId, boolean checked);
    public static native void onOptionsMenuClosed(Menu menu);

    public static native void onCreateContextMenu(ContextMenu menu);
    public static native void fillContextMenu(Menu menu);
    public static native boolean onContextItemSelected(int itemId, boolean checked);
    public static native void onContextMenuClosed(Menu menu);
    // menu methods

    // activity methods
    public static native void onActivityResult(int requestCode, int resultCode, Intent data);
    public static native void onNewIntent(Intent data);

    public static native void runPendingCppRunnables();

    public static native void sendRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults);
    // activity methods

    // service methods
    public static native IBinder onBind(Intent intent);
    // service methods
}
