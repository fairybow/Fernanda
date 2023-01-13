// colorbar.cpp, Fernanda

#include "colorbar.h"

ColorBar::ColorBar(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(bar);
    bar->setAttribute(Qt::WA_TransparentForMouseEvents);
    bar->setMaximumHeight(3);
    bar->setTextVisible(false);
    bar->setRange(0, 100);
    bar->hide();
    bar->setObjectName("colorBar");
    connect(barTimer, &QTimer::timeout, this, [&]()
        {
            bar->hide();
            bar->reset();
        });
}

void ColorBar::delayedStartUp()
{
    QTimer::singleShot(1500, this, [&]() { run(Run::Pastels); });
}

void ColorBar::toggleSelf(bool checked)
{
    hasSelf = checked;
    Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::T_ColorBar, checked);
}

void ColorBar::setAlignment(QString alignment)
{
    (alignment == "Bottom")
        ? layout->setAlignment(Qt::AlignBottom)
        : layout->setAlignment(Qt::AlignTop);
    Ud::saveConfig(Ud::ConfigGroup::Window, Ud::ConfigVal::BarAlign, alignment);
}

bool ColorBar::hasStartUp()
{
    return runOnStartUp;
}

void ColorBar::toggleStartUp(bool checked)
{
    runOnStartUp = checked;
}

void ColorBar::run(Run theme)
{
    if (theme == Run::None) return;
    if (!hasSelf) return;
    style(theme);
    auto bar_fill = new QTimeLine(125, this);
    connect(bar_fill, &QTimeLine::frameChanged, bar, &QProgressBar::setValue);
    bar_fill->setFrameRange(0, 100);
    bar->show();
    barTimer->start(1000);
    bar_fill->start();
}

void ColorBar::style(Run theme)
{
    QString style_sheet;
    switch (theme) {
    case Run::Red:
        style_sheet = Io::readFile(":/themes/bar/red.qss");
        break;
    case Run::Green:
        style_sheet = Io::readFile(":/themes/bar/green.qss");
        break;
    case Run::Pastels:
        style_sheet = Io::readFile(":/themes/bar/pastels.qss");
        break;
    }
    bar->setStyleSheet(style_sheet);
}

// colorbar.cpp, Fernanda
