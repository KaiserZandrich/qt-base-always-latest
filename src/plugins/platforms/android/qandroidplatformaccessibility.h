/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#ifndef QANDROIDPLATFORMACCESSIBILITY_H
#define QANDROIDPLATFORMACCESSIBILITY_H

#include <qpa/qplatformaccessibility.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformAccessibility: public QPlatformAccessibility
{
public:
    QAndroidPlatformAccessibility();
    ~QAndroidPlatformAccessibility();

    void notifyAccessibilityUpdate(QAccessibleEvent *event) override;
    void setRootObject(QObject *obj) override;
};

QT_END_NAMESPACE

#endif
