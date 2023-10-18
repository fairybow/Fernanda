#pragma once

#include <QString>
#include <QTextBlock>
#include <QTextDocument>
#include <QVariant>

class TextRecord : public QTextDocument
{
public:
	struct CursorSpan {
		int cursor = 0;
		int anchor = 0;
	};

	TextRecord(
		const QString& text,
		const QString& originalText,
		const QString& title = QString(),
		const QVariant& data = QVariant(),
		QObject* parent = nullptr)
		: QTextDocument(text, parent),
		m_originalText(originalText),
		m_title(title),
		m_data(data) {}

	CursorSpan cursorSpan() const { return m_cursorSpan; }
	QVariant data() const { return m_data; }
	bool isEdited() const { return toPlainText() != m_originalText; }
	bool isUntitled() const { return m_title == QString(); }
	QString firstBlockText() const { return firstBlock().text(); }
	QString originalText() const { return m_originalText; }
	QString text() const { return toPlainText(); }
	QString title() const { return m_title; }

	void setCursorSpan(int cursor, int anchor) { m_cursorSpan = CursorSpan(cursor, anchor); }
	void setData(const QVariant& data) { m_data = data; }
	void setText(const QString& text) { setPlainText(text); }
	void setTitle(const QString& title) { m_title = title; }

private:
	CursorSpan m_cursorSpan{};
	QVariant m_data;
	const QString m_originalText;
	QString m_title;
};
