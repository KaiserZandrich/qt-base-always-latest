/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QLoggingCategory>
#include "qxlibeglintegration_p.h"

Q_LOGGING_CATEGORY(lcXlibEglDebug, "qt.egl.xlib.debug")

VisualID QXlibEglIntegration::getCompatibleVisualId(Display *display, EGLDisplay eglDisplay, EGLConfig config)
{
    VisualID    visualId = 0;
    EGLint      eglValue = 0;

    EGLint major,minor;
    eglInitialize(eglDisplay,&major,&minor);

    EGLint configRedSize = 0;
    eglGetConfigAttrib(eglDisplay, config, EGL_RED_SIZE, &configRedSize);

    EGLint configGreenSize = 0;
    eglGetConfigAttrib(eglDisplay, config, EGL_GREEN_SIZE, &configGreenSize);

    EGLint configBlueSize = 0;
    eglGetConfigAttrib(eglDisplay, config, EGL_BLUE_SIZE, &configBlueSize);

    EGLint configAlphaSize = 0;
    eglGetConfigAttrib(eglDisplay, config, EGL_ALPHA_SIZE, &configAlphaSize);

    eglGetConfigAttrib(eglDisplay, config, EGL_CONFIG_ID, &eglValue);
    int configId = eglValue;

    // See if EGL provided a valid VisualID:
    eglGetConfigAttrib(eglDisplay, config, EGL_NATIVE_VISUAL_ID, &eglValue);
    visualId = (VisualID)eglValue;

    EGLint attributes[] = {EGL_RED_SIZE,configRedSize,EGL_GREEN_SIZE, configGreenSize, EGL_BLUE_SIZE, configBlueSize,
                         EGL_SAMPLES,configSamples,EGL_NONE};

    bool configFound(false);
    int configCount(0);
    // check if the current configuration is valid
    configFound = eglChooseConfig(eglDisplay, attributes,&config,1,&configCount);
    if(configFound)
    {
        return visualId; // Most ideal path
    }

    configFound = eglChooseConfig(eglDisplay, attributes,NULL,0,&configCount); // Else get all EGL configurations
    // Allocate the EGLConfig structure array and fill it
    EGLConfig * matchingConfigs(new EGLConfig[configCount]);
    configFound = eglChooseConfig(eglDisplay, attributes,matchingConfigs,configCount,&configCount);

    // Manually search the configurations while looking for a valid match
    EGLint  idFound(0);
    EGLBoolean no_error(false);
    EGLint red(0),green(0),blue(0);
    for(int index(0); index < (configCount && !no_error); index++)
    {
        no_error = eglGetConfigAttrib(eglDisplay,matchingConfigs[index],EGL_RED_SIZE, &red);
        no_error &= eglGetConfigAttrib(eglDisplay,matchingConfigs[index],EGL_BLUE_SIZE, &blue);
        no_error &= eglGetConfigAttrib(eglDisplay,matchingConfigs[index],EGL_GREEN_SIZE, &green);
        if((no_error == EGL_TRUE) && (red == configRedSize) && (green == configGreenSize) && (blue == configBlueSize))
        {
            eglGetConfigAttrib(eglDisplay,matchingConfigs[index],EGL_NATIVE_VISUAL_ID, &idFound);
        }
        else
            no_error = false;
    }
    delete []matchingConfigs;

    if(idFound != 0)
    {
        return idFound; // Return the value found
    }

    if (visualId) {
        // EGL has suggested a visual id, so get the rest of the visual info for that id:
        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        visualInfoTemplate.visualid = visualId;

        XVisualInfo *chosenVisualInfo;
        int matchingCount = 0;
        chosenVisualInfo = XGetVisualInfo(display, VisualIDMask, &visualInfoTemplate, &matchingCount);
        if (chosenVisualInfo) {
            // Skip size checks if implementation supports non-matching visual
            // and config (QTBUG-9444).
            if (q_hasEglExtension(eglDisplay,"EGL_NV_post_convert_rounding")) {
                XFree(chosenVisualInfo);
                return visualId;
            }
            // Skip also for i.MX6 where 565 visuals are suggested for the default 444 configs and it works just fine.
            const char *vendor = eglQueryString(eglDisplay, EGL_VENDOR);
            if (vendor && strstr(vendor, "Vivante")) {
                XFree(chosenVisualInfo);
                return visualId;
            }

            int visualRedSize = qPopulationCount(chosenVisualInfo->red_mask);
            int visualGreenSize = qPopulationCount(chosenVisualInfo->green_mask);
            int visualBlueSize = qPopulationCount(chosenVisualInfo->blue_mask);
            int visualAlphaSize = chosenVisualInfo->depth == 32 ? 8 : 0;

            const bool visualMatchesConfig = visualRedSize == configRedSize
                && visualGreenSize == configGreenSize
                && visualBlueSize == configBlueSize
                && visualAlphaSize == configAlphaSize;

            // In some cases EGL tends to suggest a 24-bit visual for 8888
            // configs. In such a case we have to fall back to XGetVisualInfo.
            if (!visualMatchesConfig) {
                visualId = 0;
                qCDebug(lcXlibEglDebug,
                        "EGL suggested using X Visual ID %d (%d %d %d depth %d) for EGL config %d"
                        "(%d %d %d %d), but this is incompatible",
                        (int)visualId, visualRedSize, visualGreenSize, visualBlueSize, chosenVisualInfo->depth,
                        configId, configRedSize, configGreenSize, configBlueSize, configAlphaSize);
            }
        } else {
            qCDebug(lcXlibEglDebug, "EGL suggested using X Visual ID %d for EGL config %d, but that isn't a valid ID",
                    (int)visualId, configId);
            visualId = 0;
        }
        XFree(chosenVisualInfo);
    }
    else
        qCDebug(lcXlibEglDebug, "EGL did not suggest a VisualID (EGL_NATIVE_VISUAL_ID was zero) for EGLConfig %d", configId);

    if (visualId) {
        qCDebug(lcXlibEglDebug, configAlphaSize > 0
                ? "Using ARGB Visual ID %d provided by EGL for config %d"
                : "Using Opaque Visual ID %d provided by EGL for config %d", (int)visualId, configId);
        return visualId;
    }

    // Finally, try to use XGetVisualInfo and only use the bit depths to match on:
    if (!visualId) {
        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        XVisualInfo *matchingVisuals;
        int matchingCount = 0;

        visualInfoTemplate.depth = configRedSize + configGreenSize + configBlueSize + configAlphaSize;
        matchingVisuals = XGetVisualInfo(display,
                                         VisualDepthMask,
                                         &visualInfoTemplate,
                                         &matchingCount);
        if (!matchingVisuals) {
            // Try again without taking the alpha channel into account:
            visualInfoTemplate.depth = configRedSize + configGreenSize + configBlueSize;
            matchingVisuals = XGetVisualInfo(display,
                                             VisualDepthMask,
                                             &visualInfoTemplate,
                                             &matchingCount);
        }

        if (matchingVisuals) {
            visualId = matchingVisuals[0].visualid;
            XFree(matchingVisuals);
        }
    }

    if (visualId) {
        qCDebug(lcXlibEglDebug, "Using Visual ID %d provided by XGetVisualInfo for EGL config %d", (int)visualId, configId);
        return visualId;
    }

    qWarning("Unable to find an X11 visual which matches EGL config %d", configId);
    return (VisualID)0;
}
