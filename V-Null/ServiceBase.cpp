#include "ServiceBase.h"
#include <assert.h>

CServiceBase* CServiceBase::s_service = NULL;

CServiceBase::CServiceBase(PWSTR pszServiceName,
    BOOL fCanStop,
    BOOL fCanShutdown,
    BOOL fCanPauseContinue)
{
    m_pszServiceName = (pszServiceName == NULL) ? const_cast<PWSTR>(L"") : pszServiceName;

    m_statusHandle = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_status.dwControlsAccepted = 0;
    if (fCanStop) m_status.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
    if (fCanShutdown) m_status.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
    if (fCanPauseContinue) m_status.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;

    m_status.dwWin32ExitCode = NO_ERROR;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}

CServiceBase::~CServiceBase(void) { }

BOOL CServiceBase::Run(CServiceBase& service)
{
    s_service = &service;

    SERVICE_TABLE_ENTRYW serviceTable[] =
    {
        { service.m_pszServiceName, (LPSERVICE_MAIN_FUNCTIONW)ServiceMain },
        { NULL, NULL }
    };

    return StartServiceCtrlDispatcherW(serviceTable);
}

void WINAPI CServiceBase::ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv)
{
    assert(s_service != NULL);

    s_service->m_statusHandle = RegisterServiceCtrlHandlerW(
        s_service->m_pszServiceName, Handler);

    if (s_service->m_statusHandle == NULL) return;

    s_service->Start(dwArgc, lpszArgv);
}

void WINAPI CServiceBase::Handler(DWORD dwControl)
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP: s_service->Stop(); break;
    case SERVICE_CONTROL_PAUSE: s_service->OnPause(); break;
    case SERVICE_CONTROL_CONTINUE: s_service->OnContinue(); break;
    case SERVICE_CONTROL_SHUTDOWN: s_service->OnShutdown(); break;
    default: break;
    }
}

void CServiceBase::Start(DWORD dwArgc, PWSTR* pszArgv)
{
    try
    {
        SetServiceStatus(SERVICE_START_PENDING);
        OnStart(dwArgc, pszArgv);
        SetServiceStatus(SERVICE_RUNNING);
    }
    catch (...)
    {
        WriteEventLogEntry(const_cast<PWSTR>(L"Service failed to start."), EVENTLOG_ERROR_TYPE);
        SetServiceStatus(SERVICE_STOPPED);
    }
}

void CServiceBase::Stop()
{
    SetServiceStatus(SERVICE_STOP_PENDING);
    try
    {
        OnStop();
        SetServiceStatus(SERVICE_STOPPED);
    }
    catch (...)
    {
        WriteEventLogEntry(const_cast<PWSTR>(L"Service failed to stop."), EVENTLOG_ERROR_TYPE);
        SetServiceStatus(SERVICE_STOPPED);
    }
}

void CServiceBase::OnStart(DWORD dwArgc, PWSTR* pszArgv) {}
void CServiceBase::OnStop() {}
void CServiceBase::OnPause() {}
void CServiceBase::OnContinue() {}
void CServiceBase::OnShutdown() {}

void CServiceBase::SetServiceStatus(DWORD dwCurrentState,
    DWORD dwWin32ExitCode,
    DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    m_status.dwCurrentState = dwCurrentState;
    m_status.dwWin32ExitCode = dwWin32ExitCode;
    m_status.dwWaitHint = dwWaitHint;

    m_status.dwCheckPoint = ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) ? 0 : dwCheckPoint++;

    ::SetServiceStatus(m_statusHandle, &m_status);
}

void CServiceBase::WriteEventLogEntry(PWSTR pszMessage, WORD wType)
{
    HANDLE hEventSource = RegisterEventSourceW(NULL, m_pszServiceName);
    if (hEventSource)
    {
        PCWSTR lpszStrings[2] = { m_pszServiceName, pszMessage };
        ReportEventW(hEventSource, wType, 0, 0, NULL, 2, 0, lpszStrings, NULL);
        DeregisterEventSource(hEventSource);
    }
}