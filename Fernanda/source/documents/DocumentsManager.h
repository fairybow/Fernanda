#pragma once

#include "../common/Io.hpp"
#include "../common/Path.hpp"
#include "../common/StringTools.hpp"
#include "../common/TextRecord.hpp"
#include "DocumentsCache.h"
#include "IdBank.hpp"

#include <QFileDialog>
#include <QString>
#include <QTimer>
#include <QUuid>

#include <filesystem>

namespace StdFs = std::filesystem;

class DocumentsManager : public QObject
{
public:
	using StdFsPath = StdFs::path;

	enum class PathType {
		Extant,
		New
	};

	struct Folders {
		StdFsPath user;
		StdFsPath temp;
		StdFsPath backup;
	};

	DocumentsManager(const Folders& folders, QWidget* parent = nullptr, int cacheMaxCost = 100);

	bool hasActive() const { return !m_activeId.isNull(); }
	TextRecord* active() { return retrieve(m_activeId); }
	bool isActive(const QUuid& id) { return m_activeId == id; }
	TextRecord* at(const QUuid& id) { return retrieve(id); }
	StdFsPath pathAt(const QUuid& id) { return s_idBank.path(id); };

	StdFsPath newFileDialog(const QString& name = QString());
	StdFsPath openFileDialog();
	TextRecord* setActive(const QUuid& id);
	QUuid newUnsaved();
	QUuid fromDisk(PathType type, const StdFsPath& path);
	bool toDisk();
	void close(const QUuid& id);

	void devClass()
	{
		qDebug() << __FUNCTION__;

		if (hasActive())
			qDebug() << "Active ID:" << m_activeId
			<< Qt::endl;
	}

	void devCurrentInfo()
	{
		qDebug() << __FUNCTION__;

		qDebug() << "ID:" << m_activeId;
		auto document = active();
		qDebug() << "Is edited?:" << document->isEdited();
		qDebug() << "Title:" << document->title();
		auto span = document->cursorSpan();
		qDebug() << "Cursor position:" << span.cursor;
		qDebug() << "Anchor position:" << span.anchor;
		qDebug() << "Recorded text:" << document->text();
		qDebug() << "Original text:" << document->originalText()
			<< Qt::endl;
	}

	void devIdBank() const
	{
		qDebug() << __FUNCTION__ << Qt::endl;

		if (!s_idBank.isEmpty()) {
			qDebug() << "Bank:";
			for (auto& id : s_idBank.bank())
				qDebug() << id << Qt::endl;
		}
		if (s_idBank.hasTrash()) {
			qDebug() << "Trash:";
			for (auto& id : s_idBank.trash())
				qDebug() << id << Qt::endl;
		}
		if (s_idBank.hasPaths()) {
			qDebug() << "Extant paths to IDs map:";
			for (auto& [path, id] : s_idBank.paths())
				qDebug() << "Path:" << Path::toQString(path)
				<< "\n" << id << Qt::endl;
		}
	}

private:
	static constexpr char FILE_TYPE[] = ".txt";
	static constexpr char DIALOG_FILE_TYPE[] = "Plain text file (*.txt)";

	static IdBank s_idBank;
	const StdFsPath m_userFolder;
	const StdFsPath m_tempFolder;
	const StdFsPath m_backupFolder;

	QUuid m_activeId;
	QTimer* m_continuousTempSave = new QTimer(this);

	void create(const QUuid& id, const StdFsPath& path = StdFsPath()) { retrieve(id, path); }
	QWidget* parent() const { return qobject_cast<QWidget*>(QObject::parent()); }

	TextRecord* retrieve(const QUuid& id, const StdFsPath& path = StdFsPath());
	TextRecord* newDocument(const QUuid& id, const StdFsPath& path = StdFsPath());
	bool wasEvicted(const QUuid& id);
	StdFsPath tempPath(const QUuid& id);
	void recover(const QUuid& id, QString& initialText, QString& originalText, QString& title);
	bool writeEmptyFile(const StdFsPath& path);
	void tempSave();
	void backup(const QUuid& id);
	StdFsPath backupPath(const StdFsPath& path);
	bool overwrite(const QUuid& id);
};

using DocsManager = DocumentsManager;
