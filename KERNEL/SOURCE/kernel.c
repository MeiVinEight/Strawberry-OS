void _DllMainCRTStartup()
{
	*((unsigned short *) 0x00B8000) = 0x0753;
	while (1);
}