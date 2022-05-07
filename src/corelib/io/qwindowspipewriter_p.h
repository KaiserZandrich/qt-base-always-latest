/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2021 Alex Trotsenko <alex1973tr@gmail.com>
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

#ifndef QWINDOWSPIPEWRITER_P_H
#define QWINDOWSPIPEWRITER_P_H

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

#include <qobject.h>
#include <qdeadlinetimer.h>
#include <qmutex.h>
#include <private/qringbuffer_p.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QWindowsPipeWriter : public QObject
{
    Q_OBJECT
public:
    explicit QWindowsPipeWriter(HANDLE pipeWriteEnd, QObject *parent = nullptr);
    ~QWindowsPipeWriter();

    bool write(const QByteArray &ba);
    void stop();
    bool waitForWrite(int msecs);
    bool checkForWrite() { return consumePendingAndEmit(false); }
    bool isWriteOperationActive() const;
    qint64 bytesToWrite() const;
    HANDLE syncEvent() const { return syncHandle; }

Q_SIGNALS:
    void bytesWritten(qint64 bytes);

protected:
    bool event(QEvent *e) override;

private:
    void startAsyncWriteLocked();
    static void CALLBACK waitCallback(PTP_CALLBACK_INSTANCE instance, PVOID context,
                                      PTP_WAIT wait, TP_WAIT_RESULT waitResult);
    bool writeCompleted(DWORD errorCode, DWORD numberOfBytesWritten);
    bool waitForNotification(const QDeadlineTimer &deadline);
    bool consumePendingAndEmit(bool allowWinActPosting);

    HANDLE handle;
    HANDLE eventHandle;
    HANDLE syncHandle;
    PTP_WAIT waitObject;
    OVERLAPPED overlapped;
    QRingBuffer writeBuffer;
    qint64 pendingBytesWrittenValue;
    mutable QMutex mutex;
    DWORD lastError;
    bool stopped;
    bool writeSequenceStarted;
    bool bytesWrittenPending;
    bool winEventActPosted;
};

QT_END_NAMESPACE

#endif // QWINDOWSPIPEWRITER_P_H
