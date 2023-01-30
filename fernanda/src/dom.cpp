/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

// dom.cpp, Fernanda

#include "dom.h"

void Dom::set(QString xmlDocument)
{
	self.setContent(xmlDocument);
	initialSelf.setContent(string());
	cutElements.setContent(QStringLiteral("<root/>"));
}

const QString Dom::string(Document document)
{
	QString result = nullptr;
	switch (document) {
	case Document::Current:
		result = self.toString();
		break;
	case Document::Cuts:
		result = cutElements.toString();
		break;
	case Document::Initial:
		result = initialSelf.toString();
		break;
	}
	return result;
}

bool Dom::hasChanges()
{
	if (string() != string(Document::Initial)) return true;
	return false;
}

void Dom::move(QString pivotKey, QString fulcrumKey, Io::Move position)
{
	auto pivot = element<QDomElement>(pivotKey);
	auto fulcrum = element<QDomElement>(fulcrumKey);
	StdFsPath new_pivot_path;
	StdFsPath new_pivot_parent_path;
	auto pivot_name = element<QString>(pivotKey, Element::Name);
	if (isFile(pivot))
		pivot_name = pivot_name + Io::extension;
	switch (position) {
	case Io::Move::Above:
		fulcrum.parentNode().insertBefore(pivot, fulcrum);
		movePaths(new_pivot_path, new_pivot_parent_path, pivot_name, fulcrumKey);
		break;
	case Io::Move::Below:
		fulcrum.parentNode().insertAfter(pivot, fulcrum);
		movePaths(new_pivot_path, new_pivot_parent_path, pivot_name, fulcrumKey);
		break;
	case Io::Move::On:
		fulcrum.appendChild(pivot);
		if (isDir(fulcrum))
		{
			auto fulcrum_path = element<StdFsPath>(fulcrumKey, Element::Path);
			new_pivot_path = fulcrum_path / Path::toStdFs(pivot_name);
			new_pivot_parent_path = fulcrum_path;
		}
		else if (isFile(fulcrum))
			movePaths(new_pivot_path, new_pivot_parent_path, pivot_name, fulcrumKey);
		break;
	case Io::Move::Viewport:
		self.documentElement().appendChild(pivot);
		new_pivot_parent_path = Io::storyRoot;
		new_pivot_path = new_pivot_parent_path / Path::toStdFs(pivot_name);
		break;
	}
	StdFsPath children_base_path;
	if (isDir(pivot))
		children_base_path = new_pivot_path;
	else if (isFile(pivot))
		children_base_path = new_pivot_parent_path;
	auto renames = prepareChildRenames_recursor(pivot, children_base_path);
	renames << Io::ArchiveRename{ pivotKey, new_pivot_path };
	for (auto& rename : renames)
		write(rename.key, Path::toQString(rename.relativePath, true), Write::Rename);
}

void Dom::rename(QString newName, QString key)
{
	if (newName == nullptr) return;
	auto elem = element<QDomElement>(key);
	if (isFile(elem))
		newName = newName + Io::extension;
	auto parent_path = element<StdFsPath>(key, Element::ParentDirPath);
	auto new_path = parent_path / Path::toStdFs(newName);
	QVector<Io::ArchiveRename> renames = { Io::ArchiveRename{ key, new_path } };
	if (isDir(elem))
		renames << prepareChildRenames_recursor(elem, new_path, ChildRenames::InPlace);
	for (auto& rename : renames)
		write(rename.key, Path::toQString(rename.relativePath, true), Write::Rename);
}

void Dom::add(QString newName, Path::Type type, QString parentKey)
{
	if (newName == nullptr) return;
	QString tag_name;
	QString name;
	switch (type) {
	case Path::Type::Dir:
		tag_name = tagDir;
		name = newName;
		break;
	case Path::Type::File:
		tag_name = tagFile;
		name = newName + Io::extension;
		break;
	}
	auto elem = self.createElement(tag_name);
	auto key = QUuid::createUuid().toString(QUuid::WithoutBraces);
	elem.setAttribute(attributeKey, key);
	elem.setAttribute(attributeExpanded, "false");
	QDomElement parent_element;
	StdFsPath nearest_dir;
	if (parentKey != nullptr)
	{
		parent_element = element<QDomElement>(parentKey);
		if (isDir(parent_element))
			nearest_dir = element<StdFsPath>(parentKey, Element::Path);
		else
		{
			auto parent_dir_key = element<QString>(parentKey, Element::ParentDirKey);
			if (parent_dir_key != nullptr)
				nearest_dir = element<StdFsPath>(parent_dir_key, Element::Path);
		}
	}
	else
		parent_element = self.documentElement();
	if (nearest_dir.empty())
		nearest_dir = Io::storyRoot;
	auto path = nearest_dir / Path::toStdFs(name);
	parent_element.appendChild(elem);
	write(key, Path::toQString(path, true), Write::Rename);
}

QStringList Dom::cut(QString key)
{
	QStringList result;
	result << key;
	auto elem = element<QDomElement>(key);
	result << childKeys_recursor(elem);
	cutElements.documentElement().appendChild(cutElements.importNode(elem, true));
	elem.parentNode().removeChild(elem);
	return result;
}

QVector<Io::ArchiveRename> Dom::cuts()
{
	QVector<Io::ArchiveRename> result;
	auto cut_elements = elements(cutElements);
	for (auto& cut_element : cut_elements)
	{
		auto key = cut_element.attribute(attributeKey);
		auto rename = Path::toStdFs(cut_element.attribute(attributeRename));
		auto relative_path = Path::toStdFs(cut_element.attribute(attributeRelativePath));
		Path::Type type{};
		(isDir(cut_element))
			? type = Path::Type::Dir
			: type = Path::Type::File;
		(relative_path.empty())
			? result << Io::ArchiveRename{ key, rename, std::optional<StdFsPath>(), type }
			: result << Io::ArchiveRename{ key, (rename.empty() ? relative_path : rename), std::optional<StdFsPath>(relative_path), type};
	}
	return result;
}

QVector<Io::ArchiveRename> Dom::renames(Finalize finalize)
{
	QVector<Io::ArchiveRename> result;
	for (auto& renamed_element : elements(attributeRename))
	{
		auto key = renamed_element.attribute(attributeKey);
		auto rename = Path::toStdFs(renamed_element.attribute(attributeRename));
		auto relative_path = Path::toStdFs(renamed_element.attribute(attributeRelativePath));
		if (relative_path == rename)
			renamed_element.removeAttribute(attributeRename);
		else
		{
			if (relative_path.empty())
			{
				Path::Type type{};
				(isDir(renamed_element))
					? type = Path::Type::Dir
					: type = Path::Type::File;
				result << Io::ArchiveRename{ key, rename, std::optional<StdFsPath>(), type };
			}
			else
				result << Io::ArchiveRename{ key, rename, relative_path };
			if (finalize == Finalize::Yes)
			{
				renamed_element.setAttribute(attributeRelativePath, Path::toQString(rename, true));
				renamed_element.removeAttribute(attributeRename);
			}
		}
	}
	return result;
}

QVector<QDomElement> Dom::elements(QDomDocument document)
{
	if (document.isNull())
		document = self;
	QVector<QDomElement> result;
	auto root = document.documentElement();
	auto next_node = root.firstChildElement();
	while (!next_node.isNull())
	{
		result << elements_recursor(next_node);
		next_node = next_node.nextSiblingElement();
	}
	return result;
}

QVector<QDomElement> Dom::elements(QString attribute, QString value)
{
	QVector<QDomElement> result;
	auto root = self.documentElement();
	auto next_node = root.firstChildElement();
	while (!next_node.isNull())
	{
		result << elementsByAttribute_recursor(next_node, attribute, value);
		next_node = next_node.nextSiblingElement();
	}
	return result;
}

QDomElement Dom::element_recursor(QDomElement node, QString key, QDomElement result)
{
	if (node.attribute(attributeKey) == key)
		result = node;
	auto child_node = node.firstChildElement();
	while (!child_node.isNull())
	{
		auto elem = element_recursor(child_node, key);
		if (!elem.isNull())
			result = elem;
		child_node = child_node.nextSiblingElement();
	}
	return result;
}

QVector<QDomElement> Dom::elements_recursor(QDomElement node, QVector<QDomElement> result)
{
	if (node.isElement())
		result << node;
	auto child_node = node.firstChildElement();
	while (!child_node.isNull())
	{
		result << elements_recursor(child_node);
		child_node = child_node.nextSiblingElement();
	}
	return result;
}

QVector<QDomElement> Dom::elementsByAttribute_recursor(QDomElement node, QString attribute, QString value, QVector<QDomElement> result)
{
	if (value != nullptr)
	{
		if (node.hasAttribute(attribute) && node.attribute(attribute) == value)
			result << node;
	}
	else
	{
		if (node.hasAttribute(attribute))
			result << node;
	}
	auto child_node = node.firstChildElement();
	while (!child_node.isNull())
	{
		result << elementsByAttribute_recursor(child_node, attribute, value);
		child_node = child_node.nextSiblingElement();
	}
	return result;
}

void Dom::movePaths(StdFsPath& newPivotPath, StdFsPath& newPivotParentPath, QString pivotName, QString fulcrumKey)
{
	auto fulcrum_parent_key = element<QString>(fulcrumKey, Element::ParentDirKey);
	(fulcrum_parent_key == nullptr)
		? newPivotParentPath = Io::storyRoot
		: newPivotParentPath = element<StdFsPath>(fulcrum_parent_key, Element::Path);
	newPivotPath = newPivotParentPath / Path::toStdFs(pivotName);
}

QStringList Dom::childKeys_recursor(QDomElement node, QStringList result)
{
	auto child_node = node.firstChildElement();
	while (!child_node.isNull())
	{
		if (isFile(child_node))
			result << child_node.attribute(attributeKey);
		result << childKeys_recursor(child_node);
		child_node = child_node.nextSiblingElement();
	}
	return result;
}

QVector<Io::ArchiveRename> Dom::prepareChildRenames_recursor(QDomElement node, StdFsPath stemPathParent, ChildRenames renameType, QVector<Io::ArchiveRename> result)
{
	auto child = node.firstChildElement();
	while (!child.isNull())
	{
		auto child_key = child.attribute(attributeKey);
		QString child_name;
		if (isDir(child))
			child_name = element<QString>(child_key, Element::Name);
		else if (isFile(child))
			child_name = element<QString>(child_key, Element::Name) + Io::extension;
		auto nearest_dir_key = element<QString>(child_key, Element::ParentDirKey);
		auto nearest_dir_name = element<QString>(nearest_dir_key, Element::Name);
		auto stem_path_name = Path::getName<QString>(stemPathParent);
		StdFsPath next_stem_path;
		if (stem_path_name == nearest_dir_name || renameType == ChildRenames::InPlace)
			next_stem_path = stemPathParent;
		else
			next_stem_path = stemPathParent / Path::toStdFs(nearest_dir_name);
		auto relative_path = next_stem_path / Path::toStdFs(child_name);
		result << Io::ArchiveRename{ child_key, relative_path };
		result << prepareChildRenames_recursor(child, next_stem_path, renameType);
		child = child.nextSiblingElement();
	}
	return result;
}

Dom::StdFsPath Dom::filterPath(QDomElement elem, Filter filter)
{
	StdFsPath result;
	switch (filter) {
	case Filter::OrigToNullptr:
		if (elem.hasAttribute(attributeRelativePath))
			result = Path::toStdFs(elem.attribute(attributeRelativePath));
		break;
	case Filter::RenameToOrig:
		(elem.hasAttribute(attributeRename))
			? result = Path::toStdFs(elem.attribute(attributeRename))
			: result = Path::toStdFs(elem.attribute(attributeRelativePath));
		break;
	}
	return result;
}

// dom.cpp, Fernanda
