#pragma once

#include "Widget.hpp"

#include <QChar>
#include <QEnterEvent>
#include <QFontDatabase>
#include <QString>
#include <QStyle>
#include <QToolButton>

#include <map>

class UiButton : public Widget<QToolButton>
{
	Q_OBJECT

public:
	enum class Ui {
		None,
		Add,
		ChevronLeft,
		ChevronDown,
		ChevronRight,
		ChevronUp,
		Close,
		Ellipse,
		Refresh
	};

	UiButton(
		const char* name,
		const QString& label,
		QWidget* parent = nullptr,
		const QString& flag = QString())
		: Widget(name, parent), m_label(label), m_flag(flag)
	{
		updateIcon();
	}

	UiButton(
		const char* name,
		Ui icon,
		QWidget* parent = nullptr,
		Ui flag = Ui{})
		: UiButton(name, fontIcon(icon), parent, fontIcon(flag))
	{
		setFont(uiFont());
	}

	bool flagged() const { return m_flagged; }
	bool hoveredOver() const { return m_hoveredOver; }
	QString label() const { return m_label; }
	QString flag() const { return m_flag; }

	void setFlagged(bool flagged)
	{
		m_flagged = flagged;
		updateIcon();
	}

protected:
	virtual void enterEvent(QEnterEvent* event) override
	{
		QToolButton::enterEvent(event);
		m_hoveredOver = true;
		updateIcon();
	}

	virtual void leaveEvent(QEvent* event) override
	{
		QToolButton::leaveEvent(event);
		m_hoveredOver = false;
		updateIcon();
	}

private:
	const QString m_label;
	const QString m_flag;
	bool m_flagged = false;
	bool m_hoveredOver = false;

	const std::map<Ui, QChar> fontChars()
	{
		return {
		{ Ui::Add, QChar(0xe145) },
		{ Ui::ChevronLeft, QChar(0xe5cb) },
		{ Ui::ChevronDown, QChar(0xe5cf) },
		{ Ui::ChevronRight, QChar(0xe5cc) },
		{ Ui::ChevronUp, QChar(0xe5ce) },
		{ Ui::Close, QChar(0xe5cd) },
		{ Ui::Ellipse, QChar(0xe061) },
		{ Ui::Refresh, QChar(0xe5d5) }
		};
	}

	const QString fontIcon(Ui icon)
	{
		auto map = fontChars();
		QChar font_icon;
		auto it = map.find(icon);
		if (it != map.end())
			font_icon = it->second;
		return QString(font_icon);
	}

	const QFont uiFont()
	{
		static const auto id = QFontDatabase::addApplicationFont(
			":/external/material-icons/MaterialIcons-Regular.ttf");
		return QFont(QFontDatabase::applicationFontFamilies(id).at(0));
	}

	bool flagShouldDisplay()
	{
		return (m_flagged && !m_hoveredOver && !m_flag.isNull());
	}

	void updateIcon()
	{
		(flagShouldDisplay())
			? setText(m_flag)
			: setText(m_label);
		updateFlaggedProperty();
	}

	void updateFlaggedProperty()
	{
		setProperty("flagged", flagShouldDisplay());
		style()->unpolish(this);
		style()->polish(this);
	}
};
