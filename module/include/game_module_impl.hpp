#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <httplib.h>

#include "singleton.hpp"
#include "NoMercyGameModule.h"
#include <string>
#include <mutex>

class CNoMercyGameModule : public CSingleton <CNoMercyGameModule>
{
	public:
		virtual ~CNoMercyGameModule();

		CNoMercyGameModule(const CNoMercyGameModule&) = delete;
		CNoMercyGameModule(CNoMercyGameModule&&) noexcept = delete;
		CNoMercyGameModule& operator=(const CNoMercyGameModule&) = delete;
		CNoMercyGameModule& operator=(CNoMercyGameModule&&) noexcept = delete;

	public:
		CNoMercyGameModule();
		
		bool Initialize();
		void Release();
		
		NG_ErrorData* GetLastErrorData();
		const char* GetSessionID();

		void SetVerbose(NG_VERBOSETYPES verbose_type, NG_VERBOSEFLAGS verbose_flags);
		
		bool ACServer_CanConnect();
		int ACServer_GetVersion();
		bool ACServer_RegisterServer(uint32_t game_id, const std::string& license_code, const std::string& api_key);
		bool ACServer_UnregisterServer();
		int Player_ValidateUserBySID(const std::string& player_session_id);
		int Player_ValidateUserByIP(const std::string& player_ip_address);
		bool Player_IsBanned(const std::string& player_hwid);
		bool Player_GetHardwareId(const std::string& player_session_id, std::string& player_hwid);
		bool Player_ForwardHeartbeatResponse(const std::string& player_session_id, const std::string& heartbeat_response);
		unsigned int Player_GetConnectTime(const std::string& player_session_id);
		int Server_GetConnectedClientCount();

	protected:
		template <typename... FormatArgs>
		void __Log(NG_VERBOSETYPES type, std::string_view string_template, FormatArgs... format_args);
		
		bool __GetRequest(const std::string& body, std::string& response, bool bSkipResponseCheck = false, bool bCommonPath = true);
		bool __StringIsNumber(const std::string& s) const;
		uint32_t __StringToNumber(const std::string& s) const;

	private:
		mutable std::recursive_mutex m_rmMutex;

		NG_VERBOSETYPES m_nVerboseType;
		NG_VERBOSEFLAGS m_nVerboseFlags;

		httplib::Client*	m_pkClient;
		NG_ErrorData*		m_pkLastError;
		std::string			m_stSessionID;

		bool m_bInitialized;
};
