/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QMetaEnum>
#include <QRandomGenerator>
#include <QString>
#include <QTest>

#include <functional>

#include <time.h>

class tst_bench_QCryptographicHash : public QObject
{
    Q_OBJECT
    QByteArray blockOfData;

    using Algorithm = QCryptographicHash::Algorithm;

public:
    tst_bench_QCryptographicHash();

private Q_SLOTS:
    void hash_data();
    void hash();
    void addData_data() { hash_data(); }
    void addData();
    void addDataChunked_data() { hash_data(); }
    void addDataChunked();
};

const int MaxBlockSize = 65536;

static void for_each_algorithm(std::function<void(QCryptographicHash::Algorithm, const char*)> f)
{
    Q_ASSERT(f);
    using A = QCryptographicHash::Algorithm;
    static const auto metaEnum = QMetaEnum::fromType<A>();
    for (int i = 0, value = metaEnum.value(i); value != -1; value = metaEnum.value(++i))
        f(A(value), metaEnum.key(i));
}

tst_bench_QCryptographicHash::tst_bench_QCryptographicHash()
    : blockOfData(MaxBlockSize, Qt::Uninitialized)
{
#ifdef Q_OS_UNIX
    QFile urandom("/dev/urandom");
    if (urandom.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        QCOMPARE(urandom.read(blockOfData.data(), blockOfData.size()), qint64(MaxBlockSize));
    } else
#endif
    {
        for (int i = 0; i < MaxBlockSize; ++i)
            blockOfData[i] = QRandomGenerator::global()->generate();
    }
}

void tst_bench_QCryptographicHash::hash_data()
{
    QTest::addColumn<Algorithm>("algo");
    QTest::addColumn<QByteArray>("data");

    static const int datasizes[] = { 0, 1, 64, 65, 512, 4095, 4096, 4097, 65536 };
    for (uint i = 0; i < sizeof(datasizes)/sizeof(datasizes[0]); ++i) {
        Q_ASSERT(datasizes[i] < MaxBlockSize);
        QByteArray data = QByteArray::fromRawData(blockOfData.constData(), datasizes[i]);

        for_each_algorithm([&] (Algorithm algo, const char *name) {
            QTest::addRow("%s-%d", name, datasizes[i]) << algo << data;
        });
    }
}

void tst_bench_QCryptographicHash::hash()
{
    QFETCH(const Algorithm, algo);
    QFETCH(QByteArray, data);

    QBENCHMARK {
        QCryptographicHash::hash(data, algo);
    }
}

void tst_bench_QCryptographicHash::addData()
{
    QFETCH(const Algorithm, algo);
    QFETCH(QByteArray, data);

    QCryptographicHash hash(algo);
    QBENCHMARK {
        hash.reset();
        hash.addData(data);
        hash.result();
    }
}

void tst_bench_QCryptographicHash::addDataChunked()
{
    QFETCH(const Algorithm, algo);
    QFETCH(QByteArray, data);

    QCryptographicHash hash(algo);
    QBENCHMARK {
        hash.reset();

        // add the data in chunks of 64 bytes
        for (int i = 0; i < data.size() / 64; ++i)
            hash.addData(data.constData() + 64 * i, 64);
        hash.addData(data.constData() + data.size() / 64 * 64, data.size() % 64);

        hash.result();
    }
}

QTEST_APPLESS_MAIN(tst_bench_QCryptographicHash)

#include "main.moc"
