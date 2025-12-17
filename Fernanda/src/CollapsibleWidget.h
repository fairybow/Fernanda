/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

//#include <QFrame>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QString>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"

namespace Fernanda {

// Collapsible container widget with animated expand/collapse.
// Based on: https://github.com/MichaelVoelkel/qt-collapsible-section
class CollapsibleWidget : public QWidget
{
    Q_OBJECT

public:
    CollapsibleWidget(
        const QString& title,
        QWidget* content,
        QWidget* parent = nullptr,
        int animationDuration = 0)
        : QWidget(parent)
        , content_(content)
        , title_(title)
        , animationDuration_(animationDuration)
    {
        setup_();
    }

    virtual ~CollapsibleWidget() override { TRACER; }

    bool isExpanded() const { return expanded_; }

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
    }

private:
    QWidget* content_;
    QString title_;
    int animationDuration_;

    int itemCount_ = -1; // -1 means don't show count
    bool expanded_ = false;

    QPushButton* header_ = nullptr;
    //QFrame* separator_ = nullptr;
    QParallelAnimationGroup* animation_ = nullptr;
    int collapsedHeight_ = 0;

    void setup_()
    {
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Header button
        header_ = new QPushButton(this);
        // header_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        // header_->setArrowType(Qt::RightArrow);
        header_->setCheckable(true);
        header_->setChecked(false);
        header_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        header_->setStyleSheet(
            "QPushButton { text-align: left; border: none; }");
        updateHeaderText_();

        // Separator line
        //separator_ = new QFrame(this);
        //separator_->setFrameShape(QFrame::HLine);
        //separator_->setFrameShadow(QFrame::Sunken);

        // Content
        content_->setParent(this);
        content_->setVisible(false);

        layout->addWidget(header_);
        //layout->addWidget(separator_);
        layout->addWidget(content_);

        // Animation setup
        setupAnimation_();

        connect(
            header_,
            &QPushButton::toggled,
            this,
            &CollapsibleWidget::toggle_);
    }

    void setupAnimation_()
    {
        if (animationDuration_ <= 0) return;

        animation_ = new QParallelAnimationGroup(this);

        auto widget_anim = new QPropertyAnimation(this, "maximumHeight", this);
        auto content_anim =
            new QPropertyAnimation(content_, "maximumHeight", this);

        animation_->addAnimation(widget_anim);
        animation_->addAnimation(content_anim);

        // Hide content after collapse animation finishes
        connect(animation_, &QParallelAnimationGroup::finished, this, [this] {
            if (!expanded_) content_->setVisible(false);
        });

        // Start collapsed
        content_->setMaximumHeight(0);
        collapsedHeight_ = sizeHint().height();
    }

    void updateHeaderText_()
    {
        if (itemCount_ >= 0)
            header_->setText(QString("%1 (%2)").arg(title_).arg(itemCount_));
        else
            header_->setText(title_);
    }

    void toggle_(bool expanded)
    {
        expanded_ = expanded;
        //header_->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);

        if (header_->isChecked() != expanded) header_->setChecked(expanded);

        if (animation_) {
            if (expanded)
                content_->setVisible(true); // Show before animating open

            auto content_height = content_->sizeHint().height();

            for (int i = 0; i < animation_->animationCount(); ++i) {
                auto anim = qobject_cast<QPropertyAnimation*>(
                    animation_->animationAt(i));
                if (!anim) continue;

                anim->setDuration(animationDuration_);

                if (anim->targetObject() == this) {
                    anim->setStartValue(collapsedHeight_);
                    anim->setEndValue(collapsedHeight_ + content_height);
                } else {
                    anim->setStartValue(0);
                    anim->setEndValue(content_height);
                }
            }

            animation_->setDirection(
                expanded ? QAbstractAnimation::Forward
                         : QAbstractAnimation::Backward);
            animation_->start();
        } else {
            content_->setVisible(expanded);
        }
    }
};

} // namespace Fernanda
