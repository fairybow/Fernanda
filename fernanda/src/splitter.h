/*  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

// splitter.h, Fernanda

#pragma once

#include "userdata.h"

#include <QByteArray>
#include <QSplitter>
#include <QVector>
#include <QWidget>

class Splitter : public QSplitter
{
    Q_OBJECT

public:
    Splitter(QWidget* parent = nullptr)
    {
        setObjectName("splitter");
        connect(this, &QSplitter::splitterMoved, this, [&]()
            {
                UserData::saveConfig(UserData::IniGroup::Window, UserData::IniValue::SplitterPosition, saveState());
            });
    }

    void addWidgets(QVector<QWidget*> widgets)
    {
        for (auto& widget : widgets)
            addWidget(widget);
        setCollapsible(0, true);
        auto editor = 1;
        setCollapsible(editor, false);
        setStretchFactor(editor, 1);
        auto preview = 2;
        setCollapsible(preview, true);
        setStretchFactor(preview, 1);
    }

    void loadConfig(QRect geometry)
    {
        auto state = UserData::loadConfig(UserData::IniGroup::Window, UserData::IniValue::SplitterPosition, QVariant()).toByteArray();
        if (state.isEmpty() || state.isNull())
            setSizes(QVector<int>{ static_cast<int>(geometry.width() * 0.2), static_cast<int>(geometry.width() * 0.8) });
        else
            restoreState(state);
    }
};

// splitter.h, Fernanda
