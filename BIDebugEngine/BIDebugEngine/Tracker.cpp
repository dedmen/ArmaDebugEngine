#include "Tracker.h"
#include <thread>
#include <sstream>
#include <windows.h>
#include <algorithm>
#include <wininet.h>
#include "Debugger.h"
#include "Serialize.h"
#include "version.h"

Tracker::Tracker() {}

#pragma comment (lib, "wininet.lib")

Tracker::~Tracker() {}
extern Debugger GlobalDebugger;
void Tracker::trackPiwik() {
	JsonArchive ar;
	GlobalDebugger.productInfo.Serialize(ar);
	JsonArchive read(*ar.getRaw());
	std::string version;
	std::string type;
	read.Serialize("gameType", type);
	read.Serialize("gameVersion", version);
	auto HI = GlobalDebugger.HI;
	std::thread([version, type, HI]() {
		std::vector<trackerCustomVariable> piwikData{
			{ 1,"gameVersion",version },
			{ 2,"gameType",type },
#ifdef X64
			{ 3,"arch","X64" },
#else
			{ 3,"arch","X86" },
#endif
			{ 4,"SvmCon",std::to_string(HI.__scriptVMConstructor) },
			{ 5,"SvmSimSt",std::to_string(HI.__scriptVMSimulateStart) },
			{ 6,"SvmSimEn",std::to_string(HI.__scriptVMSimulateEnd) },
			{ 7,"InstrBP",std::to_string(HI.__instructionBreakpoint) },
			{ 8,"WSim",std::to_string(HI.__worldSimulate) },
			{ 9,"WMEVS",std::to_string(HI.__worldMissionEventStart) },
			{ 10,"WMEVE",std::to_string(HI.__worldMissionEventEnd) },
			{ 11,"ScrErr",std::to_string(HI.__onScriptError) },
			{ 12,"PreDef",std::to_string(HI.scriptPreprocDefine) },
			{ 13,"PreCon",std::to_string(HI.scriptPreprocConstr) },
			{ 14,"ScrAass",std::to_string(HI.scriptAssert) },
			{ 15,"ScrHalt",std::to_string(HI.scriptHalt) },
			{ 16,"Alive",std::to_string(HI.engineAlive) },
			{ 17,"EnMouse",std::to_string(HI.enableMouse) },
		};
		std::stringstream request;
		request << "piwik.php?idsite=5&rec=1&url=\"piwik.dedmen.de\"";
		request << "&action_name=" << DBG_BUILD;
		request << "&rand=" << rand();
		request << "&uid=" << rand();
		request << "&cvar={";
		bool firstVar = true;
		for (const trackerCustomVariable& customVar : piwikData) {
			if (!firstVar) request << ',';
			firstVar = false;
			request << '"' << customVar.customVarID << "\":";//"1":
			request << '[';
			request << '"' << customVar.name << "\",";//"name",
			request << '"' << customVar.value << '"';//"value"
			request << ']';
			//"1":["name","value"]
		}
		request << "}";//closing cvar again
		std::string requestString = request.str();

		DWORD dwBytes;
		DWORD r = 0;
		char ch;
		if (!InternetGetConnectedState(&r, 0)) return;
		if (r & INTERNET_CONNECTION_OFFLINE) return;

		HINTERNET Initialize = InternetOpen(L"TFAR", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		HINTERNET Connection;
		Connection = InternetConnect(Initialize, L"piwik.dedmen.de", INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
		HINTERNET File = HttpOpenRequestA(Connection, NULL, requestString.c_str(), NULL, NULL, NULL, 0, 0);
		if (HttpSendRequest(File, NULL, 0, NULL, 0)) {
			while (InternetReadFile(File, &ch, 1, &dwBytes)) {
				if (dwBytes != 1) break;
			}
		}
		InternetCloseHandle(File);
		InternetCloseHandle(Connection);
		InternetCloseHandle(Initialize);
	}).detach();
}
