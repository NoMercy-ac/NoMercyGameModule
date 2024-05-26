#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#include <cstring>
#endif
#include <iostream>
#include <sstream>
#include "../include/NoMercyGameModule.h"
#include "../include/game_module_impl.hpp"

static CNoMercyGameModule* gs_pNoMercyGameModule = nullptr;

extern "C" __DLLEXPORT bool NM_CALLCONV NMInitializeServer(const char* license_id, const NMMessageCallback_t callback)
{
	if (!gs_pNoMercyGameModule)
	{
		gs_pNoMercyGameModule = new CNoMercyGameModule();
	}
	else
	{
		std::cerr << "NoMercyGameModule already initialized" << std::endl;
		return false;
	}

	return CNoMercyGameModule::Instance().Initialize(callback);
}
extern "C" __DLLEXPORT void NM_CALLCONV NMReleaseServer(void)
{
	if (gs_pNoMercyGameModule)
	{
		CNoMercyGameModule::Instance().Release();

		delete gs_pNoMercyGameModule;
		gs_pNoMercyGameModule = nullptr;
	}
	else
	{
		std::cerr << "NoMercyGameModule not initialized" << std::endl;
		return;
	}
}

extern "C" __DLLEXPORT NM_ErrorData* NM_CALLCONV NMGetLastErrorData(void)
{
	return CNoMercyGameModule::Instance().GetLastErrorData();
}
extern "C" __DLLEXPORT const char* NM_CALLCONV NMGetSessionID(void)
{
	return CNoMercyGameModule::Instance().GetSessionID();
}

extern "C" __DLLEXPORT void NM_CALLCONV NMSetVerbose(uint8_t verbose_type, uint8_t verbose_flags)
{
	return CNoMercyGameModule::Instance().SetVerbose(
		static_cast<NM_VERBOSETYPES>(verbose_type),
		static_cast<NM_VERBOSEFLAGS>(verbose_flags)
	);
}


extern "C" __DLLEXPORT bool NM_CALLCONV ACServer_CanConnect(void)
{
	return CNoMercyGameModule::Instance().ACServer_CanConnect();
}
extern "C" __DLLEXPORT uint8_t NM_CALLCONV ACServer_GetVersion(void)
{
	return CNoMercyGameModule::Instance().ACServer_GetVersion();
}
extern "C" __DLLEXPORT bool NM_CALLCONV ACServer_RegisterServer(uint32_t game_id, const char* license_code, const char* api_key)
{
	return CNoMercyGameModule::Instance().ACServer_RegisterServer(game_id, license_code, api_key);
}
extern "C" __DLLEXPORT bool NM_CALLCONV ACServer_UnregisterServer()
{
	return CNoMercyGameModule::Instance().ACServer_UnregisterServer();
}
extern "C" __DLLEXPORT bool NM_CALLCONV Player_IsBanned(const char* player_hwid)
{
	return CNoMercyGameModule::Instance().Player_IsBanned(player_hwid);
}
extern "C" __DLLEXPORT bool NM_CALLCONV Player_GetHardwareId(const char* player_session_id, char* player_hwid)
{
	std::string hwid;
	if (!CNoMercyGameModule::Instance().Player_GetHardwareId(player_session_id, hwid))
		return false;

	strncpy(player_hwid, hwid.c_str(), hwid.size() > gs_nomercy_hwid_max_length ? gs_nomercy_hwid_max_length : hwid.size());
	return true;
}
extern "C" __DLLEXPORT bool NM_CALLCONV Player_ForwardHeartbeatResponse(const char* player_session_id, const char* heartbeat_response)
{
	return CNoMercyGameModule::Instance().Player_ForwardHeartbeatResponse(player_session_id, heartbeat_response);
}
extern "C" __DLLEXPORT int NM_CALLCONV Player_ValidateUserBySID(const char* player_session_id)
{
	return CNoMercyGameModule::Instance().Player_ValidateUserBySID(player_session_id);
}
extern "C" __DLLEXPORT int NM_CALLCONV Player_ValidateUserByIP(const char* ip_address)
{
	return CNoMercyGameModule::Instance().Player_ValidateUserByIP(ip_address);
}
extern "C" __DLLEXPORT unsigned int NM_CALLCONV Player_GetConnectTime(const char* player_session_id)
{
	return CNoMercyGameModule::Instance().Player_GetConnectTime(player_session_id);
}
extern "C" __DLLEXPORT int NM_CALLCONV Server_GetConnectedClientCount(void)
{
	return CNoMercyGameModule::Instance().Server_GetConnectedClientCount();
}
