/*
This code will inject a given DLL into the memory space of a given process id. 
Bing your own DLL. 

Created by Ernesto Vazquez
*/
#include <Windows.h>

DWORD CreateThread(DWORD processId, wchar_t* dllPath) {
    DWORD dwSize = (lstrlen(dllPath) + 1) * sizeof(wchar_t);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION
        | PROCESS_CREATE_THREAD
        | PROCESS_VM_OPERATION
        | PROCESS_VM_WRITE, FALSE, processId);
    if (hProcess == NULL) {
        return -1;
    }
    LPVOID pszLibFileRemote = (PWSTR)VirtualAllocEx(hProcess, NULL, dwSize,
        MEM_COMMIT, PAGE_READWRITE);
    if (pszLibFileRemote == NULL) {
        return -1;
    }
    DWORD n = WriteProcessMemory(hProcess, pszLibFileRemote, (PVOID)dllPath,
        dwSize, NULL);
    if (n == 0) {
        return -1;
    }
    PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
    if (pfnThreadRtn == NULL) {
        return -1;
    }
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn,
        pszLibFileRemote, 0, NULL);
    if (hThread == NULL) {
        return -1;
    }
    WaitForSingleObject(hThread, INFINITE);
    if (pszLibFileRemote != NULL) {
        VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);
    }
    if (hThread != NULL) {
        CloseHandle(hThread);
    }
    if (hProcess != NULL) {
        CloseHandle(hProcess);
    }
    return 1;
}

int main(int argc, char **argv) 
{
    if (argc < 3) {
        return -1;
    }
    int processId = atoi(argv[1]);
    wchar_t* dllPath = new wchar_t[strlen(argv[2]) + 1];
    mbstowcs_s(NULL, dllPath, strlen(argv[2]) + 1, argv[2], strlen(argv[2]));
    if (CreateThread(processId, dllPath) != 1) {
        return -1;
    }
    return 0;
}
