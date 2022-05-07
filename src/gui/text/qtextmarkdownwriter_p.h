/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QTEXTMARKDOWNWRITER_P_H
#define QTEXTMARKDOWNWRITER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/QTextStream>

#include "qtextdocument_p.h"
#include "qtextdocumentwriter.h"

QT_BEGIN_NAMESPACE

class QAbstractItemModel;

class Q_GUI_EXPORT QTextMarkdownWriter
{
public:
    QTextMarkdownWriter(QTextStream &stream, QTextDocument::MarkdownFeatures features);
    bool writeAll(const QTextDocument *document);
#if QT_CONFIG(itemmodel)
    void writeTable(const QAbstractItemModel *table);
#endif

    int writeBlock(const QTextBlock &block, bool table, bool ignoreFormat, bool ignoreEmpty);
    void writeFrame(const QTextFrame *frame);

private:
    struct ListInfo {
        bool loose;
    };

    ListInfo listInfo(QTextList *list);

private:
    QTextStream &m_stream;
    QTextDocument::MarkdownFeatures m_features;
    QMap<QTextList *, ListInfo> m_listInfo;
    QString m_linePrefix;
    QString m_codeBlockFence;
    int m_wrappedLineIndent = 0;
    int m_lastListIndent = 1;
    bool m_doubleNewlineWritten = false;
    bool m_indentedCodeBlock = false;
    bool m_fencedCodeBlock = false;
};

QT_END_NAMESPACE

#endif // QTEXTMARKDOWNWRITER_P_H
