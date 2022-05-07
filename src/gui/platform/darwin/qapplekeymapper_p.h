/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef QAPPLEKEYMAPPER_H
#define QAPPLEKEYMAPPER_H

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

#ifdef Q_OS_MACOS
#include <Carbon/Carbon.h>
#endif

#include <QtCore/QList>
#include <QtGui/QKeyEvent>

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAppleKeyMapper
{
public:
    static Qt::KeyboardModifiers queryKeyboardModifiers();
    QList<int> possibleKeys(const QKeyEvent *event) const;
    static Qt::Key fromNSString(Qt::KeyboardModifiers qtMods, NSString *characters,
                                NSString *charactersIgnoringModifiers, QString &text);
#ifdef Q_OS_MACOS
    static Qt::KeyboardModifiers fromCocoaModifiers(NSEventModifierFlags cocoaModifiers);
    static NSEventModifierFlags toCocoaModifiers(Qt::KeyboardModifiers);

    static QChar toCocoaKey(Qt::Key key);
    static Qt::Key fromCocoaKey(QChar keyCode);
#else
    static Qt::Key fromUIKitKey(NSString *keyCode);
    static Qt::KeyboardModifiers fromUIKitModifiers(ulong uikitModifiers);
    static ulong toUIKitModifiers(Qt::KeyboardModifiers);
#endif
private:
#ifdef Q_OS_MACOS
    static constexpr int kNumModifierCombinations = 16;
    struct KeyMap : std::array<char32_t, kNumModifierCombinations>
    {
        // Initialize first element to a sentinel that allows us
        // to distinguish an uninitialized map from an initialized.
        // Using 0 would not allow us to map U+0000 (NUL), however
        // unlikely that is.
        KeyMap() : std::array<char32_t, 16>{Qt::Key_unknown} {}
    };

    bool updateKeyboard();

    using VirtualKeyCode = unsigned short;
    const KeyMap &keyMapForKey(VirtualKeyCode virtualKey) const;

    QCFType<TISInputSourceRef> m_currentInputSource = nullptr;

    enum { NullMode, UnicodeMode, OtherMode } m_keyboardMode = NullMode;
    const UCKeyboardLayout *m_keyboardLayoutFormat = nullptr;
    KeyboardLayoutKind m_keyboardKind = kKLKCHRuchrKind;

    mutable QHash<VirtualKeyCode, KeyMap> m_keyMap;
#endif
};

QT_END_NAMESPACE

#endif

