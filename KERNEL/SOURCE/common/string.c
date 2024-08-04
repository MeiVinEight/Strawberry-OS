#include <common/string.h>

QWORD strlen(char *x)
{
	char *y = x;
	for (; *y; y++);
	return y - x;
}
char *LeadingWhitespace(char *beg, char *end)
{
	while (end > beg && *--end <= 0x20)
	{
		*end = 0;
	}
	while (beg < end && *beg <= 0x20)
	{
		beg++;
	}
	return beg;
}