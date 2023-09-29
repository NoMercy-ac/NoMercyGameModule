#ifndef __NOMERCY_GAME_MODULE__
#define __NOMERCY_GAME_MODULE__
#include <stdint.h>

#ifdef _MSC_VER
	#define __DLLEXPORT		__declspec(dllexport)
	#define NG_CALLCONV		__cdecl
#else
	#define __DLLEXPORT		__attribute__ ((visibility("default")))
	#define NG_CALLCONV
#endif

enum NG_VERBOSETYPES
{
	NG_VERBOSE_NONE,
	NG_VERBOSE_DEBUG,
	NG_VERBOSE_INFO,
	NG_VERBOSE_WARNING,
	NG_VERBOSE_ERROR
};

enum NG_VERBOSEFLAGS
{
	NG_VERBOSE_FLAG_NONE,
	NG_VERBOSE_FLAG_FILE,
	NG_VERBOSE_FLAG_CONSOLE,
	NG_VERBOSE_FLAG_DEBUG
};

enum NG_ERRORTYPES
{
	ERROR_TYPE_UNDEFINED = 0,
	CURL_ENGINE_ERROR = 1,
	HTTP_SERVER_CONN_FAIL = 2,
	HTTP_STATUS_NOT_VALID = 3,
	RESPONSE_IS_NULL = 4,
	RESPONSE_IS_NOT_VALID = 5
};

enum NG_USERSTATUS
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

struct NG_ErrorData
{
	int32_t error_type;
	int32_t error_code;
	char	response[255];

#ifdef __cplusplus
	NG_ErrorData()
	{
		error_type = 0;
		error_code = 0;
		memset(&response, 0, sizeof(response));
	}
#endif
};

typedef bool (NG_CALLCONV* NGInitializeServer_t)(const char* license_id);
typedef void (NG_CALLCONV* NGReleaseServer_t)(void);
typedef NG_ErrorData* (NG_CALLCONV* NGGetLastErrorData_t)(void);
typedef const char* (NG_CALLCONV* NGGetSessionID_t)(void);
typedef void (NG_CALLCONV* NGSetVerbose_t)(uint8_t verbose_type, uint8_t verbose_flags);
typedef bool (NG_CALLCONV* ACServer_CanConnect_t)(void);
typedef uint8_t (NG_CALLCONV* ACServer_GetVersion_t)(void);
typedef bool (NG_CALLCONV* ACServer_RegisterServer_t)(uint32_t game_id, const char* license_code, const char* api_key);
typedef bool (NG_CALLCONV* ACServer_UnregisterServer_t)();
typedef bool (NG_CALLCONV* Player_IsBanned_t)(const char* player_hwid);
typedef const char* (NG_CALLCONV* Player_GetHardwareId_t)(const char* player_session_id);
typedef bool (NG_CALLCONV* Player_ClearHardwareId_t)(const char* player_hwid);
typedef bool (NG_CALLCONV* Player_ForwardHeartbeatResponse_t)(const char* player_session_id, const char* heartbeat_response);
typedef int (NG_CALLCONV* Player_ValidateUserBySID_t)(const char* player_session_id);
typedef int (NG_CALLCONV* Player_ValidateUserByIP_t)(const char* ip_address);
typedef unsigned int (NG_CALLCONV* Player_GetConnectTime_t)(const char* player_session_id);
typedef int (NG_CALLCONV* Server_GetConnectedClientCount_t)(void);

#endif // __NOMERCY_GAME_MODULE__
