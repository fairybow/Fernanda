/*
 * Hearth — a plain-text-first workbench for creative writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QColor>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QSizePolicy>
#include <QString>
#include <QWidget>

#include <Coco/Path.h>

#include "core/Debug.h"
#include "core/Tr.h"
#include "menus/MenuBuilder.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

class NotebookColorChip : public QWidget
{
    Q_OBJECT

public:
    NotebookColorChip(const Coco::Path& fnx, QWidget* parent = nullptr)
        : QWidget(parent)
        , fnx_(fnx)
    {
        setup_();
    }

    virtual ~NotebookColorChip() override { TRACER; }

    Coco::Path fnx() const noexcept { return fnx_; }

    void setFnx(const Coco::Path& fnx)
    {
        if (fnx_ == fnx) return;
        fnx_ = fnx;
        updateDerivedProperties_();
    }

    QColor chipColor() const noexcept
    {
        return chipColorOverride_.isValid() ? chipColorOverride_
                                            : generatedColor_;
    }

    void setChipColor(const QColor& color)
    {
        chipColorOverride_ = color;
        update();
    }

    QColor textColor() const noexcept
    {
        return textColorOverride_.isValid() ? textColorOverride_
                                            : generatedColor_.lighter(180);
    }

    void setTextColor(const QColor& color)
    {
        textColorOverride_ = color;
        update();
    }

signals:
    void colorChanged();

protected:
    virtual void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        auto chip_rect = rect().adjusted(1, 1, -1, -1);
        auto radius = chip_rect.height() / 2.0;

        // Pill background
        QPainterPath path{};
        path.addRoundedRect(QRectF(chip_rect), radius, radius);
        painter.fillPath(path, chipColor());

        // Text
        QFont font = painter.font();
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(textColor());
        painter.drawText(chip_rect, Qt::AlignCenter, name_);
    }

    virtual void contextMenuEvent(QContextMenuEvent* event) override
    {
        MenuBuilder(MenuBuilder::ContextMenu, this)
            .action(Tr::chipColor())
            .onUserTrigger(
                this,
                [this] { pickColor_(chipColor(), &chipColorOverride_); })
            .action(Tr::chipTextColor())
            .onUserTrigger(
                this,
                [this] { pickColor_(textColor(), &textColorOverride_); })
            .popup(event->globalPos());
    }

private:
    Coco::Path fnx_;

    QString name_{};
    QColor generatedColor_{};
    QColor chipColorOverride_{};
    QColor textColorOverride_{};

    inline static constexpr auto PADDING_H = 12;
    inline static constexpr auto PADDING_V = 2;

    static QColor colorFromName_(const QString& name)
    {
        static constexpr auto salt = 90u;

        auto hash = qHash(name) ^ salt;

        // Mix bits for better distribution (xorshift-style)
        hash ^= (hash << 13);
        hash ^= (hash >> 7);
        hash ^= (hash << 17);

        auto hue = static_cast<int>(hash % 360);
        auto sat = 140 + static_cast<int>((hash >> 8) % 60); // 140-199
        auto val = 130 + static_cast<int>((hash >> 16) % 70); // 130-199

        return QColor::fromHsv(hue, sat, val);
    }

    void setup_()
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        updateDerivedProperties_();
    }

    void updateDerivedProperties_()
    {
        setToolTip(fnx_.prettyQString());

        // TODO: Common method w/ TextFileModel::onDocContentsChanged
        auto fnx_name = fnx_.nameQString();
        auto new_name = fnx_name.left(27);
        if (fnx_name.length() > 27) new_name += u"..."_s;
        name_ = new_name;

        generatedColor_ = colorFromName_(name_);

        QFontMetrics metrics(font());
        auto text_width = metrics.horizontalAdvance(name_);
        auto text_height = metrics.height();

        setFixedSize(
            text_width + (PADDING_H * 2),
            text_height + (PADDING_V * 2));

        update();
    }

    void pickColor_(const QColor& current, QColor* targetMember)
    {
        QColor previous = *targetMember;
        auto dialog = new QColorDialog(current, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);

        connect(
            dialog,
            &QColorDialog::currentColorChanged,
            this,
            [this, targetMember](const QColor& color) {
                *targetMember = color;
                update();
            });

        connect(
            dialog,
            &QDialog::finished,
            this,
            [this, targetMember, previous](int result) {
                if (result == QDialog::Accepted) {
                    emit colorChanged();
                } else {
                    *targetMember = previous;
                }

                update();
            });

        dialog->open();
    }
};

} // namespace Fernanda
