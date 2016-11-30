#include <core/common-help.h>

void splitstring(char *str, char split, std::vector<char *> &ret)
{
	bool found = false;
	char *last = str;
	while (true)
	{
		if (*str==0x00)
		{
			ret.push_back(last);
			break;
		}

		if (found)
		{
			ret.push_back(last);
			last = str;
		}

		if (*str==split)
		{
			*str = 0x00;
			found = true;
		}
		else
		{
			found = false;
		}

		str++;
	}
}