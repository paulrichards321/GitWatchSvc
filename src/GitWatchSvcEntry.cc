// GitWatch.cpp : Defines the entry point for the application.
//
// header.h : include file for standard system include files,
// or project specific include files
//

#include <string>
#include <iostream>
#include <sstream>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <fileapi.h>
#include "GitWatch.h"
#include "GitWatchUtil.h"
#include "GitWatchSvc.h"
#include "GitWatchSvcHelper.h"

extern GitWatchSvc gitWatchSvc;
extern GitWatchSvcHelper gitWatchSvcHelper;

void getAccountDetails(std::string *pUsername, std::string *pPassword);

//
// Purpose: 
//   Entry point for the process
//
// Parameters:
//   None
// 
// Return value:
//   None, defaults to 0 (zero)
//
int __cdecl main(int argc, char *argv[]) 
{ 
  int status = 0;
  std::string params;
  // If command-line parameter is "install", install the service. 
  // Otherwise, the service is probably being started by the SCM.

  for (int c = 1; c < argc; c++)
  {
    params.append(argv[c]);
    params.append(" ");
  }

  if (argc > 1)
  {
    std::string msg, input;
    if (lstrcmpi(argv[1], "install") == 0)
    {
      std::string username, password;
      if (gitWatchSvcHelper.elevatePrivileges(params)==FALSE)
      {
        return 0;
      }
      getAccountDetails(&username, &password);
      status = gitWatchSvcHelper.install(username, password, msg);
      std::cout << msg << std::endl << "Press return to continue..." << std::endl;
      getline(std::cin, input);
      return status > 0 ? 0 : 1;
    }
    else if (lstrcmpi(argv[1], "delete") == 0 || lstrcmpi(argv[1], "remove") == 0)
    {
      if (gitWatchSvcHelper.elevatePrivileges(params)==FALSE)
      {
        return 0;
      }
      status = gitWatchSvcHelper.deleteSvc(msg);
      std::cout << msg << std::endl << "Press return to continue..." << std::endl;
      getline(std::cin, input);
      return status > 0 ? 0 : 1;
    }
    else if (lstrcmpi(argv[1], "start") == 0)
    {
      if (gitWatchSvcHelper.elevatePrivileges(params)==FALSE)
      {
        return 0;
      }
      status = gitWatchSvcHelper.start(msg);
      std::cout << msg << std::endl << "Press return to continue..." << std::endl;
      getline(std::cin, input);
      return status > 0 ? 0 : 1;
    }
    else if (lstrcmpi(argv[1], "stop") == 0)
    {
      if (gitWatchSvcHelper.elevatePrivileges(params)==FALSE)
      {
        return 0;
      }
      status = gitWatchSvcHelper.stop(msg);
      std::cout << msg << std::endl << "Press return to continue..." << std::endl;
      getline(std::cin, input);
      return status > 0 ? 0 : 1;
    }
    else if (lstrcmpi(argv[1], "interrogate") == 0 || lstrcmpi(argv[1], "query") == 0)
    {
      status = (int) gitWatchSvcHelper.checkRunning(msg, NULL);
      std::cout << msg << std::endl;
      return status;
    }
    else
    {
      std::cout << "GitWatchSvc [argument]" << std::endl;
      std::cout << "Argument is one of: install, delete, remove, start, stop, query" << std::endl;
      std::cout << std::endl;
      std::cout << "Example: GitWatchSvc install" << std::endl;
      std::cout << "Note: install will prompt for account type, or optional account name." << std::endl;
      std::cout << "Note: delete or remove will remove the service from the Windows services list and remove from automatic startup (see services.msc)" << std::endl;
      std::cout << std::endl;
      return 0;
    }
  }  

  // TO_DO: Add any additional services for the process to this table.
  SERVICE_TABLE_ENTRY dispatchTable[] = 
    { 
      { gitWatchSvc.getSvcName(), (LPSERVICE_MAIN_FUNCTION) GitWatchSvc::main }, 
      { NULL, NULL } 
    }; 
 
  // This call returns when the service has stopped. 
  // The process should simply terminate when the call returns.
  if (StartServiceCtrlDispatcher(dispatchTable)==0) 
  { 
    DWORD errorCode = GetLastError();
    status = errorCode;
    gitWatchSvc.reportEvent("StartServiceCtrlDispatcher", errorCode); 
    switch (errorCode)
    {
      case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
        std::cout << "ERROR_FAILED_SERVICE_CONTROLLER_CONNECT" << std::endl;
        break;
      case ERROR_INVALID_DATA:
        std::cout << "ERROR_INVALID_DATA" << std::endl;
        break;
      case ERROR_SERVICE_ALREADY_RUNNING:
        std::cout << "ERROR_SERVICE_ALREADY_RUNNING" << std::endl;
        break;
      default:
      {
        std::string error = getLastErrorAsString(errorCode);
        std::cout << "ERROR RETURNED: " << error << std::endl;
        break;
      }
    }
  } 
  return status;
} 


void getAccountDetails(std::string *pUsername, std::string *pPassword)
{
  std::string username, rawUsername;
  std::string password, passwordConfirm;
  std::string prepend;
  std::string input;
  int accountType = 1;
  bool needUsername = false;
  bool goodInput = false;
  do
  {
    // For some reason Windows 10 expects regular local user accounts to be prepended with ".\", for example: ".\johndoe" 
    std::cout << "What type of account do you want to use with the GitWatch service?" << std::endl;
    std::cout << std::endl;
    std::cout << "\t[1]: Normal service account (will not have access to any share " << std::endl;
    std::cout << "\t                             drives or possibly some user folders " << std::endl;
    std::cout << "\t                             or files that have restrictive permissions)" << std::endl;
    std::cout << "\t[2]: Normal local user account (example: johndoe)" << std::endl;
    std::cout << "\t[3]: Domain account or system account (example: sarahdoe@somecorp.com)" << std::endl;
    std::cout << std::endl;
    std::cout << "Please enter 1, 2, or 3: ";
    input.clear();
    getline(std::cin, input);
    if (input.length() > 0 && input.find('1') != std::string::npos)
    {
      accountType = 1;
      goodInput = true;
    }
    else if (input.length() > 0 && input.find('2') != std::string::npos)
    {
      accountType = 2;
      goodInput = true;
      needUsername = true;
    }
    else if (input.length() > 0 && input.find('3') != std::string::npos)
    {
      accountType = 3;
      goodInput = true;
      needUsername = true;
    }
    else
    {
      std::cout << std::endl;
      std::cout << "Please enter 1, 2, or 3, then hit the enter key, or hit ctrl-c to cancel installation." << std::endl << std::endl;
    }
    std::cout << std::endl;
  } while (goodInput == false);
      
  if (needUsername == true)
  {
    std::cout << "Normal PC username accounts have names like johndoe or for domain accounts like sarahdoe@somecorp.com." << std::endl;
    std::cout << "What is your username? ";
    getline(std::cin, rawUsername);
    username = rawUsername;
    if (accountType == 2)
    {
      // Make sure they didn't already prepend with ".\"
      if (username.length() < 2 || (username.length() > 1 && (username.at(0) != '.' && username.at(1) != '\\')))
      {
        prepend = ".\\";
        prepend.append(username);
        username = prepend;
      }
    }
    std::cout << std::endl;
  }
  if (username.length() > 0)
  {
    int tries = 0;
    do
    {
      passwordConfirm.clear();
      password.clear();
      if (tries > 0)
      {
        std::cout << "Passwords do not match." << std::endl << std::endl;
      }
      
      std::cout << "Password for account '" << rawUsername << "': ";
      HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
      if (hStdin == 0)
      {
        return;
      }
      DWORD mode = 0;
      GetConsoleMode(hStdin, &mode);
      SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
        
      getline(std::cin, password);
      SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT);
      std::cout << std::endl;
      for (size_t i = 0; i < password.length(); i++)
      {
        if (password.at(i) == 0x0D || password.at(i) == 0x0A)
        {
          password = password.substr(0, i);
        }
      }

      std::cout << "Confirm password for account '" << rawUsername << "': ";
      mode = 0;
      GetConsoleMode(hStdin, &mode);
      SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
        
      getline(std::cin, passwordConfirm);
      SetConsoleMode(hStdin, mode | ENABLE_ECHO_INPUT);
      std::cout << std::endl << std::endl;
      for (size_t i = 0; i < passwordConfirm.length(); i++)
      {
        if (passwordConfirm.at(i) == 0x0D || passwordConfirm.at(i) == 0x0A)
        {
          passwordConfirm = passwordConfirm.substr(0, i);
        }
      }
      tries++;
    }
    while (password.compare(passwordConfirm) != 0);
    std::cout << "Password matches." << std::endl << std::endl;
  }
  *pUsername = username;
  *pPassword = password;
}


//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI GitWatchSvc::main(DWORD dwArgc, LPSTR *lpszArgv)
{
  DWORD errorCode = 0;
  // Register the handler function for the service
  mSvcStatusHandle = RegisterServiceCtrlHandler(getSvcName(), ctrlHandler);

  if (!mSvcStatusHandle)
  { 
    errorCode = GetLastError();
    reportEvent("RegisterServiceCtrlHandler", errorCode); 
    return; 
  } 

  // These SERVICE_STATUS members remain as set here
  mSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
  mSvcStatus.dwServiceSpecificExitCode = 0;    

  // Report initial status to the SCM
  reportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

  // Perform service-specific initialization and work.
  init(dwArgc, lpszArgv);
}

//
// Purpose: 
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None
//
void GitWatchSvc::init(DWORD dwArgc, LPTSTR *lpszArgv)
{
  UNUSED(dwArgc);
  UNUSED(lpszArgv);
  // TO_DO: Declare and set any required variables.
  //   Be sure to periodically call reportSvcStatus() with 
  //   SERVICE_START_PENDING. If initialization fails, call
  //   reportSvcStatus with SERVICE_STOPPED.

  // Create an event. The control handler function, ctrlHandler,
  // signals this event when it receives the stop control code.
  mhSvcStopEvent = CreateEvent(
    NULL,    // default security attributes
    TRUE,    // manual reset event
    FALSE,   // not signaled
    NULL);   // no name

  if (mhSvcStopEvent == NULL)
  {
    reportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
    return;
  }

  // Report running status when initialization is complete.
  reportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

  // TO_DO: Perform work until service stops.

  gitWatch.init(mhSvcStopEvent);

  // Check whether to stop the service.
  reportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}
