// indicator.h, Fernanda

#pragma once

#include "text.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

class Indicator : public QWidget
{
    Q_OBJECT

public:
    Indicator(QWidget* parent = nullptr);

    struct Has {
        bool linePos = true;
        bool colPos = true;
        bool lineCount = true;
        bool wordCount = true;
        bool charCount = true;
    } has;

public slots:
    void updatePositions(const int cursorBlockNumber, const int cursorPosInBlock);
    void updateCounts(const QString text, const int blockCount);
    void updateSelection(const QString selectedText, const int lineCount);

private:
    QHBoxLayout* layout = new QHBoxLayout(this);
    QLabel* positions = new QLabel(this);
    QLabel* separator = new QLabel(this);
    QLabel* counts = new QLabel(this);

    bool hideOrShow(QLabel* label, bool feature1, bool feature2, bool feature3 = false);
    QGraphicsOpacityEffect* opacity(double qreal);

signals:
    void toggled();
};

// indicator.h, Fernanda
