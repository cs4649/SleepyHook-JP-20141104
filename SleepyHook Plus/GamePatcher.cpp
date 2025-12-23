#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#include <Windows.h>
#include "Main.h"
#include "MetaHook.h"
#include "Utils.h"
#include "GamePatcher.h"
#define INRANGE(x,a,b)   (x >= a && x <= b)
#define GET_BYTE( x )    (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))
#define GET_BITS( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))

DWORD WINAPI GameUI_Patcher();

ChattingManager* g_pChattingManager;
cl_enginefunc_t* g_pEngine;
int(__thiscall* Ori_OnKeyCodeTyped)(DWORD, DWORD);

typedef int(__thiscall* tLoginDlg_OnCommand)(void* _this, const char* command);
tLoginDlg_OnCommand g_pfnLoginDlg_OnCommand;

DWORD g_pfnBot_Add;
bool MP3VolChanged = false;

unsigned long m_iHostIPAddress;
unsigned short m_iHostPort;

int __fastcall hk_OnKeyCodeTyped(int a1, int a2, int a3)
{
	int v4;
	v4 = a1;
	if (a3 == 95)
		(*(void(__thiscall**)(int, int))(*(DWORD*)a1 + 0x258))(a1, a2);
	return Ori_OnKeyCodeTyped(v4, a3);
}

void __fastcall LoginDlg_OnCommand(void* _this, int r, const char* command)
{
	if (!strcmp(command, "Login"))
	{
		DWORD** v3 = (DWORD**)_this;
		char login[256];
		char password[256];

		//void* pLoginTextEntry = g_pfnPanel_FindChildByName(_this, "1");
		//void* pPasswordTextEntry = g_pfnPanel_FindChildByName(_this, "1");
		(*(void(__thiscall**)(DWORD*, char*, signed int))(*v3[89] + 596))(v3[89], login, 256); // textentry->GetText() // before 23.12.23 *v3[109] + 620
		(*(void(__thiscall**)(DWORD*, char*, signed int))(*v3[90] + 596))(v3[90], password, 256);

		wchar_t buf[256];
		swprintf(buf, L"/login %S %S", login, password);
		if (g_pChattingManager)
			g_pChattingManager->PrintToChat(1, buf);
		return;
	}
	else if (!strcmp(command, "Register"))
	{
		DWORD** v3 = (DWORD**)_this;
		char login[256];
		char password[256];

		(*(void(__thiscall**)(DWORD*, char*, signed int))(*v3[89] + 596))(v3[89], login, 256); // textentry->GetText()
		(*(void(__thiscall**)(DWORD*, char*, signed int))(*v3[90] + 596))(v3[90], password, 256);

		wchar_t buf[256];
		swprintf(buf, L"/register %S %S", login, password);
		if (g_pChattingManager)
			g_pChattingManager->PrintToChat(1, buf);
		return;
	}

	g_pfnLoginDlg_OnCommand(_this, command);
}

void SH_GameBotAdd()
{
	static auto fnAddBot = reinterpret_cast<int(__cdecl*)(DWORD, DWORD)>(g_pfnBot_Add);
	int ArgVal1 = 0;
	int ArgVal2 = 0;
	int Argc = g_pEngine->Cmd_Argc();
	if (Argc > 0)
	{
		ArgVal1 = atoi(g_pEngine->Cmd_Argv(1));
		if (Argc >= 2)
		{
			ArgVal2 = atoi(g_pEngine->Cmd_Argv(2));
		}
	}
	fnAddBot(ArgVal1, ArgVal2);
}

DWORD WINAPI SH_Init(LPVOID lpThreadParameter)
{
	while (!(GetModuleHandleA("client.dll")) || !(GetModuleHandleA("gameui.dll")))
		Sleep(10);
	DWORD g_pFrame_OnKeyCodeTyped = SH_FindSignature("gameui.dll", "55 8B EC 83 EC 0C 89 4D F8 8B 45 F8 8B 10 8B 4D F8 FF 92 ? ? ? ? 25 ? ? ? ? 85 C0 74 46");
	if (!g_pFrame_OnKeyCodeTyped)
		MessageBoxA(0, "Cannot Find OnKeyCodeTyped \nMake sure you have the latest version of the game.", "Error", 0);
	MH_InlineHook((void*)g_pFrame_OnKeyCodeTyped, hk_OnKeyCodeTyped, (void*&)Ori_OnKeyCodeTyped);

	DWORD g_pLoginDlg_CallBacks = SH_FindSignature("gameui.dll", "55 8B EC 81 EC ? ? ? ? 89 8D ? ? ? ? 68 ? ? ? ? 8B 45 08 50 E8 ? ? ? ? 83 C4 08 85 C0 0F 85 8B 00 00 00 68 00 01 00 00 8D 8D");
	if (!g_pLoginDlg_CallBacks)
		MessageBoxA(0, "Cannot Find LoginDlgCallbacks \nMake sure you have the latest version of the game.", "Error", 0);
	MH_InlineHook((void*)g_pLoginDlg_CallBacks, LoginDlg_OnCommand, (void*&)g_pfnLoginDlg_OnCommand);


	g_pEngine = (cl_enginefunc_t*)(*(DWORD*)(SH_FindSignature("hw.dll", "68 ? ? ? ? FF 15 ? ? ? ? 83 C4 08 68 ? ? ? ?") + 1));
	if (!g_pEngine)
		MessageBoxA(0, "Cannot Find EngineFunc \nMake sure you have the latest version of the game.", "Error", 0);

	g_pChattingManager = (ChattingManager*)(*(int (**)(void))((DWORD)g_pEngine + 0x23C))();
	if (!g_pChattingManager)
		MessageBoxA(0, "Cannot Find ChatManager \nMake sure you have the latest version of the game.", "Error", 0);


	while (!(GetModuleHandleA("mp.dll")))
		Sleep(10);
	g_pfnBot_Add = SH_FindSignature("mp.dll", "55 8B EC 83 EC 4C 33 C0 A0 ? ? ? ? 85 C0");
	if (!g_pfnBot_Add)
		MessageBoxA(0, "Cannot Find BotAddFunc \nMake sure you have the latest version of the game.", "Error", 0);
	g_pEngine->pfnAddCommand(const_cast<char*>("cso_bot_add"), SH_GameBotAdd);

	DWORD MP3Volume = *(DWORD*)(SH_FindSignature("hw.dll", "D9 05 ? ? ? ? DD 5D F0") + 2);
	if (!MP3Volume)
		MessageBoxA(0, "Cannot Find MP3Volume \nMake sure you have the latest version of the game.", "Error", 0);
	while (!MP3VolChanged)
	{
		*reinterpret_cast <float*> (MP3Volume) = 0.1f;
		Sleep(100);
	}
	return 0;
}

uintptr_t SH_FindSignature(const char* szModule, const char* szSignature)
{
	const char* pat = szSignature;
	DWORD firstMatch = 0;
	DWORD rangeStart = (DWORD)GetModuleHandleA(szModule);
	MODULEINFO miModInfo;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)rangeStart, &miModInfo, sizeof(MODULEINFO));
	DWORD rangeEnd = rangeStart + miModInfo.SizeOfImage;
	for (DWORD pCur = rangeStart; pCur < rangeEnd; pCur++)
	{
		if (!*pat)
			return firstMatch;

		if (*(PBYTE)pat == '\?' || *(BYTE*)pCur == GET_BYTE(pat))
		{
			if (!firstMatch)
				firstMatch = pCur;

			if (!pat[2])
				return firstMatch;

			if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?')
				pat += 3;

			else
				pat += 2;
		}
		else
		{
			pat = szSignature;
			firstMatch = 0;
		}
	}
	return NULL;
}

void WriteBytes(PVOID address, void* val, int bytes) 
{
	DWORD d, ds;
	VirtualProtect(address, bytes, PAGE_EXECUTE_READWRITE, &d);
	memcpy(address, val, bytes);
	VirtualProtect(address, bytes, d, &ds);
}

namespace HackShield //日本版に必要なし HackShield関連
{
	void(__cdecl* oSetACInit)();
	void SetACInit()
	{

	}

	void(__cdecl* oHook1)();
	void Hook1()
	{

	}

	bool(__cdecl* oHook2)();
	bool Hook2()
	{
		oHook2();
		return 1;
	}
}

namespace HookFuncs
{
	char m_iEnableSSL = 0;
	int(__cdecl* oSocketManager__Constructor)(char* a1, char a2);
	int __cdecl SocketManager__Constructor(char* a1, char a2)
	{
		return oSocketManager__Constructor(a1, m_iEnableSSL);
	}

	int(__thiscall* oIpRedirector)(void* pThis, int ip, __int16 port, char a4);
	int __fastcall IpRedirector(void* pThis, void* edx, int ip, __int16 port, char a4)
	{
		return oIpRedirector(pThis, m_iHostIPAddress, m_iHostPort, a4);
	}

	int(__cdecl* oHolePunchFuncSetServerInfo)(int ip, __int16 port);
	int __cdecl HolePunchFuncSetServerInfo(int ip, __int16 port)
	{
		return oHolePunchFuncSetServerInfo(m_iHostIPAddress, m_iHostPort);
	}
}

void GamePatcher() 
{
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)GameUI_Patcher, 0, 0, 0);

	std::string sIpAddr = CommandLine()->GetParmValue("-ip");
	if (sIpAddr.empty())
		sIpAddr = "127.0.0.1";

	unsigned short nPort = CommandLine()->GetParmValue("-port", 30002);

	m_iHostIPAddress = inet_addr(sIpAddr.c_str());
	m_iHostPort = htons(nPort);

	if (CommandLine()->CheckParm("-nossl") != NULL)
		HookFuncs::m_iEnableSSL = 0;

	Utils::ConsolePrint("IP: %s\n", sIpAddr.c_str());
	Utils::ConsolePrint("Port: %d\n", nPort);

	//MH_InlineHook((void*)0x37297D80, HackShield::SetACInit, (void*&)HackShield::oSetACInit); //日本版に多分必要なし TW 2013から変更してない
	//MH_InlineHook((void*)0x37442F60, HackShield::Hook2, (void*&)HackShield::oHook2);//日本版に多分必要なし TW 2013から変更してない
	//WriteBytes((void*)0x37297AE2, (void*)"\xEB\x5D", 2); //HackShield　日本版に多分必要なし TW 2013から変更してない
	//WriteBytes((void*)0x37298B40, (void*)"\xC3", 1); //HackShield　日本版に多分必要なし TW 2013から変更してない

	MH_InlineHook((void*)0x375CA348, HookFuncs::SocketManager__Constructor, (void*&)HookFuncs::oSocketManager__Constructor);
	MH_InlineHook((void*)0x375CE6B0, HookFuncs::IpRedirector, (void*&)HookFuncs::oIpRedirector); //ショートカットで -ip 127.0.0.1 -port 30002 などにしないと落ちる
	MH_InlineHook((void*)0x372481BA, HookFuncs::HolePunchFuncSetServerInfo, (void*&)HookFuncs::oHolePunchFuncSetServerInfo);

	//MH_InlineHook((void*)0x375CB546, CSONMWrapper::AuthUser, (void*&)CSONMWrapper::oCSONMWrapper__AuthUser);
	//MH_InlineHook((void*)0x3754C2AD, CSONMWrapper::CheckIsAge18, (void*&)CSONMWrapper::oCheckIsAge18); //日本版に必要なし 多分あってる
	//MH_InlineHook((void*)0x3739BD3C, CSONMWrapper::COutPacket__SendLoginPacket, (void*&)CSONMWrapper::oCOutPacket__SendLoginPacket); //日本版に必要なし
	WriteBytes((void*)0x3723B287, (void*)"\x0F\x85", 2); //サーバー接続
	WriteBytes((void*)0x3728E414, (void*)"\x90\x90\x90\x90\x90", 5); //サーバー接続エラーメッセージ削除
}

DWORD WINAPI GameUI_Patcher() {
	DWORD dwGameUI = NULL;
	while (!dwGameUI) {
		dwGameUI = (DWORD)GetModuleHandleA("GameUI.dll");
		Sleep(0);
	}

	// [GAME-START]をクリックしてゲームを起動してください。削除 元アドレス10008CFA
	WriteBytes((void*)(dwGameUI + 0x8CFA),(void*)"\x90\x90\x90\x90\x90", 5);

	// ログインポップアップ表示① 元アドレス10008CD0
	WriteBytes((void*)(dwGameUI + 0x8CD0),(void*)"\x90\x90\x90\x90\x90\x90\x90\x90\x90", 9);

	// ログインポップアップ表示② 元アドレス10008CB3
	WriteBytes((void*)(dwGameUI + 0x8CB3),(void*)"\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 13);

	return 1;
}
