/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

// This file is included from qnsview.mm, and only used to organize the code

@implementation QNSView (Keys)

- (bool)handleKeyEvent:(NSEvent *)nsevent eventType:(int)eventType
{
    ulong timestamp = [nsevent timestamp] * 1000;
    ulong nativeModifiers = [nsevent modifierFlags];
    Qt::KeyboardModifiers modifiers = QAppleKeyMapper::fromCocoaModifiers(nativeModifiers);
    NSString *charactersIgnoringModifiers = [nsevent charactersIgnoringModifiers];
    NSString *characters = [nsevent characters];
    if (m_inputSource != characters) {
        [m_inputSource release];
        m_inputSource = [characters retain];
    }

    // Scan codes are hardware dependent codes for each key. There is no way to get these
    // from Carbon or Cocoa, so leave it 0, as documented in QKeyEvent::nativeScanCode().
    const quint32 nativeScanCode = 0;

    // Virtual keys on the other hand are mapped to be the same keys on any system
    const quint32 nativeVirtualKey = nsevent.keyCode;

    QChar ch = QChar::ReplacementCharacter;
    int keyCode = Qt::Key_unknown;

    // If a dead key occurs as a result of pressing a key combination then
    // characters will have 0 length, but charactersIgnoringModifiers will
    // have a valid character in it. This enables key combinations such as
    // ALT+E to be used as a shortcut with an English keyboard even though
    // pressing ALT+E will give a dead key while doing normal text input.
    if ([characters length] != 0 || [charactersIgnoringModifiers length] != 0) {
        if (nativeModifiers & (NSEventModifierFlagControl | NSEventModifierFlagOption)
            && [charactersIgnoringModifiers length] != 0)
            ch = QChar([charactersIgnoringModifiers characterAtIndex:0]);
        else if ([characters length] != 0)
            ch = QChar([characters characterAtIndex:0]);
        keyCode = QAppleKeyMapper::fromCocoaKey(ch);
    }

    // we will send a key event unless the input method sets m_sendKeyEvent to false
    m_sendKeyEvent = true;
    QString text;
    // ignore text for the U+F700-U+F8FF range. This is used by Cocoa when
    // delivering function keys (e.g. arrow keys, backspace, F1-F35, etc.)
    if (!(modifiers & (Qt::ControlModifier | Qt::MetaModifier)) && (ch.unicode() < 0xf700 || ch.unicode() > 0xf8ff))
        text = QString::fromNSString(characters);

    QWindow *window = [self topLevelWindow];

    // Popups implicitly grab key events; forward to the active popup if there is one.
    // This allows popups to e.g. intercept shortcuts and close the popup in response.
    if (QCocoaWindow *popup = QCocoaIntegration::instance()->activePopupWindow()) {
        if (!popup->window()->flags().testFlag(Qt::ToolTip))
            window = popup->window();
    }

    qCDebug(lcQpaKeys) << "Handling" << nsevent << "as" << Qt::Key(keyCode)
        << "with" << modifiers << "and resulting text" << text;

    if (eventType == QEvent::KeyPress) {

        if (m_composingText.isEmpty()) {
            qCDebug(lcQpaKeys) << "Trying potential shortcuts in" << window;
            if (QWindowSystemInterface::handleShortcutEvent(window, timestamp, keyCode, modifiers,
                    nativeScanCode, nativeVirtualKey, nativeModifiers, text, [nsevent isARepeat], 1)) {
                qCDebug(lcQpaKeys) << "Found matching shortcut; will not send as key event";
                m_sendKeyEvent = false;
                // Handling a shortcut may result in closing the window
                if (!m_platformWindow)
                    return true;
            } else {
                qCDebug(lcQpaKeys) << "No matching shortcuts; continuing with key event delivery";
            }
        }

        QObject *focusObject = m_platformWindow->window()->focusObject();
        if (m_sendKeyEvent && focusObject) {
            if (auto queryResult = queryInputMethod(focusObject, Qt::ImHints)) {
                auto hints = static_cast<Qt::InputMethodHints>(queryResult.value(Qt::ImHints).toUInt());

                // Make sure we send dead keys and the next key to the input method for composition
                const bool ignoreHidden = (hints & Qt::ImhHiddenText) && !text.isEmpty() && !m_lastKeyDead;

                if (!(hints & Qt::ImhDigitsOnly || hints & Qt::ImhFormattedNumbersOnly || ignoreHidden)) {
                    // Pass the key event to the input method. Note that m_sendKeyEvent may be set
                    // to false during this call
                    qCDebug(lcQpaKeys) << "Interpreting key event for focus object" << focusObject;
                    m_currentlyInterpretedKeyEvent = nsevent;
                    [self interpretKeyEvents:@[nsevent]];

                    // If the receiver opens an editor in response to a key press, then the focus will change, the input
                    // method will be reset, and the first key press will be gone. If the focus object changes, then we
                    // need to pass the key event to the input method once more.
                    if (qApp->focusObject() != focusObject) {
                        qCDebug(lcQpaKeys) << "Interpreting key event again for new focus object" << qApp->focusObject();
                        [self interpretKeyEvents:@[nsevent]];
                    }

                    m_currentlyInterpretedKeyEvent = 0;

                    // If the last key we sent was dead, then pass the next
                    // key to the IM as well to complete composition.
                    m_lastKeyDead = text.isEmpty();
                }

            }
        }

        if (m_resendKeyEvent)
            m_sendKeyEvent = true;
    }

    bool accepted = true;
    if (m_sendKeyEvent && m_composingText.isEmpty()) {
        qCDebug(lcQpaKeys) << "Sending as regular key event";
        QWindowSystemInterface::handleExtendedKeyEvent(window, timestamp, QEvent::Type(eventType), keyCode, modifiers,
                                                       nativeScanCode, nativeVirtualKey, nativeModifiers, text, [nsevent isARepeat], 1, false);
        accepted = QWindowSystemInterface::flushWindowSystemEvents();
    }
    m_sendKeyEvent = false;
    m_resendKeyEvent = false;
    return accepted;
}

- (void)keyDown:(NSEvent *)nsevent
{
    if ([self isTransparentForUserInput])
        return [super keyDown:nsevent];

    const bool accepted = [self handleKeyEvent:nsevent eventType:int(QEvent::KeyPress)];

    // When Qt is used to implement a plugin for a native application we
    // want to propagate unhandled events to other native views. However,
    // Qt does not always set the accepted state correctly (in particular
    // for return key events), so do this for plugin applications only
    // to prevent incorrect forwarding in the general case.
    const bool shouldPropagate = QCoreApplication::testAttribute(Qt::AA_PluginApplication) && !accepted;

    // Track keyDown acceptance/forward state for later acceptance of the keyUp.
    if (!shouldPropagate)
        m_acceptedKeyDowns.insert([nsevent keyCode]);

    if (shouldPropagate)
        [super keyDown:nsevent];
}

- (void)keyUp:(NSEvent *)nsevent
{
    if ([self isTransparentForUserInput])
        return [super keyUp:nsevent];

    const bool keyUpAccepted = [self handleKeyEvent:nsevent eventType:int(QEvent::KeyRelease)];

    // Propagate the keyUp if neither Qt accepted it nor the corresponding KeyDown was
    // accepted. Qt text controls wil often not use and ignore keyUp events, but we
    // want to avoid propagating unmatched keyUps.
    const bool keyDownAccepted = m_acceptedKeyDowns.remove([nsevent keyCode]);
    if (!keyUpAccepted && !keyDownAccepted)
        [super keyUp:nsevent];
}

- (void)cancelOperation:(id)sender
{
    Q_UNUSED(sender);

    NSEvent *currentEvent = [NSApp currentEvent];
    if (!currentEvent || currentEvent.type != NSEventTypeKeyDown)
        return;

    // Handling the key event may recurse back here through interpretKeyEvents
    // (when IM is enabled), so we need to guard against that.
    if (currentEvent == m_currentlyInterpretedKeyEvent)
        return;

    // Send Command+Key_Period and Escape as normal keypresses so that
    // the key sequence is delivered through Qt. That way clients can
    // intercept the shortcut and override its effect.
    [self handleKeyEvent:currentEvent eventType:int(QEvent::KeyPress)];
}

- (void)flagsChanged:(NSEvent *)nsevent
{
    ulong timestamp = [nsevent timestamp] * 1000;
    ulong nativeModifiers = [nsevent modifierFlags];
    Qt::KeyboardModifiers modifiers = QAppleKeyMapper::fromCocoaModifiers(nativeModifiers);

    qCDebug(lcQpaKeys) << "Flags changed with" << nsevent
        << "resulting in" << modifiers;

    // Scan codes are hardware dependent codes for each key. There is no way to get these
    // from Carbon or Cocoa, so leave it 0, as documented in QKeyEvent::nativeScanCode().
    const quint32 nativeScanCode = 0;

    // Virtual keys on the other hand are mapped to be the same keys on any system
    const quint32 nativeVirtualKey = nsevent.keyCode;

    // calculate the delta and remember the current modifiers for next time
    static ulong m_lastKnownModifiers;
    ulong lastKnownModifiers = m_lastKnownModifiers;
    ulong delta = lastKnownModifiers ^ nativeModifiers;
    m_lastKnownModifiers = nativeModifiers;

    struct qt_mac_enum_mapper
    {
        ulong mac_mask;
        Qt::Key qt_code;
    };
    static qt_mac_enum_mapper modifier_key_symbols[] = {
        { NSEventModifierFlagShift, Qt::Key_Shift },
        { NSEventModifierFlagControl, Qt::Key_Meta },
        { NSEventModifierFlagCommand, Qt::Key_Control },
        { NSEventModifierFlagOption, Qt::Key_Alt },
        { NSEventModifierFlagCapsLock, Qt::Key_CapsLock },
        { 0ul, Qt::Key_unknown } };
    for (int i = 0; modifier_key_symbols[i].mac_mask != 0u; ++i) {
        uint mac_mask = modifier_key_symbols[i].mac_mask;
        if ((delta & mac_mask) == 0u)
            continue;

        Qt::Key qtCode = modifier_key_symbols[i].qt_code;
        if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
            if (qtCode == Qt::Key_Meta)
                qtCode = Qt::Key_Control;
            else if (qtCode == Qt::Key_Control)
                qtCode = Qt::Key_Meta;
        }
        QWindowSystemInterface::handleExtendedKeyEvent(m_platformWindow->window(),
                                                       timestamp,
                                                       (lastKnownModifiers & mac_mask) ? QEvent::KeyRelease
                                                                                       : QEvent::KeyPress,
                                                       qtCode,
                                                       modifiers ^ QAppleKeyMapper::fromCocoaModifiers(mac_mask),
                                                       nativeScanCode, nativeVirtualKey,
                                                       nativeModifiers ^ mac_mask);
    }
}

@end
