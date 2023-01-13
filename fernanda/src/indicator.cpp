// indicator.cpp, Fernanda

#include "indicator.h"

Indicator::Indicator(QWidget* parent)
    : QWidget(parent)
{
    separator->setText(QStringLiteral("/"));
    positions->setGraphicsEffect(opacity(0.8));
    separator->setGraphicsEffect(opacity(0.3));
    counts->setGraphicsEffect(opacity(0.8));
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    for (const auto& widget : { positions, separator, counts })
    {
        widget->setObjectName(QStringLiteral("indicator"));
        layout->addWidget(widget);
    }
    connect(this, &Indicator::toggled, this, [&]()
        {
            if (positions->isHidden() || counts->isHidden())
            separator->hide();
            else
                separator->show();
        });
}

void Indicator::updatePositions(const int cursorBlockNumber, const int cursorPosInBlock)
{
    if (!hideOrShow(positions, has.linePos, has.colPos)) return;
    QStringList elements;
    if (has.linePos)
        elements << QStringLiteral("ln ") + QString::number(cursorBlockNumber + 1);
    if (has.colPos)
        elements << QStringLiteral("col ") + QString::number(cursorPosInBlock + 1);
    positions->setText(elements.join(QStringLiteral(", ")));
}

void Indicator::updateCounts(const QString text, const int blockCount)
{
    if (!hideOrShow(counts, has.lineCount, has.wordCount, has.charCount)) return;
    const auto word_count = text.split(Text::regex(Text::Re::Split), Qt::SkipEmptyParts).count();
    const auto char_count = text.count();
    QStringList elements;
    if (has.lineCount)
        elements << QString::number(blockCount) + QStringLiteral(" lines");
    if (has.wordCount)
        elements << QString::number(word_count) + QStringLiteral(" words");
    if (has.charCount)
        elements << QString::number(char_count) + QStringLiteral(" chars");
    counts->setText(elements.join(QStringLiteral(", ")));
}

void Indicator::updateSelection(const QString selectedText, const int lineCount)
{
    updateCounts(selectedText, lineCount);
}

bool Indicator::hideOrShow(QLabel* label, bool feature1, bool feature2, bool feature3)
{
    auto result = false;
    if (!feature1 && !feature2 && !feature3)
    {
        label->hide();
        toggled();
    }
    else
    {
        label->show();
        toggled();
        result = true;
    }
    return result;
}

QGraphicsOpacityEffect* Indicator::opacity(double qreal)
{
    auto result = new QGraphicsOpacityEffect(this);
    result->setOpacity(qreal);
    return result;
}

// indicator.cpp, Fernanda
