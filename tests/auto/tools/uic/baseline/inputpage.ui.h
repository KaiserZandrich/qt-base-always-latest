/********************************************************************************
** Form generated from reading UI file 'inputpage.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef INPUTPAGE_H
#define INPUTPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_InputPage
{
public:
    QGridLayout *gridLayout;
    QSpacerItem *spacerItem;
    QLabel *label;
    QHBoxLayout *hboxLayout;
    QLineEdit *fileLineEdit;
    QToolButton *browseButton;
    QSpacerItem *spacerItem1;

    void setupUi(QWidget *InputPage)
    {
        if (InputPage->objectName().isEmpty())
            InputPage->setObjectName("InputPage");
        InputPage->resize(417, 242);
        gridLayout = new QGridLayout(InputPage);
        gridLayout->setObjectName("gridLayout");
        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        gridLayout->addItem(spacerItem, 0, 2, 1, 1);

        label = new QLabel(InputPage);
        label->setObjectName("label");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label, 1, 0, 1, 1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(0);
        hboxLayout->setObjectName("hboxLayout");
        fileLineEdit = new QLineEdit(InputPage);
        fileLineEdit->setObjectName("fileLineEdit");

        hboxLayout->addWidget(fileLineEdit);

        browseButton = new QToolButton(InputPage);
        browseButton->setObjectName("browseButton");

        hboxLayout->addWidget(browseButton);


        gridLayout->addLayout(hboxLayout, 1, 1, 1, 2);

        spacerItem1 = new QSpacerItem(20, 31, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout->addItem(spacerItem1, 2, 1, 1, 1);


        retranslateUi(InputPage);

        QMetaObject::connectSlotsByName(InputPage);
    } // setupUi

    void retranslateUi(QWidget *InputPage)
    {
        InputPage->setWindowTitle(QCoreApplication::translate("InputPage", "Form", nullptr));
        label->setText(QCoreApplication::translate("InputPage", "File name:", nullptr));
        browseButton->setText(QCoreApplication::translate("InputPage", "...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class InputPage: public Ui_InputPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // INPUTPAGE_H
