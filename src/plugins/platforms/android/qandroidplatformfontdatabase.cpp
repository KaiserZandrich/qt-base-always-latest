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

#include <QDir>

#include "qandroidplatformfontdatabase.h"

QT_BEGIN_NAMESPACE

QString QAndroidPlatformFontDatabase::fontDir() const
{
    return QLatin1String("/system/fonts");
}

void QAndroidPlatformFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();
    QDir dir(fontpath);

    if (Q_UNLIKELY(!dir.exists())) {
        qFatal("QFontDatabase: Cannot find font directory %s - is Qt installed correctly?",
               qPrintable(fontpath));
    }

    QStringList nameFilters;
    nameFilters << QLatin1String("*.ttf")
                << QLatin1String("*.otf")
                << QLatin1String("*.ttc");

    const auto entries = dir.entryInfoList(nameFilters, QDir::Files);
    for (const QFileInfo &fi : entries) {
        const QByteArray file = QFile::encodeName(fi.absoluteFilePath());
        QFreeTypeFontDatabase::addTTFile(QByteArray(), file);
    }
}

QStringList QAndroidPlatformFontDatabase::fallbacksForFamily(const QString &family,
                                                             QFont::Style style,
                                                             QFont::StyleHint styleHint,
                                                             QChar::Script script) const
{
    QStringList result;
    if (styleHint == QFont::Monospace || styleHint == QFont::Courier)
        result.append(QString(qgetenv("QT_ANDROID_FONTS_MONOSPACE")).split(QLatin1Char(';')));
    else if (styleHint == QFont::Serif)
        result.append(QString(qgetenv("QT_ANDROID_FONTS_SERIF")).split(QLatin1Char(';')));
    else
        result.append(QString(qgetenv("QT_ANDROID_FONTS")).split(QLatin1Char(';')));
    result.append(QFreeTypeFontDatabase::fallbacksForFamily(family, style, styleHint, script));

    return result;
}

QT_END_NAMESPACE
