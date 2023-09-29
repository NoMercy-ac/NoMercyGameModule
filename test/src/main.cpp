#ifdef _MSC_VER
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#include <cstring>
#endif
#include <iostream>
#include <sstream>
#include "../../module/include/NoMercyGameModule.h"

//=============================================================================================================

static const char* c_szLicenseId = "ac_lic_cod";
static const uint32_t c_nGameID = 31;
static const char* c_szApiKey = "1234";

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
static HMODULE 	hModule = NULL;
#else
static void* 	hModule	= NULL;
#endif

static NGInitializeServer_t					NGInitializeServer				= NULL;
static NGReleaseServer_t					NGReleaseServer					= NULL;
static NGGetLastErrorData_t					NGGetLastErrorData				= NULL;
static NGGetSessionID_t						NGGetSessionID					= NULL;
static NGSetVerbose_t						NGSetVerbose					= NULL;
static ACServer_CanConnect_t				ACServer_CanConnect				= NULL;
static ACServer_GetVersion_t				ACServer_GetVersion				= NULL;
static ACServer_RegisterServer_t			ACServer_RegisterServer			= NULL;
static ACServer_UnregisterServer_t			ACServer_UnregisterServer		= NULL;
static Player_IsBanned_t					Player_IsBanned					= NULL;
static Player_GetHardwareId_t				Player_GetHardwareId			= NULL;
static Player_ClearHardwareId_t				Player_ClearHardwareId			= NULL;
static Player_ForwardHeartbeatResponse_t	Player_ForwardHeartbeatResponse = NULL;
static Player_ValidateUserBySID_t			Player_ValidateUserBySID		= NULL;
static Player_ValidateUserByIP_t			Player_ValidateUserByIP			= NULL;
static Player_GetConnectTime_t				Player_GetConnectTime			= NULL;
static Server_GetConnectedClientCount_t		Server_GetConnectedClientCount	= NULL;

//=============================================================================================================

bool LoadServerPlugin(const char* c_szFileName)
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
	DECLARE_API(Player_ClearHardwareId);
	DECLARE_API(Player_ForwardHeartbeatResponse);
	DECLARE_API(Player_ValidateUserBySID);
	DECLARE_API(Player_ValidateUserByIP);
	DECLARE_API(Player_GetConnectTime);

	DECLARE_API(Server_GetConnectedClientCount);
	
	return true;
}

//=============================================================================================================

void UnloadServerPlugin(void)
{
	if (hModule)
	{
		NGReleaseServer();

#ifdef _MSC_VER
		FreeLibrary(hModule);
#else
		dlclose(hModule);
#endif

		hModule = NULL;
	}
}

//=============================================================================================================

int RunRestRoutines()
{
	// Check API server status
	{
		if (ACServer_CanConnect())
		{
			std::cout << "NoMercy API server is operational!" << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "ACServer_CanConnect failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 3;
		}
	}

	// Check API server version
	{
		uint8_t version = ACServer_GetVersion();
		if (version)
		{
			std::cout << "NoMercy API server version: " << std::to_string(version) << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "ACServer_GetVersion failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 4;
		}
	}

	// Register game server to NoMercy API server
	{
		if (ACServer_RegisterServer(c_nGameID, c_szLicenseId, c_szApiKey))
		{
			std::cout << "Game server succesfully registered to NoMercy API server! Session ID: " << NGGetSessionID() << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "ACServer_RegisterServer failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 4;
		}
	}

	// Check banned player
	{
		if (Player_IsBanned("banned_hwid_test"))
		{
			std::cout << "Hwid ban query succesfully completed!" << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "Player_IsBanned failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 5;
		}
	}

	// Get player hwid by session id
	{
		const char* c_szHWID = Player_GetHardwareId("player_sessionid_test");
		if (c_szHWID && *c_szHWID)
		{
			std::cout << "Session ID to Hwid query succesfully completed! Result: " << c_szHWID << std::endl;
			Player_ClearHardwareId(c_szHWID);
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "Player_GetHardwareId failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 6;
		}
	}

	// Forward player heartbeat
	{
		if (Player_ForwardHeartbeatResponse("player_sessionid_test", "heartbeat_response_test"))
		{
			std::cout << "Forward heartbeat response succesfully completed!" << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "Player_ForwardHeartbeatResponse failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 7;
		}
	}

	// Player query by session id
	{
		int nUserStatus = Player_ValidateUserBySID("player_sessionid_test");
		if (nUserStatus == USER_INITIALIZED)
		{
			std::cout << "Player validation by session id succesfully completed! Result: " << std::to_string(nUserStatus) << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "Player_ValidateUserBySID failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 8;
		}
	}

	// Player query by ip address
	{
		int nUserStatus = Player_ValidateUserByIP("player_ip_test");
		if (nUserStatus == USER_INITIALIZED)
		{
			std::cout << "Player validation by ip address succesfully completed! Result: " << std::to_string(nUserStatus) << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "Player_ValidateUserByIP failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 9;
		}
	}

	// Player get connected timestamp
	{
		unsigned int nConnectedTime = Player_GetConnectTime("player_sessionid_test");
		if (nConnectedTime > 0)
		{
			std::cout << "Player connect time query succesfully completed! Result: " << std::to_string(nConnectedTime) << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "Player_GetConnectTime failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 10;
		}
	}

	// Player query by ip address
	{
		int nClientCount = Server_GetConnectedClientCount();
		if (nClientCount > 0)
		{
			std::cout << "Client count query succesfully completed! Client count: " << std::to_string(nClientCount) << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "Server_GetConnectedClientCount failed, Response: " << last_error->response << " Return: " << std::to_string(nClientCount) << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 11;
		}
	}

	// Unregister game server
	{
		if (ACServer_UnregisterServer())
		{
			std::cout << "Server unregister succesfully completed!" << std::endl;
		}
		else
		{
			NG_ErrorData* last_error = NGGetLastErrorData();
			if (last_error)
				std::cerr << "Server_Unregister failed, Response: " << last_error->response << " Error: " << std::to_string(last_error->error_type) << " " << std::to_string(last_error->error_code) << std::endl;

			return 12;
		}
	}

	std::cout << "All tests completed successfully!" << std::endl;
	return 0;
}

int main(void)
{
	std::cout << "Loading NoMercy Server Plugin..." << std::endl;

	// Create module filename
	char szPluginName[512];
	snprintf(szPluginName, sizeof(szPluginName), "./nomercy_game_module_x%s%s", c_szPluginArch, c_szPluginExtension);

	// Load NoMercy game module plugin
	if (!LoadServerPlugin(szPluginName))
	{
		std::cerr << "LoadServerPlugin( " << szPluginName << " ) failed" << std::endl;
		return 1;
	}
	std::cout << "NoMercy Server Plugin loaded to: " << std::hex << hModule << std::endl;
	
	// Initialize NoMercy module
	if (!NGInitializeServer(c_szLicenseId))
	{
		std::cerr << "InitializeServer( " << c_szLicenseId << " ) failed" << std::endl;
		return 2;
	}
	std::cout << "NoMercy Server Plugin succesfully initialized" << std::endl;
	
	// Set verbose mode
	NGSetVerbose(NG_DEFAULT_VERBOSE_TYPE, NG_DEFAULT_VERBOSE_LEVEL);

	// Run sample rest APIs
	std::cout << "Press any key to run rest APIs..." << std::endl;
	std::cin.get();

	int ret = RunRestRoutines();
	
	std::cout << "Press any key to exit..." << std::endl;
	std::cin.get();

	NGReleaseServer();
	return ret;
}
