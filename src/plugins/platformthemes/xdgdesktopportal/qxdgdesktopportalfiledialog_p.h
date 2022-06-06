/****************************************************************************
**
** Copyright (C) 2017-2018 Red Hat, Inc
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
#ifndef QXDGDESKTOPPORTALFILEDIALOG_P_H
#define QXDGDESKTOPPORTALFILEDIALOG_P_H

#include <qpa/qplatformdialoghelper.h>
#include <QList>

QT_BEGIN_NAMESPACE

class QXdgDesktopPortalFileDialogPrivate;

class QXdgDesktopPortalFileDialog : public QPlatformFileDialogHelper
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QXdgDesktopPortalFileDialog)
public:
    enum ConditionType : uint {
        GlobalPattern = 0,
        MimeType = 1
    };
    // Filters a(sa(us))
    // Example: [('Images', [(0, '*.ico'), (1, 'image/png')]), ('Text', [(0, '*.txt')])]
    struct FilterCondition {
        ConditionType type;
        QString pattern; // E.g. '*ico' or 'image/png'
    };
    typedef QList<FilterCondition> FilterConditionList;

    struct Filter {
        QString name; // E.g. 'Images' or 'Text
        FilterConditionList filterConditions;; // E.g. [(0, '*.ico'), (1, 'image/png')] or [(0, '*.txt')]
    };
    typedef QList<Filter> FilterList;

    QXdgDesktopPortalFileDialog(QPlatformFileDialogHelper *nativeFileDialog = nullptr);
    ~QXdgDesktopPortalFileDialog();

    bool defaultNameFilterDisables() const override;
    QUrl directory() const override;
    void setDirectory(const QUrl &directory) override;
    void selectFile(const QUrl &filename) override;
    QList<QUrl> selectedFiles() const override;
    void setFilter() override;
    void selectNameFilter(const QString &filter) override;
    QString selectedNameFilter() const override;
    void selectMimeTypeFilter(const QString &filter) override;
    QString selectedMimeTypeFilter() const override;

    void exec() override;
    bool show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent) override;
    void hide() override;

private Q_SLOTS:
    void gotResponse(uint response, const QVariantMap &results);

private:
    void initializeDialog();
    void openPortal();
    bool useNativeFileDialog() const;

    QScopedPointer<QXdgDesktopPortalFileDialogPrivate> d_ptr;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QXdgDesktopPortalFileDialog::FilterCondition);
Q_DECLARE_METATYPE(QXdgDesktopPortalFileDialog::FilterConditionList);
Q_DECLARE_METATYPE(QXdgDesktopPortalFileDialog::Filter);
Q_DECLARE_METATYPE(QXdgDesktopPortalFileDialog::FilterList);

#endif // QXDGDESKTOPPORTALFILEDIALOG_P_H

