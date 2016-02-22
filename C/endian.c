#include <stdio.h>

int main()
{
	union
	{
		short s;
		char c[sizeof(short)];
	} un;

	un.s = 0x0102;

	if (2 != sizeof(short))
	{
		printf("sizeof(short) = %ld.\n", sizeof(short));
		return -1;
	}

	if (0x01 == un.c[0] && 0x02 == un.c[1])
	{
		printf("big-endian.\n");
	}
	else if (0x02 == un.c[0] && 0x01 == un.c[1])
	{
		printf("little-endian.\n");
	}
	else
	{
		printf("unknown.\n");
	}

	return 0;
}



