#include "stdafx.h"

#ifndef _MSC_VER
#include <unistd.h>
#include <dlfcn.h>
#include <cstring>
#endif

#include "NoMercyServer.h"

#ifndef GAME_IMPLEMENTATION
#include <iostream>
#include <sstream>
#include "../../module/include/NoMercyGameModule.h"
#else
#include <NoMercyGameModule.h>
#include "stdafx.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "desc.h"
#endif

//=============================================================================================================

inline bool __IsFileExists(const char* filename)
{
	FILE* file = fopen(filename, "r");
	if (file)
	{
		fclose(file);
		return true;
	}

	return false;
}

#ifndef GAME_IMPLEMENTATION
inline const char* convert(const std::string& str)		{ return str.c_str();	}
inline const char* convert(const char* str)				{ return str;			}
template <typename T> inline T convert(const T& val)	{ return val;			}

template <typename... Args>
void sys_log(int lv, const char* fmt, Args... args) {
	fprintf(stdout, fmt, convert(args)...);
	fprintf(stdout, "\n");
}
template <typename... Args>
void sys_err(const char* fmt, Args... args) {
	fprintf(stderr, fmt, convert(args)...);
	fprintf(stderr, "\n");
}
#endif

#ifdef _MSC_VER

using TPluginModule = HMODULE;

#define GET_API_PTR(mod, api)\
	GetProcAddress(mod, api)

#define GET_ERR_STR()\
	std::to_string(GetLastError())

#else
using TPluginModule = void*;

#define GET_API_PTR(mod, api)\
	dlsym(mod, api)

#define GET_ERR_STR()\
	dlerror()

#endif

static TPluginModule gs_hPlugin = NULL;

#define DECLARE_API(api)\
	api = reinterpret_cast<api##_t>(GET_API_PTR(gs_hPlugin, #api));\
	if (!api) {\
		sys_err("Failed to load API function: %s with error: %s", #api, GET_ERR_STR());\
		return false;\
	}

#define NM_DEFAULT_VERBOSE_LEVEL NM_VERBOSE_FLAG_FILE | NM_VERBOSE_FLAG_CONSOLE
#ifdef _DEBUG
#define NM_DEFAULT_VERBOSE_TYPE NM_VERBOSE_DEBUG | NM_VERBOSE_INFO | NM_VERBOSE_ERROR
#else
#define NM_DEFAULT_VERBOSE_TYPE NM_VERBOSE_ERROR
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

static NMInitializeServer_t					NMInitializeServer				= NULL;
static NMReleaseServer_t					NMReleaseServer					= NULL;
static NMGetLastErrorData_t					NMGetLastErrorData				= NULL;
static NMGetSessionID_t						NMGetSessionID					= NULL;
static NMSetVerbose_t						NMSetVerbose					= NULL;
static ACServer_CanConnect_t				ACServer_CanConnect				= NULL;
static ACServer_GetVersion_t				ACServer_GetVersion				= NULL;
static ACServer_RegisterServer_t			ACServer_RegisterServer			= NULL;
static ACServer_UnregisterServer_t			ACServer_UnregisterServer		= NULL;
static Player_IsBanned_t					Player_IsBanned					= NULL;
static Player_GetHardwareId_t				Player_GetHardwareId			= NULL;
static Player_ForwardHeartbeatResponse_t	Player_ForwardHeartbeatResponse = NULL;
static Player_ValidateUserBySID_t			Player_ValidateUserBySID		= NULL;
static Player_ValidateUserByIP_t			Player_ValidateUserByIP			= NULL;
static Player_GetConnectTime_t				Player_GetConnectTime			= NULL;
static Server_GetConnectedClientCount_t		Server_GetConnectedClientCount	= NULL;

//=============================================================================================================

bool CNoMercyServer::__LoadServerPlugin(const char* c_szFileName)
{
	if (!__IsFileExists(c_szFileName))
	{
		sys_err("NoMercy module file: %s does not exist!", c_szFileName);
		return false;		
	}

#ifdef _MSC_VER
	if (!(gs_hPlugin = LoadLibraryA(c_szFileName)))
#else
	if (!(gs_hPlugin = dlopen(c_szFileName, RTLD_LAZY)))
#endif
	{
		sys_err("__LoadServerPlugin failed with %s", GET_ERR_STR());
		return false;
	}

	DECLARE_API(NMInitializeServer);
	DECLARE_API(NMReleaseServer);
	
	DECLARE_API(NMGetLastErrorData);
	DECLARE_API(NMGetSessionID);

	DECLARE_API(NMSetVerbose);

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
void CNoMercyServer::__UnloadServerPlugin()
{
	if (gs_hPlugin)
	{
		NMReleaseServer();

#ifdef _MSC_VER
		FreeLibrary(gs_hPlugin);
#else
		dlclose(gs_hPlugin);
#endif

		gs_hPlugin = NULL;
	}
}

//=============================================================================================================

void OnNomercyMessage(const uint8_t message, LPVOID data)
{
	sys_log(0, "NMMessageCallback> Message: %d Data: %p", message, data);

	switch (message)
	{
	case NM_MSG_REQUIRE_RESTART:
		sys_log(0, "NMMessageCallback> Require restart message received!");
		CNoMercyServer::Instance().ReconnectToNoMercyServer();
		break;
	default:
		sys_err("NMMessageCallback> Unknown message received: %d", message);
		break;
	}
}

CNoMercyServer::CNoMercyServer() :
	m_strLicenseID(), m_nGameID(0), m_strApiKey()
{
}
CNoMercyServer::~CNoMercyServer() noexcept
{
}

bool CNoMercyServer::Initialize(const char* c_szLicenseID, const unsigned int c_nGameID, const char* c_szApiKey)
{
	sys_log(0, "Loading NoMercy Server Plugin...");

	// Create NoMercy module name
	char szPluginName[512]{ '\0' };
	snprintf(szPluginName, sizeof(szPluginName), "nomercy_game_module_x%s%s", c_szPluginArch, c_szPluginExtension);

	// Load NoMercy module
	if (!__LoadServerPlugin(szPluginName))
	{
		sys_err("NoMercyLoadServerPlugin( %s ) failed", szPluginName);
		return false;
	}
	sys_log(0, "NoMercy Server Plugin loaded to: %p", gs_hPlugin);

	// Initialize NoMercy module
	if (!NMInitializeServer(c_szLicenseID, &OnNomercyMessage))
	{
		sys_err("NMInitializeServer( %s ) failed", c_szLicenseID);
		return false;
	}
	sys_log(0, "NoMercy Server Plugin succesfully initialized");

	// Set NoMercy module variables
	m_strLicenseID = c_szLicenseID;
	m_nGameID = c_nGameID;
	m_strApiKey = c_szApiKey;

	// Set verbose mode
	NMSetVerbose(NM_DEFAULT_VERBOSE_TYPE, NM_DEFAULT_VERBOSE_LEVEL);

	// Connect to NoMercy server
	if (!ReconnectToNoMercyServer())
	{
		sys_err("__ConnectToNoMercyServer failed");
		return false;
	}

	return true;
}
void CNoMercyServer::Release()
{
	// Unregister game server
	if (!ACServer_UnregisterServer())
	{
		NM_ErrorData* last_error = NMGetLastErrorData();
		if (last_error)
			sys_err("ACServer_UnregisterServer failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);
		else
			sys_err("ACServer_UnregisterServer failed");
	}

	// Release NoMercy module
	__UnloadServerPlugin();
}

bool CNoMercyServer::ReconnectToNoMercyServer()
{
	// Register game server to NoMercy API server
	if (!ACServer_CanConnect())
	{
		NM_ErrorData* last_error = NMGetLastErrorData();
		if (last_error)
			sys_err("ACServer_CanConnect failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);
		else
			sys_err("ACServer_CanConnect failed");

		return false;
	}
	else if (ACServer_GetVersion() == 0)
	{
		NM_ErrorData* last_error = NMGetLastErrorData();
		if (last_error)
			sys_err("ACServer_GetVersion failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);
		else
			sys_err("ACServer_GetVersion failed");

		return false;
	}
	else if (!ACServer_RegisterServer(m_nGameID, m_strLicenseID.c_str(), m_strApiKey.c_str()))
	{
		NM_ErrorData* last_error = NMGetLastErrorData();
		if (last_error)
			sys_err("ACServer_RegisterServer failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);
		else
			sys_err("ACServer_RegisterServer failed");

		return false;
	}

	return true;
}

#ifndef GAME_IMPLEMENTATION
int CNoMercyServer::TestFlight()
{
	// Check banned player
	{
		if (Player_IsBanned("banned_hwid_test"))
		{
			sys_log(0, "Hwid ban query succesfully completed!");
		}
		else
		{
			NM_ErrorData* last_error = NMGetLastErrorData();
			if (last_error)
				sys_err("Player_IsBanned failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);

			return 5;
		}
	}

	// Get player hwid by session id
	{
		std::unique_ptr<char[]> upHWID(new char[gs_nomercy_hwid_max_length] { '\0' });
		if (Player_GetHardwareId("player_sessionid_test", upHWID.get()))
		{
			std::string stHWID =  std::string(upHWID.get(), gs_nomercy_hwid_max_length);
			sys_log(0, "Session ID to Hwid query succesfully completed! Result: %s", stHWID.c_str());
		}
		else
		{
			NM_ErrorData* last_error = NMGetLastErrorData();
			if (last_error)
				sys_err("Player_GetHardwareId failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);

			return 6;
		}
	}

	// Forward player heartbeat
	{
		if (Player_ForwardHeartbeatResponse("player_sessionid_test", "heartbeat_response_test"))
		{
			sys_log(0, "Forward heartbeat response succesfully completed!");
		}
		else
		{
			NM_ErrorData* last_error = NMGetLastErrorData();
			if (last_error)
				sys_err("Player_ForwardHeartbeatResponse failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);
			
			return 7;
		}
	}

	// Player query by session id
	{
		int nUserStatus = Player_ValidateUserBySID("player_sessionid_test");
		if (nUserStatus == USER_INITIALIZED)
		{
			sys_log(0, "Player validation by session id succesfully completed! Result: %d", nUserStatus);
		}
		else
		{
			NM_ErrorData* last_error = NMGetLastErrorData();
			if (last_error)
				sys_err("Player_ValidateUserBySID failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);
			
			return 8;
		}
	}

	// Player query by ip address
	{
		int nUserStatus = Player_ValidateUserByIP("player_ip_test");
		if (nUserStatus == USER_INITIALIZED)
		{
			sys_log(0, "Player validation by ip address succesfully completed! Result: %d", nUserStatus);
		}
		else
		{
			NM_ErrorData* last_error = NMGetLastErrorData();
			if (last_error)
				sys_err("Player_ValidateUserByIP failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);
		
			return 9;
		}
	}

	// Player get connected timestamp
	{
		unsigned int nConnectedTime = Player_GetConnectTime("player_sessionid_test");
		if (nConnectedTime > 0)
		{
			sys_log(0, "Player connect time query succesfully completed! Result: %d", nConnectedTime);
		}
		else
		{
			NM_ErrorData* last_error = NMGetLastErrorData();
			if (last_error)
				sys_err("Player_GetConnectTime failed, Response: %s Error: %d %d", last_error->response, last_error->error_type, last_error->error_code);
		
			return 10;
		}
	}

	// Player query by ip address
	{
		int nClientCount = Server_GetConnectedClientCount();
		if (nClientCount > 0)
		{
			sys_log(0, "Client count query succesfully completed! Client count: %d", nClientCount);
		}
		else
		{
			NM_ErrorData* last_error = NMGetLastErrorData();
			if (last_error)
				sys_err("Server_GetConnectedClientCount failed, Response: %s Return: %d Error: %d %d", last_error->response, nClientCount, last_error->error_type, last_error->error_code);
		
			return 11;
		}
	}

	sys_log(0, "All tests completed successfully!");

	// Emulate player login
	OnLoginPlayer(1);

	// Wait for 15 seconds
	std::this_thread::sleep_for(std::chrono::seconds(15));

	// Emulate player logout
	OnLogoutPlayer(1);

	sys_log(0, "Player check event completed!");
	return 0;
}
#else
EVENTINFO(nomercy_event_info)
{
	unsigned int nPlayerID;

	nomercy_event_info()
		: nPlayerID(0)
	{
	}
};

EVENTFUNC(nomercy_event)
{
	nomercy_event_info* info = dynamic_cast<nomercy_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("nomercy_event> <Factor> Null pointer");
		return 0;
	}

	unsigned int nPlayerID = info->nPlayerID;

	if (NULL == nPlayerID)
	{
		sys_err("nomercy_event: character id is null");
		return 0;
	}

	CNoMercyServer::instance().OnPlayerCheckTick(nPlayerID);

	return PASSES_PER_SEC(5);
}
#endif

void CNoMercyServer::OnLoginPlayer(const unsigned int c_nPlayerID)
{
#ifndef GAME_IMPLEMENTATION
	auto fnWorker = [this, c_nPlayerID](std::atomic<bool>& terminate) {
		while (!terminate.load())
		{
			if (!this->OnPlayerCheckTick(c_nPlayerID))
				break;

			std::this_thread::sleep_for(std::chrono::seconds(5));
		}
	};

	std::lock_guard <std::mutex> lock(m_mapCheckEventsMutex);
	SCheckEvent* checkEvent = new SCheckEvent();
	checkEvent->terminate.store(false);
	checkEvent->thread = std::thread(fnWorker, std::ref(checkEvent->terminate));
	m_mapCheckEvents.insert({ c_nPlayerID, checkEvent });
#else
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(c_nPlayerID);
	if (ch == NULL)
	{
		sys_err("Player check tick character not found: %d", c_nPlayerID);
		return;
	}

	LPDESC d = ch->GetDesc();
	if (d == NULL)
	{
		sys_err("Player check tick descriptor not found: %d", c_nPlayerID);
		return;
	}

	const std::string c_strHWID = GetHWID(d->GetNoMercySID());
	if (c_strHWID.empty())
	{
		sys_err("Player check tick hwid not found: %d", c_nPlayerID);
		return;
	}

	if (IsBannedPlayer(c_strHWID.c_str()))
	{
		sys_err("NoMercy player hwid is banned: %d", c_nPlayerID);
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	sys_log(0, "NoMercy player check event started: %d", c_nPlayerID);

	nomercy_event_info* info = AllocEventInfo<nomercy_event_info>();

	info->nPlayerID = c_nPlayerID;

	LPEVENT pkEvent = event_create(nomercy_event, info, 1);

	m_mapCheckEvents.insert({ c_nPlayerID, pkEvent });
#endif
}
void CNoMercyServer::OnLogoutPlayer(const unsigned int c_nPlayerID)
{
#ifndef GAME_IMPLEMENTATION
	std::lock_guard<std::mutex> lock(m_mapCheckEventsMutex);

	auto it = m_mapCheckEvents.find(c_nPlayerID);
	if (it != m_mapCheckEvents.end())
	{
		it->second->terminate.store(true);

		if (it->second->thread.joinable())
			it->second->thread.join();

		delete it->second;

		m_mapCheckEvents.erase(it);
	}
#else
	auto it = m_mapCheckEvents.find(c_nPlayerID);
	if (it != m_mapCheckEvents.end())
	{
		if (NULL != it->second)
		{
			event_cancel(&it->second);
			it->second = NULL;
		}

		m_mapCheckEvents.erase(it);
	}
#endif
}

bool CNoMercyServer::OnPlayerCheckTick(const unsigned int c_nPlayerID)
{
	sys_log(0, "Player check tick: %d", c_nPlayerID);

	if (m_mapCheckEvents.find(c_nPlayerID) == m_mapCheckEvents.end())
	{
		sys_err("Player check tick event not found: %d", c_nPlayerID);
		return false;
	}

	std::string stCheckKey;
#ifdef GAME_IMPLEMENTATION
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(c_nPlayerID);
	if (ch == NULL)
	{
		sys_err("Player check tick character not found: %d", c_nPlayerID);
		return false;
	}

	LPDESC d = ch->GetDesc();
	if (d == NULL)
	{
		sys_err("Player check tick descriptor not found: %d", c_nPlayerID);
		return false;
	}

	stCheckKey = d->GetNoMercySID();
	const int nUserStatus = Player_ValidateUserBySID(stCheckKey.c_str());
#else
	stCheckKey = "player_ip_test";
	const int nUserStatus = Player_ValidateUserByIP(stCheckKey.c_str());
#endif
	
	if (nUserStatus == USER_INITIALIZED)
	{
		sys_log(0, "Player validation by ip address succesfully completed!");
	}
	else
	{
		const NM_ErrorData* last_error = NMGetLastErrorData();
		sys_err(
			"Player_ValidateUserByIP (%u/%s) failed, Status=%d, Response=%s, Error=%d/%d",
			c_nPlayerID, stCheckKey.c_str(),
			nUserStatus,
			last_error ? last_error->response : "NULL",
			last_error ? last_error->error_type : 0,
			last_error ? last_error->error_code : 0
		);

#ifdef GAME_IMPLEMENTATION
		d->SetPhase(PHASE_CLOSE);
#endif
		return false;
	}

	return true;
}

bool CNoMercyServer::IsBannedPlayer(const char* c_szHWID)
{
	return Player_IsBanned(c_szHWID);
}
std::string CNoMercyServer::GetHWID(const char* c_szSessionID)
{
	std::unique_ptr<char[]> upHWID(new char[gs_nomercy_hwid_max_length] { '\0' });
	if (Player_GetHardwareId(c_szSessionID, upHWID.get()))
		return std::string(upHWID.get(), gs_nomercy_hwid_max_length);
	
	return std::string();
}

bool CNoMercyServer::ForwardHeartbeatResponse(const char* c_szSessionID, const char* c_szResponse)
{
	return Player_ForwardHeartbeatResponse(c_szSessionID, c_szResponse);
}
bool CNoMercyServer::ValidateBySID(const char* c_szSessionID)
{
	return Player_ValidateUserBySID(c_szSessionID) == USER_INITIALIZED;
}
bool CNoMercyServer::ValidateByIP(const char* c_szIPAddress)
{
	return Player_ValidateUserByIP(c_szIPAddress) == USER_INITIALIZED;
}

unsigned int CNoMercyServer::GetConnectTime(const char* c_szSessionID)
{
	return Player_GetConnectTime(c_szSessionID);
}
int CNoMercyServer::GetConnectedClientCount()
{
	return Server_GetConnectedClientCount();
}
