// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QScreen>
#include <QStyle>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWindow>

// Format enumeration value and strip off the class name
// added by QDebug: "QStyle::StandardPixmap(SP_Icon)" -> "SP_Icon".
template <typename Enum>
static inline QString formatEnumValue(Enum value)
{
    QString result;
    QDebug(&result) << value;
    int index = result.indexOf(QLatin1Char('('));
    if (index > 0) { // "QStyle::StandardPixmap(..)".
        result.remove(0, index + 1);
        index = result.lastIndexOf(QLatin1Char(')'));
        if (index > 0)
            result.truncate(index);
    }
    return result;
}

static QTextStream &operator<<(QTextStream &str, const QSize &s)
{
    str << s.width();
    if (s.width() != s.height())
        str << 'x' << s.height();
    return str;
}

struct StyleIconEntry
{
    QString name;
    QStyle::StandardPixmap pixmap;
};

static bool operator<(const StyleIconEntry &e1, const StyleIconEntry &e2)
{
    return e1.name < e2.name;
}

static QList<StyleIconEntry> styleIconEntries()
{
    QList<StyleIconEntry> result;
    const int count = int(QStyle::SP_LineEditClearButton) + 1;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        QStyle::StandardPixmap px = static_cast<QStyle::StandardPixmap>(i);
        result.append({formatEnumValue(px), px});
    }
    std::sort(result.begin(), result.end());
    return result;
}

class IconDisplayWidget : public QGroupBox
{
public:
    explicit IconDisplayWidget(const QString &title, const QString &text,
                               const QPixmap &pixmap, QWidget *p = nullptr);
};

IconDisplayWidget::IconDisplayWidget(const QString &title, const QString &text,
                                     const QPixmap &pixmap, QWidget *p)
    : QGroupBox(title, p)
{
    QHBoxLayout *hLayout = new QHBoxLayout(this);
    hLayout->addWidget(new QLabel(text, this));
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(pixmap);
    hLayout->addWidget(iconLabel);
}

static IconDisplayWidget *createStandardPixmapDisplay(const StyleIconEntry &e, QWidget *parent)
{
    QPixmap pixmap = parent->style()->standardPixmap(e.pixmap, nullptr, parent);
    QString description;
    QTextStream str(&description);
    str << pixmap.size();
    return new IconDisplayWidget(e.name, description, pixmap, parent);
}

enum : int { maxColumns = 6 };

// Display pixmaps returned by QStyle::standardPixmap() in a grid.
static QWidget *createStandardPixmapPage(QWidget *parent)
{
    QWidget *result = new QWidget(parent);
    QGridLayout *grid = new QGridLayout(result);
    int row = 0;
    int column = 0;
    QList<StyleIconEntry> entries = styleIconEntries();
    for (int i = 0, size = entries.size(); i < size; ++i) {
        grid->addWidget(createStandardPixmapDisplay(entries.at(i), parent), row, column++);
        if (column >= maxColumns) {
            ++row;
            column = 0;
        }
    }
    return result;
}

// Display icons returned by QStyle::standardIcon() in a grid.
static IconDisplayWidget *createStandardIconDisplay(const StyleIconEntry &e,
                                                    const QSize &displaySize,
                                                    QWidget *parent)
{
    QIcon icon = parent->style()->standardIcon(e.pixmap, nullptr, parent);
    QString description;
    QTextStream str(&description);
    auto availableSizes = icon.availableSizes();
    std::sort(availableSizes.begin(), availableSizes.end(),
              [](QSize s1, QSize s2) { return s1.width() < s2.width(); });
    for (int i =0; i < availableSizes.size(); ++i) {
        if (i)
            str << ',';
        str << availableSizes.at(i);
    }
    return new IconDisplayWidget(e.name, description, icon.pixmap(displaySize), parent);
}

static QWidget *createStandardIconPage(QWidget *parent)
{
    QWidget *result = new QWidget(parent);
    QGridLayout *grid = new QGridLayout(result);
    int row = 0;
    int column = 0;
    const int largeIconSize = parent->style()->pixelMetric(QStyle::PM_LargeIconSize);
    const QSize displaySize(largeIconSize, largeIconSize);
    QList<StyleIconEntry> entries = styleIconEntries();
    for (int i = 0, size = entries.size(); i < size; ++i) {
        grid->addWidget(createStandardIconDisplay(entries.at(i), displaySize, parent), row, column++);
        if (column >= maxColumns) {
            ++row;
            column = 0;
        }
    }
    return result;
}

// Display values returned by QStyle::pixelMetric().
static QWidget *createMetricsPage(QWidget *parent)
{
    QPlainTextEdit *result = new QPlainTextEdit(parent);
    QString text;
    QTextStream str(&text);
    for (int i = 0; i <= int(QStyle::PM_HeaderDefaultSectionSizeVertical); ++i) {
        const QStyle::PixelMetric m = static_cast<QStyle::PixelMetric>(i);
        str << formatEnumValue(m) << '(' << int(m) << ")="
            << result->style()->pixelMetric(m, nullptr, result) << '\n';
    }
    result->setPlainText(text);
    return result;
}

// Display values returned by QStyle::styleHint()
static QWidget *createHintsPage(QWidget *parent)
{
    QPlainTextEdit *result = new QPlainTextEdit(parent);
    QString text;
    QTextStream str(&text);
    for (int i = 0; i <= int(QStyle::SH_Menu_SubMenuDontStartSloppyOnLeave); ++i) {
        const QStyle::StyleHint h = static_cast<QStyle::StyleHint>(i);
        str << formatEnumValue(h) << '(' << int(h) << ")="
            << result->style()->styleHint(h, nullptr, result) << '\n';
    }
    result->setPlainText(text);
    return result;
}

// Display palette colors
static QWidget *createColorsPage(QWidget *parent)
{
    QWidget *result = new QWidget(parent);
    QGridLayout *grid = new QGridLayout;
    const QPalette palette = QGuiApplication::palette();
    for (int r = 0; r < int(QPalette::NColorRoles); ++r) {
        const QPalette::ColorRole role = static_cast<QPalette::ColorRole>(r);
        const QString description =
                formatEnumValue(role) + QLatin1Char('(') + QString::number(r)
                + QLatin1String(") ") + palette.color(QPalette::Active, role).name(QColor::HexArgb);
        grid->addWidget(new QLabel(description), r, 0);
        int col = 1;
        for (int g : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            const QColor color = palette.color(QPalette::ColorGroup(g), role);
            if (color.isValid()) {
                QLabel *displayLabel = new QLabel;
                QPixmap pixmap(20, 20);
                pixmap.fill(color);
                displayLabel->setPixmap(pixmap);
                displayLabel->setFrameShape(QFrame::Box);
                grid->addWidget(displayLabel, r, col);
            }
            ++col;
        }
    }
    QHBoxLayout *hBox = new QHBoxLayout;
    hBox->addLayout(grid);
    hBox->addStretch();
    QVBoxLayout *vBox = new QVBoxLayout(result);
    vBox->addLayout(hBox);
    vBox->addStretch();
    return result;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

public slots:
    void updateDescription();

private:
    QTabWidget *m_tabWidget;
    QLabel *m_descriptionLabel;
};

MainWindow::MainWindow()
    : m_tabWidget(new QTabWidget)
    , m_descriptionLabel(new QLabel)
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *a = fileMenu->addAction("Quit", this, &QWidget::close);
    a->setShortcut(Qt::CTRL | Qt::Key_Q);

    QWidget *central = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->addWidget(m_descriptionLabel);
    mainLayout->addWidget(m_tabWidget);
    m_tabWidget->addTab(createStandardIconPage(m_tabWidget), "Standard Icon");
    m_tabWidget->addTab(createStandardPixmapPage(m_tabWidget), "Standard Pixmaps");
    m_tabWidget->addTab(createHintsPage(m_tabWidget), "Hints");
    m_tabWidget->addTab(createMetricsPage(m_tabWidget), "Pixel Metrics");
    m_tabWidget->addTab(createColorsPage(m_tabWidget), "Colors");
    setCentralWidget(central);

    setWindowTitle(QLatin1String("Style Tester (Qt") + QLatin1String(QT_VERSION_STR)
                   + QLatin1String(", ") + style()->objectName() + QLatin1Char(')'));
}

void MainWindow::updateDescription()
{
    QString text;
    QTextStream str(&text);
    str << "Qt " << QT_VERSION_STR << ", platform: " << QGuiApplication::platformName()
        << ", Style: \"" << style()->objectName() << "\", DPR=" << devicePixelRatio()
        << ' ' << logicalDpiX() << ',' << logicalDpiY() << "DPI";
    if (const QWindow *w = windowHandle())
        str << ", Screen: \"" << w->screen()->name() << '"';
    m_descriptionLabel->setText(text);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow mw;
    mw.show();
    mw.updateDescription();
    QObject::connect(mw.windowHandle(), &QWindow::screenChanged,
                     &mw, &MainWindow::updateDescription);
    return app.exec();
}

#include "main.moc"
