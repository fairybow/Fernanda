// indicator.h, Fernanda

#pragma once

#include "text.h"
#include "userdata.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

class Indicator : public QWidget
{
    Q_OBJECT

public:
    Indicator(QWidget* parent = nullptr);

    enum class Has {
        CharCount,
        ColumnPosition,
        LineCount,
        LinePosition,
        WordCount
    };

    void toggle(bool checked, Has has);

public slots:
    void updatePositions(const int cursorBlockNumber, const int cursorPositionInBlock);
    void updateCounts(const QString text, const int blockCount);
    void updateSelection(const QString selectedText, const int lineCount);

private:
    QHBoxLayout* layout = new QHBoxLayout(this);
    QLabel* positions = new QLabel(this);
    QLabel* separator = new QLabel(this);
    QLabel* counts = new QLabel(this);

    bool hasLinePosition = true;
    bool hasColumnPosition = true;
    bool hasLineCount = true;
    bool hasWordCount = true;
    bool hasCharCount = true;

    bool hideOrShow(QLabel* label, bool feature1, bool feature2, bool feature3 = false);
    QGraphicsOpacityEffect* opacity(double qreal);

signals:
    void toggled();
    void askSignalTextChanged();
    void askSignalCursorPositionChanged();
};

// indicator.h, Fernanda
