/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#ifndef QURLQUERY_H
#define QURLQUERY_H

#include <QtCore/qpair.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qurl.h>

#include <initializer_list>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT size_t qHash(const QUrlQuery &key, size_t seed = 0) noexcept;

class QUrlQueryPrivate;
class Q_CORE_EXPORT QUrlQuery
{
public:
    QUrlQuery();
    explicit QUrlQuery(const QUrl &url);
    explicit QUrlQuery(const QString &queryString);
    QUrlQuery(std::initializer_list<QPair<QString, QString>> list)
        : QUrlQuery()
    {
        for (const QPair<QString, QString> &item : list)
            addQueryItem(item.first, item.second);
    }

    QUrlQuery(const QUrlQuery &other);
    QUrlQuery &operator=(const QUrlQuery &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QUrlQuery)
    ~QUrlQuery();

    bool operator==(const QUrlQuery &other) const;
    bool operator!=(const QUrlQuery &other) const
    { return !(*this == other); }

    void swap(QUrlQuery &other) noexcept { d.swap(other.d); }

    bool isEmpty() const;
    bool isDetached() const;
    void clear();

    QString query(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    void setQuery(const QString &queryString);
    QString toString(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const
    { return query(encoding); }

    void setQueryDelimiters(QChar valueDelimiter, QChar pairDelimiter);
    QChar queryValueDelimiter() const;
    QChar queryPairDelimiter() const;

    void setQueryItems(const QList<QPair<QString, QString> > &query);
    QList<QPair<QString, QString> > queryItems(QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;

    bool hasQueryItem(const QString &key) const;
    void addQueryItem(const QString &key, const QString &value);
    void removeQueryItem(const QString &key);
    QString queryItemValue(const QString &key, QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    QStringList allQueryItemValues(const QString &key, QUrl::ComponentFormattingOptions encoding = QUrl::PrettyDecoded) const;
    void removeAllQueryItems(const QString &key);

    static constexpr char16_t defaultQueryValueDelimiter() noexcept { return u'='; }
    static constexpr char16_t defaultQueryPairDelimiter() noexcept { return u'&'; }

private:
    friend class QUrl;
    friend Q_CORE_EXPORT size_t qHash(const QUrlQuery &key, size_t seed) noexcept;
    QSharedDataPointer<QUrlQueryPrivate> d;

public:
    typedef QSharedDataPointer<QUrlQueryPrivate> DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

Q_DECLARE_SHARED(QUrlQuery)

QT_END_NAMESPACE

#endif // QURLQUERY_H
