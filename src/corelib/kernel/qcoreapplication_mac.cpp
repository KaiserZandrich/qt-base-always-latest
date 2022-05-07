/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
****************************************************************************/

#include "qcoreapplication.h"
#include "private/qcoreapplication_p.h"
#include <private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  QCoreApplication utility functions
 *****************************************************************************/
QString qAppFileName()
{
    static QString appFileName;
    if (appFileName.isEmpty()) {
        QCFType<CFURLRef> bundleURL(CFBundleCopyExecutableURL(CFBundleGetMainBundle()));
        if (bundleURL) {
            QCFString cfPath(CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle));
            if (cfPath)
                appFileName = cfPath;
        }
    }
    return appFileName;
}

QT_END_NAMESPACE
