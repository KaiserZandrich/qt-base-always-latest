/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QANDROIDPLATFORMINTERATION_H
#define QANDROIDPLATFORMINTERATION_H

#include "qandroidinputcontext.h"
#include "qandroidplatformscreen.h"

#include <QtGui/qtguiglobal.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformmenu.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformoffscreensurface.h>

#include <EGL/egl.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QAndroidPlatformServices;
class QAndroidSystemLocale;
class QPlatformAccessibility;

struct AndroidStyle;
class QAndroidPlatformNativeInterface: public QPlatformNativeInterface
{
public:
    void *nativeResourceForIntegration(const QByteArray &resource) override;
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
    std::shared_ptr<AndroidStyle> m_androidStyle;

protected:
    void customEvent(QEvent *event) override;
};

class QAndroidPlatformIntegration : public QPlatformIntegration
                                  , QNativeInterface::Private::QEGLIntegration
                                  , QNativeInterface::Private::QAndroidOffScreenIntegration
{
    friend class QAndroidPlatformScreen;

public:
    QAndroidPlatformIntegration(const QStringList &paramList);
    ~QAndroidPlatformIntegration();

    void initialize() override;

    bool hasCapability(QPlatformIntegration::Capability cap) const override;

    QPlatformWindow *createPlatformWindow(QWindow *window) const override;
    QPlatformWindow *createForeignWindow(QWindow *window, WId nativeHandle) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
    QOpenGLContext *createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const override;
    QAbstractEventDispatcher *createEventDispatcher() const override;
    QAndroidPlatformScreen *screen() { return m_primaryScreen; }
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const override;
    QOffscreenSurface *createOffscreenSurface(ANativeWindow *nativeSurface) const override;

    void setAvailableGeometry(const QRect &availableGeometry);
    void setPhysicalSize(int width, int height);
    void setScreenSize(int width, int height);
    // The 3 methods above were replaced by a new one, so that we could have
    // a better control over "geometry changed" event handling. Technically
    // they are no longer used and can be removed. Not doing it now, because
    // I'm not sure if it might be helpful to have them or not.
    void setScreenSizeParameters(const QSize &physicalSize, const QSize &screenSize,
                                 const QRect &availableGeometry);
    void setRefreshRate(qreal refreshRate);
    bool isVirtualDesktop() { return true; }

    QPlatformFontDatabase *fontDatabase() const override;

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *clipboard() const override;
#endif

    QPlatformInputContext *inputContext() const override;
    QPlatformNativeInterface *nativeInterface() const override;
    QPlatformServices *services() const override;

#ifndef QT_NO_ACCESSIBILITY
    virtual QPlatformAccessibility *accessibility() const override;
#endif

    QVariant styleHint(StyleHint hint) const override;
    Qt::WindowState defaultWindowState(Qt::WindowFlags flags) const override;

    QStringList themeNames() const override;
    QPlatformTheme *createPlatformTheme(const QString &name) const override;

    static void setDefaultDisplayMetrics(int availableLeft, int availableTop, int availableWidth,
                                         int availableHeight, int physicalWidth, int physicalHeight,
                                         int screenWidth, int screenHeight);
    static void setScreenOrientation(Qt::ScreenOrientation currentOrientation,
                                     Qt::ScreenOrientation nativeOrientation);

    QPointingDevice *touchDevice() const { return m_touchDevice; }
    void setTouchDevice(QPointingDevice *touchDevice) { m_touchDevice = touchDevice; }

    void flushPendingUpdates();

#if QT_CONFIG(vulkan)
    QPlatformVulkanInstance *createPlatformVulkanInstance(QVulkanInstance *instance) const override;
#endif

private:
    EGLDisplay m_eglDisplay;
    QPointingDevice *m_touchDevice;

    QAndroidPlatformScreen *m_primaryScreen;

    QThread *m_mainThread;

    static QRect m_defaultAvailableGeometry;
    static QSize m_defaultPhysicalSize;
    static QSize m_defaultScreenSize;

    static Qt::ScreenOrientation m_orientation;
    static Qt::ScreenOrientation m_nativeOrientation;
    static bool m_showPasswordEnabled;

    QPlatformFontDatabase *m_androidFDB;
    QAndroidPlatformNativeInterface *m_androidPlatformNativeInterface;
    QAndroidPlatformServices *m_androidPlatformServices;

#ifndef QT_NO_CLIPBOARD
    QPlatformClipboard *m_androidPlatformClipboard;
#endif

    QAndroidSystemLocale *m_androidSystemLocale;
#ifndef QT_NO_ACCESSIBILITY
    mutable QPlatformAccessibility *m_accessibility;
#endif

    QScopedPointer<QPlatformInputContext> m_inputContext;
};

QT_END_NAMESPACE

#endif
