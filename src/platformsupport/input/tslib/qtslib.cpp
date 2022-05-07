/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#include "qtslib_p.h"

#include <QSocketNotifier>
#include <QStringList>
#include <QPoint>
#include <QLoggingCategory>

#include <qpa/qwindowsysteminterface.h>

#include <errno.h>
#include <tslib.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcTsLib, "qt.qpa.input")

QTsLibMouseHandler::QTsLibMouseHandler(const QString &key,
                                       const QString &specification,
                                       QObject *parent)
    : QObject(parent),
    m_rawMode(!key.compare(QLatin1String("TslibRaw"), Qt::CaseInsensitive))
{
    qCDebug(qLcTsLib) << "Initializing tslib plugin" << key << specification;
    setObjectName(QLatin1String("TSLib Mouse Handler"));

    m_dev = ts_setup(nullptr, 1);
    if (!m_dev) {
        qErrnoWarning(errno, "ts_setup() failed");
        return;
    }

#ifdef TSLIB_VERSION_EVENTPATH /* also introduced in 1.15 */
    qCDebug(qLcTsLib) << "tslib device is" << ts_get_eventpath(m_dev);
#endif
    m_notify = new QSocketNotifier(ts_fd(m_dev), QSocketNotifier::Read, this);
    connect(m_notify, &QSocketNotifier::activated, this, &QTsLibMouseHandler::readMouseData);
}

QTsLibMouseHandler::~QTsLibMouseHandler()
{
    if (m_dev)
        ts_close(m_dev);
}

static bool get_sample(struct tsdev *dev, struct ts_sample *sample, bool rawMode)
{
    if (rawMode)
        return (ts_read_raw(dev, sample, 1) == 1);
    else
        return (ts_read(dev, sample, 1) == 1);
}

void QTsLibMouseHandler::readMouseData()
{
    ts_sample sample;

    while (get_sample(m_dev, &sample, m_rawMode)) {
        bool pressed = sample.pressure;
        int x = sample.x;
        int y = sample.y;

        // coordinates on release events can contain arbitrary values, just ignore them
        if (sample.pressure == 0) {
            x = m_x;
            y = m_y;
        }

        if (!m_rawMode) {
            //filtering: ignore movements of 2 pixels or less
            int dx = x - m_x;
            int dy = y - m_y;
            if (dx*dx <= 4 && dy*dy <= 4 && pressed == m_pressed)
                continue;
        }
        QPoint pos(x, y);

        QWindowSystemInterface::handleMouseEvent(nullptr, pos, pos,
                                                 pressed ? Qt::LeftButton : Qt::NoButton,
                                                 Qt::NoButton, QEvent::None);

        m_x = x;
        m_y = y;
        m_pressed = pressed;
    }
}

QT_END_NAMESPACE

#include "moc_qtslib_p.cpp"
