/* (Ñ) AHOHNMYC, 2017
 * part of Universal GUI project
 * CC-NC-SA
 */

#include <windows.h>

#define MAX_PARAM_COUNT 256
LPCSTR * CommandLineToArgvA (LPSTR lpCmdLine, int *pNumArgs) {
	DWORD i = 0, lpCmdLineLen = strlen(lpCmdLine);
	LPCSTR * argv = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, MAX_PARAM_COUNT*sizeof(LPCSTR));
	BOOL quoteFlag = FALSE;

	argv[0] = lpCmdLine;

	CHAR stopSymbol = (lpCmdLine[0] == '"') ? '"' : ' ';
	while ( lpCmdLine[++i] != stopSymbol );

	if (stopSymbol == '"') argv[0]++, lpCmdLine[i] = '\0';

	*pNumArgs = 1;
	for (; i < lpCmdLineLen; i++) {
		if (lpCmdLine[i] == '"')
			lpCmdLine[i] = '\0', quoteFlag = !quoteFlag;
		if (lpCmdLine[i] == ' ' && !quoteFlag)
			lpCmdLine[i] = '\0';
		if (lpCmdLine[i-1] == '\0' && lpCmdLine[i] != '\0')
			argv[++*pNumArgs-1] = lpCmdLine+i;
	}
	return argv;
}
