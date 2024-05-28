#ifndef __NOMERCY_GAME_MODULE__
#define __NOMERCY_GAME_MODULE__
#include <stdint.h>
#include <cstring>

#ifdef _MSC_VER
	#define __DLLEXPORT		__declspec(dllexport)
	#define NM_CALLCONV		__cdecl
#else
	#define __DLLEXPORT		__attribute__ ((visibility("default")))
	#define NM_CALLCONV
#endif

enum NM_VERBOSETYPES
{
	NM_VERBOSE_NONE,
	NM_VERBOSE_DEBUG,
	NM_VERBOSE_INFO,
	NM_VERBOSE_WARNINM,
	NM_VERBOSE_ERROR
};

enum NM_VERBOSEFLAGS
{
	NM_VERBOSE_FLAG_NONE,
	NM_VERBOSE_FLAG_FILE,
	NM_VERBOSE_FLAG_CONSOLE,
	NM_VERBOSE_FLAG_DEBUG
};

enum NM_ERRORTYPES
{
	ERROR_TYPE_UNDEFINED = 0,
	CURL_ENGINE_ERROR = 1,
	HTTP_SERVER_CONN_FAIL = 2,
	HTTP_STATUS_NOT_VALID = 3,
	RESPONSE_IS_NULL = 4,
	RESPONSE_IS_NOT_VALID = 5,
	REQUIRE_RESTART = 6,
	UNALLOWED_LICENSE_TYPE = 7,
};

enum NM_USERSTATUS
{
	USER_UNKNOWNSTATUS = 0,
	USER_RESPONSE_HANDLE_FAIL = -1,
	USER_RESPONSE_PARSE_FAIL = -2,
	USER_CORRUPTED_REQUEST = -3,
	USER_NOTCONNECTED = 1,
	USER_CONNECTING = 2,
	USER_INITIALIZING = 3,
	USER_INITIALIZED = 4,
	USER_AUTH_FAILED = 5,
	USER_BANNED = 6,
	USER_VIOLATION = 7,
	USER_SUSPECTED_EVENTS = 8
};

enum NM_MESSAGE_IDS
{
	NM_MSG_NONE,
	NM_MSG_REQUIRE_RESTART,
	NM_MSG_UNALLOWED_LICENSE_TYPE
};

struct NM_ErrorData
{
	int32_t error_type;
	int32_t error_code;
	char	response[255];

#ifdef __cplusplus
	NM_ErrorData()
	{
		error_type = 0;
		error_code = 0;
		memset(&response, 0, sizeof(response));
	}
#endif
};

static uint8_t gs_nomercy_hwid_max_length = 255;

typedef void (NM_CALLCONV* NMMessageCallback_t)(const uint8_t message, void* data);
typedef bool (NM_CALLCONV* NMInitializeServer_t)(const char* license_id, const NMMessageCallback_t callback);
typedef void (NM_CALLCONV* NMReleaseServer_t)(void);
typedef NM_ErrorData* (NM_CALLCONV* NMGetLastErrorData_t)(void);
typedef const char* (NM_CALLCONV* NMGetSessionID_t)(void);
typedef void (NM_CALLCONV* NMSetVerbose_t)(uint8_t verbose_type, uint8_t verbose_flags);
typedef bool (NM_CALLCONV* ACServer_CanConnect_t)(void);
typedef uint8_t(NM_CALLCONV* ACServer_GetVersion_t)(void);
typedef bool (NM_CALLCONV* ACServer_RegisterServer_t)(uint32_t game_id, const char* license_code, const char* api_key);
typedef bool (NM_CALLCONV* ACServer_UnregisterServer_t)();
typedef bool (NM_CALLCONV* Player_IsBanned_t)(const char* player_hwid);
typedef bool (NM_CALLCONV* Player_GetHardwareId_t)(const char* player_session_id, char* player_hwid);
typedef bool (NM_CALLCONV* Player_ForwardHeartbeatResponse_t)(const char* player_session_id, const char* heartbeat_response);
typedef int (NM_CALLCONV* Player_ValidateUserBySID_t)(const char* player_session_id);
typedef int (NM_CALLCONV* Player_ValidateUserByIP_t)(const char* ip_address);
typedef unsigned int (NM_CALLCONV* Player_GetConnectTime_t)(const char* player_session_id);
typedef int (NM_CALLCONV* Server_GetConnectedClientCount_t)(void);

#endif // __NOMERCY_GAME_MODULE__
