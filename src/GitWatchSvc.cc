// GitWatch.cpp : Defines the entry point for the application.
//
// header.h : include file for standard system include files,
// or project specific include files
//

#include <string>
//#include <iostream>
#include <sstream>
#include <windows.h>
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

char GitWatchSvc::mSvcName[32] = "Gitch Watch Service";
SERVICE_STATUS_HANDLE GitWatchSvc::mSvcStatusHandle = NULL;
HANDLE GitWatchSvc::mhSvcStopEvent = NULL;
SERVICE_STATUS GitWatchSvc::mSvcStatus;

GitWatchSvc gitWatchSvc;


char * GitWatchSvc::getSvcName()
{
  return mSvcName;
}


//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI GitWatchSvc::ctrlHandler(DWORD dwCtrl)
{
  // Handle the requested control code. 

  switch(dwCtrl) 
  {  
    case SERVICE_CONTROL_STOP: 
    {
      reportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

      // Signal the service to stop.
      SetEvent(mhSvcStopEvent);
      reportSvcStatus(mSvcStatus.dwCurrentState, NO_ERROR, 0);
         
      return;
    }
    case SERVICE_CONTROL_INTERROGATE: 
    {
      break; 
    }
    default: 
    {
      break;
    }
  } 
}


//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
void GitWatchSvc::reportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
  static DWORD dwCheckPoint = 1;

  // Fill in the SERVICE_STATUS structure.
  mSvcStatus.dwCurrentState = dwCurrentState;
  mSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
  mSvcStatus.dwWaitHint = dwWaitHint;

  if (dwCurrentState == SERVICE_START_PENDING)
    mSvcStatus.dwControlsAccepted = 0;
  else mSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

  if ( (dwCurrentState == SERVICE_RUNNING) ||
       (dwCurrentState == SERVICE_STOPPED) )
    mSvcStatus.dwCheckPoint = 0;
  else mSvcStatus.dwCheckPoint = dwCheckPoint++;

  // Report the status of the service to the SCM.
  SetServiceStatus(mSvcStatusHandle, &mSvcStatus );
}


//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
void GitWatchSvc::reportEvent(const char* function, DWORD errorCode) 
{ 
  HANDLE hEventSource;
  LPCTSTR lpszStrings[2];
  TCHAR buff[1024];

  hEventSource = RegisterEventSource(NULL, mSvcName);

  if (hEventSource != NULL)
  {
    std::stringstream strBuf;
    strBuf << function << " failed with: " << getLastErrorAsString(errorCode);
    std::string strError = strBuf.str();
    StringCchPrintf(buff, sizeof(buff), "%s", strError.c_str());

    lpszStrings[0] = mSvcName;
    lpszStrings[1] = buff;

    ReportEvent(hEventSource,        // event log handle
                EVENTLOG_ERROR_TYPE, // event type
                0,                   // event category
                11,                  // event identifier
                NULL,                // no security identifier
                2,                   // size of lpszStrings array
                0,                   // no binary data
                lpszStrings,         // array of strings
                NULL);               // no binary data

    DeregisterEventSource(hEventSource);
  }
}
