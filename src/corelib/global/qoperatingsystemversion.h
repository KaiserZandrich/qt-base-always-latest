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
******************************************************************************/

#include <QtCore/qglobal.h>
#include <QtCore/qversionnumber.h>

#ifndef QOPERATINGSYSTEMVERSION_H
#define QOPERATINGSYSTEMVERSION_H

QT_BEGIN_NAMESPACE

class QString;

class Q_CORE_EXPORT QOperatingSystemVersion
{
public:
    enum OSType {
        Unknown = 0,
        Windows,
        MacOS,
        IOS,
        TvOS,
        WatchOS,
        Android
    };

    static const QOperatingSystemVersion Windows7;
    static const QOperatingSystemVersion Windows8;
    static const QOperatingSystemVersion Windows8_1;
    static const QOperatingSystemVersion Windows10;

    static const QOperatingSystemVersion OSXMavericks;
    static const QOperatingSystemVersion OSXYosemite;
    static const QOperatingSystemVersion OSXElCapitan;
    static const QOperatingSystemVersion MacOSSierra;
    static const QOperatingSystemVersion MacOSHighSierra;
    static const QOperatingSystemVersion MacOSMojave;
    static const QOperatingSystemVersion MacOSCatalina;
    static const QOperatingSystemVersion MacOSBigSur;

    static const QOperatingSystemVersion AndroidJellyBean;
    static const QOperatingSystemVersion AndroidJellyBean_MR1;
    static const QOperatingSystemVersion AndroidJellyBean_MR2;
    static const QOperatingSystemVersion AndroidKitKat;
    static const QOperatingSystemVersion AndroidLollipop;
    static const QOperatingSystemVersion AndroidLollipop_MR1;
    static const QOperatingSystemVersion AndroidMarshmallow;
    static const QOperatingSystemVersion AndroidNougat;
    static const QOperatingSystemVersion AndroidNougat_MR1;
    static const QOperatingSystemVersion AndroidOreo;
    static const QOperatingSystemVersion AndroidOreo_MR1;
    static const QOperatingSystemVersion AndroidPie;
    static const QOperatingSystemVersion Android10;
    static const QOperatingSystemVersion Android11;

    constexpr QOperatingSystemVersion(OSType osType,
                                             int vmajor, int vminor = -1, int vmicro = -1)
        : m_os(osType),
          m_major(qMax(-1, vmajor)),
          m_minor(vmajor < 0 ? -1 : qMax(-1, vminor)),
          m_micro(vmajor < 0 || vminor < 0 ? -1 : qMax(-1, vmicro))
    { }

    static QOperatingSystemVersion current();

    static constexpr OSType currentType()
    {
#if defined(Q_OS_WIN)
        return Windows;
#elif defined(Q_OS_MACOS)
        return MacOS;
#elif defined(Q_OS_IOS)
        return IOS;
#elif defined(Q_OS_TVOS)
        return TvOS;
#elif defined(Q_OS_WATCHOS)
        return WatchOS;
#elif defined(Q_OS_ANDROID)
        return Android;
#else
        return Unknown;
#endif
    }

    QVersionNumber version() const { return QVersionNumber(m_major, m_minor, m_micro); }

    constexpr int majorVersion() const { return m_major; }
    constexpr int minorVersion() const { return m_minor; }
    constexpr int microVersion() const { return m_micro; }

    constexpr int segmentCount() const
    { return m_micro >= 0 ? 3 : m_minor >= 0 ? 2 : m_major >= 0 ? 1 : 0; }

    bool isAnyOfType(std::initializer_list<OSType> types) const;
    constexpr OSType type() const { return m_os; }
    QString name() const;

    friend bool operator>(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
    { return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) > 0; }

    friend bool operator>=(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
    { return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) >= 0; }

    friend bool operator<(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
    { return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) < 0; }

    friend bool operator<=(const QOperatingSystemVersion &lhs, const QOperatingSystemVersion &rhs)
    { return lhs.type() == rhs.type() && QOperatingSystemVersion::compare(lhs, rhs) <= 0; }

private:
    QOperatingSystemVersion() = default;
    OSType m_os;
    int m_major;
    int m_minor;
    int m_micro;

    static int compare(const QOperatingSystemVersion &v1, const QOperatingSystemVersion &v2);
};
Q_DECLARE_TYPEINFO(QOperatingSystemVersion, Q_PRIMITIVE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_CORE_EXPORT QDebug operator<<(QDebug debug, const QOperatingSystemVersion &ov);
#endif

QT_END_NAMESPACE

#endif // QOPERATINGSYSTEMVERSION_H
