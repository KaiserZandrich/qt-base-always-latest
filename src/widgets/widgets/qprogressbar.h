/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QPROGRESSBAR_H
#define QPROGRESSBAR_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qframe.h>

QT_REQUIRE_CONFIG(progressbar);

QT_BEGIN_NAMESPACE

class QProgressBarPrivate;
class QStyleOptionProgressBar;

class Q_WIDGETS_EXPORT QProgressBar : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool textVisible READ isTextVisible WRITE setTextVisible)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
    Q_PROPERTY(bool invertedAppearance READ invertedAppearance WRITE setInvertedAppearance)
    Q_PROPERTY(Direction textDirection READ textDirection WRITE setTextDirection)
    Q_PROPERTY(QString format READ format WRITE setFormat RESET resetFormat)

public:
    enum Direction { TopToBottom, BottomToTop };
    Q_ENUM(Direction)

    explicit QProgressBar(QWidget *parent = nullptr);
    ~QProgressBar();

    int minimum() const;
    int maximum() const;

    int value() const;

    virtual QString text() const;
    void setTextVisible(bool visible);
    bool isTextVisible() const;

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    Qt::Orientation orientation() const;

    void setInvertedAppearance(bool invert);
    bool invertedAppearance() const;
    void setTextDirection(QProgressBar::Direction textDirection);
    QProgressBar::Direction textDirection() const;

    void setFormat(const QString &format);
    void resetFormat();
    QString format() const;

public Q_SLOTS:
    void reset();
    void setRange(int minimum, int maximum);
    void setMinimum(int minimum);
    void setMaximum(int maximum);
    void setValue(int value);
    void setOrientation(Qt::Orientation);

Q_SIGNALS:
    void valueChanged(int value);

protected:
    bool event(QEvent *e) override;
    void paintEvent(QPaintEvent *) override;
    virtual void initStyleOption(QStyleOptionProgressBar *option) const;

private:
    Q_DECLARE_PRIVATE(QProgressBar)
    Q_DISABLE_COPY(QProgressBar)
};

QT_END_NAMESPACE

#endif // QPROGRESSBAR_H
