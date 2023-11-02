// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

package org.qtproject.qt.android.bindings;

import android.os.Bundle;

import org.qtproject.qt.android.QtActivityBase;

public class QtActivity extends QtActivityBase
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        setAppDetails();
        super.onCreate(savedInstanceState);
    }

    private void setAppDetails()
    {
        // Use this variable to add any environment variables to your application.
        // the env vars must be separated with "\t"
        // e.g. "ENV_VAR1=1\tENV_VAR2=2\t"
        // Currently the following vars are used by the android plugin:
        // * QT_USE_ANDROID_NATIVE_DIALOGS - 1 to use the android native dialogs.
        ENVIRONMENT_VARIABLES = "QT_USE_ANDROID_NATIVE_DIALOGS=1";
    }
}
