#include "ActionGroup.h"

ActionGroup::Bespoke ActionGroup::bespoke(QVariant data, QString label)
{
	return Bespoke{ data, label };
}

ActionGroup* ActionGroup::fromQrc(const QStringList& qrcPaths, QStringList extensions,
	StdFsPathList systemPaths, QObject* parent, Slot slot)
{
	auto group = new ActionGroup(parent);
	checkExtensions(extensions);
	auto entries = Path::gatherQStringFilePaths(qrcPaths, extensions);
	for (auto& entry : entries) {
		auto label = Path::qStringName(entry);
		addActionToGroup(group, label, entry, parent, slot);
	}
	alphabetize(group);
	group->setExclusive(true);
	return group;
}

ActionGroup* ActionGroup::fromQrc(const QString& qrcPath, QString extension,
	StdFsPath systemPath, QObject* parent, Slot slot)
{
	return fromQrc(QStringList{ qrcPath }, QStringList{ extension },
		StdFsPathList{ systemPath }, parent, slot);
}

ActionGroup* ActionGroup::fromQrc(const QStringList& qrcPaths, QStringList extensions,
	StdFsPath systemPath, QObject* parent, Slot slot)
{
	return fromQrc(qrcPaths, extensions, StdFsPathList{ systemPath }, parent, slot);
}

ActionGroup* ActionGroup::fromBespoke(BespokeList entries, QObject* parent,
	Slot slot)
{
	auto group = new ActionGroup(parent);
	for (auto& data_pair : entries) {
		auto& label = data_pair.label;
		if (label.isEmpty())
			label = data_pair.data.toString();
		addActionToGroup(group, label, data_pair.data, parent, slot);
	}
	alphabetize(group);
	group->setExclusive(true);
	return group;
}

void ActionGroup::addActionToGroup(ActionGroup* actionGroup, const QString& label,
	const QVariant& data, QObject* parent, Slot slot)
{
	auto action = new QAction(tr(label.toUtf8()), actionGroup);
	action->setData(data);
	action->setCheckable(true);
	if (slot)
		connect(action, &QAction::toggled, parent, slot);
}

void ActionGroup::checkExtensions(QStringList& extensions)
{
	for (auto& extension : extensions)
		if (!extension.startsWith("*"))
			extension = "*" + extension;
}

void ActionGroup::alphabetize(ActionGroup* actionGroup)
{
	QVector<QAction*> sorted_actions = actionGroup->actions();
	std::sort(sorted_actions.begin(), sorted_actions.end(), [](auto& lhs, auto& rhs) {
		return lhs->text() < rhs->text();
		});
	for (auto& action : actionGroup->actions())
		actionGroup->removeAction(action);
	for (auto& action : sorted_actions)
		actionGroup->addAction(action);
}
