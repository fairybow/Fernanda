/*#pragma once

#include "../common/Layout.hpp"
#include "../common/Widget.hpp"
#include "TrueSplitter.hpp"

#include <QPushButton>
#include <QTimer>
#include <QVector>

#include <compare>
#include <numeric>
#include <optional>

class Splitter : public Widget<>
{
	Q_OBJECT

public:
	Splitter(const char* name, Qt::Orientation orientation, QWidgetList widgets, QWidget* parent);

	void initialize(QVector<double> fallbacks, int centralWidgetIndex);

	QByteArray saveState() const { return m_trueSplitter->saveState(); }
	bool restoreState(const QByteArray& state) const { return m_trueSplitter->restoreState(state); }

signals:
	QRect askWindowSize();

private:
	enum class Alignment {
		Central,
		Left,
		Right
	};
	enum class State {
		Collapsed,
		Expanded,
		HoveredOver
	};

	struct Meta {
		int widgetIndex;
		std::optional<QPushButton*> handleButton;
		State state = State::Expanded;
		int expandedWidth = -1;
		QPushButton* button() { return handleButton.has_value() ? handleButton.value() : nullptr; }
	};

	TrueSplitter* m_trueSplitter = new TrueSplitter(this);

	int m_centralWidgetIndex = -1;
	QVector<Meta> m_metas;

	Alignment findAlignment(int widgetIndex) const;
	QVector<int> verifyFallbacks(QVector<double> fallbacks);
	void moveButtons();

	bool is(Alignment alignment, int widgetIndex) const { return findAlignment(widgetIndex) == alignment; }
	bool isCentral(int widgetIndex) const { return is(Alignment::Central, widgetIndex); }
	bool isLeft(int widgetIndex) const { return is(Alignment::Left, widgetIndex); }
	bool isRight(int widgetIndex) const { return is(Alignment::Right, widgetIndex); }

	int associatedHandle(int widgetIndex) { return (widgetIndex < m_centralWidgetIndex) ? widgetIndex + 1 : widgetIndex; }

private slots:
	void showOrHideButtons(int widgetIndex, TrueSplitter::WidgetWas visibility);
};
*/
