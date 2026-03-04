#include <iostream>
#include <conio.h>
#include <Windows.h>
#include <string>
#include <vector>
#include "ServiceConfig.h"
#include "ServiceBase.h"
#include "ServiceInstaller.h"
#include "VNullService.h"

static void show_splash ( )
{
    HANDLE hCon = GetStdHandle ( STD_OUTPUT_HANDLE );
    SetConsoleTitle ( SERVICE_DISPLAY_NAME );

    DWORD mode = 0;
    GetConsoleMode ( hCon , &mode );
    SetConsoleMode ( hCon , mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );

    wprintf ( L"\n" );
    wprintf ( L"\x1b[38;2;100;0;140m  :::     :::         ::::    ::: :::    ::: :::        :::\n" );
    wprintf ( L"\x1b[38;2;120;0;170m  :+:     :+:         :+:+:   :+: :+:    :+: :+:        :+:\n" );
    wprintf ( L"\x1b[38;2;140;20;200m  +:+     +:+         :+:+:+  +:+ +:+    +:+ +:+        +:+\n" );
    wprintf ( L"\x1b[38;2;160;40;220m  +#+     +:+  +#++:  +#+ +:+ +#+ +#+    +#+ +#+        +#+\n" );
    wprintf ( L"\x1b[38;2;180;60;235m   +#+   +#+         +#+  +#+#+# +#+    +#+ +#+        +#+\n" );
    wprintf ( L"\x1b[38;2;200;90;245m    #+#+#+#          #+#   #+#+# #+#    #+# #+#        #+#\n" );
    wprintf ( L"\x1b[38;2;220;120;255m      ###            ###    ####  ########  ######## ####### \n" );
    wprintf ( L"\x1b[0m\n" );

    const wchar_t* tagline = L"  V-Null - Say no to invasiveness and yes to privacy.";
    int len = ( int ) wcslen ( tagline );
    for ( int i = 0; i < len; i++ )
    {
        int r = 100 + ( 120 * i / len );
        int g =   0 + ( 120 * i / len );
        int b = 140 + ( 115 * i / len );
        wprintf ( L"\x1b[38;2;%d;%d;%dm%c" , r , g , b , tagline[i] );
    }
    wprintf ( L"\x1b[0m\n\n" );
    wprintf ( L"  Press any key to close . . .\n\n" );
    _getch ( );
}

int wmain ( int argc , wchar_t* argv[] )
{
    if ( ( argc > 1 ) && ( ( *argv[1] == L'-' || ( *argv[1] == L'/' ) ) ) )
    {
        if ( _wcsicmp ( L"install" , argv[1] + 1 ) == 0 )
        {
            std::wstring svc_name = GenerateServiceName ( );
            std::wstring svc_desc = GetServiceDescription ( svc_name );

            if ( !SaveServiceName ( svc_name ) )
            {
                wprintf ( L"Failed to save service identity to registry.\n" );
                return 1;
            }

            InstallService (
                svc_name.c_str ( ) ,
                svc_name.c_str ( ) ,
                svc_desc.empty ( ) ? nullptr : svc_desc.c_str ( ) ,
                SERVICE_START_TYPE ,
                SERVICE_DEPENDENCIES ,
                SERVICE_ACCOUNT ,
                SERVICE_PASSWORD
            );
        }
        else if ( _wcsicmp ( L"remove" , argv[1] + 1 ) == 0 )
        {
            std::wstring svc_name = LoadServiceName ( );
            if ( svc_name.empty ( ) )
            {
                wprintf ( L"No installed service found.\n" );
                return 1;
            }
            UninstallService ( svc_name.c_str ( ) );
        }
        else if ( _wcsicmp ( L"splash" , argv[1] + 1 ) == 0 )
        {
            show_splash ( );
        }
    }
    else
    {
        std::wstring svc_name = LoadServiceName ( );
        if ( svc_name.empty ( ) )
        {
            wprintf ( L"No service identity found in registry.\n" );
            return 1;
        }

        std::vector< wchar_t > name_buf ( svc_name.begin ( ) , svc_name.end ( ) );
        name_buf.push_back ( L'\0' );

        CVNullService service ( name_buf.data ( ) );

        if ( !CServiceBase::Run ( service ) )
        {
            wprintf ( L"Service failed to run w/err 0x%08lx\n" , GetLastError ( ) );
        }
    }

    return 0;
}
