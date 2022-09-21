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

#ifndef QFLAGS_H
#define QFLAGS_H

#include <initializer_list>

QT_BEGIN_NAMESPACE

class QDataStream;

class QFlag
{
    int i;
public:
    constexpr inline Q_IMPLICIT QFlag(int value) noexcept : i(value) {}
    constexpr inline Q_IMPLICIT operator int() const noexcept { return i; }

#if !defined(Q_CC_MSVC)
    // Microsoft Visual Studio has buggy behavior when it comes to
    // unsigned enums: even if the enum is unsigned, the enum tags are
    // always signed
#  if !defined(__LP64__) && !defined(Q_CLANG_QDOC)
    constexpr inline Q_IMPLICIT QFlag(long value) noexcept : i(int(value)) {}
    constexpr inline Q_IMPLICIT QFlag(ulong value) noexcept : i(int(long(value))) {}
#  endif
    constexpr inline Q_IMPLICIT QFlag(uint value) noexcept : i(int(value)) {}
    constexpr inline Q_IMPLICIT QFlag(short value) noexcept : i(int(value)) {}
    constexpr inline Q_IMPLICIT QFlag(ushort value) noexcept : i(int(uint(value))) {}
    constexpr inline Q_IMPLICIT operator uint() const noexcept { return uint(i); }
#endif
};
Q_DECLARE_TYPEINFO(QFlag, Q_PRIMITIVE_TYPE);

class QIncompatibleFlag
{
    int i;
public:
    constexpr inline explicit QIncompatibleFlag(int i) noexcept;
    constexpr inline Q_IMPLICIT operator int() const noexcept { return i; }
};
Q_DECLARE_TYPEINFO(QIncompatibleFlag, Q_PRIMITIVE_TYPE);

constexpr inline QIncompatibleFlag::QIncompatibleFlag(int value) noexcept : i(value) {}


template<typename Enum>
class QFlags
{
    static_assert((sizeof(Enum) <= sizeof(int)),
                  "QFlags uses an int as storage, so an enum with underlying "
                  "long long will overflow.");
    static_assert((std::is_enum<Enum>::value), "QFlags is only usable on enumeration types.");

public:
#if defined(Q_CC_MSVC) || defined(Q_CLANG_QDOC)
    // see above for MSVC
    // the definition below is too complex for qdoc
    typedef int Int;
#else
    typedef typename std::conditional<
            std::is_unsigned<typename std::underlying_type<Enum>::type>::value,
            unsigned int,
            signed int
        >::type Int;
#endif
    typedef Enum enum_type;
    // compiler-generated copy/move ctor/assignment operators are fine!
    constexpr inline QFlags() noexcept : i(0) {}
    constexpr inline Q_IMPLICIT QFlags(Enum flags) noexcept : i(Int(flags)) {}
    constexpr inline Q_IMPLICIT QFlags(QFlag flag) noexcept : i(flag) {}

    constexpr inline QFlags(std::initializer_list<Enum> flags) noexcept
        : i(initializer_list_helper(flags.begin(), flags.end())) {}

    constexpr static inline QFlags fromInt(Int i) noexcept { return QFlags(QFlag(i)); }
    constexpr inline Int toInt() const noexcept { return i; }

    constexpr inline QFlags &operator&=(int mask) noexcept { i &= mask; return *this; }
    constexpr inline QFlags &operator&=(uint mask) noexcept { i &= mask; return *this; }
    constexpr inline QFlags &operator&=(QFlags mask) noexcept { i &= mask.i; return *this; }
    constexpr inline QFlags &operator&=(Enum mask) noexcept { i &= Int(mask); return *this; }
    constexpr inline QFlags &operator|=(QFlags other) noexcept { i |= other.i; return *this; }
    constexpr inline QFlags &operator|=(Enum other) noexcept { i |= Int(other); return *this; }
    constexpr inline QFlags &operator^=(QFlags other) noexcept { i ^= other.i; return *this; }
    constexpr inline QFlags &operator^=(Enum other) noexcept { i ^= Int(other); return *this; }

    constexpr inline operator Int() const noexcept { return i; }

    constexpr inline QFlags operator|(QFlags other) const noexcept { return QFlags(QFlag(i | other.i)); }
    constexpr inline QFlags operator|(Enum other) const noexcept { return QFlags(QFlag(i | Int(other))); }
    constexpr inline QFlags operator^(QFlags other) const noexcept { return QFlags(QFlag(i ^ other.i)); }
    constexpr inline QFlags operator^(Enum other) const noexcept { return QFlags(QFlag(i ^ Int(other))); }
    constexpr inline QFlags operator&(int mask) const noexcept { return QFlags(QFlag(i & mask)); }
    constexpr inline QFlags operator&(uint mask) const noexcept { return QFlags(QFlag(i & mask)); }
    constexpr inline QFlags operator&(QFlags other) const noexcept { return QFlags(QFlag(i & other.i)); }
    constexpr inline QFlags operator&(Enum other) const noexcept { return QFlags(QFlag(i & Int(other))); }
    constexpr inline QFlags operator~() const noexcept { return QFlags(QFlag(~i)); }

    constexpr inline void operator+(QFlags other) const noexcept = delete;
    constexpr inline void operator+(Enum other) const noexcept = delete;
    constexpr inline void operator+(int other) const noexcept = delete;
    constexpr inline void operator-(QFlags other) const noexcept = delete;
    constexpr inline void operator-(Enum other) const noexcept = delete;
    constexpr inline void operator-(int other) const noexcept = delete;

    constexpr inline bool operator!() const noexcept { return !i; }

    constexpr inline bool testFlag(Enum flag) const noexcept { return testFlags(flag); }
    constexpr inline bool testFlags(QFlags flags) const noexcept { return flags.i ? ((i & flags.i) == flags.i) : i == Int(0); }
    constexpr inline bool testAnyFlag(Enum flag) const noexcept { return testAnyFlags(flag); }
    constexpr inline bool testAnyFlags(QFlags flags) const noexcept { return (i & flags.i) != Int(0); }
    constexpr inline QFlags &setFlag(Enum flag, bool on = true) noexcept
    {
        return on ? (*this |= flag) : (*this &= ~QFlags(flag));
    }

    friend constexpr inline bool operator==(QFlags lhs, QFlags rhs) noexcept
    { return lhs.i == rhs.i; }
    friend constexpr inline bool operator!=(QFlags lhs, QFlags rhs) noexcept
    { return lhs.i != rhs.i; }
    friend constexpr inline bool operator==(QFlags lhs, Enum rhs) noexcept
    { return lhs == QFlags(rhs); }
    friend constexpr inline bool operator!=(QFlags lhs, Enum rhs) noexcept
    { return lhs != QFlags(rhs); }
    friend constexpr inline bool operator==(Enum lhs, QFlags rhs) noexcept
    { return QFlags(lhs) == rhs; }
    friend constexpr inline bool operator!=(Enum lhs, QFlags rhs) noexcept
    { return QFlags(lhs) != rhs; }

private:
    constexpr static inline Int initializer_list_helper(typename std::initializer_list<Enum>::const_iterator it,
                                                               typename std::initializer_list<Enum>::const_iterator end)
    noexcept
    {
        return (it == end ? Int(0) : (Int(*it) | initializer_list_helper(it + 1, end)));
    }

    Int i;
};

#ifndef Q_MOC_RUN
#define Q_DECLARE_FLAGS(Flags, Enum)\
typedef QFlags<Enum> Flags;
#endif

#define Q_DECLARE_OPERATORS_FOR_FLAGS(Flags) \
constexpr inline QFlags<Flags::enum_type> operator|(Flags::enum_type f1, Flags::enum_type f2) noexcept \
{ return QFlags<Flags::enum_type>(f1) | f2; } \
constexpr inline QFlags<Flags::enum_type> operator|(Flags::enum_type f1, QFlags<Flags::enum_type> f2) noexcept \
{ return f2 | f1; } \
constexpr inline QFlags<Flags::enum_type> operator&(Flags::enum_type f1, Flags::enum_type f2) noexcept \
{ return QFlags<Flags::enum_type>(f1) & f2; } \
constexpr inline QFlags<Flags::enum_type> operator&(Flags::enum_type f1, QFlags<Flags::enum_type> f2) noexcept \
{ return f2 & f1; } \
constexpr inline void operator+(Flags::enum_type f1, Flags::enum_type f2) noexcept = delete; \
constexpr inline void operator+(Flags::enum_type f1, QFlags<Flags::enum_type> f2) noexcept = delete; \
constexpr inline void operator+(int f1, QFlags<Flags::enum_type> f2) noexcept = delete; \
constexpr inline void operator-(Flags::enum_type f1, Flags::enum_type f2) noexcept = delete; \
constexpr inline void operator-(Flags::enum_type f1, QFlags<Flags::enum_type> f2) noexcept = delete; \
constexpr inline void operator-(int f1, QFlags<Flags::enum_type> f2) noexcept = delete; \
constexpr inline QIncompatibleFlag operator|(Flags::enum_type f1, int f2) noexcept \
{ return QIncompatibleFlag(int(f1) | f2); } \
constexpr inline void operator+(int f1, Flags::enum_type f2) noexcept = delete; \
constexpr inline void operator+(Flags::enum_type f1, int f2) noexcept = delete; \
constexpr inline void operator-(int f1, Flags::enum_type f2) noexcept = delete; \
constexpr inline void operator-(Flags::enum_type f1, int f2) noexcept = delete;

// restore bit-wise enum-enum operators deprecated in C++20,
// but used in a few places in the API
#if __cplusplus > 201702L // assume compilers don't warn if in C++17 mode
  // in C++20 mode, provide user-defined operators to override the deprecated operations:
# define Q_DECLARE_MIXED_ENUM_OPERATOR(op, Ret, LHS, RHS) \
    [[maybe_unused]] \
    constexpr inline Ret operator op (LHS lhs, RHS rhs) noexcept \
    { return static_cast<Ret>(qToUnderlying(lhs) op qToUnderlying(rhs)); } \
    /* end */
#else
  // in C++17 mode, statically-assert that this compiler's result of the
  // operation is the same that the C++20 version would produce:
# define Q_DECLARE_MIXED_ENUM_OPERATOR(op, Ret, LHS, RHS) \
    static_assert(std::is_same_v<decltype(std::declval<LHS>() op std::declval<RHS>()), Ret>);
#endif

#define Q_DECLARE_MIXED_ENUM_OPERATORS(Ret, Flags, Enum) \
    Q_DECLARE_MIXED_ENUM_OPERATOR(|, Ret, Flags, Enum) \
    Q_DECLARE_MIXED_ENUM_OPERATOR(&, Ret, Flags, Enum) \
    Q_DECLARE_MIXED_ENUM_OPERATOR(^, Ret, Flags, Enum) \
    /* end */

#define Q_DECLARE_MIXED_ENUM_OPERATORS_SYMMETRIC(Ret, Flags, Enum) \
    Q_DECLARE_MIXED_ENUM_OPERATORS(Ret, Flags, Enum) \
    Q_DECLARE_MIXED_ENUM_OPERATORS(Ret, Enum, Flags) \
    /* end */


QT_END_NAMESPACE

#endif // QFLAGS_H
