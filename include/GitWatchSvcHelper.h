#ifndef __GITWATCHSVCHELPER_H
#define __GITWATCHSVCHELPER_H

#include <windows.h>
#include <winsvc.h>

class GitWatchSvcHelper
{
protected:
  SERVICE_NOTIFY msvcNotify;
  bool mNotifyRegistered;
  SC_HANDLE mschSCManager;
  SC_HANDLE mschService;
public:
  BOOL elevatePrivileges(std::string params);
  int install(std::string username, std::string password, std::string &msg);
  int deleteSvc(std::string &msg);
  int start(std::string &msg);
  int stop(std::string &msg);
  int checkRunning(std::string &msg, DWORD* pCurrentState);
  static VOID CALLBACK notifyCallback(PVOID pParam);
  int registerSvcNotify(HWND hStaticTxtWnd, std::string &msg);
  void closeSvcNotify();
};

#endif
