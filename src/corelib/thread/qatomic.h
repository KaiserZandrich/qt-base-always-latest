// Copyright (C) 2017 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QATOMIC_H
#define QATOMIC_H

#include <QtCore/qbasicatomic.h>

QT_BEGIN_NAMESPACE

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wextra")

// High-level atomic integer operations
template <typename T>
class QAtomicInteger : public QBasicAtomicInteger<T>
{
public:
    constexpr QAtomicInteger(T value = 0) noexcept : QBasicAtomicInteger<T>(value) {}
    using QBasicAtomicInteger<T>::operator=;

#ifdef Q_QDOC
    T loadRelaxed() const;
    T loadAcquire() const;
    void storeRelaxed(T newValue);
    void storeRelease(T newValue);

    operator T() const;
    QAtomicInteger &operator=(T);

    static constexpr bool isReferenceCountingNative();
    static constexpr bool isReferenceCountingWaitFree();

    bool ref();
    bool deref();

    static constexpr bool isTestAndSetNative();
    static constexpr bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T expectedValue, T newValue);
    bool testAndSetAcquire(T expectedValue, T newValue);
    bool testAndSetRelease(T expectedValue, T newValue);
    bool testAndSetOrdered(T expectedValue, T newValue);

    bool testAndSetRelaxed(T expectedValue, T newValue, T &currentValue);
    bool testAndSetAcquire(T expectedValue, T newValue, T &currentValue);
    bool testAndSetRelease(T expectedValue, T newValue, T &currentValue);
    bool testAndSetOrdered(T expectedValue, T newValue, T &currentValue);

    static constexpr bool isFetchAndStoreNative();
    static constexpr bool isFetchAndStoreWaitFree();

    T fetchAndStoreRelaxed(T newValue);
    T fetchAndStoreAcquire(T newValue);
    T fetchAndStoreRelease(T newValue);
    T fetchAndStoreOrdered(T newValue);

    static constexpr bool isFetchAndAddNative();
    static constexpr bool isFetchAndAddWaitFree();

    T fetchAndAddRelaxed(T valueToAdd);
    T fetchAndAddAcquire(T valueToAdd);
    T fetchAndAddRelease(T valueToAdd);
    T fetchAndAddOrdered(T valueToAdd);

    T fetchAndSubRelaxed(T valueToSub);
    T fetchAndSubAcquire(T valueToSub);
    T fetchAndSubRelease(T valueToSub);
    T fetchAndSubOrdered(T valueToSub);

    T fetchAndOrRelaxed(T valueToOr);
    T fetchAndOrAcquire(T valueToOr);
    T fetchAndOrRelease(T valueToOr);
    T fetchAndOrOrdered(T valueToOr);

    T fetchAndAndRelaxed(T valueToAnd);
    T fetchAndAndAcquire(T valueToAnd);
    T fetchAndAndRelease(T valueToAnd);
    T fetchAndAndOrdered(T valueToAnd);

    T fetchAndXorRelaxed(T valueToXor);
    T fetchAndXorAcquire(T valueToXor);
    T fetchAndXorRelease(T valueToXor);
    T fetchAndXorOrdered(T valueToXor);

    T operator++();
    T operator++(int);
    T operator--();
    T operator--(int);
    T operator+=(T value);
    T operator-=(T value);
    T operator|=(T value);
    T operator&=(T value);
    T operator^=(T value);
#endif
};

// ### Qt 7: make QAtomicInt a typedef
class QAtomicInt : public QAtomicInteger<int>
{
public:
    using QAtomicInteger<int>::QAtomicInteger;
    using QAtomicInteger<int>::operator=;
};

// High-level atomic pointer operations
template <typename T>
class QAtomicPointer : public QBasicAtomicPointer<T>
{
public:
    constexpr QAtomicPointer(T *value = nullptr) noexcept : QBasicAtomicPointer<T>(value) {}
    using QBasicAtomicPointer<T>::operator=;

#ifdef Q_QDOC
    T *loadAcquire() const;
    T *loadRelaxed() const;
    void storeRelaxed(T *newValue);
    void storeRelease(T *newValue);

    static constexpr bool isTestAndSetNative();
    static constexpr bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T *expectedValue, T *newValue);
    bool testAndSetAcquire(T *expectedValue, T *newValue);
    bool testAndSetRelease(T *expectedValue, T *newValue);
    bool testAndSetOrdered(T *expectedValue, T *newValue);

    static constexpr bool isFetchAndStoreNative();
    static constexpr bool isFetchAndStoreWaitFree();

    T *fetchAndStoreRelaxed(T *newValue);
    T *fetchAndStoreAcquire(T *newValue);
    T *fetchAndStoreRelease(T *newValue);
    T *fetchAndStoreOrdered(T *newValue);

    static constexpr bool isFetchAndAddNative();
    static constexpr bool isFetchAndAddWaitFree();

    T *fetchAndAddRelaxed(qptrdiff valueToAdd);
    T *fetchAndAddAcquire(qptrdiff valueToAdd);
    T *fetchAndAddRelease(qptrdiff valueToAdd);
    T *fetchAndAddOrdered(qptrdiff valueToAdd);
#endif
};

QT_WARNING_POP

/*!
    This is a helper for the assignment operators of implicitly
    shared classes. Your assignment operator should look like this:

    \snippet code/src.corelib.thread.qatomic.h 0
*/
template <typename T>
inline void qAtomicAssign(T *&d, T *x)
{
    if (d == x)
        return;
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

/*!
    This is a helper for the detach method of implicitly shared
    classes. Your private class needs a copy constructor which copies
    the members and sets the refcount to 1. After that, your detach
    function should look like this:

    \snippet code/src.corelib.thread.qatomic.h 1
*/
template <typename T>
inline void qAtomicDetach(T *&d)
{
    if (d->ref.loadRelaxed() == 1)
        return;
    T *x = d;
    d = new T(*d);
    if (!x->ref.deref())
        delete x;
}

QT_END_NAMESPACE
#endif // QATOMIC_H
