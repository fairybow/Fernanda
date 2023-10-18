#include "Editor.h"

Editor::Editor(const char* name, const QFont& defaultFont, QWidget* parent)
	: Widget(name, parent), m_defaultFont(defaultFont)
{
	nameObjects(name);
	setupTrueEditor();
	setupShadow();
	buildScrollBar();
	connections();
	Layout::transpareForMouse({ m_shadow, m_overlay });
	Layout::stack({ m_shadow, m_overlay, m_trueEditor, m_underlay}, this);

	// testing

	m_overlay->hide(); // <- should be able to remove or use exclusivley for locking for error message display (file not found, etc.)
}

void Editor::setWrapMode(const QString& mode)
{
	if (mode == "NoWrap")
		m_trueEditor->setWordWrapMode(QTextOption::NoWrap);
	else if (mode == "WordWrap")
		m_trueEditor->setWordWrapMode(QTextOption::WordWrap);
	else if (mode == "WrapAnywhere")
		m_trueEditor->setWordWrapMode(QTextOption::WrapAnywhere);
	else if (mode == "WrapAt")
		m_trueEditor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
	else
		m_trueEditor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
}

void Editor::setHasLineHighlight(bool state)
{
	m_hasLineHighlight = state;
	m_trueEditor->highlightCurrentLine();

}
void Editor::setHasLineNumberArea(bool state)
{
	m_lineNumberArea->setVisible(state);
	m_trueEditor->updateLineNumberAreaWidth();
}

void Editor::setDocument(TextRecord* document)
{
	m_trueEditor->setPlainText(document->text());
	auto span = document->cursorSpan();
	setCursorSpan(span.cursor, span.anchor);
	setFocus();
}

void Editor::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::StyleChange)
		m_trueEditor->setCursorStyle(styleSheet());

	QWidget::changeEvent(event);
}

void Editor::nameObjects(const char* name)
{
	m_trueEditor->setObjectName(name);
	m_lineNumberArea->setObjectName(name + QString("-line-number-area"));
	m_shadow->setObjectName(name + QString("-shadow"));
	m_overlay->setObjectName(name + QString("-overlay"));
	m_underlay->setObjectName(name + QString("-underlay"));
}

void Editor::setupTrueEditor()
{
	m_trueEditor->setLineNumberArea(m_lineNumberArea);
}

void Editor::setupShadow()
{
	m_shadow->setStyleSheet(Io::readFile(":/editor/Shadow.qss"));
	Fx::blur(15, m_shadow);
}

void Editor::buildScrollBar()
{
	//
}

void Editor::connections()
{
	trueEditorConnections();
	cursorConnections();
	//lineNumberAreaConnections();
}

void Editor::trueEditorConnections()
{	
	connect(m_trueEditor, &TrueEditor::getHasLineHighlight, this, [&] {
		return m_hasLineHighlight;
		});
	connect(m_trueEditor, &TrueEditor::getHasCursorBlink, this, [&] {
		return m_hasCursorBlink;
		});
	connect(m_trueEditor, &TrueEditor::getHasCursorBlock, this, [&] {
		return m_hasCursorBlock;
		});

	connect(m_trueEditor, &TrueEditor::selectionChanged,
		this, lambdaEmit(selectionChanged));
	connect(m_trueEditor, &TrueEditor::textChanged,
		this, lambdaEmit(textChanged));
	connect(m_trueEditor, &TrueEditor::cursorPositionChanged,
		this, lambdaEmit(cursorPositionChanged));
}

void Editor::cursorConnections()
{
	connect(m_trueEditor, &TrueEditor::textChanged, this, [&] {
		if (m_hasCursorEnsureVisible)
			m_trueEditor->ensureCursorVisible(); // it is unclear to me what this actually does...
		});

	connectMultipleSignals(m_trueEditor, this, [&] {
		if (m_hasCursorTypewriter)
			m_trueEditor->centerCursor();
		}, &TrueEditor::cursorPositionChanged, &TrueEditor::textChanged);
}

/*
void Editor::lineNumberAreaConnections()
{
	//
}*/

void Editor::setCursorSpan(int cursor, int anchor)
{
	auto text_cursor = m_trueEditor->textCursor();
	if (anchor == cursor)
		text_cursor.setPosition(cursor);
	else {
		text_cursor.setPosition(anchor, QTextCursor::MoveAnchor);
		text_cursor.setPosition(cursor, QTextCursor::KeepAnchor);
	}

	m_trueEditor->setTextCursor(text_cursor);
}
