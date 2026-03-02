#pragma once
#include "ServiceBase.h"
#include <windows.h>

class CVNullService : public CServiceBase
{
public:
    CVNullService(PWSTR pszServiceName,
        BOOL fCanStop = TRUE,
        BOOL fCanShutdown = TRUE,
        BOOL fCanPauseContinue = FALSE);

    virtual ~CVNullService(void);

protected:
    virtual void OnStart(DWORD dwArgc, PWSTR* pszArgv) override;
    virtual void OnStop() override;

private:
    BOOL m_fStopping;
    HANDLE m_hStoppedEvent;

    static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
};
