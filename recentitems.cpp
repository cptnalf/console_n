
#include "stdafx.h"
#include <windows.h>
#include <shlobj.h>

void VisitConfigFile(const wchar_t* configFile)
{
	if ((g_osInfo.dwMajorVersion >= 6)
			&& (g_osInfo.dwMinorVersion >= 1))
		{
			SHAddToRecentDocs(SHARD_PATHW, configFile);
		}
}
