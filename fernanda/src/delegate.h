// delegate.h, Fernanda

#pragma once

#include "icon.h"
#include "index.h"

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
        auto option_size = QSize(rect_height, rect_height);
        auto size_height = option_size.height();
        auto size_width = option_size.width();
        auto icon_rect = QRect((rect_left - size_width + 15), rect_top, size_width, size_height);
        auto text_rect = QRect((rect_left + 16), (rect_top + 2), rect_width, rect_height);
        auto highlight_rect = QRect(0, rect_top, paneSize.width(), rect_height);
        return Geometry{ icon_rect, text_rect, highlight_rect };
    }

    bool isDirty(QString key) const
    {
        for (const auto& entry : paintEdited)
            if (key == entry)
                return true;
        return false;
    }

    const QColor highlight() const
    {
        return QColor(0, 0, 0, 33);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        auto geometry = getRectSizes(option);
        editor->setGeometry(geometry.text);
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        painter->save();
        auto geometry = getRectSizes(option);
        if (option.state & QStyle::State_MouseOver || option.state & QStyle::State_Selected)
            painter->fillRect(geometry.highlight, highlight());
        auto name = Index::name(index);
        if (Index::isDir(index))
        {
            (option.state & QStyle::State_Open)
                ? painter->drawText(geometry.icon, Icon::draw(Icon::Name::FolderOpen))
                : (Index::hasChildren(index))
                    ? painter->drawText(geometry.icon, Icon::draw(Icon::Name::Folders))
                    : painter->drawText(geometry.icon, Icon::draw(Icon::Name::Folder));
        }
        else if (Index::isFile(index))
        {
            (option.state & QStyle::State_Open)
                ? painter->drawText(geometry.icon, Icon::draw(Icon::Name::File))
                : (Index::hasChildren(index))
                    ? painter->drawText(geometry.icon, Icon::draw(Icon::Name::Files))
                    : painter->drawText(geometry.icon, Icon::draw(Icon::Name::File));
            if (isDirty(Index::key(index)))
            {
                QFont font = painter->font();
                font.setItalic(true);
                painter->setFont(font);
                name = "*" + name;
            }
        }
        else
            painter->drawText(geometry.icon, Icon::draw(Icon::Name::QuestionMark));
        painter->drawText(geometry.text, name);
        painter->restore();
    }
};

// delegate.h, Fernanda
