#pragma once
// #define GAME_IMPLEMENTATION

#ifndef GAME_IMPLEMENTATION
#include "../../module/include/singleton.hpp"
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#endif
#include <string>

#ifdef GAME_IMPLEMENTATION
class CNoMercyServer : public singleton <CNoMercyServer>
#else
class CNoMercyServer : public CSingleton <CNoMercyServer>
#endif
{
	// Data type definitions
#ifndef GAME_IMPLEMENTATION
	struct SCheckEvent {
		std::thread thread;
		std::atomic<bool> terminate;
	};
#else
	using TCheckEventHandle = LPEVENT;
#endif

	// Lifecycle
	public:	
		virtual ~CNoMercyServer() noexcept;

		CNoMercyServer(const CNoMercyServer&) = delete;
		CNoMercyServer(CNoMercyServer&&) noexcept = delete;
		CNoMercyServer& operator=(const CNoMercyServer&) = delete;
		CNoMercyServer& operator=(CNoMercyServer&&) noexcept = delete;
		
	// Public methods
	public:
		// Constructor
		CNoMercyServer();

		// Common methods
		bool Initialize(const char* c_szLicenseID, const unsigned int c_nGameID, const char* c_szApiKey);
		void Release();
		bool ReconnectToNoMercyServer();

#ifndef GAME_IMPLEMENTATION
		int TestFlight();
#endif

		// Player event
		void OnLoginPlayer(const unsigned int c_nPlayerID);
		void OnLogoutPlayer(const unsigned int c_nPlayerID);
		bool OnPlayerCheckTick(const unsigned int c_nPlayerID);

		// Validation methods
		bool ForwardHeartbeatResponse(const char* c_szSessionID, const char* c_szResponse);
		bool ValidateBySID(const char* c_szSessionID);
		bool ValidateByIP(const char* c_szIPAddress);

		// Query methods
		bool IsBannedPlayer(const char* c_szHWID);
		std::string GetHWID(const char* c_szSessionID);
		unsigned int GetConnectTime(const char* c_szSessionID);
		int GetConnectedClientCount();

	// Class methods
	protected:
		bool __LoadServerPlugin(const char* c_szFileName);
		void __UnloadServerPlugin();

	// Class members
	private:
		std::string m_strLicenseID;
		unsigned int m_nGameID;
		std::string m_strApiKey;

#ifndef GAME_IMPLEMENTATION
		std::unordered_map <unsigned int /* nPlayerID */, SCheckEvent*> m_mapCheckEvents;
		std::mutex m_mapCheckEventsMutex;
#else
		std::unordered_map <unsigned int /* nPlayerID */, TCheckEventHandle> m_mapCheckEvents;
#endif
};
