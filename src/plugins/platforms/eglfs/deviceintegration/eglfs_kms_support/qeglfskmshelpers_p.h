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

#ifndef QEGLFSKMSHELPERS_H
#define QEGLFSKMSHELPERS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QString>

QT_BEGIN_NAMESPACE

inline QString q_fourccToString(uint code)
{
    QString s;
    s.reserve(4);
    s.append(code & 0xff);
    s.append((code >> 8) & 0xff);
    s.append((code >> 16) & 0xff);
    s.append((code >> 24) & 0xff);
    return s;
}

QT_END_NAMESPACE

#endif // QEGLFSKMSHELPERS_H
