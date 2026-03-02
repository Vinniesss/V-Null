#include "ProcessWatcher.h"
#include "Blacklist.h"

#include <comdef.h>
#include <Wbemidl.h>
#include <string>

#pragma comment ( lib , "wbemuuid.lib" )

void watch_processes ( BOOL* pfStopping )
{
    HRESULT hr = CoInitializeEx ( 0 , COINIT_MULTITHREADED );
    if ( FAILED ( hr ) ) return;

    hr = CoInitializeSecurity (
        NULL , -1 , NULL , NULL ,
        RPC_C_AUTHN_LEVEL_DEFAULT ,
        RPC_C_IMP_LEVEL_IMPERSONATE ,
        NULL , EOAC_NONE , NULL );

    IWbemLocator* locator = NULL;

    hr = CoCreateInstance (
        CLSID_WbemLocator , NULL , CLSCTX_INPROC_SERVER ,
        IID_IWbemLocator , ( LPVOID* ) &locator );

    if ( FAILED ( hr ) )
    {
        CoUninitialize ( );
        return;
    }

    IWbemServices* services = NULL;

    hr = locator->ConnectServer (
        _bstr_t ( L"ROOT\\CIMV2" ) ,
        NULL , NULL , 0 , NULL , 0 , 0 , &services );

    if ( FAILED ( hr ) )
    {
        locator->Release ( );
        CoUninitialize ( );
        return;
    }

    hr = CoSetProxyBlanket (
        services ,
        RPC_C_AUTHN_WINNT , RPC_C_AUTHZ_NONE , NULL ,
        RPC_C_AUTHN_LEVEL_CALL ,
        RPC_C_IMP_LEVEL_IMPERSONATE ,
        NULL , EOAC_NONE );

    IEnumWbemClassObject* event_enum = NULL;

    hr = services->ExecNotificationQuery (
        _bstr_t ( "WQL" ) ,
        _bstr_t ( "SELECT * FROM __InstanceCreationEvent WITHIN 0.1 "
                   "WHERE TargetInstance ISA 'Win32_Process'" ) ,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY ,
        NULL , &event_enum );

    if ( FAILED ( hr ) )
    {
        services->Release ( );
        locator->Release ( );
        CoUninitialize ( );
        return;
    }

    auto blacklist = get_blacklist ( );

    while ( !( *pfStopping ) )
    {
        IWbemClassObject* event_obj = NULL;
        ULONG returned = 0;

        hr = event_enum->Next ( 500 , 1 , &event_obj , &returned );

        if ( hr == WBEM_S_TIMEDOUT )
            continue;

        if ( FAILED ( hr ) || !returned )
            continue;

        VARIANT var;
        hr = event_obj->Get ( L"TargetInstance" , 0 , &var , 0 , 0 );

        if ( SUCCEEDED ( hr ) && var.vt == VT_UNKNOWN )
        {
            IWbemClassObject* proc_obj = NULL;
            var.punkVal->QueryInterface ( IID_IWbemClassObject , ( void** ) &proc_obj );

            VARIANT name_var;
            proc_obj->Get ( L"Name" , 0 , &name_var , 0 , 0 );

            if ( name_var.vt == VT_BSTR )
            {
                std::wstring exe_name = name_var.bstrVal;
                std::wstring category;

                if ( is_blacklisted ( exe_name , blacklist , category ) )
                {
                    VARIANT pid_var;
                    proc_obj->Get ( L"ProcessId" , 0 , &pid_var , 0 , 0 );

                    HANDLE proc = OpenProcess ( PROCESS_TERMINATE , FALSE , pid_var.uintVal );
                    if ( proc )
                    {
                        TerminateProcess ( proc , 1 );
                        CloseHandle ( proc );

                        std::wstring msg = L"[V-Null] Blocked: " + exe_name + L" (" + category + L")\n";
                        OutputDebugStringW ( msg.c_str ( ) );
                    }

                    VariantClear ( &pid_var );
                }
            }

            VariantClear ( &name_var );
            proc_obj->Release ( );
        }

        VariantClear ( &var );
        event_obj->Release ( );
    }

    event_enum->Release ( );
    services->Release ( );
    locator->Release ( );
    CoUninitialize ( );
}
