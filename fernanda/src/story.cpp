// story.cpp, Fernanda

#include "story.h"

Story::Story(FsPath filePath, Op opt)
{
	activeArcPath = filePath;
	if (!QFile(activeArcPath).exists())
		make(opt);
	dom->set(xml());
}

const QString Story::devGetDom(Dom::Doc doc)
{
	return dom->string(doc);
}

QVector<Io::ArcRename> Story::devGetRenames()
{
	return dom->renames();
}

const QStringList Story::devGetEditedKeys()
{
	return editedKeys;
}

const Story::FsPath Story::devGetActiveTemp()
{
	return Ud::userData(Ud::Op::GetTemp) / name<FsPath>();
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

const QString Story::key()
{
	return activeKey;
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

bool Story::hasChanges()
{
	if (!editedKeys.isEmpty() || dom->hasChanges()) return true;
	return false;
}

void Story::setItemExpansion(QString key, bool isExpanded)
{
	dom->write(key, isExpanded, Dom::Write::Expanded);
}

void Story::move(QString pivotKey, QString fulcrumKey, Io::Move position)
{
	dom->move(pivotKey, fulcrumKey, position);
}

void Story::rename(QString newName, QString key)
{
	dom->rename(newName, key);
}

void Story::add(QString newName, Path::Type type, QString parentKey)
{
	dom->add(newName, type, parentKey);
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
		archiver->cut(activeArcPath, cuts);
	if (!renames.isEmpty())
		archiver->save(activeArcPath, renames);
	if (!editedKeys.isEmpty())
	{
		QVector<Io::ArcWRPaths> edits;
		for (auto& edited_key : editedKeys)
			edits << Io::ArcWRPaths{ dom->element<FsPath>(edited_key, Dom::Element::Path), tempPath(edited_key) };
		archiver->add(activeArcPath, edits);
		editedKeys.clear();
		tempOpen(activeKey);
	}
	archiver->add(activeArcPath, Io::ArcWrite{ dom->string(), "story.xml" });
	dom->set(xml());
}

void Story::make(Op opt)
{
	QVector<Io::ArcWRPaths> wr_paths;
	wr_paths << Io::ArcWRPaths{ Io::storyRoot };
	if (opt == Op::Sample)
		wr_paths << Sample::make();
	archiver->create(activeArcPath, wr_paths);
}

const QString Story::xml()
{
	QString result;
	auto xml_file = "story.xml";
	result = archiver->read(activeArcPath, xml_file);
	if (result == nullptr)
	{
		newXml();
		result = archiver->read(activeArcPath, xml_file);
	}
	return result;
}

void Story::newXml()
{
	QTemporaryDir temp_dir;
	auto temp_dir_path = Path::toFs(temp_dir.path());
	QString xml;
	archiver->extract(activeArcPath, temp_dir_path);
	QXmlStreamWriter writer(&xml);
	writer.setAutoFormatting(true);
	writer.writeStartDocument();
	writer.writeStartElement(dom->tagRoot);
	writer.writeAttribute(dom->attrRelPath, Path::toQString(Io::storyRoot));
	newXml_recursor(writer, temp_dir_path / Io::storyRoot);
	writer.writeEndDocument();
	QTemporaryDir temp_dir_2;
	auto xml_file = "story.xml";
	auto temp_dir_2_path = Path::toFs(temp_dir_2.path());
	auto temp_xml_path = temp_dir_2_path / xml_file;
	Io::writeFile(temp_xml_path, xml);
	archiver->add(activeArcPath, temp_xml_path, xml_file);
}

void Story::newXml_recursor(QXmlStreamWriter& writer, FsPath rPath, FsPath rootPath)
{
	if (rootPath.empty())
		rootPath = rPath;
	QDirIterator it(Path::toQString(rPath), QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
	auto expanded_value = "false";
	while (it.hasNext())
	{
		it.next();
		auto rel_path = Io::storyRoot / std::filesystem::relative(Path::toFs(it.filePath()), rootPath);
		if (it.fileInfo().isDir())
		{
			writer.writeStartElement(dom->tagDir);
			writer.writeAttribute(dom->attrRelPath, Path::toQString(rel_path, true));
			writer.writeAttribute(dom->attrKey, QUuid::createUuid().toString(QUuid::WithoutBraces));
			writer.writeAttribute(dom->attrExpanded, expanded_value);
			newXml_recursor(writer, rPath / Path::toFs(it.fileName()), rootPath);
			writer.writeEndElement();
		}
		else
		{
			writer.writeStartElement(dom->tagFile);
			writer.writeAttribute(dom->attrRelPath, Path::toQString(rel_path, true));
			writer.writeAttribute(dom->attrKey, QUuid::createUuid().toString(QUuid::WithoutBraces));
			writer.writeAttribute(dom->attrExpanded, expanded_value);
			writer.writeEndElement();
		}
	}
}

QStandardItem* Story::items_recursor(QXmlStreamReader& reader)
{
	auto type = reader.name().toString();
	auto key = reader.attributes().value(dom->attrKey).toString();
	auto name = dom->element<QString>(key, Dom::Element::Name);
	auto expanded = reader.attributes().value(dom->attrExpanded).toString();
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
	auto arc_r_path = dom->element<FsPath>(newKey, Dom::Element::OrigPath);
	(!arc_r_path.empty())
		? cleanText = archiver->read(activeArcPath, arc_r_path)
		: cleanText = nullptr;
	auto temp_path = tempPath(newKey);
	QString result;
	(QFile(temp_path).exists())
		? result = Io::readFile(temp_path)
		: result = cleanText;
	return result;
}

const Story::FsPath Story::tempPath(QString key)
{
	auto rel_path = Path::toFs(key + Io::tempExt);
	auto temps_dir = Ud::userData(Ud::Op::GetTemp);
	auto proj_temp = temps_dir / name<FsPath>();
	return proj_temp / rel_path;
}

void Story::amendEditsList(AmendEdits op, QString key)
{
	if (key == nullptr && activeKey == nullptr) return;
	if (key == nullptr)
		key = activeKey;
	switch (op) {
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

bool Story::isEdited(QString key)
{
	if (editedKeys.contains(key)) return true;
	return false;
}

void Story::bak()
{
	auto underscore = "_";
	auto timestamp = Ud::timestamp().replace(Text::regex(Text::Re::Forbidden), underscore).replace(Text::regex(Text::Re::Space), underscore).replace(Text::regex(Text::Re::NewLine), nullptr).toLower();
	timestamp.replace(QRegularExpression("(__)"), underscore).replace(QRegularExpression("(_$)"), nullptr);
	auto bak_file_name = name<QString>() + ".story." + timestamp + ".bak";
	auto bak_path = Ud::userData(Ud::Op::GetRollback) / Path::toFs(bak_file_name);
	auto qf_bak = QFile(bak_path);
	if (qf_bak.exists())
		qf_bak.moveToTrash();
	std::filesystem::copy_file(activeArcPath, bak_path);
}

// story.cpp, Fernanda
