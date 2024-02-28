// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOLORTRANSFERFUNCTION_P_H
#define QCOLORTRANSFERFUNCTION_P_H

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

#include <QtGui/private/qtguiglobal_p.h>

#include <cmath>

QT_BEGIN_NAMESPACE

// Defines a ICC parametric curve type 4
class Q_GUI_EXPORT QColorTransferFunction
{
public:
    QColorTransferFunction() noexcept
            : m_a(1.0f), m_b(0.0f), m_c(1.0f), m_d(0.0f), m_e(0.0f), m_f(0.0f), m_g(1.0f)
            , m_flags(quint32(Hints::Calculated) | quint32(Hints::IsGamma) | quint32(Hints::IsIdentity))
    { }
    QColorTransferFunction(float a, float b, float c, float d, float e, float f, float g) noexcept
            : m_a(a), m_b(b), m_c(c), m_d(d), m_e(e), m_f(f), m_g(g), m_flags(0)
    { }

    bool isGamma() const
    {
        updateHints();
        return m_flags & quint32(Hints::IsGamma);
    }
    bool isIdentity() const
    {
        updateHints();
        return m_flags & quint32(Hints::IsIdentity);
    }
    bool isSRgb() const
    {
        updateHints();
        return m_flags & quint32(Hints::IsSRgb);
    }

    float apply(float x) const
    {
        if (x < m_d)
            return m_c * x + m_f;
        else
            return std::pow(m_a * x + m_b, m_g) + m_e;
    }

    QColorTransferFunction inverted() const
    {
        float a, b, c, d, e, f, g;

        d = m_c * m_d + m_f;

        if (!qFuzzyIsNull(m_c)) {
            c = 1.0f / m_c;
            f = -m_f / m_c;
        } else {
            c = 0.0f;
            f = 0.0f;
        }

        if (!qFuzzyIsNull(m_a) && !qFuzzyIsNull(m_g)) {
            a = std::pow(1.0f / m_a, m_g);
            b = -a * m_e;
            e = -m_b / m_a;
            g = 1.0f / m_g;
        } else {
            a = 0.0f;
            b = 0.0f;
            e = 1.0f;
            g = 1.0f;
        }

        return QColorTransferFunction(a, b, c, d, e, f, g);
    }

    // A few predefined curves:
    static QColorTransferFunction fromGamma(float gamma)
    {
        return QColorTransferFunction(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, gamma,
                                      quint32(Hints::Calculated) | quint32(Hints::IsGamma) |
                                      (paramCompare(gamma, 1.0f) ? quint32(Hints::IsIdentity) : 0));
    }
    static QColorTransferFunction fromSRgb()
    {
        return QColorTransferFunction(1.0f / 1.055f, 0.055f / 1.055f, 1.0f / 12.92f, 0.04045f, 0.0f, 0.0f, 2.4f,
                                      quint32(Hints::Calculated) | quint32(Hints::IsSRgb));
    }
    static QColorTransferFunction fromProPhotoRgb()
    {
        return QColorTransferFunction(1.0f, 0.0f, 1.0f / 16.0f, 16.0f / 512.0f, 0.0f, 0.0f, 1.8f,
                                      quint32(Hints::Calculated));
    }
    bool matches(const QColorTransferFunction &o) const
    {
        return paramCompare(m_a, o.m_a) && paramCompare(m_b, o.m_b)
            && paramCompare(m_c, o.m_c) && paramCompare(m_d, o.m_d)
            && paramCompare(m_e, o.m_e) && paramCompare(m_f, o.m_f)
            && paramCompare(m_g, o.m_g);
    }
    friend inline bool operator==(const QColorTransferFunction &f1, const QColorTransferFunction &f2);
    friend inline bool operator!=(const QColorTransferFunction &f1, const QColorTransferFunction &f2);

    float m_a;
    float m_b;
    float m_c;
    float m_d;
    float m_e;
    float m_f;
    float m_g;

private:
    QColorTransferFunction(float a, float b, float c, float d, float e, float f, float g, quint32 flags) noexcept
        : m_a(a), m_b(b), m_c(c), m_d(d), m_e(e), m_f(f), m_g(g), m_flags(flags)
    { }
    static inline bool paramCompare(float p1, float p2)
    {
        // Much fuzzier than fuzzy compare.
        // It tries match parameters that has been passed through a 8.8
        // fixed point form.
        return (qAbs(p1 - p2) <= (1.0f / 512.0f));
    }

    void updateHints() const
    {
        if (m_flags & quint32(Hints::Calculated))
            return;
        // We do not consider the case with m_d = 1.0f linear or simple,
        // since it wouldn't be linear for applyExtended().
        bool simple = paramCompare(m_a, 1.0f) && paramCompare(m_b, 0.0f)
                                              && paramCompare(m_d, 0.0f)
                                              && paramCompare(m_e, 0.0f);
        if (simple) {
            m_flags |= quint32(Hints::IsGamma);
            if (qFuzzyCompare(m_g, 1.0f))
                m_flags |= quint32(Hints::IsIdentity);
        } else {
            if (*this == fromSRgb())
                m_flags |= quint32(Hints::IsSRgb);
        }
        m_flags |= quint32(Hints::Calculated);
    }
    enum class Hints : quint32 {
        Calculated = 1,
        IsGamma = 2,
        IsIdentity = 4,
        IsSRgb = 8
    };
    mutable quint32 m_flags;
};

inline bool operator==(const QColorTransferFunction &f1, const QColorTransferFunction &f2)
{
    return f1.matches(f2);
}
inline bool operator!=(const QColorTransferFunction &f1, const QColorTransferFunction &f2)
{
    return !f1.matches(f2);
}

QT_END_NAMESPACE

#endif // QCOLORTRANSFERFUNCTION_P_H
