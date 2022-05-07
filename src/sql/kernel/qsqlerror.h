/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
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

#ifndef QSQLERROR_H
#define QSQLERROR_H

#include <QtSql/qtsqlglobal.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QSqlErrorPrivate;

class Q_SQL_EXPORT QSqlError
{
public:
    enum ErrorType {
        NoError,
        ConnectionError,
        StatementError,
        TransactionError,
        UnknownError
    };
    QSqlError(const QString &driverText = QString(),
              const QString &databaseText = QString(),
              ErrorType type = NoError,
              const QString &errorCode = QString());
    QSqlError(const QSqlError& other);
    QSqlError(QSqlError &&other) noexcept : d(other.d) { other.d = nullptr; }
    QSqlError& operator=(const QSqlError& other);
    QSqlError &operator=(QSqlError &&other) noexcept { swap(other); return *this; }

    bool operator==(const QSqlError& other) const;
    bool operator!=(const QSqlError& other) const;
    ~QSqlError();

    void swap(QSqlError &other) noexcept { qt_ptr_swap(d, other.d); }

    QString driverText() const;
    QString databaseText() const;
    ErrorType type() const;
    QString nativeErrorCode() const;
    QString text() const;
    bool isValid() const;

private:
    QSqlErrorPrivate *d = nullptr;
};

Q_DECLARE_SHARED(QSqlError)

#ifndef QT_NO_DEBUG_STREAM
Q_SQL_EXPORT QDebug operator<<(QDebug, const QSqlError &);
#endif

QT_END_NAMESPACE

#endif // QSQLERROR_H
