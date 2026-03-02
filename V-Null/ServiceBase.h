#pragma once
#pragma once
#include <windows.h>

class CServiceBase
{
public:
    // This is the "Handshake" function that connects your EXE to Windows.
    static BOOL Run(CServiceBase& service);

    CServiceBase(PWSTR pszServiceName,
        BOOL fCanStop = TRUE,
        BOOL fCanShutdown = TRUE,
        BOOL fCanPauseContinue = FALSE);

    virtual ~CServiceBase(void);

    void Stop();

protected:
    // These are "Hooks." You will put your actual logic inside these 
    // when we get to Step 5.
    virtual void OnStart(DWORD dwArgc, PWSTR* pszArgv);
    virtual void OnStop();
    virtual void OnPause();
    virtual void OnContinue();
    virtual void OnShutdown();

    // This allows the service to write messages to the Event Viewer.
    void WriteEventLogEntry(PWSTR pszMessage, WORD wType);

private:
    // Internal Windows callbacks—Windows calls these directly.
    static void WINAPI ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv);
    static void WINAPI Handler(DWORD dwControl);

    void Start(DWORD dwArgc, PWSTR* pszArgv);
    void SetServiceStatus(DWORD dwCurrentState,
        DWORD dwWin32ExitCode = NO_ERROR,
        DWORD dwWaitHint = 0);

    static CServiceBase* s_service;
    PWSTR m_pszServiceName;
    SERVICE_STATUS m_status;
    SERVICE_STATUS_HANDLE m_statusHandle;
};
