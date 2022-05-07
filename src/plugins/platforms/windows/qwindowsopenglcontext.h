/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QWINDOWSOPENGLCONTEXT_H
#define QWINDOWSOPENGLCONTEXT_H

#include <QtGui/qopenglcontext.h>
#include <qpa/qplatformopenglcontext.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_OPENGL

class QWindowsOpenGLContext;

class QWindowsStaticOpenGLContext
{
    Q_DISABLE_COPY_MOVE(QWindowsStaticOpenGLContext)
public:
    static QWindowsStaticOpenGLContext *create();
    virtual ~QWindowsStaticOpenGLContext() = default;

    virtual QWindowsOpenGLContext *createContext(QOpenGLContext *context) = 0;
    virtual QWindowsOpenGLContext *createContext(HGLRC context, HWND window) = 0;
    virtual void *moduleHandle() const = 0;
    virtual QOpenGLContext::OpenGLModuleType moduleType() const = 0;
    virtual bool supportsThreadedOpenGL() const { return false; }

    // If the windowing system interface needs explicitly created window surfaces (like EGL),
    // reimplement these.
    virtual void *createWindowSurface(void * /*nativeWindow*/, void * /*nativeConfig*/, int * /*err*/) { return nullptr; }
    virtual void destroyWindowSurface(void * /*nativeSurface*/) { }

protected:
    QWindowsStaticOpenGLContext() = default;

private:
    static QWindowsStaticOpenGLContext *doCreate();
};

class QWindowsOpenGLContext : public QPlatformOpenGLContext
{
    Q_DISABLE_COPY_MOVE(QWindowsOpenGLContext)
public:
    // These should be implemented only for some winsys interfaces, for example EGL.
    // For others, like WGL, they are not relevant.
    virtual void *nativeDisplay() const { return nullptr; }
    virtual void *nativeConfig() const { return 0; }

protected:
    QWindowsOpenGLContext() = default;
};

#endif // QT_NO_OPENGL

QT_END_NAMESPACE

#endif // QWINDOWSOPENGLCONTEXT_H
