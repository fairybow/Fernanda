#pragma once

#include "../common/Fx.hpp"
#include "../common/Io.hpp"
#include "../common/Layout.hpp"
#include "../common/TextRecord.hpp"
#include "../common/Widget.hpp"
#include "TrueEditor.h"
// potentially move block cursor and lna here and manage
// the interactions between them with this container class

#include <QFont>
#include <QLabel>
#include <QVector>

class Editor : public Widget<>
{
	Q_OBJECT

public:
	Editor(const char* name, const QFont& defaultFont = QFont(), QWidget* parent = nullptr);

	void setWrapMode(const QString& mode); // refactor to use QTextOption enum (`void setWordWrapMode(QTextOption::WrapMode policy)`), move to intermediaries
	void setHasLineHighlight(bool state);
	void setHasLineNumberArea(bool state);
	void setDocument(TextRecord* document);

	QFont defaulFont() const { return m_defaultFont; }
	int defaulTabStop() const { return 40; }
	QString defaultWrap() const { return "WrapAt"; }
	int selectedLineCount() const { return m_trueEditor->selectedLineCount(); }
	QString firstBlock() const { return m_trueEditor->document()->begin().text(); }

	void setHasShadow(bool state) { m_shadow->setVisible(state); }
	void setHasCursorBlink(bool state) { m_hasCursorBlink = state; }
	void setHasCursorBlock(bool state) { m_hasCursorBlock = state; }
	void setHasCursorEnsureVisible(bool state) { m_hasCursorEnsureVisible = state; }
	void setHasCursorTypewriter(bool state) { m_hasCursorTypewriter = state; }

	// TrueEditor intermediaries
	void setCenterOnScroll(bool enabled) { m_trueEditor->setCenterOnScroll(enabled); };
	void setFocus() { m_trueEditor->setFocus(); }
	void setFont(const QFont& font) { m_trueEditor->setFont(font); }
	void setTabStopDistance(qreal distance) { m_trueEditor->setTabStopDistance(qBound<qreal>(0, static_cast<int>(distance), 500)); }
	void setReadOnly(bool readOnly) { m_trueEditor->setReadOnly(readOnly); }
	bool isReadOnly() const { return m_trueEditor->isReadOnly(); }
	QString toPlainText() const { return m_trueEditor->toPlainText(); }
	int blockCount() const { return m_trueEditor->blockCount(); }
	QString selectedText() const { return m_trueEditor->textCursor().selectedText(); }
	int cursorBlockNumber() const { return m_trueEditor->textCursor().blockNumber(); }
	int cursorPositionInBlock() const { return m_trueEditor->textCursor().positionInBlock(); }
	int cursorPosition() const { return m_trueEditor->textCursor().position(); }
	int cursorAnchor() const { return m_trueEditor->textCursor().anchor(); }
	bool hasSelection() const { return m_trueEditor->textCursor().hasSelection(); }
	void clear() { m_trueEditor->clear(); }

signals:
	void selectionChanged();
	void textChanged();
	void cursorPositionChanged();

protected:
	virtual void changeEvent(QEvent* event) override;

private:
	const QFont m_defaultFont;
	TrueEditor* m_trueEditor = new TrueEditor(this);
	LineNumberArea* m_lineNumberArea = new LineNumberArea(m_trueEditor);
	QLabel* m_shadow = new QLabel(this);
	QLabel* m_overlay = new QLabel(this);
	QLabel* m_underlay = new QLabel(this);

	bool m_hasLineHighlight = true;
	bool m_hasCursorBlink = true;
	bool m_hasCursorBlock = true;
	bool m_hasCursorEnsureVisible = true;
	bool m_hasCursorTypewriter = false;

	void nameObjects(const char* name);
	void setupTrueEditor();
	void setupShadow();
	void buildScrollBar();
	void connections();
	void trueEditorConnections();
	void cursorConnections();
	//void lineNumberAreaConnections();
	void setCursorSpan(int cursor, int anchor);
};
