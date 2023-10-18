#pragma once

#include "../common/Layout.hpp"
#include "../common/Path.hpp"
#include "../common/Widget.hpp"
#include "AddTab.hpp"
#include "CloseTab.hpp"
#include "ScrollTabs.hpp"
#include "TrueTabBar.hpp"

#include <QApplication>
#include <QString>
#include <QUuid>
#include <QVariantMap>
#include <QVector>
#include <QWheelEvent>

#include <filesystem>

class TabBar : public Widget<>
{
	Q_OBJECT

public:
	using StdFsPath = std::filesystem::path;

	TabBar(const char* name, int minTabSize = 25, int maxTabSize = 100, QWidget* parent = nullptr);

	int serve(const QUuid& id, const QString& title = QString(), bool switchTo = true);
	bool isUntitled();
	void setUntitledDisplay(const QString& text, int charLimit = 30);
	void close(const QUuid& id);
	bool isFull();
	bool isEmpty();
	void updateEditedState(const QUuid& id, bool edited); // update this to generalized "flagged" language throughout
	void updateTitle(const QUuid& id, const QString& title);

	void devCurrentInfo()
	{
		qDebug() << __FUNCTION__;

		auto index = m_trueTabBar->currentIndex();
		qDebug() << "Index:" << index;
		auto data_map = m_trueTabBar->tabData(index).toMap();
		qDebug() << "ID:" << data_map[DATA_ID].toUuid();
		qDebug() << "Title:" << data_map[DATA_TITLE].toString()
			<< Qt::endl;
	}

signals:
	void currentChanged(const QUuid& id);
	void askAdd();
	void askClearForClose(const QUuid& id);

protected:
	virtual void wheelEvent(QWheelEvent* event) override;

private:
	static constexpr char DATA_ID[] = "tab_id";
	static constexpr char DATA_TITLE[] = "tab_title";

	TrueTabBar* m_trueTabBar;
	AddTab* m_add = new AddTab;
	ScrollTabs* m_scrollLeft = new ScrollTabs(m_trueTabBar, ScrollTabs::Side::Left);
	ScrollTabs* m_scrollRight = new ScrollTabs(m_trueTabBar, ScrollTabs::Side::Right);

	void setupWidgets();
	void connections();
	QUuid idAt(int index);
	int indexFor(const QUuid& id);
	const QString title(int index);
	int create(const QUuid& id, const QString& title = QString());
	void setButton(int index, const QUuid& id);
	void setData(int index, const QUuid& id, const QString& title = QString());
	CloseTab* closeButton(const QUuid& id);
	void blockAllSignals(bool block);

private slots:
	void adjustControls();
};
