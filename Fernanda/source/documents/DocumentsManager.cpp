#include "DocumentsManager.h"

IdBank DocumentsManager::s_idBank;

DocumentsManager::DocumentsManager(
	const Folders& folders,
	QWidget* parent,
	int cacheMaxCost)
	: QObject(parent),
	m_userFolder(folders.user),
	m_tempFolder(folders.temp),
	m_backupFolder(folders.backup)
{
	DocumentsCache::setMaxCost(cacheMaxCost);
	connect(m_continuousTempSave, &QTimer::timeout, this, [&] {
		tempSave();
		});
}

DocumentsManager::StdFsPath DocumentsManager::newFileDialog(const QString& name)
{
	auto file_name = Path::toStdFs(name);
	auto path = QFileDialog::getSaveFileName(
		parent(), tr("Create a new file..."),
		Path::toQString(m_userFolder / file_name), tr(DIALOG_FILE_TYPE));

	return Path::toStdFs(path);
}

DocumentsManager::StdFsPath DocumentsManager::openFileDialog()
{
	auto path = QFileDialog::getOpenFileName(
		parent(), tr("Open an existing file..."),
		Path::toQString(m_userFolder), tr(DIALOG_FILE_TYPE));

	return Path::toStdFs(path);
}

TextRecord* DocumentsManager::setActive(const QUuid& id)
{
	tempSave();
	m_activeId = id;
	m_continuousTempSave->start(25000);
	return active();
}

QUuid DocumentsManager::newUnsaved()
{
	auto id = s_idBank.recordNew();
	create(id);
	return id;
}

QUuid DocumentsManager::fromDisk(PathType type, const StdFsPath& path)
{
	QUuid id = s_idBank.fromPath(path);
	if (type == PathType::New)
		writeEmptyFile(path);
	create(id, path);
		
	return id;
}

bool DocumentsManager::toDisk()
{
	if (!hasActive()) return false;
	tempSave();

	auto extant_path = s_idBank.path(m_activeId);
	if (Path::isValid(extant_path))
		backup(m_activeId);
	else {
		auto unsaved_file_name = active()->firstBlockText().left(30);
		auto new_path = newFileDialog(unsaved_file_name);
		if (new_path.empty()) return false;

		writeEmptyFile(new_path);
		s_idBank.associate(new_path, m_activeId);
	}

	return overwrite(m_activeId);
}

void DocumentsManager::close(const QUuid& id)
{
	if (m_activeId == id)
		m_activeId = QUuid();

	auto& cache = DocsCache::instance();
	cache.remove(id);
	s_idBank.remove(id);
	StdFs::remove(tempPath(id));
}

TextRecord* DocumentsManager::retrieve(const QUuid& id, const StdFsPath& path)
{
	auto& cache = DocsCache::instance();
	auto document = cache.document(id);
	if (!document)
		document = newDocument(id, path);

	return document;
}

TextRecord* DocumentsManager::newDocument(const QUuid& id, const StdFsPath& path)
{
	QString initial_text;
	QString original_text;
	QString title;

	if (wasEvicted(id))
		recover(id, initial_text, original_text, title);
	else if (!path.empty()) {
		title = Path::qStringName(path);
		Io::toStrings(path, initial_text, original_text);
	}

	auto document = new TextRecord(
		initial_text, original_text, title, id);
	auto& cache = DocsCache::instance();
	cache.add(document);

	return document;
}

bool DocumentsManager::wasEvicted(const QUuid& id)
{
	if (!s_idBank.contains(id)) return false;
	return StdFs::exists(tempPath(id));
}

DocumentsManager::StdFsPath DocumentsManager::tempPath(const QUuid& id)
{
	return m_tempFolder / Path::toStdFs(id.toString() + FILE_TYPE + "~");
}

void DocumentsManager::recover(const QUuid& id, QString& initialText, QString& originalText, QString& title)
{
	qDebug() << __FUNCTION__;

	auto temp_path = tempPath(id);
	if (StdFs::exists(temp_path))
		initialText = Io::readFile(temp_path);

	auto extant_path = s_idBank.path(id);
	if (Path::isValid(extant_path)) {
		originalText = Io::readFile(extant_path);
		title = Path::qStringName(extant_path);
	}

	// handle deleted original
	// file system watcher
}

bool DocumentsManager::writeEmptyFile(const StdFsPath& path)
{
	return Io::writeFile(path);
}

void DocumentsManager::tempSave()
{
	if (!hasActive()) return;
	Io::writeFile(tempPath(m_activeId), active()->text());
}

void DocumentsManager::backup(const QUuid& id)
{
	auto extant_path = s_idBank.path(id);
	auto backup_path = backupPath(extant_path);
	Path::copy(extant_path, backup_path);
}

DocumentsManager::StdFsPath DocumentsManager::backupPath(const StdFsPath& path)
{
	auto name = Path::qStringName(path);
	name += "---" + StringTools::time();
	name = StringTools::removeForbidden(name);

	return m_backupFolder / Path::toStdFs(name + ".bak");
}

bool DocumentsManager::overwrite(const QUuid& id)
{
	auto path = s_idBank.path(id);
	auto temp_path = tempPath(id);

	if (!Path::areValid(path, temp_path) ||
		!Path::move(temp_path, path, true))
		return false;

	auto& cache = DocsCache::instance();
	cache.remove(m_activeId);
	create(m_activeId, path);

	return true;
}
