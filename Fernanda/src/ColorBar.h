#pragma once

#include <optional>

#include <QEvent>
#include <QLinearGradient>
#include <QMainWindow>
#include <QMenuBar>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QStatusBar>
#include <QTimeLine>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>

#include "Coco/Concepts.h"
#include "Coco/Debug.h"
#include "Coco/Fx.h"

namespace Fernanda {

template <typename T>
concept QMenuOrStatusBar = Coco::Concepts::DerivedPointer<QMenuBar, T>
                           || Coco::Concepts::DerivedPointer<QStatusBar, T>;

// A colorful gradient progress bar for visual feedback on save and startup. It
// can be positioned above or below the window's menu bar or status bar
class ColorBar : public QWidget
{
    Q_OBJECT

public:
    enum Position
    {
        Top,
        BelowMenuBar,
        AboveStatusBar,
        Bottom
    };

    enum Color
    {
        Green,
        Pastel,
        Red
    };

    explicit ColorBar(QMainWindow* parentWindow = nullptr)
        : QWidget(parentWindow)
        , window_(parentWindow)
    {
        initialize_();
    }

    virtual ~ColorBar() override { COCO_TRACER; }

    Position position() const noexcept { return position_; }
    void setPosition(Position position) { position_ = position; }

    void run(Color color, int delay = 0)
    {
        if (!isVisible()) return;
        startAnimation_(color, delay);
    }

    void runOnResult(bool result, int delay = 0)
    {
        result ? run(Green, delay) : run(Red, delay);
    }

    virtual bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == window_) {
            switch (event->type()) {
            case QEvent::LayoutRequest:
            case QEvent::Resize:
                updateGeometry_();
                break;
            default:
                break;
            }
        }

        return QWidget::eventFilter(watched, event);
    }

protected:
    virtual void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QRect rect = this->rect();

        auto progress =
            (currentProgress_ - MIN_RANGE_) / (MAX_RANGE_ - MIN_RANGE_);
        auto visible_width = static_cast<int>(rect.width() * progress);

        auto gradient = gradientFor_(currentColor_);

        QRect visible_rect(0, 0, visible_width, rect.height());
        painter.setClipRect(visible_rect);
        painter.fillRect(rect, gradient);
    }

private:
    QMainWindow* window_;

    static constexpr qreal MIN_RANGE_ = 0.0;
    static constexpr qreal MAX_RANGE_ = 100.00;
    qreal currentProgress_ = MIN_RANGE_;
    Color currentColor_ = Pastel;
    Position position_ = Top;
    QTimer* lingerTimer_ = new QTimer(this);

    // Cache
    std::optional<QLinearGradient> greenGradient_{};
    std::optional<QLinearGradient> redGradient_{};
    std::optional<QLinearGradient> pastelGradient_{};

    void initialize_()
    {
        setFixedHeight(3);
        setAttribute(Qt::WA_TransparentForMouseEvents);

        lingerTimer_->setSingleShot(true);
        connect(lingerTimer_, &QTimer::timeout, this, [&] {
            currentProgress_ = MIN_RANGE_;
            update(); // Trigger repaint when resetting
        });

        if (window_) {
            window_->installEventFilter(this);
            updateGeometry_();
        }
    }

    template <QMenuOrStatusBar T> T getVisibleBar() const
    {
        auto bar = window_->findChild<T>();
        return (bar && bar->isVisible()) ? bar : nullptr;
    }

    int calculateYPosition_() const
    {
        if (!window_) return 0;

        // menuBar() and statusBar() are lazy constructors, so querying with
        // these will create them! We don't want that here.

        switch (position_) {
        case Position::BelowMenuBar:
            if (auto menu_bar = getVisibleBar<QMenuBar*>())
                return menu_bar->y() + menu_bar->height();
            [[fallthrough]];
        default:
        case Position::Top:
            return 0;

        case Position::AboveStatusBar:
            if (auto status_bar = getVisibleBar<QStatusBar*>())
                return status_bar->y() - height();
            [[fallthrough]];
        case Position::Bottom:
            return window_->height() - height();
        }
    }

    void updateGeometry_()
    {
        if (!window_) return;
        move(0, calculateYPosition_());
        setFixedWidth(window_->width());
        raise();
    }

    void startAnimation_(Color color, int delay)
    {
        delay = qBound(0, delay, 3000);

        // Store the color for paintEvent to use
        currentColor_ = color;

        constexpr auto fill_time = 300;
        auto time_line = new QTimeLine(fill_time, this);
        time_line->setFrameRange(MIN_RANGE_, MAX_RANGE_);

        connect(time_line, &QTimeLine::frameChanged, this, [=](int frame) {
            currentProgress_ = frame;
            update(); // Trigger repaint
        });

        connect(
            time_line,
            &QTimeLine::finished,
            time_line,
            &QTimeLine::deleteLater);

        QTimer::singleShot(delay, this, [=] {
            lingerTimer_->start(1500);
            time_line->start();
            update(); // Trigger initial repaint
        });
    }

    QLinearGradient gradientFor_(Color color)
    {
        switch (color) {
        case Green:
            if (!greenGradient_) {
                greenGradient_ = Coco::Fx::bandedGradient(
                    0,
                    0,
                    width(),
                    0,
                    "#00e878",
                    "#92ff00",
                    "#61e1bf",
                    "#00d4ff");
            }

            return *greenGradient_;

        case Red:
            if (!redGradient_) {
                redGradient_ = Coco::Fx::bandedGradient(
                    0,
                    0,
                    width(),
                    0,
                    "#b43a3a",
                    "#fd1d1d",
                    "#fd6430",
                    "#fcb045");
            }

            return *redGradient_;

        default:
        case Pastel:
            if (!pastelGradient_) {
                pastelGradient_ = Coco::Fx::bandedGradient(
                    0,
                    0,
                    width(),
                    0,
                    "#7ce1f9",
                    "#3bb0f3",
                    "#9194f2",
                    "#f9b3f9");
            }

            return *pastelGradient_;
        }
    }
};

} // namespace Fernanda
