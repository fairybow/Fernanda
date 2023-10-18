#pragma once

namespace StyleRegex
{
	constexpr char THEME_SHEET_LINE[] = \
		"(@.*\\n?)";

	constexpr char THEME_SHEET_VALUE[] = \
		"(\\s=.*;)";

	constexpr char THEME_SHEET_VARIABLE[] = \
		"(@.*=\\s)";

	constexpr char CURSOR_BLOCK[] = \
		"Cursor[^}]*}";

	constexpr char CURSOR_COLOR_LINE[] = \
		"(\\scolor = )(.*)(;)";

	constexpr char CURSOR_UNDER_COLOR_LINE[] = \
		"(\\sunder-color = )(.*)(;)";
}
