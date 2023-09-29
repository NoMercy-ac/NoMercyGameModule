#pragma once
#include <NoMercyGameModule.h>

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
static Player_ForwardHeartbeatResponse_t	Player_ForwardHeartbeatResponse = NULL;
static Player_ValidateUserBySID_t			Player_ValidateUserBySID		= NULL;
static Player_ValidateUserByIP_t			Player_ValidateUserByIP			= NULL;
static Player_GetConnectTime_t				Player_GetConnectTime			= NULL;
static Server_GetConnectedClientCount_t		Server_GetConnectedClientCount	= NULL;

extern bool NoMercyInitialize(const char* c_szLicenseId);
extern void NoMercyUnloadServerPlugin(void);
