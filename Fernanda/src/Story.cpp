/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Story.cpp, Fernanda

#include "Story.h"

Story::Story(StdFsPath filePath, Mode mode)
{
	activeArchivePath = filePath;
	if (!QFile(activeArchivePath).exists())
		make(mode);
	dom->set(xml());
}

QVector<QStandardItem*> Story::items()
{
	QVector<QStandardItem*> result;
	QXmlStreamReader reader(dom->string());
	while (!reader.atEnd())
	{
		auto name = reader.name().toString();
		if (reader.isStartElement() && name == dom->tagRoot)
			reader.readNext();
		else if (reader.isStartElement() && name != dom->tagRoot)
		{
			auto item = items_recursor(reader);
			result << item;
		}
		else
			reader.readNextStartElement();
	}
	return result;
}

const QString Story::tempSaveOld_openNew(QString newKey, QString oldText)
{
	if (activeKey != nullptr)
		tempSave(activeKey, oldText);
	return tempOpen(newKey);
}

void Story::autoTempSave(QString text)
{
	if (activeKey == nullptr) return;
	tempSave(activeKey, text);
}

QStringList Story::edits(QString currentText)
{
	(cleanText != currentText)
		? amendEditsList(AmendEdits::Add)
		: amendEditsList(AmendEdits::Remove);
	return editedKeys;
}

bool Story::cut(QString key)
{
	auto result = false;
	auto keys = dom->cut(key);
	for (auto& _key : keys)
	{
		if (_key == activeKey)
		{
			activeKey = nullptr;
			result = true;
		}
		amendEditsList(AmendEdits::Remove, _key);
	}
	return result;
}

void Story::save(QString text)
{
	bak();
	if (activeKey != nullptr && isEdited(activeKey))
		tempSave(activeKey, text);
	auto cuts = dom->cuts();
	auto renames = dom->renames(Dom::Finalize::Yes);
	if (!cuts.isEmpty())
		archiver->cut(activeArchivePath, cuts);
	if (!renames.isEmpty())
		archiver->save(activeArchivePath, renames);
	if (!editedKeys.isEmpty())
	{
		QVector<Io::ArchiveWriteReadPaths> edits;
		for (auto& edited_key : editedKeys)
			edits << Io::ArchiveWriteReadPaths{ dom->element<StdFsPath>(edited_key, Dom::Element::Path), tempPath(edited_key) };
		archiver->add(activeArchivePath, edits);
		editedKeys.clear();
		tempOpen(activeKey);
	}
	archiver->add(activeArchivePath, Io::ArchiveWrite{ dom->string(), "story.xml" });
	dom->set(xml());
}

const Story::TotalCounts Story::totalCounts()
{
	auto result = TotalCounts{};
	QString file_content;
	for (auto& element : dom->elements())
	{
		if (element.tagName() != dom->tagFile) continue;
		auto key = element.attribute(dom->attributeKey);
		auto temp_path = tempPath(key);
		QFile(temp_path).exists()
			? file_content = Io::readFile(temp_path)
			: file_content = archiver->read(activeArchivePath, dom->element<StdFsPath>(key, Dom::Element::OrigPath));
		result.lines = result.lines + file_content.split(Text::regex(Text::Regex::NewLine)).count();
		result.words = result.words + file_content.split(Text::regex(Text::Regex::Split), Qt::SkipEmptyParts).count();
		result.characters = result.characters + file_content.count();
	}
	return result;
}

void Story::exportTo(StdFsPath path, To type)
{
	switch (type) {
	case To::Directory:
		archiver->extract(activeArchivePath, path / name<StdFsPath>());
		break;
	case To::PDF:
		toPdf(path);
		break;
	case To::PlainText:
		toPlainText(path);
		break;
	}
}

void Story::make(Mode mode)
{
	QVector<Io::ArchiveWriteReadPaths> write_read_paths;
	write_read_paths << Io::ArchiveWriteReadPaths{ Io::storyRoot };
	if (mode == Mode::Sample)
		write_read_paths << Sample::make();
	archiver->create(activeArchivePath, write_read_paths);
}

const QString Story::xml()
{
	QString result;
	auto xml_file = "story.xml";
	result = archiver->read(activeArchivePath, xml_file);
	if (result == nullptr)
	{
		newXml();
		result = archiver->read(activeArchivePath, xml_file);
	}
	return result;
}

void Story::newXml()
{
	QTemporaryDir temp_dir;
	auto temp_dir_path = Path::toStdFs(temp_dir.path());
	QString xml;
	archiver->extract(activeArchivePath, temp_dir_path);
	QXmlStreamWriter writer(&xml);
	writer.setAutoFormatting(true);
	writer.writeStartDocument();
	writer.writeStartElement(dom->tagRoot);
	writer.writeAttribute(dom->attributeRelativePath, Path::toQString(Io::storyRoot));
	newXml_recursor(writer, temp_dir_path / Io::storyRoot);
	writer.writeEndDocument();
	QTemporaryDir temp_dir_2;
	auto xml_file = "story.xml";
	auto temp_dir_2_path = Path::toStdFs(temp_dir_2.path());
	auto temp_xml_path = temp_dir_2_path / xml_file;
	Io::writeFile(temp_xml_path, xml);
	archiver->add(activeArchivePath, temp_xml_path, xml_file);
}

void Story::newXml_recursor(QXmlStreamWriter& writer, StdFsPath readPath, StdFsPath rootPath)
{
	if (rootPath.empty())
		rootPath = readPath;
	QDirIterator it(Path::toQString(readPath), QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
	auto expanded_value = "false";
	while (it.hasNext())
	{
		it.next();
		auto relative_path = Io::storyRoot / std::filesystem::relative(Path::toStdFs(it.filePath()), rootPath);
		if (it.fileInfo().isDir())
		{
			writer.writeStartElement(dom->tagDir);
			writer.writeAttribute(dom->attributeRelativePath, Path::toQString(relative_path, true));
			writer.writeAttribute(dom->attributeKey, QUuid::createUuid().toString(QUuid::WithoutBraces));
			writer.writeAttribute(dom->attributeExpanded, expanded_value);
			newXml_recursor(writer, readPath / Path::toStdFs(it.fileName()), rootPath);
			writer.writeEndElement();
		}
		else
		{
			writer.writeStartElement(dom->tagFile);
			writer.writeAttribute(dom->attributeRelativePath, Path::toQString(relative_path, true));
			writer.writeAttribute(dom->attributeKey, QUuid::createUuid().toString(QUuid::WithoutBraces));
			writer.writeAttribute(dom->attributeExpanded, expanded_value);
			writer.writeEndElement();
		}
	}
}

QStandardItem* Story::items_recursor(QXmlStreamReader& reader)
{
	auto type = reader.name().toString();
	auto key = reader.attributes().value(dom->attributeKey).toString();
	auto name = dom->element<QString>(key, Dom::Element::Name);
	auto expanded = reader.attributes().value(dom->attributeExpanded).toString();
	auto parent = dom->element<QDomElement>(key).hasChildNodes();
	auto result = new QStandardItem;
	result->setData(type, Qt::UserRole);
	result->setData(key, Qt::UserRole + 1);
	result->setData(name, Qt::UserRole + 2);
	result->setData(expanded, Qt::UserRole + 3);
	result->setData(parent, Qt::UserRole + 4);
	reader.readNext();
	while (!reader.isEndElement())
	{
		if (reader.isStartElement())
		{
			auto child_item = items_recursor(reader);
			result->appendRow(child_item);
		}
		else
			reader.readNext();
	}
	reader.readNext();
	return result;
}

void Story::tempSave(QString key, QString text)
{
	auto temp_file = tempPath(key);
	Io::writeFile(temp_file, text);
}

const QString Story::tempOpen(QString newKey)
{
	activeKey = newKey;
	auto archive_read_path = dom->element<StdFsPath>(newKey, Dom::Element::OrigPath);
	!archive_read_path.empty()
		? cleanText = archiver->read(activeArchivePath, archive_read_path)
		: cleanText = nullptr;
	auto temp_path = tempPath(newKey);
	QString result;
	QFile(temp_path).exists()
		? result = Io::readFile(temp_path)
		: result = cleanText;
	return result;
}

const Story::StdFsPath Story::tempPath(QString key)
{
	auto relative_path = Path::toStdFs(key + Io::tempExtension);
	auto temps_dir = UserData::doThis(UserData::Operation::GetActiveTemp);
	auto project_temp = temps_dir / name<StdFsPath>();
	return project_temp / relative_path;
}

void Story::amendEditsList(AmendEdits operation, QString key)
{
	if (key == nullptr && activeKey == nullptr) return;
	if (key == nullptr)
		key = activeKey;
	switch (operation) {
	case AmendEdits::Add:
		if (!editedKeys.contains(key))
			editedKeys << key;
		break;
	case AmendEdits::Remove:
		if (editedKeys.contains(key))
			editedKeys.removeAll(key);
		break;
	}
}

void Story::bak()
{
	auto underscore = "_";
	auto timestamp = UserData::timestamp().replace(Text::regex(Text::Regex::Forbidden), underscore).replace(Text::regex(Text::Regex::Space), underscore).replace(Text::regex(Text::Regex::NewLine), nullptr).toLower();
	timestamp.replace(QRegularExpression("(__)"), underscore).replace(QRegularExpression("(_$)"), nullptr);
	auto bak_file_name = name<QString>() + ".story." + timestamp + ".bak";
	auto bak_path = UserData::doThis(UserData::Operation::GetRollback) / Path::toStdFs(bak_file_name);
	auto q_file_bak = QFile(bak_path);
	if (q_file_bak.exists())
		q_file_bak.moveToTrash();
	Path::copy(activeArchivePath, bak_path);
}

const QString Story::readAllForExport()
{
	QString result;
	auto elements = dom->elements();
	auto& last_element = elements.last();
	for (auto& element : elements)
	{
		if (element.tagName() != dom->tagFile) continue;
		auto key = element.attribute(dom->attributeKey);
		auto temp_path = tempPath(key);
		QFile(temp_path).exists()
			? result = result + Io::readFile(temp_path)
			: result = result + archiver->read(activeArchivePath, dom->element<StdFsPath>(key, Dom::Element::OrigPath));
		if (element != last_element)
			result = result + Text::newLines();
	}
	return result;
}

void Story::toPdf(StdFsPath path)
{
	QPrinter printer;
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(Path::toQString(path));
	QTextDocument doc;
	doc.setPlainText(readAllForExport());
	doc.print(&printer);
}

// Story.cpp, Fernanda
