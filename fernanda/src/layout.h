/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// layout.h, Fernanda

#pragma once

#include <QStackedLayout>
#include <QVector>
#include <QWidget>

namespace Layout
{
    inline QWidget* stackWidgets(QVector<QWidget*> widgets, QWidget* parent = nullptr)
    {
        QWidget* container = new QWidget(parent);
        QStackedLayout* stack_layout = new QStackedLayout(container);
        stack_layout->setStackingMode(QStackedLayout::StackAll);
        for (auto& widget : widgets)
            stack_layout->addWidget(widget);
        return container;
    }

    inline QStackedLayout* stackLayout(QVector<QWidget*> widgets, QWidget* parent = nullptr)
    {
        QStackedLayout* stack_layout = new QStackedLayout(parent);
        stack_layout->setStackingMode(QStackedLayout::StackAll);
        for (auto& widget : widgets)
            stack_layout->addWidget(widget);
        return stack_layout;
    }
}

// layout.h, Fernanda
