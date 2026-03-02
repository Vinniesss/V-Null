#include "VNullService.h"
#include "ProcessWatcher.h"
#include <stdio.h>
#include <string>
#include <WtsApi32.h>
#include <userenv.h>

#pragma comment ( lib , "Wtsapi32.lib" )
#pragma comment ( lib , "Userenv.lib" )

static void launch_splash_on_desktop ( )
{
    DWORD session_id = WTSGetActiveConsoleSessionId ( );
    if ( session_id == 0xFFFFFFFF ) return;

    HANDLE hToken = NULL;
    if ( !WTSQueryUserToken ( session_id , &hToken ) ) return;

    HANDLE hDupToken = NULL;
    DuplicateTokenEx ( hToken , MAXIMUM_ALLOWED , NULL ,
        SecurityIdentification , TokenPrimary , &hDupToken );
    CloseHandle ( hToken );
    if ( !hDupToken ) return;

    LPVOID pEnv = NULL;
    CreateEnvironmentBlock ( &pEnv , hDupToken , FALSE );

    wchar_t exe_path[MAX_PATH];
    GetModuleFileNameW ( NULL , exe_path , MAX_PATH );

    std::wstring cmd_line = L"\"" + std::wstring ( exe_path ) + L"\" -splash";

    STARTUPINFOW si = { sizeof ( si ) };
    si.lpDesktop = const_cast<LPWSTR> ( L"winsta0\\default" );
    PROCESS_INFORMATION pi = { };

    CreateProcessAsUserW (
        hDupToken , NULL , &cmd_line[0] , NULL , NULL , FALSE ,
        CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT ,
        pEnv , NULL , &si , &pi );

    if ( pi.hProcess ) CloseHandle ( pi.hProcess );
    if ( pi.hThread )  CloseHandle ( pi.hThread );
    if ( pEnv )        DestroyEnvironmentBlock ( pEnv );
    CloseHandle ( hDupToken );
}

CVNullService::CVNullService(PWSTR pszServiceName,
    BOOL fCanStop,
    BOOL fCanShutdown,
    BOOL fCanPauseContinue)
    : CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
    m_fStopping = FALSE;
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CVNullService::~CVNullService(void)
{
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}

void CVNullService::OnStart(DWORD dwArgc, PWSTR* pszArgv)
{
    launch_splash_on_desktop ( );

    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, this, 0, NULL);
    if (hThread)
        CloseHandle(hThread);
}

void CVNullService::OnStop()
{
    m_fStopping = TRUE;

    if (WaitForSingleObject(m_hStoppedEvent, 3000) != WAIT_OBJECT_0)
    {
    }
}

DWORD WINAPI CVNullService::ServiceWorkerThread(LPVOID lpParam)
{
    CVNullService* pService = (CVNullService*)lpParam;

    watch_processes ( &pService->m_fStopping );

    SetEvent(pService->m_hStoppedEvent);
    return 0;
}
