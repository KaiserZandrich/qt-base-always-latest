// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "currency.h"
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QLocale>

CurrencyWidget::CurrencyWidget()
{
    QGridLayout *l = new QGridLayout;

    currencySymbolLabel = new QLabel("Symbol:");
    currencySymbol = new QLineEdit;
    currencyISOLabel = new QLabel("ISO Code:");
    currencyISO = new QLineEdit;
    currencyNameLabel = new QLabel("Display name:");
    currencyName = new QLineEdit;
    currencyFormattingLabel = new QLabel("Currency formatting:");
    currencyFormattingValue = new QLineEdit(QString::number(1234.56, 'f', 2));
    currencyFormattingSymbolLabel = new QLabel("currency:");
    currencyFormattingSymbol = new QLineEdit;
    currencyFormatting = new QLineEdit;

    currencyFormattingValue->setFixedWidth(150);
    currencyFormattingSymbol->setFixedWidth(50);

    l->addWidget(currencySymbolLabel, 0, 0);
    l->addWidget(currencySymbol, 0, 1, 1, 4);
    l->addWidget(currencyISOLabel, 1, 0);
    l->addWidget(currencyISO, 1, 1, 1, 4);
    l->addWidget(currencyNameLabel, 2, 0);
    l->addWidget(currencyName, 2, 1, 1, 4);
    l->addWidget(currencyFormattingLabel, 3, 0);
    l->addWidget(currencyFormattingValue, 3, 1);
    l->addWidget(currencyFormattingSymbolLabel, 3, 2);
    l->addWidget(currencyFormattingSymbol, 3, 3);
    l->addWidget(currencyFormatting, 3, 4);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addLayout(l);
    v->addStretch();

    connect(currencyFormattingSymbol, SIGNAL(textChanged(QString)),
            this, SLOT(updateCurrencyFormatting()));
    connect(currencyFormattingValue, SIGNAL(textChanged(QString)),
            this, SLOT(updateCurrencyFormatting()));
}

void CurrencyWidget::updateCurrencyFormatting()
{
    QString result;
    bool ok;
    QString symbol = currencyFormattingSymbol->text();
    QString value = currencyFormattingValue->text();
    int i = value.toInt(&ok);
    if (ok) {
        result = locale().toCurrencyString(i, symbol);
    } else {
        double d = value.toDouble(&ok);
        if (ok)
            result = locale().toCurrencyString(d, symbol);
    }
    currencyFormatting->setText(result);
}

void CurrencyWidget::localeChanged(QLocale locale)
{
    setLocale(locale);
    currencySymbol->setText(locale.currencySymbol());
    currencyISO->setText(locale.currencySymbol(QLocale::CurrencyIsoCode));
    currencyName->setText(locale.currencySymbol(QLocale::CurrencyDisplayName));
    updateCurrencyFormatting();
}

