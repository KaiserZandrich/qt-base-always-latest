// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/QtCompare>
#include <QtTest/QTest>

#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif // __cpp_lib_three_way_comparison

class tst_QCompare: public QObject
{
    Q_OBJECT
private slots:
    void partialOrdering();
    void weakOrdering();
    void strongOrdering();
    void conversions();
    void is_eq_overloads();
};

void tst_QCompare::partialOrdering()
{
    static_assert(QPartialOrdering::Unordered == QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Less != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Less == QPartialOrdering::Less);
    static_assert(QPartialOrdering::Less != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Less != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Equivalent == QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Greater != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Greater != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Greater != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Greater == QPartialOrdering::Greater);

    static_assert(!is_eq  (QPartialOrdering::Unordered));
    static_assert(!is_neq (QPartialOrdering::Unordered));
    static_assert(!is_lt  (QPartialOrdering::Unordered));
    static_assert(!is_lteq(QPartialOrdering::Unordered));
    static_assert(!is_gt  (QPartialOrdering::Unordered));
    static_assert(!is_gteq(QPartialOrdering::Unordered));

    static_assert(!(QPartialOrdering::Unordered == 0));
    static_assert(!(QPartialOrdering::Unordered != 0));
    static_assert(!(QPartialOrdering::Unordered <  0));
    static_assert(!(QPartialOrdering::Unordered <= 0));
    static_assert(!(QPartialOrdering::Unordered >  0));
    static_assert(!(QPartialOrdering::Unordered >= 0));

    static_assert(!(0 == QPartialOrdering::Unordered));
    static_assert(!(0 != QPartialOrdering::Unordered));
    static_assert(!(0 <  QPartialOrdering::Unordered));
    static_assert(!(0 <= QPartialOrdering::Unordered));
    static_assert(!(0 >  QPartialOrdering::Unordered));
    static_assert(!(0 >= QPartialOrdering::Unordered));


    static_assert(!is_eq  (QPartialOrdering::Less));
    static_assert( is_neq (QPartialOrdering::Less));
    static_assert( is_lt  (QPartialOrdering::Less));
    static_assert( is_lteq(QPartialOrdering::Less));
    static_assert(!is_gt  (QPartialOrdering::Less));
    static_assert(!is_gteq(QPartialOrdering::Less));

    static_assert(!(QPartialOrdering::Less == 0));
    static_assert( (QPartialOrdering::Less != 0));
    static_assert( (QPartialOrdering::Less <  0));
    static_assert( (QPartialOrdering::Less <= 0));
    static_assert(!(QPartialOrdering::Less >  0));
    static_assert(!(QPartialOrdering::Less >= 0));

    static_assert(!(0 == QPartialOrdering::Less));
    static_assert( (0 != QPartialOrdering::Less));
    static_assert(!(0 <  QPartialOrdering::Less));
    static_assert(!(0 <= QPartialOrdering::Less));
    static_assert( (0 >  QPartialOrdering::Less));
    static_assert( (0 >= QPartialOrdering::Less));


    static_assert( is_eq  (QPartialOrdering::Equivalent));
    static_assert(!is_neq (QPartialOrdering::Equivalent));
    static_assert(!is_lt  (QPartialOrdering::Equivalent));
    static_assert( is_lteq(QPartialOrdering::Equivalent));
    static_assert(!is_gt  (QPartialOrdering::Equivalent));
    static_assert( is_gteq(QPartialOrdering::Equivalent));

    static_assert( (QPartialOrdering::Equivalent == 0));
    static_assert(!(QPartialOrdering::Equivalent != 0));
    static_assert(!(QPartialOrdering::Equivalent <  0));
    static_assert( (QPartialOrdering::Equivalent <= 0));
    static_assert(!(QPartialOrdering::Equivalent >  0));
    static_assert( (QPartialOrdering::Equivalent >= 0));

    static_assert( (0 == QPartialOrdering::Equivalent));
    static_assert(!(0 != QPartialOrdering::Equivalent));
    static_assert(!(0 <  QPartialOrdering::Equivalent));
    static_assert( (0 <= QPartialOrdering::Equivalent));
    static_assert(!(0 >  QPartialOrdering::Equivalent));
    static_assert( (0 >= QPartialOrdering::Equivalent));


    static_assert(!is_eq  (QPartialOrdering::Greater));
    static_assert( is_neq (QPartialOrdering::Greater));
    static_assert(!is_lt  (QPartialOrdering::Greater));
    static_assert(!is_lteq(QPartialOrdering::Greater));
    static_assert( is_gt  (QPartialOrdering::Greater));
    static_assert( is_gteq(QPartialOrdering::Greater));

    static_assert(!(QPartialOrdering::Greater == 0));
    static_assert( (QPartialOrdering::Greater != 0));
    static_assert(!(QPartialOrdering::Greater <  0));
    static_assert(!(QPartialOrdering::Greater <= 0));
    static_assert( (QPartialOrdering::Greater >  0));
    static_assert( (QPartialOrdering::Greater >= 0));

    static_assert(!(0 == QPartialOrdering::Greater));
    static_assert( (0 != QPartialOrdering::Greater));
    static_assert( (0 <  QPartialOrdering::Greater));
    static_assert( (0 <= QPartialOrdering::Greater));
    static_assert(!(0 >  QPartialOrdering::Greater));
    static_assert(!(0 >= QPartialOrdering::Greater));
}

void tst_QCompare::weakOrdering()
{
    static_assert(QWeakOrdering::Less == QWeakOrdering::Less);
    static_assert(QWeakOrdering::Less != QWeakOrdering::Equivalent);
    static_assert(QWeakOrdering::Less != QWeakOrdering::Greater);

    static_assert(QWeakOrdering::Equivalent != QWeakOrdering::Less);
    static_assert(QWeakOrdering::Equivalent == QWeakOrdering::Equivalent);
    static_assert(QWeakOrdering::Equivalent != QWeakOrdering::Greater);

    static_assert(QWeakOrdering::Greater != QWeakOrdering::Less);
    static_assert(QWeakOrdering::Greater != QWeakOrdering::Equivalent);
    static_assert(QWeakOrdering::Greater == QWeakOrdering::Greater);

    static_assert(!is_eq  (QWeakOrdering::Less));
    static_assert( is_neq (QWeakOrdering::Less));
    static_assert( is_lt  (QWeakOrdering::Less));
    static_assert( is_lteq(QWeakOrdering::Less));
    static_assert(!is_gt  (QWeakOrdering::Less));
    static_assert(!is_gteq(QWeakOrdering::Less));

    static_assert(!(QWeakOrdering::Less == 0));
    static_assert( (QWeakOrdering::Less != 0));
    static_assert( (QWeakOrdering::Less <  0));
    static_assert( (QWeakOrdering::Less <= 0));
    static_assert(!(QWeakOrdering::Less >  0));
    static_assert(!(QWeakOrdering::Less >= 0));

    static_assert(!(0 == QWeakOrdering::Less));
    static_assert( (0 != QWeakOrdering::Less));
    static_assert(!(0 <  QWeakOrdering::Less));
    static_assert(!(0 <= QWeakOrdering::Less));
    static_assert( (0 >  QWeakOrdering::Less));
    static_assert( (0 >= QWeakOrdering::Less));


    static_assert( is_eq  (QWeakOrdering::Equivalent));
    static_assert(!is_neq (QWeakOrdering::Equivalent));
    static_assert(!is_lt  (QWeakOrdering::Equivalent));
    static_assert( is_lteq(QWeakOrdering::Equivalent));
    static_assert(!is_gt  (QWeakOrdering::Equivalent));
    static_assert( is_gteq(QWeakOrdering::Equivalent));

    static_assert( (QWeakOrdering::Equivalent == 0));
    static_assert(!(QWeakOrdering::Equivalent != 0));
    static_assert(!(QWeakOrdering::Equivalent <  0));
    static_assert( (QWeakOrdering::Equivalent <= 0));
    static_assert(!(QWeakOrdering::Equivalent >  0));
    static_assert( (QWeakOrdering::Equivalent >= 0));

    static_assert( (0 == QWeakOrdering::Equivalent));
    static_assert(!(0 != QWeakOrdering::Equivalent));
    static_assert(!(0 <  QWeakOrdering::Equivalent));
    static_assert( (0 <= QWeakOrdering::Equivalent));
    static_assert(!(0 >  QWeakOrdering::Equivalent));
    static_assert( (0 >= QWeakOrdering::Equivalent));


    static_assert(!is_eq  (QWeakOrdering::Greater));
    static_assert( is_neq (QWeakOrdering::Greater));
    static_assert(!is_lt  (QWeakOrdering::Greater));
    static_assert(!is_lteq(QWeakOrdering::Greater));
    static_assert( is_gt  (QWeakOrdering::Greater));
    static_assert( is_gteq(QWeakOrdering::Greater));

    static_assert(!(QWeakOrdering::Greater == 0));
    static_assert( (QWeakOrdering::Greater != 0));
    static_assert(!(QWeakOrdering::Greater <  0));
    static_assert(!(QWeakOrdering::Greater <= 0));
    static_assert( (QWeakOrdering::Greater >  0));
    static_assert( (QWeakOrdering::Greater >= 0));

    static_assert(!(0 == QWeakOrdering::Greater));
    static_assert( (0 != QWeakOrdering::Greater));
    static_assert( (0 <  QWeakOrdering::Greater));
    static_assert( (0 <= QWeakOrdering::Greater));
    static_assert(!(0 >  QWeakOrdering::Greater));
    static_assert(!(0 >= QWeakOrdering::Greater));
}

void tst_QCompare::strongOrdering()
{
    static_assert(QStrongOrdering::Less == QStrongOrdering::Less);
    static_assert(QStrongOrdering::Less != QStrongOrdering::Equal);
    static_assert(QStrongOrdering::Less != QStrongOrdering::Equivalent);
    static_assert(QStrongOrdering::Less != QStrongOrdering::Greater);

    static_assert(QStrongOrdering::Equal != QStrongOrdering::Less);
    static_assert(QStrongOrdering::Equal == QStrongOrdering::Equal);
    static_assert(QStrongOrdering::Equal == QStrongOrdering::Equivalent);
    static_assert(QStrongOrdering::Equal != QStrongOrdering::Greater);

    static_assert(QStrongOrdering::Equivalent != QStrongOrdering::Less);
    static_assert(QStrongOrdering::Equivalent == QStrongOrdering::Equal);
    static_assert(QStrongOrdering::Equivalent == QStrongOrdering::Equivalent);
    static_assert(QStrongOrdering::Equivalent != QStrongOrdering::Greater);

    static_assert(QStrongOrdering::Greater != QStrongOrdering::Less);
    static_assert(QStrongOrdering::Greater != QStrongOrdering::Equal);
    static_assert(QStrongOrdering::Greater != QStrongOrdering::Equivalent);
    static_assert(QStrongOrdering::Greater == QStrongOrdering::Greater);

    static_assert(!is_eq  (QStrongOrdering::Less));
    static_assert( is_neq (QStrongOrdering::Less));
    static_assert( is_lt  (QStrongOrdering::Less));
    static_assert( is_lteq(QStrongOrdering::Less));
    static_assert(!is_gt  (QStrongOrdering::Less));
    static_assert(!is_gteq(QStrongOrdering::Less));

    static_assert(!(QStrongOrdering::Less == 0));
    static_assert( (QStrongOrdering::Less != 0));
    static_assert( (QStrongOrdering::Less <  0));
    static_assert( (QStrongOrdering::Less <= 0));
    static_assert(!(QStrongOrdering::Less >  0));
    static_assert(!(QStrongOrdering::Less >= 0));

    static_assert(!(0 == QStrongOrdering::Less));
    static_assert( (0 != QStrongOrdering::Less));
    static_assert(!(0 <  QStrongOrdering::Less));
    static_assert(!(0 <= QStrongOrdering::Less));
    static_assert( (0 >  QStrongOrdering::Less));
    static_assert( (0 >= QStrongOrdering::Less));


    static_assert( is_eq  (QStrongOrdering::Equal));
    static_assert(!is_neq (QStrongOrdering::Equal));
    static_assert(!is_lt  (QStrongOrdering::Equal));
    static_assert( is_lteq(QStrongOrdering::Equal));
    static_assert(!is_gt  (QStrongOrdering::Equal));
    static_assert( is_gteq(QStrongOrdering::Equal));

    static_assert( (QStrongOrdering::Equal == 0));
    static_assert(!(QStrongOrdering::Equal != 0));
    static_assert(!(QStrongOrdering::Equal <  0));
    static_assert( (QStrongOrdering::Equal <= 0));
    static_assert(!(QStrongOrdering::Equal >  0));
    static_assert( (QStrongOrdering::Equal >= 0));

    static_assert( (0 == QStrongOrdering::Equal));
    static_assert(!(0 != QStrongOrdering::Equal));
    static_assert(!(0 <  QStrongOrdering::Equal));
    static_assert( (0 <= QStrongOrdering::Equal));
    static_assert(!(0 >  QStrongOrdering::Equal));
    static_assert( (0 >= QStrongOrdering::Equal));


    static_assert( is_eq  (QStrongOrdering::Equivalent));
    static_assert(!is_neq (QStrongOrdering::Equivalent));
    static_assert(!is_lt  (QStrongOrdering::Equivalent));
    static_assert( is_lteq(QStrongOrdering::Equivalent));
    static_assert(!is_gt  (QStrongOrdering::Equivalent));
    static_assert( is_gteq(QStrongOrdering::Equivalent));

    static_assert( (QStrongOrdering::Equivalent == 0));
    static_assert(!(QStrongOrdering::Equivalent != 0));
    static_assert(!(QStrongOrdering::Equivalent <  0));
    static_assert( (QStrongOrdering::Equivalent <= 0));
    static_assert(!(QStrongOrdering::Equivalent >  0));
    static_assert( (QStrongOrdering::Equivalent >= 0));

    static_assert( (0 == QStrongOrdering::Equivalent));
    static_assert(!(0 != QStrongOrdering::Equivalent));
    static_assert(!(0 <  QStrongOrdering::Equivalent));
    static_assert( (0 <= QStrongOrdering::Equivalent));
    static_assert(!(0 >  QStrongOrdering::Equivalent));
    static_assert( (0 >= QStrongOrdering::Equivalent));


    static_assert(!is_eq  (QStrongOrdering::Greater));
    static_assert( is_neq (QStrongOrdering::Greater));
    static_assert(!is_lt  (QStrongOrdering::Greater));
    static_assert(!is_lteq(QStrongOrdering::Greater));
    static_assert( is_gt  (QStrongOrdering::Greater));
    static_assert( is_gteq(QStrongOrdering::Greater));

    static_assert(!(QStrongOrdering::Greater == 0));
    static_assert( (QStrongOrdering::Greater != 0));
    static_assert(!(QStrongOrdering::Greater <  0));
    static_assert(!(QStrongOrdering::Greater <= 0));
    static_assert( (QStrongOrdering::Greater >  0));
    static_assert( (QStrongOrdering::Greater >= 0));

    static_assert(!(0 == QStrongOrdering::Greater));
    static_assert( (0 != QStrongOrdering::Greater));
    static_assert( (0 <  QStrongOrdering::Greater));
    static_assert( (0 <= QStrongOrdering::Greater));
    static_assert(!(0 >  QStrongOrdering::Greater));
    static_assert(!(0 >= QStrongOrdering::Greater));
}

void tst_QCompare::conversions()
{
    // QWeakOrdering -> QPartialOrdering
    {
        constexpr QPartialOrdering less = QWeakOrdering::Less;
        static_assert(less == QPartialOrdering::Less);
        constexpr QPartialOrdering equivalent = QWeakOrdering::Equivalent;
        static_assert(equivalent == QPartialOrdering::Equivalent);
        constexpr QPartialOrdering greater = QWeakOrdering::Greater;
        static_assert(greater == QPartialOrdering::Greater);
    }
    // QStrongOrdering -> QPartialOrdering
    {
        constexpr QPartialOrdering less = QStrongOrdering::Less;
        static_assert(less == QPartialOrdering::Less);
        constexpr QPartialOrdering equal = QStrongOrdering::Equal;
        static_assert(equal == QPartialOrdering::Equivalent);
        constexpr QPartialOrdering equivalent = QStrongOrdering::Equivalent;
        static_assert(equivalent == QPartialOrdering::Equivalent);
        constexpr QPartialOrdering greater = QStrongOrdering::Greater;
        static_assert(greater == QPartialOrdering::Greater);
    }
    // QStrongOrdering -> QWeakOrdering
    {
        constexpr QWeakOrdering less = QStrongOrdering::Less;
        static_assert(less == QWeakOrdering::Less);
        constexpr QWeakOrdering equal = QStrongOrdering::Equal;
        static_assert(equal == QWeakOrdering::Equivalent);
        constexpr QWeakOrdering equivalent = QStrongOrdering::Equivalent;
        static_assert(equivalent == QWeakOrdering::Equivalent);
        constexpr QWeakOrdering greater = QStrongOrdering::Greater;
        static_assert(greater == QWeakOrdering::Greater);
    }
    // Mixed types
    {
        static_assert(QPartialOrdering::Less == QStrongOrdering::Less);
        static_assert(QPartialOrdering::Equivalent != QStrongOrdering::Less);
        static_assert(QPartialOrdering::Equivalent == QStrongOrdering::Equal);
        static_assert(QPartialOrdering::Greater == QStrongOrdering::Greater);

        static_assert(QPartialOrdering::Less == QWeakOrdering::Less);
        static_assert(QPartialOrdering::Equivalent == QWeakOrdering::Equivalent);
        static_assert(QPartialOrdering::Greater == QWeakOrdering::Greater);

        static_assert(QWeakOrdering::Less == QStrongOrdering::Less);
        static_assert(QWeakOrdering::Equivalent != QStrongOrdering::Greater);
        static_assert(QWeakOrdering::Equivalent == QStrongOrdering::Equal);
        static_assert(QWeakOrdering::Greater == QStrongOrdering::Greater);

        static_assert(QWeakOrdering::Less == QPartialOrdering::Less);
        static_assert(QWeakOrdering::Equivalent == QPartialOrdering::Equivalent);
        static_assert(QWeakOrdering::Greater == QPartialOrdering::Greater);

        static_assert(QStrongOrdering::Less == QPartialOrdering::Less);
        static_assert(QStrongOrdering::Equivalent == QPartialOrdering::Equivalent);
        static_assert(QStrongOrdering::Equal == QPartialOrdering::Equivalent);
        static_assert(QStrongOrdering::Greater == QPartialOrdering::Greater);

        static_assert(QStrongOrdering::Less == QWeakOrdering::Less);
        static_assert(QStrongOrdering::Equivalent == QWeakOrdering::Equivalent);
        static_assert(QStrongOrdering::Equal == QWeakOrdering::Equivalent);
        static_assert(QStrongOrdering::Greater == QWeakOrdering::Greater);
    }
#ifdef __cpp_lib_three_way_comparison
    // QPartialOrdering <-> std::partial_ordering
    {
        static_assert(QPartialOrdering::Less == std::partial_ordering::less);
        static_assert(QPartialOrdering::Less != std::partial_ordering::greater);
        static_assert(std::partial_ordering::unordered != QPartialOrdering::Equivalent);
        static_assert(std::partial_ordering::unordered == QPartialOrdering::Unordered);

        static_assert((QPartialOrdering(std::partial_ordering::less) ==
                       std::partial_ordering::less));
        static_assert((QPartialOrdering(std::partial_ordering::equivalent) ==
                       std::partial_ordering::equivalent));
        static_assert((QPartialOrdering(std::partial_ordering::greater) ==
                       std::partial_ordering::greater));
        static_assert((QPartialOrdering(std::partial_ordering::unordered) ==
                       std::partial_ordering::unordered));
    }
    // QWeakOrdering <-> std::weak_ordering
    {
        static_assert(QWeakOrdering::Less == std::weak_ordering::less);
        static_assert(QWeakOrdering::Less != std::weak_ordering::equivalent);
        static_assert(std::weak_ordering::greater != QWeakOrdering::Less);
        static_assert(std::weak_ordering::equivalent == QWeakOrdering::Equivalent);

        static_assert((QWeakOrdering(std::weak_ordering::less) ==
                       std::weak_ordering::less));
        static_assert((QWeakOrdering(std::weak_ordering::equivalent) ==
                       std::weak_ordering::equivalent));
        static_assert((QWeakOrdering(std::weak_ordering::greater) ==
                       std::weak_ordering::greater));
    }
    // QStrongOrdering <-> std::strong_ordering
    {
        static_assert(QStrongOrdering::Less == std::strong_ordering::less);
        static_assert(QStrongOrdering::Less != std::strong_ordering::equivalent);
        static_assert(std::strong_ordering::greater != QStrongOrdering::Less);
        static_assert(std::strong_ordering::equivalent == QStrongOrdering::Equivalent);

        static_assert((QStrongOrdering(std::strong_ordering::less) ==
                       std::strong_ordering::less));
        static_assert((QStrongOrdering(std::strong_ordering::equivalent) ==
                       std::strong_ordering::equivalent));
        static_assert((QStrongOrdering(std::strong_ordering::greater) ==
                       std::strong_ordering::greater));
    }
    // Mixed Q*Ordering <> std::*ordering types
    {
        static_assert(QStrongOrdering::Less == std::partial_ordering::less);
        static_assert(QStrongOrdering::Less != std::partial_ordering::greater);
        static_assert(QStrongOrdering::Equal == std::weak_ordering::equivalent);
        static_assert(QStrongOrdering::Equivalent != std::weak_ordering::less);

        static_assert(QWeakOrdering::Less != std::partial_ordering::greater);
        static_assert(QWeakOrdering::Less == std::partial_ordering::less);
        static_assert(QWeakOrdering::Equivalent == std::strong_ordering::equivalent);
        static_assert(QWeakOrdering::Equivalent != std::strong_ordering::less);

        static_assert(QPartialOrdering::Less != std::weak_ordering::greater);
        static_assert(QPartialOrdering::Less == std::weak_ordering::less);
        static_assert(QPartialOrdering::Equivalent == std::strong_ordering::equivalent);
        static_assert(QPartialOrdering::Equivalent != std::strong_ordering::less);
    }
#endif

}

void tst_QCompare::is_eq_overloads()
{
#ifndef __cpp_lib_three_way_comparison
    QSKIP("This test requires C++20 three-way-comparison support enabled in the stdlib.");
#else
    constexpr auto u = std::partial_ordering::unordered;
    constexpr auto l = std::weak_ordering::less;
    constexpr auto g = std::strong_ordering::greater;
    constexpr auto e = std::weak_ordering::equivalent;
    constexpr auto s = std::strong_ordering::equal;

    // This is a compile-time check that unqualified name lookup of
    // std::is_eq-like functions isn't ambiguous, so we can recommend it to our
    // users for minimizing porting on the way to C++20.

    // The goal is to check each std::ordering and each is_eq function at least
    // once, not to test all combinations (we're not the stdlib test suite here).

    QVERIFY(is_eq(s));
    QVERIFY(is_neq(u));
    QVERIFY(is_lt(l));
    QVERIFY(is_gt(g));
    QVERIFY(is_lteq(e));
    QVERIFY(is_gteq(s));
#endif // __cpp_lib_three_way_comparison
}

QTEST_MAIN(tst_QCompare)
#include "tst_qcompare.moc"
