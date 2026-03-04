#pragma once
#include <windows.h>
#include <string>

std::wstring GenerateServiceName ( );
std::wstring GetServiceDescription ( const std::wstring& name );
bool         SaveServiceName ( const std::wstring& name );
std::wstring LoadServiceName ( );
void         ClearServiceName ( );

void InstallService(PCWSTR pszServiceName,
    PCWSTR pszDisplayName,
    PCWSTR pszDescription,
    DWORD dwStartType,
    PCWSTR pszDependencies,
    PCWSTR pszAccount,
    PCWSTR pszPassword);

void UninstallService(PCWSTR pszServiceName);
