// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef GUITEST_H
#define GUITEST_H

#include <QAccessibleInterface>
#include <QSet>
#include <QWidget>
#include <QPainter>

QT_USE_NAMESPACE

/*
    GuiTest provides tools for:
     - navigating the Qt Widget hiearchy using the accessibilty APIs.
     - Simulating platform mouse and keybord events.
*/

class TestBase {
public:
    virtual bool operator()(QAccessibleInterface *candidate) = 0;
    virtual ~TestBase() {}
};

/*
    WidgetNavigator navigates a Qt GUI hierarchy using the QAccessibility APIs.
*/
class WidgetNavigator {
public:
    WidgetNavigator() {}
    ~WidgetNavigator();

    void printAll(QWidget *widget);
    void printAll(QAccessibleInterface *interface);

    QAccessibleInterface *find(QAccessible::Text textType, const QString &text, QWidget *start);
    QAccessibleInterface *find(QAccessible::Text textType, const QString &text, QAccessibleInterface *start);

    QAccessibleInterface *recursiveSearch(TestBase *test, QAccessibleInterface *iface);

    static QWidget *getWidget(QAccessibleInterface *interface);
private:
    QSet<QAccessibleInterface *> interfaces;
};

/*
    NativeEvents contains platform-specific code for simulating mouse and keybord events.
    (Implemented so far: mouseClick on Mac)
*/
namespace NativeEvents {
    /*
        Simulates a mouse click with button at globalPos.
    */
    void mouseClick(const QPoint &globalPos, Qt::MouseButtons buttons);
};

class ColorWidget : public QWidget
{
public:
    ColorWidget(QWidget *parent = nullptr, QColor color = QColor(Qt::red))
       : QWidget(parent), color(color) {}

    QColor color;

protected:
    void paintEvent(QPaintEvent  *)
    {
        QPainter p(this);
        p.fillRect(this->rect(), color);
    }
};

class DelayedAction : public QObject
{
Q_OBJECT
public:
    DelayedAction() : delay(0), next(0) {}
    virtual ~DelayedAction(){}
public slots:
    virtual void run();
public:
    int delay;
    DelayedAction *next;
};

class ClickLaterAction : public DelayedAction
{
Q_OBJECT
public:
    ClickLaterAction(QAccessibleInterface *interface, Qt::MouseButtons buttons = Qt::LeftButton);
    ClickLaterAction(QWidget *widget, Qt::MouseButtons buttons = Qt::LeftButton);
protected slots:
    void run();
private:
    bool useInterface;
    QAccessibleInterface *interface;
    QWidget *widget;
    Qt::MouseButtons buttons;
};

/*

*/
class GuiTester : public QObject
{
Q_OBJECT
public:
    GuiTester();
    ~GuiTester();
    enum Direction {Horizontal = 1, Vertical = 2, HorizontalAndVertical = 3};
    Q_DECLARE_FLAGS(Directions, Direction)
    bool isFilled(const QImage image, const QRect &rect, const QColor &color);
    bool isContent(const QImage image, const QRect &rect, Directions directions = HorizontalAndVertical);
protected slots:
    void exitLoopSlot();
protected:
    void clickLater(QAccessibleInterface *interface, Qt::MouseButtons buttons = Qt::LeftButton, int delay = 300);
    void clickLater(QWidget *widget, Qt::MouseButtons buttons = Qt::LeftButton, int delay = 300);

    void clearSequence();
    void addToSequence(DelayedAction *action, int delay = 0);
    void runSequence();
    WidgetNavigator wn;
private:
    QSet<DelayedAction *> actions;
    DelayedAction *startAction;
    DelayedAction *lastAction;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GuiTester::Directions)

#endif
