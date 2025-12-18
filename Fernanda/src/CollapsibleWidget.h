/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QFrame>
#include <QPushButton>
#include <QString>
#include <QToolButton> /// *
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

// TODO: Collapse if dragging downward and the widget can't shrink any more?

// Collapsible container widget
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

    /*bool isExpanded() const { return expanded_; }

    void setExpanded(bool expanded)
    {
        if (expanded_ == expanded) return;
        toggle_(expanded);
    }

    void setTitle(const QString& title)
    {
        title_ = title;
        updateHeaderText_();
    }

    void setItemCount(int count)
    {
        itemCount_ = count;
        updateHeaderText_();
    }*/

private:
    QWidget* content_;
    QString title_;

    QToolButton* header_ = nullptr; /// *
    // QPushButton* header_ = nullptr;
    int collapsedHeight_ = 0;
    int itemCount_ = -1; // -1 means don't show count

    void setup_()
    {
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // header_ = new QPushButton(this);
        header_ = new QToolButton(this); /// *
        header_->setStyleSheet("QToolButton {border: none;}"); /// *
        header_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); /// *

        header_->setCheckable(true);
        header_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        // header_->setStyleSheet(
        //"QPushButton { text-align: left; border: none; }");
        updateHeaderText_();

        content_->setParent(this);
        setContentExpanded_(false);

        layout->addWidget(header_);
        layout->addWidget(content_);

        connect(
            header_,
            &QPushButton::toggled,
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
        header_->setChecked(expanded);
        header_->setArrowType(
            expanded ? Qt::DownArrow
                     : Qt::RightArrow); /// * (these are very ugly)
        content_->setMaximumHeight(
            expanded ? content_->sizeHint().height() : 0);
    }
};

} // namespace Fernanda
