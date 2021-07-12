/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>

#include <QtCore/private/qduplicatetracker_p.h>

#include <QObject>
#include <utility>

class tst_QDuplicateTracker : public QObject
{
    Q_OBJECT
private slots:
    void hasSeen();
    void appendTo();
    void appendTo_special();
};

void tst_QDuplicateTracker::hasSeen()
{
    {
        QDuplicateTracker<int, 2> tracker;
        QVERIFY(!tracker.hasSeen(0));
        QVERIFY(tracker.hasSeen(0));
        QVERIFY(!tracker.hasSeen(1));
        QVERIFY(tracker.hasSeen(1));
        // past the prealloc amount
        QVERIFY(!tracker.hasSeen(2));
        QVERIFY(tracker.hasSeen(2));
    }

    {
        QDuplicateTracker<QString, 2> tracker;
        QString string1("string1");
        QString string2("string2");
        QString string2_2("string2");
        QString string3("string3");

        // Move when seen
        QVERIFY(!tracker.hasSeen(string1));
        QVERIFY(tracker.hasSeen(std::move(string1)));

        // Move when unseen
        QVERIFY(!tracker.hasSeen(std::move(string2)));
        QVERIFY(tracker.hasSeen(string2_2));

        // Past the prealloc amount
        QVERIFY(!tracker.hasSeen(string3));
        QVERIFY(tracker.hasSeen(string3));
    }
}

void tst_QDuplicateTracker::appendTo()
{
    QDuplicateTracker<int, 2> tracker;
    QVERIFY(!tracker.hasSeen(0));
    QVERIFY(!tracker.hasSeen(1));
    QList<int> a;
    a.append(-1);
    tracker.appendTo(a);
    std::sort(a.begin(), a.end());
    QCOMPARE(a, QList<int>({ -1, 0, 1 }));

    QList<int> b;
    tracker.appendTo(b);
    std::sort(b.begin(), b.end());
    QCOMPARE(b, QList<int>({ 0, 1 }));

    QVERIFY(!tracker.hasSeen(2));
    QList<int> c;
    std::move(tracker).appendTo(c);
    std::sort(c.begin(), c.end());
    QCOMPARE(c, QList<int>({ 0, 1, 2 }));
    if (QDuplicateTracker<int, 2>::uses_pmr) {
        // the following is only true if we use the std container
        QVERIFY(!tracker.hasSeen(0));
        QVERIFY(!tracker.hasSeen(1));
        QVERIFY(!tracker.hasSeen(2));
    }
}

struct ConstructionCounted
{
    ConstructionCounted(int i) : i(i) { }
    ConstructionCounted(ConstructionCounted &&other) noexcept
        : i(other.i), copies(other.copies), moves(other.moves + 1)
    {
        // set to some easily noticeable values
        other.i = -64;
        other.copies = -64;
        other.moves = -64;
    }
    ConstructionCounted &operator=(ConstructionCounted &&other) noexcept
    {
        ConstructionCounted moved = std::move(other);
        swap(moved);
        // set to some easily noticeable values
        other.i = -64;
        other.copies = -64;
        other.moves = -64;
        return *this;
    }
    ConstructionCounted(const ConstructionCounted &other) noexcept
        : i(other.i), copies(other.copies + 1), moves(other.moves)
    {
    }
    ConstructionCounted &operator=(const ConstructionCounted &other) noexcept
    {
        ConstructionCounted copy = other;
        swap(copy);
        return *this;
    }
    ~ConstructionCounted() = default;

    friend bool operator==(const ConstructionCounted &lhs, const ConstructionCounted &rhs)
    {
        return lhs.i == rhs.i;
    }

    QString toString() { return QString::number(i); }

    void swap(ConstructionCounted &other)
    {
        std::swap(copies, other.copies);
        std::swap(i, other.i);
        std::swap(moves, other.moves);
    }

    int i;
    int copies = 0;
    int moves = 0;
};

// for std::unordered_set
namespace std {
template<>
struct hash<ConstructionCounted>
{
    std::size_t operator()(const ConstructionCounted &c) const noexcept { return c.i; }
};
}

// for QSet
size_t qHash(const ConstructionCounted &c, std::size_t seed = 0)
{
    return qHash(c.i, seed);
}

void tst_QDuplicateTracker::appendTo_special()
{
    QDuplicateTracker<ConstructionCounted> tracker;
    tracker.reserve(3);
    QVERIFY(!tracker.hasSeen(1));
    QVERIFY(!tracker.hasSeen(2));
    QVERIFY(!tracker.hasSeen(3));

    QVERIFY(tracker.hasSeen(1));
    QVERIFY(tracker.hasSeen(2));
    QVERIFY(tracker.hasSeen(3));
    {
        QList<ConstructionCounted> a;
        a.reserve(3);
        tracker.appendTo(a);
        for (const auto &counter : a) {
            QCOMPARE(counter.moves, 1);
            QCOMPARE(counter.copies, 1);
        }
    }
    QVERIFY(tracker.hasSeen(1));
    QVERIFY(tracker.hasSeen(2));
    QVERIFY(tracker.hasSeen(3));
    {
        QList<ConstructionCounted> a;
        a.reserve(3);
        std::move(tracker).appendTo(a);
        if (QDuplicateTracker<ConstructionCounted>::uses_pmr) {
            // the following is only true if we use the std container
            for (const auto &counter : a) {
                QCOMPARE(counter.moves, 2);
                QCOMPARE(counter.copies, 0);
            }
            QVERIFY(!tracker.hasSeen(1));
            QVERIFY(!tracker.hasSeen(2));
            QVERIFY(!tracker.hasSeen(3));
        }
    }
}

QTEST_MAIN(tst_QDuplicateTracker)

#include "tst_qduplicatetracker.moc"
