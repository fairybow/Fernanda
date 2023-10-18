/*#pragma once

#include "../common/Widget.hpp"
#include "StyledItemDelegate.hpp"

#include <QScrollBar>
#include <QStandardItemModel>
#include <QTreeView>

class TreeView : public Widget<QTreeView>
{
public:
	TreeView(const char* name, QWidget* parent = nullptr)
		: Widget(name, parent)
	{
		setItemDelegate(m_delegate);
		setModel(m_itemModel);
	}

private:
	QStandardItemModel* m_itemModel = new QStandardItemModel(this);
	StyledItemDelegate* m_delegate = new StyledItemDelegate(this);
};
*/
