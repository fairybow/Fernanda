#pragma once

#include <QComboBox>
#include <QString>
#include <QStyleOptionComboBox>
#include <QStylePainter>

class ComboBox : public QComboBox
{
public:
	ComboBox(QWidget* parent = nullptr)
		: QComboBox(parent) {}

	ComboBox(const QString& idleText, QWidget* parent = nullptr)
		: QComboBox(parent), m_idleText(idleText) {}

protected:
	virtual void paintEvent(QPaintEvent* event)
	{
		if (m_idleText.isEmpty()) {
			QComboBox::paintEvent(event);
			return;
		}

		QStylePainter painter(this);
		QStyleOptionComboBox option;
		initStyleOption(&option);
		option.currentText = m_idleText;
		painter.drawComplexControl(QStyle::CC_ComboBox, option);
		painter.drawControl(QStyle::CE_ComboBoxLabel, option);
	}

private:
	const QString m_idleText;
};
