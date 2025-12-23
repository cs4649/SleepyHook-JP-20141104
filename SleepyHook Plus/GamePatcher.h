#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <HLSDK/Manager/IChatManager.h>
void GamePatcher();

DWORD WINAPI SH_Init(LPVOID lpThreadParameter);
uintptr_t SH_FindSignature(const char* szModule, const char* szSignature);
extern cl_enginefunc_t* g_pEngine;