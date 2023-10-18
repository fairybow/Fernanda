#pragma once

#include <QSize>
#include <QWidget>

class TrueEditor;

class LineNumberArea : public QWidget
{
public:
	LineNumberArea(TrueEditor* parent = nullptr);

	virtual QSize sizeHint() const override;

protected:
	virtual void paintEvent(QPaintEvent* event) override;

private:
	TrueEditor* m_parent;
};
