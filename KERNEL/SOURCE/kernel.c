void _DllMainCRTStartup()
{
	*((char *) 0x00B8000) = 0x20;
	while (1);
}