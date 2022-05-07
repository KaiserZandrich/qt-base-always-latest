/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Android port of the Qt Toolkit.
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

package org.qtproject.qt.android;

import android.content.Context;
import android.text.InputType;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

public class QtEditText extends View
{
    int m_initialCapsMode = 0;
    int m_imeOptions = 0;
    int m_inputType = InputType.TYPE_CLASS_TEXT;
    boolean m_optionsChanged = false;
    QtActivityDelegate m_activityDelegate;

    public void setImeOptions(int m_imeOptions)
    {
        if (m_imeOptions == this.m_imeOptions)
            return;
        this.m_imeOptions = m_imeOptions;
        m_optionsChanged = true;
    }

    public void setInitialCapsMode(int m_initialCapsMode)
    {
        if (m_initialCapsMode == this.m_initialCapsMode)
            return;
        this.m_initialCapsMode = m_initialCapsMode;
        m_optionsChanged = true;
    }


    public void setInputType(int m_inputType)
    {
        if (m_inputType == this.m_inputType)
            return;
        this.m_inputType = m_inputType;
        m_optionsChanged = true;
    }

    public QtEditText(Context context, QtActivityDelegate activityDelegate)
    {
        super(context);
        setFocusable(true);
        setFocusableInTouchMode(true);
        m_activityDelegate = activityDelegate;
    }
    public QtActivityDelegate getActivityDelegate()
    {
        return m_activityDelegate;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs)
    {
        outAttrs.inputType = m_inputType;
        outAttrs.imeOptions = m_imeOptions;
        outAttrs.initialCapsMode = m_initialCapsMode;
        outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_EXTRACT_UI;
        return new QtInputConnection(this);
    }

// // DEBUG CODE
//    @Override
//    protected void onDraw(Canvas canvas) {
//        canvas.drawARGB(127, 255, 0, 255);
//        super.onDraw(canvas);
//    }
}
