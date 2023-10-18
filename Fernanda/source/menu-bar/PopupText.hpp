#pragma once

#include "../common/HtmlString.hpp"
#include "../Version.hpp"

namespace PopupText
{
	namespace
	{
		inline const QString repo()
		{
			return HtmlString::link("https://github.com/fairybow/Fernanda");
		}
	}

	inline const QString about()
	{
		return
		{
			HtmlString::heading("About Fernanda", 3)
			% "Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)"
			% "It's a personal project and a work-in-progress."
			% HtmlString::heading("Version", 3)
			% VER_FILEVERSION_STR
			% "See " + repo() + " for more information."
		};
	}
}
