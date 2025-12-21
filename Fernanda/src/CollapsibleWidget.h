/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QObject>
#include <QSizePolicy>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

// Expandable/collapsible section with a toggle header and content area. When
// collapsed, only the header is visible (content max height = 0). When
// expanded, content can grow freely within available space. Optionally displays
// an item count (not automatic) in the header: "Title (N)". Designed for use
// within AccordionWidget, which manages space distribution across multiple
// CollapsibleWidgets.
//
// Based on: https://github.com/MichaelVoelkel/qt-collapsible-section
class CollapsibleWidget : public QWidget
{
    Q_OBJECT

public:
    CollapsibleWidget(
        const QString& title,
        QWidget* content,
        QWidget* parent = nullptr)
        : QWidget(parent)
        , content_(content)
        , title_(title)
    {
        setup_();
    }

    virtual ~CollapsibleWidget() override { TRACER; }

    bool isExpanded() const noexcept { return expanded_; }
    int itemCount() const noexcept { return itemCount_; }

    void setItemCount(int count)
    {
        itemCount_ = count;
        updateHeaderText_();
    }

signals:
    void expandedChanged(bool expanded);

private:
    QWidget* content_;
    QString title_;

    QToolButton* header_ = nullptr;
    int itemCount_ = -1; // -1 means don't show count
    bool expanded_ = false;

    void setup_()
    {
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        header_ = new QToolButton(this);
        header_->setStyleSheet("QToolButton {border: none;}");
        header_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        header_->setCheckable(true);
        header_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateHeaderText_();

        content_->setParent(this);
        setContentExpanded_(false);

        layout->addWidget(header_, 0);
        layout->addWidget(content_, 1);

        connect(
            header_,
            &QToolButton::toggled,
            this,
            &CollapsibleWidget::setContentExpanded_);
    }

    void updateHeaderText_()
    {
        if (itemCount_ >= 0)
            header_->setText(QString("%1 (%2)").arg(title_).arg(itemCount_));
        else
            header_->setText(title_);
    }

private slots:
    void setContentExpanded_(bool expanded)
    {
        expanded_ = expanded;
        header_->setChecked(expanded);
        header_->setArrowType(
            expanded ? Qt::DownArrow
                     : Qt::RightArrow); // TODO: these are very ugly

        content_->setMaximumHeight(expanded ? QWIDGETSIZE_MAX : 0);
        emit expandedChanged(expanded);
    }
};

} // namespace Fernanda
