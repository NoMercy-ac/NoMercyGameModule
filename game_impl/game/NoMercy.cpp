#ifdef _MSC_VER
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#include <cstring>
#endif
#include <iostream>
#include <sstream>
#include "NoMercy.h"

//=============================================================================================================

#ifdef _MSC_VER

#define GET_API_PTR(mod, api)\
	GetProcAddress(mod, api)

#define GET_ERR_STR()\
	std::to_string(GetLastError())

#else

#define GET_API_PTR(mod, api)\
	dlsym(mod, api)

#define GET_ERR_STR()\
	dlerror()

#endif

#define DECLARE_API(api)\
	api = reinterpret_cast<api##_t>(GET_API_PTR(hModule, #api));\
	if (!api) {\
		std::cerr << "Failed to load API function: " << #api << " with error: " << GET_ERR_STR() << std::endl;\
		return false;\
	}

#define NG_DEFAULT_VERBOSE_LEVEL NG_VERBOSE_FLAG_FILE | NG_VERBOSE_FLAG_CONSOLE
#ifdef _DEBUG
#define NG_DEFAULT_VERBOSE_TYPE NG_VERBOSE_DEBUG | NG_VERBOSE_INFO | NG_VERBOSE_ERROR
#else
#define NG_DEFAULT_VERBOSE_TYPE NG_VERBOSE_ERROR
#endif

//=============================================================================================================

#if !defined(__i386__) && !defined(_M_IX86)
static const char* c_szPluginArch = "64";
#else
static const char* c_szPluginArch = "86";
#endif

#if defined(_MSC_VER)
static const char* c_szPluginExtension = ".dll";
#elif defined(__APPLE__)
static const char* c_szPluginExtension = ".dylib";
#else
static const char* c_szPluginExtension = ".so";
#endif

#ifdef _MSC_VER
static HMODULE 	hPlugin = NULL;
#else
static void* 	hPlugin	= NULL;
#endif

//=============================================================================================================

bool NoMercyLoadServerPlugin(const char* c_szFileName)
{
#ifdef _MSC_VER
	if (!(hModule = LoadLibraryA(c_szFileName)))
	{
		std::cerr << "LoadLibraryA failed with " << std::to_string(GetLastError()) << std::endl;
		return false;
	}
#else
	if (!(hModule = dlopen(c_szFileName, RTLD_LAZY)))
	{
		std::cerr << "dlopen() failed with " << dlerror() << std::endl;
		return false;
	}
#endif

	DECLARE_API(NGInitializeServer);
	DECLARE_API(NGReleaseServer);
	
	DECLARE_API(NGGetLastErrorData);
	DECLARE_API(NGGetSessionID);

	DECLARE_API(NGSetVerbose);

	DECLARE_API(ACServer_CanConnect);
	DECLARE_API(ACServer_GetVersion);
	DECLARE_API(ACServer_RegisterServer);
	DECLARE_API(ACServer_UnregisterServer);
	
	DECLARE_API(Player_IsBanned);
	DECLARE_API(Player_GetHardwareId);
	DECLARE_API(Player_ForwardHeartbeatResponse);
	DECLARE_API(Player_ValidateUserBySID);
	DECLARE_API(Player_ValidateUserByIP);
	DECLARE_API(Player_GetConnectTime);

	DECLARE_API(Server_GetConnectedClientCount);
	
	return true;
}

//=============================================================================================================

void NoMercyUnloadServerPlugin(void)
{
	if (hPlugin)
	{
		NGReleaseServer();

#ifdef _MSC_VER
		FreeLibrary(hPlugin);
#else
		dlclose(hPlugin);
#endif

		hPlugin = NULL;
	}
}

//=============================================================================================================

bool NoMercyInitialize(const char* c_szLicenseId)
{
	std::cout << "Loading NoMercy Server Plugin..." << std::endl;

	char szPluginName[512];
	snprintf(szPluginName, sizeof(szPluginName), "/nomercy_game_module_x%s%s", c_szPluginArch, c_szPluginExtension);

	if (!NoMercyLoadServerPlugin(szPluginName))
	{
		std::cerr << "NoMercyLoadServerPlugin( " << szPluginName << " ) failed" << std::endl;
		return false;
	}
	std::cout << "NoMercy Server Plugin loaded to: " << std::hex << hModule << std::endl;

	// Initialize NoMercy module
	if (!NGInitializeServer(c_szLicenseId))
	{
		std::cerr << "NGInitializeServer( " << c_szLicenseId << " ) failed" << std::endl;
		return false;
	}
	
	// Set verbose mode
	NGSetVerbose(NG_DEFAULT_VERBOSE_TYPE, NG_DEFAULT_VERBOSE_LEVEL);

	// Test flight
#if 0	
	int status = ValidateUserByIp("1337");
	if (status)
	{
		std::cout << "Target state: " << std::to_string(status) << std::endl;
	}
	else
	{
		NG_ErrorData* last_error = GetLastErrorData();
		if (last_error)
			std::cerr << "Validation failed, Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;
	}
#endif

	return true;
}
