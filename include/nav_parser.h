#ifndef NABENABE_NAV_PARSER_H
#define NABENABE_NAV_PARSER_H

class NABE_KeyValues;

class NavParser
{
public:
	NavParser()
	{
	}

	bool Parse(const char* file_path, const char* maps_path, const char* navs_path, NABE_KeyValues& out_keyvalues);
};

#endif // NABENABE_NAV_PARSER_H