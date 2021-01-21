/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QWIDGETREPAINTMANAGER_P_H
#define QWIDGETREPAINTMANAGER_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QDebug>
#include <QtWidgets/qwidget.h>
#include <private/qwidget_p.h>
#include <QtGui/qbackingstore.h>

QT_BEGIN_NAMESPACE

class QPlatformTextureList;
class QPlatformTextureListWatcher;
class QWidgetRepaintManager;

class Q_AUTOTEST_EXPORT QWidgetRepaintManager
{
    Q_GADGET
public:
    enum UpdateTime {
        UpdateNow,
        UpdateLater
    };
    Q_ENUM(UpdateTime)

    enum BufferState{
        BufferValid,
        BufferInvalid
    };
    Q_ENUM(BufferState)

    QWidgetRepaintManager(QWidget *t);
    ~QWidgetRepaintManager();

    QBackingStore *backingStore() const { return store; }
    void setBackingStore(QBackingStore *backingStore) { store = backingStore; }

    template <class T>
    void markDirty(const T &r, QWidget *widget, UpdateTime updateTime = UpdateLater,
                   BufferState bufferState = BufferValid);

    void removeDirtyWidget(QWidget *w);

    void sync(QWidget *exposedWidget, const QRegion &exposedRegion);
    void sync();

    void markNeedsFlush(QWidget *widget, const QRegion &region, const QPoint &topLevelOffset);

    void addStaticWidget(QWidget *widget);
    void moveStaticWidgets(QWidget *reparented);
    void removeStaticWidget(QWidget *widget);
    QRegion staticContents(QWidget *widget = nullptr, const QRect &withinClipRect = QRect()) const;

    bool bltRect(const QRect &rect, int dx, int dy, QWidget *widget);

private:
    void updateLists(QWidget *widget);

    void addDirtyWidget(QWidget *widget, const QRegion &rgn);
    void resetWidget(QWidget *widget);

    void addDirtyRenderToTextureWidget(QWidget *widget);

    void sendUpdateRequest(QWidget *widget, UpdateTime updateTime);

    bool syncAllowed();
    void paintAndFlush();

    void markNeedsFlush(QWidget *widget, const QRegion &region = QRegion());

    void flush();
    void flush(QWidget *widget, const QRegion &region, QPlatformTextureList *widgetTextures);

    bool isDirty() const;

    bool hasStaticContents() const;
    void updateStaticContentsSize();

    QWidget *tlw = nullptr;
    QBackingStore *store = nullptr;

    QRegion dirty; // needsRepaint
    QVector<QWidget *> dirtyWidgets;
    QVector<QWidget *> dirtyRenderToTextureWidgets;

    QRegion topLevelNeedsFlush;
    QVector<QWidget *> needsFlushWidgets;

    QList<QWidget *> staticWidgets;

    QPlatformTextureListWatcher *textureListWatcher = nullptr;

    bool updateRequestSent = false;

    QElapsedTimer perfTime;
    int perfFrames = 0;

    Q_DISABLE_COPY_MOVE(QWidgetRepaintManager)
};

QT_END_NAMESPACE

#endif // QWIDGETREPAINTMANAGER_P_H
