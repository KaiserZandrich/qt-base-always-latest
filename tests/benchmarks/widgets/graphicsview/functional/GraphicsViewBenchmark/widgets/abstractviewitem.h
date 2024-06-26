// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef ABSTRACTVIEWITEM_H
#define ABSTRACTVIEWITEM_H

#include <QModelIndex>

#include "gvbwidget.h"
#include "abstractitemview.h"
#include "listitem.h"

class QGraphicsWidget;

class AbstractViewItem : public GvbWidget
{
    Q_OBJECT
public:
    AbstractViewItem(QGraphicsWidget *parent = nullptr);
    virtual ~AbstractViewItem();

    virtual AbstractViewItem *newItemInstance() = 0;

    QModelIndex modelIndex() const;

    void setModelIndex(const QModelIndex &index);

    AbstractViewItem *prototype() const;
    AbstractItemView *itemView() const;
    void setItemView(AbstractItemView *itemView) ;
    virtual void updateItemContents();
    virtual void themeChange();

    virtual void setSubtreeCacheEnabled(bool enabled);

    virtual QSizeF effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

    virtual void setModel(QAbstractItemModel *model) = 0;
    virtual QVariant data(int role) const = 0;
    virtual void setData(const QVariant &value, int role = Qt::DisplayRole) = 0;
    virtual void setTwoColumns(const bool enabled) = 0;

protected:
    virtual bool event(QEvent *e);

    QPersistentModelIndex m_index;

private:
    Q_DISABLE_COPY(AbstractViewItem)

    AbstractItemView *m_itemView;
    AbstractViewItem *m_prototype;

};

#endif // ABSTRACTVIEWITEM_H
