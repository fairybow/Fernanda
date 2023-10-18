#pragma once

namespace Regex
{
	constexpr char URL_BEGINNING[] = \
		"(https:\\/\\/|www.)";

	constexpr char LEADING_WHITESPACE[] = \
		"(\\s|\\n|\\r|\U00002029|^)+";

	constexpr char FORMAT_XML_ATTRIBUTE_CSSVALUE[] = \
		"%1:[^;]+";

	constexpr char FORBIDDEN[] = \
		R"((<|>|:|\/|\\|\||\?|\*|\"))";
}
