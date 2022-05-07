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

#ifndef QWINDOWSOLE_H
#define QWINDOWSOLE_H

#include "qwindowscombase.h"
#include <QtCore/qt_windows.h>

#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qpointer.h>

#include <objidl.h>

QT_BEGIN_NAMESPACE

class QMimeData;
class QWindow;

class QWindowsOleDataObject : public QWindowsComBase<IDataObject>
{
public:
    explicit QWindowsOleDataObject(QMimeData *mimeData);
    ~QWindowsOleDataObject() override;

    void releaseQt();
    QMimeData *mimeData() const;
    DWORD reportedPerformedEffect() const;

    // IDataObject methods
    STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium) override;
    STDMETHOD(GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium) override;
    STDMETHOD(QueryGetData)(LPFORMATETC pformatetc) override;
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut) override;
    STDMETHOD(SetData)(LPFORMATETC pformatetc, STGMEDIUM FAR *pmedium, BOOL fRelease) override;
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC FAR *ppenumFormatEtc) override;
    STDMETHOD(DAdvise)
    (FORMATETC FAR *pFormatetc, DWORD advf, LPADVISESINK pAdvSink,
     DWORD FAR *pdwConnection) override;
    STDMETHOD(DUnadvise)(DWORD dwConnection) override;
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA FAR *ppenumAdvise) override;

private:
    QPointer<QMimeData> data;
    const int CF_PERFORMEDDROPEFFECT;
    DWORD performedEffect = DROPEFFECT_NONE;
};

class QWindowsOleEnumFmtEtc : public QWindowsComBase<IEnumFORMATETC>
{
public:
    explicit QWindowsOleEnumFmtEtc(const QList<FORMATETC> &fmtetcs);
    explicit QWindowsOleEnumFmtEtc(const QList<LPFORMATETC> &lpfmtetcs);
    ~QWindowsOleEnumFmtEtc() override;

    bool isNull() const;

    // IEnumFORMATETC methods
    STDMETHOD(Next)(ULONG celt, LPFORMATETC rgelt, ULONG FAR *pceltFetched) override;
    STDMETHOD(Skip)(ULONG celt) override;
    STDMETHOD(Reset)(void) override;
    STDMETHOD(Clone)(LPENUMFORMATETC FAR *newEnum) override;

private:
    bool copyFormatEtc(LPFORMATETC dest, const FORMATETC *src) const;

    ULONG m_nIndex = 0;
    QList<LPFORMATETC> m_lpfmtetcs;
    bool m_isNull = false;
};

QT_END_NAMESPACE

#endif // QWINDOWSOLE_H
