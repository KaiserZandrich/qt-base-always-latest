// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CHAT_H
#define CHAT_H

#include <QStringList>

#include "ui_chatmainwindow.h"

class ChatMainWindow: public QMainWindow, Ui::ChatMainWindow
{
    Q_OBJECT
    QString m_nickname;
    QStringList m_messages;
public:
    ChatMainWindow();
    ~ChatMainWindow();

    void rebuildHistory();

signals:
    void message(const QString &nickname, const QString &text);
    void action(const QString &nickname, const QString &text);

private slots:
    void messageSlot(const QString &nickname, const QString &text);
    void actionSlot(const QString &nickname, const QString &text);
    void textChangedSlot(const QString &newText);
    void sendClickedSlot();
    bool changeNickname(bool initial = false);
    void aboutQt();
    void exiting();
};

#endif // CHAT_H
