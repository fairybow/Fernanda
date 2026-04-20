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
#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QSet>
#include <QSize>
#include <QString>
#include <QWidget>

#include "core/Debug.h"
#include "ui/Icons.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

class StyleContext : public QObject
{
    Q_OBJECT

public:
    static constexpr auto PROPERTY_KEY = "_styleContext";

    explicit StyleContext(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual ~StyleContext() override { TRACER; }

    // Static API for widgets

    static QPixmap icon(QWidget* widget, UiIcon type, const QSize& size)
    {
        auto context = forWidget_(widget);
        if (!context) return {};
        return context->icon_(type, size, widget->devicePixelRatio());
    }

    // TODO: Any other colors for widgets/things we want to be style-aware that
    // can't accept QSS (block cursor color, perhaps)

    // StyleModule API

    // TODO: Clearer name?
    void attach(QWidget* window)
    {
        if (!window) return;

        trackRequester_(window);
        window->setProperty(PROPERTY_KEY, qVar(this));
    }

    static QColor defaultIconColor() { return DEFAULT_ICON_COLOR_; }

    void setIconColor(const QColor& color)
    {
        if (iconColor_ == color) return;
        iconColor_ = color;
        iconCache_.clear();
        updateRequesters_();
    }

    // Other setters...

private:
    static constexpr auto DEFAULT_ICON_COLOR_ = "#404040";
    QColor iconColor_{};
    mutable QHash<QString, QPixmap> iconCache_{};
    mutable QSet<QWidget*> requesters_{};

    // TODO: Better/clearer name?
    static StyleContext* forWidget_(QWidget* widget)
    {
        if (!widget) return nullptr;
        auto window = widget->window();
        if (!window) return nullptr;

        return window->property(PROPERTY_KEY).value<StyleContext*>();
    }

    void trackRequester_(QWidget* widget) const
    {
        if (!widget || requesters_.contains(widget)) return;

        requesters_ << widget;
        connect(widget, &QObject::destroyed, this, [this, widget] {
            requesters_.remove(widget);
        });
    }

    void updateRequesters_()
    {
        for (auto& requester : requesters_) {
            if (requester) requester->update();
        }
    }

    QString iconCacheKey_(UiIcon type, const QSize& size, qreal dpr) const
    {
        return u"%1_%2x%3@%4_%5"_s.arg(static_cast<int>(type))
            .arg(size.width())
            .arg(size.height())
            .arg(dpr)
            .arg(iconColor_.name());
    }

    QPixmap icon_(UiIcon type, const QSize& size, qreal dpr) const
    {
        if (!size.isValid()) return {};

        auto key = iconCacheKey_(type, size, dpr);
        if (auto it = iconCache_.find(key); it != iconCache_.end()) {
            return it.value();
        }

        auto pixmap = Icons::get(type, size, iconColor_, dpr);
        if (!pixmap.isNull()) iconCache_[key] = pixmap;
        return pixmap;
    }
};

} // namespace Fernanda
