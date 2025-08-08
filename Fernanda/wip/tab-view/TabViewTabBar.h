#pragma once

#include <QObject>
#include <QSize>
#include <QTabBar>
#include <QtGlobal>
#include <QWidget>
#include <QStyle>

#include "Coco/Debug.h"

class TabViewTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit TabViewTabBar(QWidget* parent = nullptr)
        : QTabBar(parent)
    {
        initialize_();
    }

    virtual ~TabViewTabBar() override { COCO_TRACER; }

    virtual QSize minimumSizeHint() const override
    {
        return { 0, height() };
    }

    virtual QSize sizeHint() const override
    {
        auto hint = QTabBar::sizeHint();
        return { hint.width(), height() };
    }

    int minimumTabWidth() const noexcept { return minimumTabWidth_; }
    int maximumTabWidth() const noexcept { return maximumTabWidth_; }
    QSize minimumTabSize() const { return { minimumTabWidth_, height() }; }
    QSize maximumTabSize() const { return { maximumTabWidth_, height() }; }

    void setMinimumTabWidth(int width)
    {
        minimumTabWidth_ = qMax(0, qMin(width, maximumTabWidth_));
    }

    void setMaximumTabWidth(int width)
    {
        maximumTabWidth_ = qMax(qMax(0, width), minimumTabWidth_);
    }

protected:
    virtual QSize minimumTabSizeHint(int index) const override
    {
        return { minimumTabWidth_, height() };
    }

    virtual QSize tabSizeHint(int index) const override
    {
        return { maximumTabWidth_, height() };
    }

private:
    int minimumTabWidth_ = 75;
    int maximumTabWidth_ = 75;

    void initialize_()
    {
        setMovable(true);
        setAutoHide(false);
        setTabsClosable(false); // Using custom buttons
        setExpanding(false); // Should have no effect regardless, given our size hints
        setDrawBase(false);
    }
};
