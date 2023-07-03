// GitWatch.cpp : Defines the entry point for the application.
//
// header.h : include file for standard system include files,
// or project specific include files
//

#include <string>
#include <iostream>
#include <sstream>
#include <windows.h>
#include <processthreadsapi.h>
#include <tchar.h>
#include <strsafe.h>
//#include "internal.h"
// C RunTime Header Files
//#include <cmalloc>
//#include <cmemory>
#include <fileapi.h>
#include "GitWatch.h"
#include "GitWatchUtil.h"
#include "GitWatchSvc.h"
#include "GitWatchSvcHelper.h"

extern GitWatchSvc gitWatchSvc;
GitWatchSvcHelper gitWatchSvcHelper;

#ifndef SVC_ERROR
#define SVC_ERROR 11
#endif

std::string GetServiceStateTxt(DWORD dwCurrentState);

BOOL GitWatchSvcHelper::elevatePrivileges(std::string params)
{
  char exePath[MAX_PATH+1];
  char curDir[MAX_PATH+1];
  DWORD returnLength = 0;
  TOKEN_ELEVATION te;
  
  memset(&te, 0, sizeof(te));

  HANDLE hToken =  0;
  OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);  
  GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &returnLength);
  if (returnLength > 0 && te.TokenIsElevated == 0)
  {
    memset(exePath, 0, sizeof(exePath));
    memset(curDir, 0, sizeof(curDir));
    GetModuleFileName(NULL, exePath, sizeof(exePath));
    GetCurrentDirectory(sizeof(curDir), curDir);
    ShellExecute(NULL, "runas", exePath, params.c_str(), curDir, SW_SHOWNORMAL);
  }
  CloseHandle(hToken);
  return te.TokenIsElevated ? TRUE : FALSE;
}


//
// Purpose: 
//   Installs a service in the SCM database
//
// Parameters:
//   std::string err
// 
// Return value:
//   0: fail, 1: success
//
int GitWatchSvcHelper::install(std::string username, std::string password, std::string &msg)
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  TCHAR unquotedPath[MAX_PATH];
  std::stringstream strBuf;

  if (!GetModuleFileName(NULL, unquotedPath, MAX_PATH))
  {
    DWORD errorCode = GetLastError();
    strBuf << "Cannot install service: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    return 0;
  }

  // In case the path contains a space, it must be quoted so that
  // it is correctly interpreted. For example,
  // "d:\my share\myservice.exe" should be specified as
  // ""d:\my share\myservice.exe"".
  TCHAR path[MAX_PATH];
  StringCbPrintf(path, MAX_PATH, "\"%s\"", unquotedPath);

  std::string realPath = path;
  std::string exeGuiName = "GitWatchGui.exe";
  std::string exeSvcName = "GitWatchSvc.exe";

  size_t pos = realPath.rfind(exeGuiName);
  if (pos != std::string::npos)
  {
    realPath.replace(pos, exeGuiName.length(), exeSvcName);
  }


  //MessageBox(0, realPath.c_str(), "Path", MB_OK);

  // Get a handle to the SCM database. 
 
  schSCManager = OpenSCManager( 
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    SC_MANAGER_ALL_ACCESS);  // full access rights 
 
  if (schSCManager == NULL) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "OpenSCManager failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    return 0;
  }

  // Create the service

  if (username.length() > 0)
  {
    schService = CreateService( 
      schSCManager,              // SCM database 
      GitWatchSvc::getSvcName(), // name of service 
      GitWatchSvc::getSvcName(), // service name to display 
      SERVICE_ALL_ACCESS,        // desired access 
      SERVICE_WIN32_OWN_PROCESS, // service type 
      SERVICE_AUTO_START,        // start type 
      SERVICE_ERROR_NORMAL,      // error control type 
      realPath.c_str(),          // path to service's binary 
      NULL,                      // no load ordering group 
      NULL,                      // no tag identifier 
      NULL,                      // no dependencies 
      username.c_str(),
      password.c_str());
  }
  else
  {
    schService = CreateService( 
      schSCManager,              // SCM database 
      GitWatchSvc::getSvcName(), // name of service 
      GitWatchSvc::getSvcName(), // service name to display 
      SERVICE_ALL_ACCESS,        // desired access 
      SERVICE_WIN32_OWN_PROCESS, // service type 
      SERVICE_AUTO_START,        // start type 
      SERVICE_ERROR_NORMAL,      // error control type 
      realPath.c_str(),          // path to service's binary 
      NULL,                      // no load ordering group 
      NULL,                      // no tag identifier 
      NULL,                      // no dependencies 
      NULL,                      // LocalSystem account 
      NULL);                     // no password 
  }
 
  if (schService == NULL) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "CreateService failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    CloseServiceHandle(schSCManager);
    return 0;
  }
  else
  {
    msg = "Service installed successfully.";
  }

  CloseServiceHandle(schService); 
  CloseServiceHandle(schSCManager);
  return 1;
}


//
// Purpose: 
//   Deletes a service from the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int GitWatchSvcHelper::deleteSvc(std::string &msg)
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  std::stringstream strBuf;
  int success = 0;

  // Get a handle to the SCM database. 
 
  schSCManager = OpenSCManager( 
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    SC_MANAGER_ALL_ACCESS);  // full access rights 
 
  if (schSCManager == NULL) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "OpenSCManager failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    return 0;
  }

  // Get a handle to the service.

  schService = OpenService( 
    schSCManager,       // SCM database 
    GitWatchSvc::getSvcName(),           // name of service 
    DELETE);            // need delete access 
 
  if (schService == NULL)
  { 
    DWORD errorCode = GetLastError();
    strBuf << "OpenService failed: " << getLastErrorAsString(errorCode); 
    msg = strBuf.str();
    CloseServiceHandle(schSCManager);
    return 0;
  }

  // Delete the service.
 
  if (!DeleteService(schService)) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "DeleteService failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    success = 0;
  }
  else
  {
    msg = "Service deleted successfully."; 
    success = 1;
  }
 
  CloseServiceHandle(schService); 
  CloseServiceHandle(schSCManager);
  return success;
}


//
// Purpose: 
//   Deletes a service from the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int GitWatchSvcHelper::start(std::string &msg)
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  std::stringstream strBuf;
  int success = 0;

  // Get a handle to the SCM database. 
 
  schSCManager = OpenSCManager( 
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    GENERIC_READ | GENERIC_EXECUTE);
 
  if (schSCManager == NULL) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "OpenSCManager failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    return 0;
  }

  // Get a handle to the service.

  schService = OpenService( 
    schSCManager,               // SCM database 
    GitWatchSvc::getSvcName(),  // name of service 
    SERVICE_START);             // need start access 
 
  if (schService == NULL)
  { 
    DWORD errorCode = GetLastError();
    strBuf << "OpenService failed: " << getLastErrorAsString(errorCode); 
    msg = strBuf.str();
    CloseServiceHandle(schSCManager);
    return 0;
  }

  // Delete the service.
 
  if (!StartService(schService, 0, NULL)) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "StartService failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    success = 0;
  }
  else
  {
    msg = "Started service successfully."; 
    success = 1;
  }
 
  CloseServiceHandle(schService); 
  CloseServiceHandle(schSCManager);
  return success;
}


//
// Purpose: 
//   Deletes a service from the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int GitWatchSvcHelper::stop(std::string &msg)
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  std::stringstream strBuf;
  int success = 0;

  // Get a handle to the SCM database. 
 
  schSCManager = OpenSCManager( 
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    SC_MANAGER_CONNECT | STANDARD_RIGHTS_EXECUTE);  
 
  if (NULL == schSCManager) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "OpenSCManager failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    return 0;
  }

  // Get a handle to the service.

  schService = OpenService( 
    schSCManager,       // SCM database 
    GitWatchSvc::getSvcName(),           // name of service 
    SERVICE_STOP);      // need stop access 
 
  if (schService == NULL)
  { 
    DWORD errorCode = GetLastError();
    strBuf << "OpenService failed: " << getLastErrorAsString(errorCode); 
    msg = strBuf.str();
    CloseServiceHandle(schSCManager);
    return 0;
  }

  // Delete the service.
 
  SERVICE_STATUS svcStatus;
  if (!ControlService(schService, SERVICE_CONTROL_STOP, &svcStatus)) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "StopService failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    success = 0;
  }
  else
  {
    msg = "Stopped service successfully."; 
    success = 1;
  }
 
  CloseServiceHandle(schService); 
  CloseServiceHandle(schSCManager);
  return success;
}


//
// Purpose: 
//   Deletes a service from the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int GitWatchSvcHelper::checkRunning(std::string &msg, DWORD * pCurrentState)
{
  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  std::stringstream strBuf;
  int status = 0;

  // Get a handle to the SCM database. 
  if (pCurrentState)
    *pCurrentState = 0;
 
  schSCManager = OpenSCManager( 
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    SC_MANAGER_CONNECT | STANDARD_RIGHTS_READ);
 
  if (schSCManager == NULL) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "OpenSCManager failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    return 0;
  }

  // Get a handle to the service.

  schService = OpenService( 
    schSCManager,               // SCM database 
    GitWatchSvc::getSvcName(),  // name of service 
    SERVICE_INTERROGATE);       // need interrogate access 
 
  if (schService == NULL)
  { 
    DWORD errorCode = GetLastError();
    strBuf << "OpenService failed: " << getLastErrorAsString(errorCode); 
    msg = strBuf.str();
    CloseServiceHandle(schSCManager);
    return 0;
  }

  // Interrogate the service.
 
  SERVICE_STATUS svcStatus = {};
  if (!ControlService(schService, SERVICE_CONTROL_INTERROGATE, &svcStatus)) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "ControlService failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
  }
  else
  {
    msg = GetServiceStateTxt(svcStatus.dwCurrentState);
    if (pCurrentState)
      *pCurrentState = svcStatus.dwCurrentState;
    status = 1;
  }
 
  CloseServiceHandle(schService); 
  CloseServiceHandle(schSCManager);
  return status;
}


std::string GetServiceStateTxt(DWORD dwCurrentState)
{
  std::string msg;

  switch (dwCurrentState)
  {
  case SERVICE_CONTINUE_PENDING:
    msg = "Service Continue Pending.";
    break;
  case SERVICE_PAUSE_PENDING:
    msg = "Service Pause Pending.";
    break;
  case SERVICE_PAUSED:
    msg = "Service Paused.";
    break;
  case SERVICE_RUNNING:
    msg = "Service Running.";
    break;
  case SERVICE_START_PENDING:
    msg = "Service Start Pending.";
    break;
  case SERVICE_STOP_PENDING:
    msg = "Service Stop Pending.";
    break;
  case SERVICE_STOPPED:
    msg = "Service Stopped.";
    break;
  default:
    msg = "Unknown Service Status.";
    break;
  }
  return msg;
}

//
// Purpose: 
//   Deletes a service from the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
int GitWatchSvcHelper::registerSvcNotify(HWND hStaticTxtWnd, std::string &msg)
{
  std::stringstream strBuf;
  int success = 0;
  
  // Get a handle to the SCM database. 
 
  mschSCManager = OpenSCManager( 
    NULL,                    // local computer
    NULL,                    // ServicesActive database 
    SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE); 
 
  if (mschSCManager == NULL) 
  {
    DWORD errorCode = GetLastError();
    strBuf << "OpenSCManager failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    return 0;
  }

  // Get a handle to the service.

  mschService = OpenService( 
    mschSCManager,              // SCM database 
    GitWatchSvc::getSvcName(), // name of service 
    SERVICE_QUERY_STATUS);     // need interrogate access 
 
  if (mschService == NULL)
  { 
    DWORD errorCode = GetLastError();
    strBuf << "OpenService failed: " << getLastErrorAsString(errorCode); 
    msg = strBuf.str();
    CloseServiceHandle(mschSCManager);
    return 0;
  }

  // Interrogate the service.
 
  memset(&msvcNotify, 0, sizeof(msvcNotify));
  msvcNotify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
  msvcNotify.pfnNotifyCallback = notifyCallback;
  msvcNotify.pContext = hStaticTxtWnd;

  DWORD notifyMask = SERVICE_NOTIFY_RUNNING |
                     SERVICE_NOTIFY_PAUSED |
                     SERVICE_NOTIFY_CONTINUE_PENDING |
                     SERVICE_NOTIFY_DELETE_PENDING |
                     SERVICE_NOTIFY_PAUSE_PENDING |
                     SERVICE_NOTIFY_START_PENDING |
                     SERVICE_NOTIFY_STOP_PENDING |
                     SERVICE_NOTIFY_STOPPED;

  DWORD errorCode = NotifyServiceStatusChange(mschService, notifyMask, (PSERVICE_NOTIFY) &msvcNotify);
  if (errorCode == ERROR_SUCCESS) 
  {
    msg = "NotifyServiceStatusChange == ERROR_SUCCESS";
    success = 1;
  }
  else
  {
    strBuf << "ControlService failed: " << getLastErrorAsString(errorCode);
    msg = strBuf.str();
    closeSvcNotify();
    success = 0;
  }
  return success;
}


VOID CALLBACK GitWatchSvcHelper::notifyCallback(PVOID pParam)
{
  SERVICE_NOTIFY * pSvcNotify = (SERVICE_NOTIFY*) pParam;

  MessageBox(0, "NotifyCallback called!", "Message", MB_OK);
  if (pSvcNotify->pContext)
  {
    std::string msg = GetServiceStateTxt(pSvcNotify->dwNotificationStatus);
    SetWindowText((HWND) pSvcNotify->pContext, msg.c_str());
    MessageBox(0, msg.c_str(), "GitWatch Message", MB_OK);
  }
}


void GitWatchSvcHelper::closeSvcNotify()
{
  if (mschService)
  {
    CloseServiceHandle(mschService); 
  }  
  
  if (mschSCManager)
  {
    CloseServiceHandle(mschSCManager);
  }
  mschService = NULL;
  mschSCManager = NULL;
}
