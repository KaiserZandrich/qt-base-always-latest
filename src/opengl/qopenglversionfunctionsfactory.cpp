/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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
**
** This file was generated by glgen version 0.1
** Command line was: glgen
**
** glgen is Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
**
** This is an auto-generated file.
** Do not edit! All changes made to it will be lost.
**
****************************************************************************/

#include "qopenglversionfunctionsfactory.h"
#include "qopenglversionfunctions_p.h"

#if !QT_CONFIG(opengles2)
#include "qopenglfunctions_4_5_core.h"
#include "qopenglfunctions_4_5_compatibility.h"
#include "qopenglfunctions_4_4_core.h"
#include "qopenglfunctions_4_4_compatibility.h"
#include "qopenglfunctions_4_3_core.h"
#include "qopenglfunctions_4_3_compatibility.h"
#include "qopenglfunctions_4_2_core.h"
#include "qopenglfunctions_4_2_compatibility.h"
#include "qopenglfunctions_4_1_core.h"
#include "qopenglfunctions_4_1_compatibility.h"
#include "qopenglfunctions_4_0_core.h"
#include "qopenglfunctions_4_0_compatibility.h"
#include "qopenglfunctions_3_3_core.h"
#include "qopenglfunctions_3_3_compatibility.h"
#include "qopenglfunctions_3_2_core.h"
#include "qopenglfunctions_3_2_compatibility.h"
#include "qopenglfunctions_3_1.h"
#include "qopenglfunctions_3_0.h"
#include "qopenglfunctions_2_1.h"
#include "qopenglfunctions_2_0.h"
#include "qopenglfunctions_1_5.h"
#include "qopenglfunctions_1_4.h"
#include "qopenglfunctions_1_3.h"
#include "qopenglfunctions_1_2.h"
#include "qopenglfunctions_1_1.h"
#include "qopenglfunctions_1_0.h"
#else
#include "qopenglfunctions_es2.h"
#endif

QT_BEGIN_NAMESPACE

static QAbstractOpenGLFunctions *createFunctions(const QOpenGLVersionProfile &versionProfile)
{
#if !QT_CONFIG(opengles2)
    const int major = versionProfile.version().first;
    const int minor = versionProfile.version().second;

    if (versionProfile.hasProfiles()) {
        switch (versionProfile.profile()) {
        case QSurfaceFormat::CoreProfile:
            if (major == 4 && minor == 5)
                return new QOpenGLFunctions_4_5_Core;
            else if (major == 4 && minor == 4)
                return new QOpenGLFunctions_4_4_Core;
            else if (major == 4 && minor == 3)
                return new QOpenGLFunctions_4_3_Core;
            else if (major == 4 && minor == 2)
                return new QOpenGLFunctions_4_2_Core;
            else if (major == 4 && minor == 1)
                return new QOpenGLFunctions_4_1_Core;
            else if (major == 4 && minor == 0)
                return new QOpenGLFunctions_4_0_Core;
            else if (major == 3 && minor == 3)
                return new QOpenGLFunctions_3_3_Core;
            else if (major == 3 && minor == 2)
                return new QOpenGLFunctions_3_2_Core;
            break;

        case QSurfaceFormat::CompatibilityProfile:
            if (major == 4 && minor == 5)
                return new QOpenGLFunctions_4_5_Compatibility;
            else if (major == 4 && minor == 4)
                return new QOpenGLFunctions_4_4_Compatibility;
            else if (major == 4 && minor == 3)
                return new QOpenGLFunctions_4_3_Compatibility;
            else if (major == 4 && minor == 2)
                return new QOpenGLFunctions_4_2_Compatibility;
            else if (major == 4 && minor == 1)
                return new QOpenGLFunctions_4_1_Compatibility;
            else if (major == 4 && minor == 0)
                return new QOpenGLFunctions_4_0_Compatibility;
            else if (major == 3 && minor == 3)
                return new QOpenGLFunctions_3_3_Compatibility;
            else if (major == 3 && minor == 2)
                return new QOpenGLFunctions_3_2_Compatibility;
            break;

        case QSurfaceFormat::NoProfile:
        default:
            break;
        };
    } else {
        if (major == 3 && minor == 1)
            return new QOpenGLFunctions_3_1;
        else if (major == 3 && minor == 0)
            return new QOpenGLFunctions_3_0;
        else if (major == 2 && minor == 1)
            return new QOpenGLFunctions_2_1;
        else if (major == 2 && minor == 0)
            return new QOpenGLFunctions_2_0;
        else if (major == 1 && minor == 5)
            return new QOpenGLFunctions_1_5;
        else if (major == 1 && minor == 4)
            return new QOpenGLFunctions_1_4;
        else if (major == 1 && minor == 3)
            return new QOpenGLFunctions_1_3;
        else if (major == 1 && minor == 2)
            return new QOpenGLFunctions_1_2;
        else if (major == 1 && minor == 1)
            return new QOpenGLFunctions_1_1;
        else if (major == 1 && minor == 0)
            return new QOpenGLFunctions_1_0;
    }
    return nullptr;
#else
    Q_UNUSED(versionProfile);
    return new QOpenGLFunctions_ES2;
#endif
}

/*!
    \class QOpenGLVersionFunctionsFactory
    \inmodule QtOpenGL
    \since 6.0
    \brief Provides access to OpenGL functions for a specified version and
           profile.
*/

/*!
    \fn static T *QOpenGLVersionFunctionsFactory::get(QOpenGLContext *context)

    \overload get()

    Returns a pointer to an object that provides access to all functions for
    the version and profile of this context. There is no need to call
    QAbstractOpenGLFunctions::initializeOpenGLFunctions() as long as the \a context
    is current. It is also possible to call this function when the \a context is not
    current, but in that case it is the caller's responsibility to ensure proper
    initialization by calling QAbstractOpenGLFunctions::initializeOpenGLFunctions()
    afterwards.

    Usually one would use the template version of this function to automatically
    have the result cast to the correct type.

    \code
        auto funcs = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(context);
        if (!funcs) {
            qFatal("Could not obtain required OpenGL context version");
        }
    \endcode

    It is possible to request a functions object for a different version and profile
    than that for which the context was created. To do this either use the template
    version of this function specifying the desired functions object type as the
    template parameter or by passing in a QOpenGLVersionProfile object as an argument
    to the non-template function.

    Note that requests for function objects of other versions or profiles can fail and
    in doing so will return \nullptr. Situations in which creation of the functions
    object can fail are if the request cannot be satisfied due to asking for functions
    that are not in the version or profile of this context. For example:

    \list
        \li Requesting a 3.3 core profile functions object would succeed.
        \li Requesting a 3.3 compatibility profile functions object would fail. We would fail
            to resolve the deprecated functions.
        \li Requesting a 4.3 core profile functions object would fail. We would fail to resolve
            the new core functions introduced in versions 4.0-4.3.
        \li Requesting a 3.1 functions object would succeed. There is nothing in 3.1 that is not
            also in 3.3 core.
    \endlist

    Note that if creating a functions object via this method that the QOpenGLContext
    retains ownership of the object. This is to allow the object to be cached and shared.
*/

/*!
    Returns a pointer to an object that provides access to all functions for the
    \a versionProfile of the \a context. There is no need to call
    QAbstractOpenGLFunctions::initializeOpenGLFunctions() as long as the \a context
    is current. It is also possible to call this function when the \a context is not
    current, but in that case it is the caller's responsibility to ensure proper
    initialization by calling QAbstractOpenGLFunctions::initializeOpenGLFunctions()
    afterwards.

    Usually one would use the template version of this function to automatically
    have the result cast to the correct type.
*/
QAbstractOpenGLFunctions *QOpenGLVersionFunctionsFactory::get(const QOpenGLVersionProfile &versionProfile, QOpenGLContext *context)
{
    if (!context)
        context = QOpenGLContext::currentContext();

    if (!context) {
        qWarning("versionFunctions: No OpenGL context");
        return nullptr;
    }

#if !QT_CONFIG(opengles2)
    if (context->isOpenGLES()) {
        qWarning("versionFunctions: Not supported on OpenGL ES");
        return nullptr;
    }
#endif // !QT_CONFIG(opengles2)

    const QSurfaceFormat f = context->format();

    // Ensure we have a valid version and profile. Default to context's if none specified
    QOpenGLVersionProfile vp = versionProfile;
    if (!vp.isValid())
        vp = QOpenGLVersionProfile(f);

    // Check that context is compatible with requested version
    const QPair<int, int> v = qMakePair(f.majorVersion(), f.minorVersion());
    if (v < vp.version())
        return nullptr;

    // If this context only offers core profile functions then we can't create
    // function objects for legacy or compatibility profile requests
    if (((vp.hasProfiles() && vp.profile() != QSurfaceFormat::CoreProfile) || vp.isLegacyVersion())
        && f.profile() == QSurfaceFormat::CoreProfile)
        return nullptr;

    // Create object if suitable one not cached
    QAbstractOpenGLFunctions* funcs = nullptr;
    // TODO: replace with something else
    auto *data = QOpenGLContextVersionData::forContext(context);
    auto it = data->functions.constFind(vp);
    if (it == data->functions.constEnd()) {
        funcs = createFunctions(vp);
        if (funcs) {
            funcs->setOwningContext(context);
            data->functions.insert(vp, funcs);
        }
    } else {
        funcs = it.value();
    }

    if (funcs && QOpenGLContext::currentContext() == context)
        funcs->initializeOpenGLFunctions();

    return funcs;
}

QT_END_NAMESPACE
