#pragma once
#include <windows.h>

void InstallService(PCWSTR pszServiceName,
    PCWSTR pszDisplayName,
    DWORD dwStartType,
    PCWSTR pszDependencies,
    PCWSTR pszAccount,
    PCWSTR pszPassword);

void UninstallService(PCWSTR pszServiceName);
