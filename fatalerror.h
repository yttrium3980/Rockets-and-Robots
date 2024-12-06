void fatalerror(char *str)
{
	MessageBox(NULL, str, "Fatal Error", MB_OK);
	ExitProcess(1);
}