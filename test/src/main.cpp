#ifdef _MSC_VER
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#include <cstring>
#endif
#include <iostream>
#include <sstream>
#include "NoMercyServer.h"

//=============================================================================================================

static const char* c_szLicenseId = "ac_lic_cod";
static const uint32_t c_nGameID = 31;
static const char* c_szApiKey = "1234";

//=============================================================================================================

int main(void)
{
	static CNoMercyServer s_kNoMercyServer;

	if (!CNoMercyServer::Instance().Initialize(c_szLicenseId, c_nGameID, c_szApiKey))
	{
		std::cerr << "Failed to initialize the server." << std::endl;
		return -1;
	}

	CNoMercyServer::Instance().TestFlight();

	CNoMercyServer::Instance().Release();
	return 0;
}
