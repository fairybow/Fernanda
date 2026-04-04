#pragma once

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QStringList>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>

#include "core/Debug.h"

namespace Fernanda {

// A segmented toggle with N labels and a sliding highlight pill. Emits
// indexChanged(int) when the active segment changes (by click or setIndex)
/// TODO MU: Holding click (which switches cursor back to arrow) on the button
/// causes pointing hand cursor to not reappear on hover again - feels weird
class MultiSwitch : public QWidget
{
    Q_OBJECT

public:
    explicit MultiSwitch(
        const QStringList& labels,
        int startingIndex,
        QWidget* parent = nullptr)
        : QWidget(parent)
        , labels_(labels)
    {
        ASSERT(!labels_.isEmpty());
        setup_(qBound(0, startingIndex, labels_.size()));
    }

    virtual ~MultiSwitch() override { TRACER; }

    int count() const { return labels_.size(); }
    int index() const noexcept { return index_; }

    void setIndex(int index)
    {
        if (index < 0 || index >= labels_.size()) return;
        if (index == index_) return;

        index_ = index;
        animateTo_(segmentX_(index_));
        emit indexChanged(index_);
    }

    virtual QSize sizeHint() const override
    {
        QFontMetrics metrics(font());
        auto segment_width = 0;

        for (auto& label : labels_)
            segment_width =
                qMax(segment_width, metrics.horizontalAdvance(label));

        segment_width += LABEL_PAD_X_ * 2;

        int total_width = segment_width * labels_.size() + TRACK_PAD_ * 2;
        int total_height = metrics.height() + LABEL_PAD_Y_ * 2 + TRACK_PAD_ * 2;

        return { total_width, total_height };
    }

    virtual QSize minimumSizeHint() const override { return sizeHint(); }

signals:
    void indexChanged(int index);

protected:
    virtual void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        auto track = trackRect_();
        auto segment_width = segmentWidth_();
        auto pill = pillRect_(segment_width);

        // Track
        painter.setPen(Qt::NoPen);
        painter.setBrush(palette().button());
        painter.drawRoundedRect(track, RADIUS_, RADIUS_);

        // Pill
        painter.setBrush(QColor(0, 0, 0, 40));
        painter.drawRoundedRect(pill, PILL_RADIUS_, PILL_RADIUS_);

        // Labels
        for (auto i = 0; i < labels_.size(); ++i) {
            auto label_rect = segmentRect_(i, segment_width);

            painter.setPen(palette().buttonText().color());
            painter.drawText(label_rect, Qt::AlignCenter, labels_[i]);
        }
    }

    virtual void mousePressEvent(QMouseEvent* event) override
    {
        // if (event->button() != Qt::LeftButton) return;

        auto track = trackRect_();
        if (!track.contains(event->pos())) return;

        auto segment_width = segmentWidth_();
        auto relative_x = event->pos().x() - track.x();
        auto clicked_index = qBound(
            0,
            static_cast<int>(relative_x / segment_width),
            labels_.size() - 1);

        setIndex(clicked_index);
    }

    virtual void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);

        if (animation_->state() != QAbstractAnimation::Running) {
            highlightX_ = segmentX_(index_);
        }
    }

private:
    constexpr static int ANIMATION_MS_ = 175;
    constexpr static int TRACK_PAD_ = 3;
    constexpr static int PILL_PAD_ = 2;
    constexpr static int LABEL_PAD_X_ = 12;
    constexpr static int LABEL_PAD_Y_ = 4;
    constexpr static qreal RADIUS_ = 6.0;
    constexpr static qreal PILL_RADIUS_ = 4.0;

    QStringList labels_{};
    int index_ = -1;
    qreal highlightX_ = 0.0;
    QVariantAnimation* animation_ = new QVariantAnimation(this);

    void setup_(int startingIndex)
    {
        index_ = startingIndex;

        // setFocusPolicy(Qt::NoFocus); // Can set this to avoid button-like
        // bounce
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);

        animation_->setDuration(ANIMATION_MS_);
        animation_->setEasingCurve(QEasingCurve::InOutCubic);

        connect(
            animation_,
            &QVariantAnimation::valueChanged,
            this,
            [this](const QVariant& value) {
                highlightX_ = value.toReal();
                update();
            });
    }

    QRect trackRect_() const { return rect(); }

    qreal segmentWidth_() const
    {
        return static_cast<qreal>(trackRect_().width() - TRACK_PAD_ * 2)
               / labels_.size();
    }

    qreal segmentX_(int index) const
    {
        return TRACK_PAD_ + index * segmentWidth_();
    }

    QRectF segmentRect_(int index, qreal segmentWidth) const
    {
        auto track = trackRect_();
        return QRectF(
            track.x() + TRACK_PAD_ + index * segmentWidth,
            track.y() + TRACK_PAD_,
            segmentWidth,
            track.height() - TRACK_PAD_ * 2);
    }

    QRectF pillRect_(qreal segmentWidth) const
    {
        auto track = trackRect_();
        return QRectF(
            highlightX_ + PILL_PAD_,
            track.y() + TRACK_PAD_ + PILL_PAD_,
            segmentWidth - PILL_PAD_ * 2,
            track.height() - (TRACK_PAD_ + PILL_PAD_) * 2);
    }

    void animateTo_(qreal targetX)
    {
        animation_->stop();
        animation_->setStartValue(highlightX_);
        animation_->setEndValue(targetX);
        animation_->start();
    }
};

} // namespace Fernanda
