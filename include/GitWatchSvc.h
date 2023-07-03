#ifndef __GITWATCH_SVC_H
#define __GITWATCH_SVC_H

#include <windows.h>
#include <winsvc.h>

class GitWatchSvc
{
protected:
  static char mSvcName[32];
  static SERVICE_STATUS_HANDLE mSvcStatusHandle; 
  static HANDLE mhSvcStopEvent;
  static SERVICE_STATUS mSvcStatus; 
public:
  static VOID WINAPI main(DWORD, LPSTR*);
  static VOID WINAPI ctrlHandler(DWORD); 
  static void reportSvcStatus(DWORD, DWORD, DWORD);
  static void init(DWORD, LPSTR*); 
  static void reportEvent(const char*, DWORD);
  static char* getSvcName();
};

#endif
