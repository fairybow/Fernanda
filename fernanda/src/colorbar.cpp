/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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

void ColorBar::toggle(bool checked, Has has)
{
    switch (has) {
    case Has::RunOnStartUp:
        hasRunOnStartUp = checked;
        break;
    case Has::Self:
        hasSelf = checked;
        UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::ToggleColorBar, checked);
        break;
    }
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

void ColorBar::setAlignment(QString alignment)
{
    (alignment == "Bottom")
        ? layout->setAlignment(Qt::AlignBottom)
        : layout->setAlignment(Qt::AlignTop);
    UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::ColorBarAlignment, alignment);
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
