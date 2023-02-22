/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Delegate.h, Fernanda

#pragma once

#include "Icon.h"
#include "Index.h"

#include <QColor>
#include <QFont>
#include <QObject>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QWidget>

class PaneDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QStringList paintEdited;
    QString paintActive;
    QSize paneSize;

private:
    struct Geometry {
        QRect icon;
        QRect text;
        QRect highlight;
    };

    const Geometry getRectSizes(const QStyleOptionViewItem& option) const
    {
        auto option_rect = option.rect;
        auto rect_height = option_rect.height();
        auto rect_width = option_rect.width();
        auto rect_top = option_rect.top();
        auto rect_left = option_rect.left();
        auto icon_square = QSize(rect_height, rect_height);
        auto icon_height = icon_square.height();
        auto icon_width = icon_square.width();
        auto icon_rect = QRect((rect_left - icon_width + 15), rect_top, icon_width, icon_height);
        auto text_rect = QRect((rect_left + 16), (rect_top + 1), (rect_width - 20), rect_height);
        auto highlight_rect = QRect(0, rect_top, paneSize.width(), rect_height);
        return Geometry{ icon_rect, text_rect, highlight_rect };
    }

    bool isDirty(const QModelIndex& index) const
    {
        for (const auto& entry : paintEdited)
            if (Index::key(index) == entry)
                return true;
        return false;
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        auto geometry = getRectSizes(option);
        editor->setGeometry(geometry.text);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        painter->save();
        QFont font = painter->font();
        auto geometry = getRectSizes(option);
        if (option.state & QStyle::State_MouseOver || option.state & QStyle::State_Selected)
            painter->fillRect(geometry.highlight, highlight());
        auto name = Index::name(index);
        if (Index::isDir(index))
        {
            (option.state & QStyle::State_Open)
                ? painter->drawText(geometry.icon, Icon::draw(Icon::Name::FolderOpen))
                : Index::hasChildren(index)
                    ? painter->drawText(geometry.icon, Icon::draw(Icon::Name::Folders))
                    : painter->drawText(geometry.icon, Icon::draw(Icon::Name::Folder));
        }
        else if (Index::isFile(index))
        {
            (option.state & QStyle::State_Open)
                ? painter->drawText(geometry.icon, Icon::draw(Icon::Name::File))
                : Index::hasChildren(index)
                    ? painter->drawText(geometry.icon, Icon::draw(Icon::Name::Files))
                    : painter->drawText(geometry.icon, Icon::draw(Icon::Name::File));
            if (isDirty(index))
            {
                font.setItalic(true);
                painter->setFont(font);
                name = "*" + name;
            }
            if (isSelected(index))
                name = "> " + name;
        }
        else
            painter->drawText(geometry.icon, Icon::draw(Icon::Name::QuestionMark));
        painter->drawText(geometry.text, name);
        painter->restore();
    }

    const QColor highlight() const { return QColor(0, 0, 0, 33); }
    bool isSelected(const QModelIndex& index) const { return (Index::key(index) == paintActive); }
};

// Delegate.h, Fernanda
