// icon.h, Fernanda

#pragma once

#include <QString>

namespace Icon
{
	enum class Name {
		ArrowDown,
		ArrowNext,
		ArrowPrevious,
		ArrowUp,
		Balloon,
		File,
		Files,
		Folder,
		Folders,
		FolderOpen,
		Pushpin,
		QuestionMark
	};

	inline const QString draw(Name name)
	{
		QString result;
		switch (name) {
		case Name::ArrowDown:
			result = QStringLiteral("\U000023F7");
			break;
		case Name::ArrowNext:
			result = QStringLiteral("\U000023F7");
			break;
		case Name::ArrowPrevious:
			result = QStringLiteral("\U000023F6");
			break;
		case Name::ArrowUp:
			result = QStringLiteral("\U000023F6");
			break;
		case Name::Balloon:
			result = QStringLiteral("\U0001F388");
			break;
		case Name::File:
			result = QStringLiteral("\U0001F4C4");
			break;
		case Name::Files:
			result = QStringLiteral("\U0001F4D1");
			break;
		case Name::Folder:
			result = QStringLiteral("\U0001F4C1");
			break;
		case Name::Folders:
			result = QStringLiteral("\U0001F5C2");
			break;
		case Name::FolderOpen:
			result = QStringLiteral("\U0001F4C2");
			break;
		case Name::Pushpin:
			result = QStringLiteral("\U0001F4CC");
			break;
		case Name::QuestionMark:
			result = QStringLiteral("\U00002754");
			break;
		}
		return result;
	}
}

// icon.h, Fernanda
