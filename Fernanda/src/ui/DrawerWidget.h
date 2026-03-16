/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QList>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "core/Debug.h"
#include "ui/DrawerWidgetHeader.h"

namespace Fernanda {

class DrawerWidget : public QWidget
{
    Q_OBJECT

public:
    DrawerWidget(
        const QString& title,
        QWidget* content,
        QSplitter* splitter = nullptr)
        : QWidget(splitter)
        , content_(content)
        , splitter_(splitter)
    {
        setup_(title);
    }

    virtual ~DrawerWidget() override { TRACER; }

    bool isExpanded() const noexcept { return expanded_; }

private:
    QWidget* content_;
    QSplitter* splitter_;

    DrawerWidgetHeader* header_ = new DrawerWidgetHeader(this);
    bool expanded_ = false;
    int lastExpandedSize_ = -1;

    void setup_(const QString& title)
    {
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        header_->setText(title);
        content_->setParent(this);
        content_->setVisible(false);

        layout->addWidget(header_, 0);
        layout->addWidget(content_, 1);

        // Start collapsed: lock to header height
        setFixedHeight(header_->sizeHint().height());

        connect(
            header_,
            &DrawerWidgetHeader::toggled,
            this,
            &DrawerWidget::setExpanded_);
    }

    // Asks the parent QSplitter (if any) to give this widget enough room to
    // show content, taking space from the sibling above. Without this,
    // expanding would leave the content clipped at zero height until the user
    // manually drags the handle
    void requestSplitterSpace_(int headerHeight)
    {
        if (!splitter_) return;

        auto index = splitter_->indexOf(this);
        if (index < 1) return;

        auto sizes = splitter_->sizes();

        auto total = 0;
        for (auto s : sizes)
            total += s;

        // Restore last size, or fall back to roughly 1/3 of the splitter
        // (capped at 250px)
        auto target = lastExpandedSize_ > headerHeight ? lastExpandedSize_
                                                       : qMin(total / 3, 250);
        if (target <= headerHeight) return;

        auto diff = target - sizes[index];
        if (diff <= 0) return;

        sizes[static_cast<qsizetype>(index) - 1] -= diff;
        sizes[index] = target;
        splitter_->setSizes(sizes);
    }

    void releaseSplitterSpace_(int headerHeight)
    {
        if (!splitter_) return;

        auto index = splitter_->indexOf(this);
        if (index < 1) return;

        auto sizes = splitter_->sizes();

        auto freed = sizes[index] - headerHeight;
        if (freed <= 0) return;

        sizes[static_cast<qsizetype>(index) - 1] += freed;
        sizes[index] = headerHeight;
        splitter_->setSizes(sizes);
    }

private slots:
    void setExpanded_(bool expanded)
    {
        expanded_ = expanded;
        content_->setVisible(expanded);
        header_->setChecked(expanded);

        auto header_h = header_->sizeHint().height();

        if (expanded) {
            // Undo setFixedHeight: allow the splitter to resize freely. Minimum
            // is just the header so the user can shrink almost all the way down
            setMinimumHeight(header_h);
            setMaximumHeight(QWIDGETSIZE_MAX);

            // Nudge the parent splitter to allocate visible content space
            requestSplitterSpace_(header_h);
        } else {
            // Save size, lock to header-only height, then force the splitter to
            // reclaim the space immediately
            lastExpandedSize_ = height();
            setFixedHeight(header_h);
            releaseSplitterSpace_(header_h);
        }
    }
};

} // namespace Fernanda
