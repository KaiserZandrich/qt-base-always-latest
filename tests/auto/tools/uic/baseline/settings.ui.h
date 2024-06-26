/********************************************************************************
** Form generated from reading UI file 'settings.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *hboxLayout;
    QLabel *label;
    QComboBox *deviceCombo;
    QHBoxLayout *hboxLayout1;
    QLabel *label_6;
    QComboBox *audioEffectsCombo;
    QHBoxLayout *hboxLayout2;
    QLabel *crossFadeLabel;
    QVBoxLayout *vboxLayout;
    QSlider *crossFadeSlider;
    QHBoxLayout *hboxLayout3;
    QLabel *label_3;
    QSpacerItem *spacerItem;
    QLabel *label_5;
    QSpacerItem *spacerItem1;
    QLabel *label_4;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName("Dialog");
        Dialog->resize(392, 176);
        verticalLayout = new QVBoxLayout(Dialog);
        verticalLayout->setObjectName("verticalLayout");
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        label = new QLabel(Dialog);
        label->setObjectName("label");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);
        label->setMinimumSize(QSize(90, 0));
        label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        hboxLayout->addWidget(label);

        deviceCombo = new QComboBox(Dialog);
        deviceCombo->setObjectName("deviceCombo");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(deviceCombo->sizePolicy().hasHeightForWidth());
        deviceCombo->setSizePolicy(sizePolicy1);

        hboxLayout->addWidget(deviceCombo);


        verticalLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName("hboxLayout1");
        label_6 = new QLabel(Dialog);
        label_6->setObjectName("label_6");
        sizePolicy.setHeightForWidth(label_6->sizePolicy().hasHeightForWidth());
        label_6->setSizePolicy(sizePolicy);
        label_6->setMinimumSize(QSize(90, 0));
        label_6->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        hboxLayout1->addWidget(label_6);

        audioEffectsCombo = new QComboBox(Dialog);
        audioEffectsCombo->setObjectName("audioEffectsCombo");
        sizePolicy1.setHeightForWidth(audioEffectsCombo->sizePolicy().hasHeightForWidth());
        audioEffectsCombo->setSizePolicy(sizePolicy1);

        hboxLayout1->addWidget(audioEffectsCombo);


        verticalLayout->addLayout(hboxLayout1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setObjectName("hboxLayout2");
        crossFadeLabel = new QLabel(Dialog);
        crossFadeLabel->setObjectName("crossFadeLabel");
        sizePolicy.setHeightForWidth(crossFadeLabel->sizePolicy().hasHeightForWidth());
        crossFadeLabel->setSizePolicy(sizePolicy);
        crossFadeLabel->setMinimumSize(QSize(90, 0));
        crossFadeLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        hboxLayout2->addWidget(crossFadeLabel);

        vboxLayout = new QVBoxLayout();
        vboxLayout->setObjectName("vboxLayout");
        crossFadeSlider = new QSlider(Dialog);
        crossFadeSlider->setObjectName("crossFadeSlider");
        sizePolicy1.setHeightForWidth(crossFadeSlider->sizePolicy().hasHeightForWidth());
        crossFadeSlider->setSizePolicy(sizePolicy1);
        crossFadeSlider->setMinimum(-20);
        crossFadeSlider->setMaximum(20);
        crossFadeSlider->setSingleStep(1);
        crossFadeSlider->setPageStep(2);
        crossFadeSlider->setValue(0);
        crossFadeSlider->setOrientation(Qt::Horizontal);
        crossFadeSlider->setTickPosition(QSlider::TicksBelow);

        vboxLayout->addWidget(crossFadeSlider);

        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setObjectName("hboxLayout3");
        label_3 = new QLabel(Dialog);
        label_3->setObjectName("label_3");
        QFont font;
        font.setPointSize(9);
        label_3->setFont(font);

        hboxLayout3->addWidget(label_3);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout3->addItem(spacerItem);

        label_5 = new QLabel(Dialog);
        label_5->setObjectName("label_5");
        label_5->setFont(font);

        hboxLayout3->addWidget(label_5);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout3->addItem(spacerItem1);

        label_4 = new QLabel(Dialog);
        label_4->setObjectName("label_4");
        label_4->setFont(font);

        hboxLayout3->addWidget(label_4);


        vboxLayout->addLayout(hboxLayout3);


        hboxLayout2->addLayout(vboxLayout);


        verticalLayout->addLayout(hboxLayout2);

        buttonBox = new QDialogButtonBox(Dialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(Dialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, Dialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, Dialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("Dialog", "Dialog", nullptr));
        label->setText(QCoreApplication::translate("Dialog", "Audio device:", nullptr));
        label_6->setText(QCoreApplication::translate("Dialog", "Audio effect:", nullptr));
        crossFadeLabel->setText(QCoreApplication::translate("Dialog", "Cross fade:", nullptr));
        label_3->setText(QCoreApplication::translate("Dialog", "-10 Sec", nullptr));
        label_5->setText(QCoreApplication::translate("Dialog", "0", nullptr));
        label_4->setText(QCoreApplication::translate("Dialog", "10 Sec", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SETTINGS_H
