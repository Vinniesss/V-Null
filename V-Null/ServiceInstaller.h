#pragma once
#include <windows.h>
#include <string>

std::wstring GenerateServiceName ( );
bool         SaveServiceName ( const std::wstring& name );
std::wstring LoadServiceName ( );
void         ClearServiceName ( );

void InstallService(PCWSTR pszServiceName,
    PCWSTR pszDisplayName,
    DWORD dwStartType,
    PCWSTR pszDependencies,
    PCWSTR pszAccount,
    PCWSTR pszPassword);

void UninstallService(PCWSTR pszServiceName);
