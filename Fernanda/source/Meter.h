#pragma once

#include "common/Fx.hpp"
#include "common/Layout.hpp"
#include "common/RegexPatterns.hpp"
#include "common/UiButton.hpp"
#include "common/Utility.hpp"
#include "common/Widget.hpp"

#include <QLabel>
#include <QRegularExpression>
#include <QString>
#include <QVector>

#include <type_traits>

class Meter : public Widget<>
{
	Q_OBJECT

public:
	enum class Type {
		All,
		Counts,
		Positions,
		Selection
	};

	struct Counts {
		QString text;
		int blockCount;
	};
	struct Positions {
		int cursorBlockNumber;
		int cursorPositionInBlock;
	};

	template <typename T>
	using CountsOrPositions = std::disjunction<std::is_same<T, Counts>, std::is_same<T, Positions>>;

	Meter(const char* name, QWidget* parent = nullptr);

	void trigger(Type type = Type{}, bool force = false);

	void setHasLinePosition(bool state) { updateOutput(m_hasLinePosition, state); }
	void setHasColumnPosition(bool state) { updateOutput(m_hasColumnPosition, state); }
	void setHasLineCount(bool state) { updateOutput(m_hasLineCount, state); }
	void setHasWordCount(bool state) { updateOutput(m_hasWordCount, state); }
	void setHasCharacterCount(bool state) { updateOutput(m_hasCharCount, state); }

	template<typename T>
	std::enable_if_t<CountsOrPositions<T>::value, void> give(T data)
	{
		if constexpr (std::is_same_v<T, Counts>)
			displayCounts(data);
		else if constexpr (std::is_same_v<T, Positions>)
			displayPositions(data);
	}

public slots:
	virtual void setVisible(bool visible);

signals:
	void separatorVisibilityCheck();
	void toggleAutoCount(bool checked);
	void editorFocusReturn();
	void askGiveCounts(bool isSelection);
	void askGivePositions();

private:
	QLabel* m_positions = new QLabel(this);
	QLabel* m_counts = new QLabel(this);
	QLabel* m_separator = new QLabel(this);
	UiButton* m_refresh = new UiButton("MeterButton", UiButton::Ui::Refresh, this);

	bool m_hasAutoCount = true;
	bool m_hasLinePosition = true;
	bool m_hasColumnPosition = true;
	bool m_hasLineCount = true;
	bool m_hasWordCount = true;
	bool m_hasCharCount = true;

	void setupLabels();
	void connections();
	void updateOutput(bool& memberBool, bool state);
	void updateCounts(bool isSelection = false);
	void displayCounts(Counts counts);
	void displayPositions(Positions positions);
	void updatePositions();
	bool toggleVisibility(QLabel* label, bool hasAnything);

	bool hasAnyCount() { return toggleVisibility(m_counts, (m_hasLineCount || m_hasWordCount || m_hasCharCount)); }
	bool hasEitherPosition() { return toggleVisibility(m_positions, (m_hasLinePosition || m_hasColumnPosition)); }
};
