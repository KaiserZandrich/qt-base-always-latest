/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// This file was generated by qlalr - DO NOT EDIT!
#ifndef QXMLSTREAMGRAMMAR_P_H
#define QXMLSTREAMGRAMMAR_P_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QXmlStreamGrammar
{
public:
    enum VariousConstants {
        EOF_SYMBOL = 0,
        AMPERSAND = 5,
        ANY = 41,
        ATTLIST = 31,
        BANG = 25,
        CDATA = 47,
        CDATA_START = 28,
        COLON = 17,
        COMMA = 19,
        DASH = 20,
        DBLQUOTE = 8,
        DIGIT = 27,
        DOCTYPE = 29,
        DOT = 23,
        ELEMENT = 30,
        EMPTY = 40,
        ENTITIES = 51,
        ENTITY = 32,
        ENTITY_DONE = 45,
        EQ = 14,
        ERROR = 43,
        FIXED = 39,
        HASH = 6,
        ID = 48,
        IDREF = 49,
        IDREFS = 50,
        IMPLIED = 38,
        LANGLE = 3,
        LBRACK = 9,
        LETTER = 26,
        LPAREN = 11,
        NDATA = 36,
        NMTOKEN = 52,
        NMTOKENS = 53,
        NOTATION = 33,
        NOTOKEN = 1,
        PARSE_ENTITY = 44,
        PCDATA = 42,
        PERCENT = 15,
        PIPE = 13,
        PLUS = 21,
        PUBLIC = 35,
        QUESTIONMARK = 24,
        QUOTE = 7,
        RANGLE = 4,
        RBRACK = 10,
        REQUIRED = 37,
        RPAREN = 12,
        SEMICOLON = 18,
        SHIFT_THERE = 56,
        SLASH = 16,
        SPACE = 2,
        STAR = 22,
        SYSTEM = 34,
        UNRESOLVED_ENTITY = 46,
        VERSION = 55,
        XML = 54,

        ACCEPT_STATE = 416,
        RULE_COUNT = 270,
        STATE_COUNT = 427,
        TERMINAL_COUNT = 57,
        NON_TERMINAL_COUNT = 84,

        GOTO_INDEX_OFFSET = 427,
        GOTO_INFO_OFFSET = 1030,
        GOTO_CHECK_OFFSET = 1030
    };

    static const char *const     spell[];
    static const short             lhs[];
    static const short             rhs[];
    static const short    goto_default[];
    static const short  action_default[];
    static const short    action_index[];
    static const short     action_info[];
    static const short    action_check[];

    static inline int nt_action (int state, int nt)
    {
        const int yyn = action_index [GOTO_INDEX_OFFSET + state] + nt;
        if (yyn < 0 || action_check [GOTO_CHECK_OFFSET + yyn] != nt)
            return goto_default [nt];

        return action_info [GOTO_INFO_OFFSET + yyn];
    }

    static inline int t_action (int state, int token)
    {
        const int yyn = action_index [state] + token;

        if (yyn < 0 || action_check [yyn] != token)
            return - action_default [state];

        return action_info [yyn];
    }
};


QT_END_NAMESPACE
#endif // QXMLSTREAMGRAMMAR_P_H

