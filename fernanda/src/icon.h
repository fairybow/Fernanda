/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

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
		File,
		Files,
		Folder,
		Folders,
		FolderOpen,
		Pushpin,
		QuestionMark,
		Tea,
		Timer
	};

	inline const QString draw(Name name)
	{
		QString result;
		switch (name) {
		case Name::ArrowDown:
			result = QStringLiteral("\U000025BC");
			break;
		case Name::ArrowNext:
			result = QStringLiteral("\U000025BC");
			break;
		case Name::ArrowPrevious:
			result = QStringLiteral("\U000025B2");
			break;
		case Name::ArrowUp:
			result = QStringLiteral("\U000025B2");
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
		case Name::Tea:
			result = QStringLiteral("\U0001F9CB");
			break;
		case Name::Timer:
			result = QStringLiteral("\U000023F2");
			break;
		}
		return result;
	}
}

// icon.h, Fernanda
