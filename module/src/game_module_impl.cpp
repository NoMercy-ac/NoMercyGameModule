#include "../include/game_module_impl.hpp"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fmt/format.h>

#define LOG_FILE_NAME "NoMercy.log"
#define REST_CERT_FILENAME "NoMercy.crt"
#define REST_KEY_FILENAME "NoMercy.key"
#define NOMERCY_API_URL "https://api-beta.nomercy.ac"
#define USER_VALIDATOR_VERSION "v1"

CNoMercyGameModule::CNoMercyGameModule() :
	m_nVerboseType(NM_VERBOSETYPES::NM_VERBOSE_ERROR), m_nVerboseFlags(NM_VERBOSEFLAGS::NM_VERBOSE_FLAG_FILE),
	m_pkClient(nullptr), m_pkLastError(nullptr), m_bInitialized(false), m_pMessageCallback(nullptr)
{
}
CNoMercyGameModule::~CNoMercyGameModule()
{
}

bool CNoMercyGameModule::Initialize(const NMMessageCallback_t callback)
{
	std::lock_guard <std::recursive_mutex> __lock(m_rmMutex);

	if (m_bInitialized)
		return false;

	m_pMessageCallback = callback;

	m_pkLastError = new NM_ErrorData();
	if (!m_pkLastError)
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "Error data memory allocation failed with error: %s", strerror(errno));
		return false;
	}

	if (!std::filesystem::exists(REST_CERT_FILENAME))
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "REST certificate file not found");
		return false;
	}
	else if (!std::filesystem::exists(REST_KEY_FILENAME))
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "REST key file not found");
		return false;
	}

#ifdef REST_API_ENGINE_HTTPLIB
	m_pkClient = new httplib::Client(NOMERCY_API_URL, REST_CERT_FILENAME, REST_KEY_FILENAME);
#else
	m_pkClient = new cpr::Session();
#endif
	if (!m_pkClient)
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "HTTP client memory allocation failed! with error: %s", strerror(errno));
		return false;
	}

#ifndef REST_API_ENGINE_HTTPLIB
	m_pkClient->SetSslOptions(
		cpr::Ssl(
			cpr::ssl::CertFile{ REST_CERT_FILENAME },
			cpr::ssl::KeyFile{ REST_KEY_FILENAME },
			cpr::ssl::TLSv1_3{},
			cpr::ssl::VerifyHost{ false },
			cpr::ssl::VerifyPeer{ false }
		)
	);
#endif

	/*
#ifdef _DEBUG
	m_pkClient->enable_server_certificate_verification(false);
#endif
	*/

	m_bInitialized = true;
	this->__Log(NM_VERBOSETYPES::NM_VERBOSE_INFO, "NoMercy game module initialized.");
	return true;
}
void CNoMercyGameModule::Release()
{
	std::lock_guard <std::recursive_mutex> __lock(m_rmMutex);

	if (m_pkLastError)
	{
		delete m_pkLastError;
		m_pkLastError = nullptr;
	}
	if (m_pkClient)
	{
		delete m_pkClient;
		m_pkClient = nullptr;
	}

	m_bInitialized = false;
}

NM_ErrorData* CNoMercyGameModule::GetLastErrorData()
{
	std::lock_guard <std::recursive_mutex> __lock(m_rmMutex);
	
	return m_pkLastError;
}
const char* CNoMercyGameModule::GetSessionID()
{
	std::lock_guard <std::recursive_mutex> __lock(m_rmMutex);

	return m_stSessionID.c_str();
}

void CNoMercyGameModule::SetVerbose(NM_VERBOSETYPES verbose_type, NM_VERBOSEFLAGS verbose_flags)
{
	std::lock_guard <std::recursive_mutex> __lock(m_rmMutex);
	
	m_nVerboseType = verbose_type;
	m_nVerboseFlags = verbose_flags;
}

bool CNoMercyGameModule::ACServer_CanConnect()
{
	std::string stResponse;
	if (!this->__GetRequest("/operational", stResponse, false, false))
		return false;
	return stResponse == "1";
}

int CNoMercyGameModule::ACServer_GetVersion()
{
	std::string stResponse;
	if (!this->__GetRequest("/version", stResponse, false, false))
		return false;
	return this->__StringToNumber(stResponse);
}

bool CNoMercyGameModule::ACServer_RegisterServer(uint32_t game_id, const std::string& license_code, const std::string& api_key)
{
	std::string stResponse;
	if (!this->__GetRequest(fmt::format("/register_game_server?game_id={0}&license_code={1}&key={2}", game_id, license_code, api_key), stResponse, true))
		return false;
	
	const auto bRet = stResponse != "0";
	if (bRet)
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_INFO, "Server registered successfully. Session ID: %s", stResponse.c_str());
		m_stSessionID = stResponse;
	}

	return bRet;
}

bool CNoMercyGameModule::ACServer_UnregisterServer()
{
	std::string stResponse;
	if (!this->__GetRequest("/unregister_game_server", stResponse, false))
		return false;
	return stResponse == "1";
}

bool CNoMercyGameModule::Player_IsBanned(const std::string& player_hwid)
{
	std::string stResponse;
	if (!this->__GetRequest(fmt::format("/player_is_banned?player_hwid={0}", player_hwid), stResponse))
		return false;
	return stResponse == "1";
}

bool CNoMercyGameModule::Player_GetHardwareId(const std::string& player_session_id, std::string& player_hwid)
{
	if (!this->__GetRequest(fmt::format("/player_get_hwid?player_session_id={0}", player_session_id), player_hwid, true))
		return false;
	return player_hwid != "0";
}

bool CNoMercyGameModule::Player_ForwardHeartbeatResponse(const std::string& player_session_id, const std::string& heartbeat_response)
{
	std::string stResponse;
	if (!this->__GetRequest(fmt::format("/player_hb_response?player_session_id={0}&heartbeat_response={1}", player_session_id, heartbeat_response), stResponse))
		return false;
	return stResponse == "1";
}

int CNoMercyGameModule::Player_ValidateUserBySID(const std::string& player_session_id)
{
	std::string stResponse;
	if (!this->__GetRequest(fmt::format("/player_can_connect?player_session_id={0}", player_session_id), stResponse))
		return -1;
	else if (!this->__StringIsNumber(stResponse))
		return -2;
	return std::atoi(stResponse.c_str());
}

int CNoMercyGameModule::Player_ValidateUserByIP(const std::string& player_ip_address)
{
	std::string stResponse;
	if (!this->__GetRequest(fmt::format("/player_can_connect?player_ip_address={0}", player_ip_address), stResponse))
		return -1;
	else if (!this->__StringIsNumber(stResponse))
		return -2;
	return std::atoi(stResponse.c_str());
}

unsigned int CNoMercyGameModule::Player_GetConnectTime(const std::string& player_session_id)
{
	std::string stResponse;
	if (!this->__GetRequest(fmt::format("/player_get_connect_time?player_session_id={0}", player_session_id), stResponse))
		return -1;
	else if (!this->__StringIsNumber(stResponse))
		return -2;
	return this->__StringToNumber(stResponse.c_str());
}

int CNoMercyGameModule::Server_GetConnectedClientCount()
{
	std::string stResponse;
	if (!this->__GetRequest("/connected_client_count", stResponse))
		return -1;
	else if (!this->__StringIsNumber(stResponse))
		return -2;
	return std::atoi(stResponse.c_str());
}


template <typename... FormatArgs>
void CNoMercyGameModule::__Log(NM_VERBOSETYPES type, std::string_view string_template, FormatArgs... format_args)
{
	auto __FormatString = [&]() {
		const size_t string_size = std::snprintf(nullptr, 0, string_template.data(), std::forward<FormatArgs>(format_args)...);
		if (string_size <= 0)
		{
			throw std::runtime_error("Output string size is malformed");
		}

		auto formatted_string = new char[string_size + 1];
		std::snprintf(formatted_string, string_size + 1, string_template.data(), std::forward<FormatArgs>(format_args)...);

		const auto stOutput = std::string(formatted_string);

		delete[] formatted_string;
		return stOutput;
	};
	auto __GetLogLevel = [&]() -> std::string {
		switch (type)
		{
			case NM_VERBOSETYPES::NM_VERBOSE_DEBUG:
				return "[DEBUG]";
			case NM_VERBOSETYPES::NM_VERBOSE_INFO:
				return "[INFO]";
			case NM_VERBOSETYPES::NM_VERBOSE_WARNINM:
				return "[WARNINM]";
			case NM_VERBOSETYPES::NM_VERBOSE_ERROR:
				return "[ERROR]";
			default:
				return fmt::format("[UNKNOWN:{0}]", type);
;		}
	};

	std::lock_guard <std::recursive_mutex> __lock(m_rmMutex);

	if (m_bInitialized && !(m_nVerboseType & type))
		return;

	const auto stLogOutput = fmt::format("{0} {1}", __GetLogLevel(), __FormatString());

	if (m_nVerboseFlags & NM_VERBOSEFLAGS::NM_VERBOSE_FLAG_FILE)
	{
		std::ofstream __log_file(LOG_FILE_NAME, std::ios_base::app);
		if (__log_file.is_open())
		{
			__log_file << stLogOutput << std::endl;
			__log_file.close();
		}
	}
	if (m_nVerboseFlags & NM_VERBOSEFLAGS::NM_VERBOSE_FLAG_CONSOLE)
	{
		std::cout << stLogOutput << std::endl;
	}
#ifdef _WIN32
	if (m_nVerboseFlags & NM_VERBOSEFLAGS::NM_VERBOSE_FLAG_DEBUG)
	{
		OutputDebugStringA(stLogOutput.c_str());
	}
#endif
}

bool CNoMercyGameModule::__GetRequest(const std::string& body, std::string& response, bool bSkipResponseCheck, bool bCommonPath)
{
	std::lock_guard <std::recursive_mutex> __lock(m_rmMutex);

	if (!m_bInitialized)
		return false;

	if (!m_pkClient)
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "NoMercy API client is not initialized!");
		return false;
	}

	const auto c_stTarget = bCommonPath ?
		fmt::format("{0}/{1}{2}", NOMERCY_API_URL, USER_VALIDATOR_VERSION, body) :
		fmt::format("{0}{1}", NOMERCY_API_URL, body);
	
	this->__Log(NM_VERBOSETYPES::NM_VERBOSE_DEBUG, "Sending request to %s", c_stTarget.c_str());

#ifdef REST_API_ENGINE_HTTPLIB
	// Allow redirect
	m_pkClient->set_follow_location(true);

	m_pkClient->set_connection_timeout(5, 0); // 5 seconds
	m_pkClient->set_read_timeout(5, 0); // 5 seconds
	m_pkClient->set_write_timeout(5, 0); // 5 seconds

	auto headers = httplib::Headers{
		{ "Content-Type", "text/html"},
		{ "User-Agent", "NoMercy_GameServer_Client" },
		{ "nm-session", m_stSessionID },
		{ "App-Version", USER_VALIDATOR_VERSION }
	};
	const auto res = m_pkClient->Get(c_stTarget.c_str(), headers);
	if (!res) // httplib error
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with error: %d", c_stTarget.c_str(), res.error());

		m_pkLastError->error_code = static_cast<int32_t>(res.error());
		m_pkLastError->error_type = HTTP_SERVER_CONN_FAIL;
		return false;
	}

	this->__Log(NM_VERBOSETYPES::NM_VERBOSE_DEBUG, "Request completed with status: %d, Response: %s", res->status, res->body.c_str());

	if (res->status != 200) // HTTP status code
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with status: %d", c_stTarget.c_str(), res->status);
		
		m_pkLastError->error_code = res->status;
		m_pkLastError->error_type = HTTP_STATUS_NOT_VALID;
		return false;
	}

	if (res->body.empty()) // Response
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with empty response", c_stTarget.c_str());
		
		m_pkLastError->error_type = RESPONSE_IS_NULL;
		m_pkLastError->error_code = 0;
		return false;
	}

	// Some special cases
	if (res->body == "REQUIRE_RESTART")
	{
		if (m_pMessageCallback)
		{
			m_pMessageCallback(NM_MESSAGE_IDS::NM_MSG_REQUIRE_RESTART, nullptr);
			return true;
		}

		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with special response: %s", c_stTarget.c_str(), res->body.c_str());

		m_pkLastError->error_type = REQUIRE_RESTART;
		m_pkLastError->error_code = 0;
		return true;
	}

	if (!bSkipResponseCheck && !this->__StringIsNumber(res->body)) // Response format
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with invalid response: %s", c_stTarget.c_str(), res->body.c_str());
		
		m_pkLastError->error_type = RESPONSE_IS_NOT_VALID;
		m_pkLastError->error_code = 0;
		strncpy(m_pkLastError->response, res->body.c_str(), sizeof(m_pkLastError->response));
		return false;
	}

	response = res->body;
#else
	cpr::Response res;
	try
	{
		m_pkClient->SetUrl(cpr::Url{ c_stTarget });
		m_pkClient->SetHeader(cpr::Header{
			{ "Content-Type", "text/html" },
			{ "User-Agent", "NoMercy_GameServer_Client" },
			{ "nm-session", m_stSessionID },
			{ "App-Version", USER_VALIDATOR_VERSION }
		});
		res = m_pkClient->Get();
	}
	catch (const cpr::Error& e)
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with error: %s", c_stTarget.c_str(), e.message.c_str());
		
		m_pkLastError->error_code = static_cast<int32_t>(e.code);
		m_pkLastError->error_type = HTTP_SERVER_CONN_FAIL;
		return false;
	}

	this->__Log(NM_VERBOSETYPES::NM_VERBOSE_DEBUG, "Request completed with status: %d, Response: %s", res.status_code, res.text.c_str());

	if (res.status_code != 200) // HTTP status code
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with status: %d", c_stTarget.c_str(), res.status_code);
		
		m_pkLastError->error_code = res.status_code;
		m_pkLastError->error_type = HTTP_STATUS_NOT_VALID;
		return false;
	}

	if (res.text.empty()) // Response
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with empty response", c_stTarget.c_str());
		
		m_pkLastError->error_type = RESPONSE_IS_NULL;
		m_pkLastError->error_code = 0;
		return false;
	}

	// Some special cases
	if (res.text == "REQUIRE_RESTART")
	{
		if (m_pMessageCallback)
		{
			m_pMessageCallback(NM_MESSAGE_IDS::NM_MSG_REQUIRE_RESTART, nullptr);
			return true;
		}

		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with special response: %s", c_stTarget.c_str(), res.text.c_str());

		m_pkLastError->error_type = REQUIRE_RESTART;
		m_pkLastError->error_code = 0;
		return true;
	}

	if (!bSkipResponseCheck && !this->__StringIsNumber(res.text)) // Response format
	{
		this->__Log(NM_VERBOSETYPES::NM_VERBOSE_ERROR, "GET request to %s failed with invalid response: %s", c_stTarget.c_str(), res.text.c_str());
		
		m_pkLastError->error_type = RESPONSE_IS_NOT_VALID;
		m_pkLastError->error_code = 0;
		strncpy(m_pkLastError->response, res.text.c_str(), sizeof(m_pkLastError->response));
		return false;
	}

	response = res.text;
#endif

	return true;
}

bool CNoMercyGameModule::__StringIsNumber(const std::string& s) const
{
	return std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

uint32_t CNoMercyGameModule::__StringToNumber(const std::string& s) const
{
	return static_cast<uint32_t>(std::strtoul(s.c_str(), nullptr, 10));
}
