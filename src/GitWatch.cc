// GitWatch.cpp : Defines the entry point for the application.
//
// header.h : include file for standard system include files,
// or project specific include files
//
#include "GitWatch.h"
#include "GitWatchUtil.h"
#include "DirDiff.h"

GitWatch gitWatch;

std::string GitWatch::mAppName = "GitWatch";

Logger *gLogger = NULL;

WatchCmd::WatchCmd()
{
  clear();
}

void WatchCmd::clear()
{
  cmd = "";
  errorCmd = "";
  successValue = 0;
  checkSuccess = false;
  errorValue = (DWORD) -1;
  checkError = false;
}


int GitWatch::init(HANDLE hSvcStopEventTmp)
{
  std::string tempPath;
  std::stringstream strBuf;
  char name[8192];

  GetModuleFileName(NULL, name, sizeof(name));
  
  tempPath = name;
  size_t slashPos = tempPath.rfind("\\");
  tempPath = tempPath.substr(0, slashPos);
  
  mConfigDir = tempPath;
  mConfigPath = tempPath;
  mConfigPath.append("\\GitWatch.ini");
  mLogPath = tempPath;
  mLogPath.append("\\GitWatch.log");
  
  mLogger.open(mLogPath);

  strBuf << GetCurrentThreadId();
  std::string strThreadId = strBuf.str();
  gLogger = &mLogger;
  mLogger.log("APP START; THREAD ID started", strThreadId);

  mhSvcStopEvent = hSvcStopEventTmp;
  mhRefreshMutex = CreateSemaphore(NULL, 0, 1, NULL);
  mhWatchAlertMutex = CreateMutex(NULL, FALSE, NULL);
  mhDirListMutex = CreateMutex(NULL, FALSE, NULL);

  mpDirListOld = NULL;

  gitWatch.msgWait();

  return 0;
}


int GitWatch::reloadScript()
{
  std::ifstream fs;
  std::string text;
  int error = 0;
  
  //logfile.open("GitWatch.log", std::fstream::out);
  //if (logfile.is_open()==false) return ERROR_LOG_FAILED;

  WaitForSingleObject(mhWatchAlertMutex, INFINITE);

  fs.open(mConfigPath, std::fstream::in);
  if (fs.is_open()==false) 
  {
    mLogger.log("ERROR_SCRIPT_FAILED_OPEN", mScriptPath);
    error = ERROR_SCRIPT_FAILED_OPEN;
  }
  else
  {
    std::vector<WatchCmd> script;
    std::string path;
    WatchCmd watchCmd;
    bool empty = true;

    mWatchPaths.clear();
    mScripts.clear();

    mLogger.log("LOGGING GitWatch.ini READ...");
    while (std::getline(fs, text))
    {
      //mLogger.log("PRETEXT", text);
      chomp(text);
      //mLogger.log("TEXT", text);      
      if (text.length() > 0)
      {
        std::string section, var, value;

        bool success = scanIniLine(text, section, var, value, true);
        //mLogger.log("SECTION", section);
        //mLogger.log("VAR", var);
        //mLogger.log("VALUE", value);
        if (success && section.length() > 0)
        {
          if (empty == false)
          {
            if (watchCmd.cmd.length() > 0)
            {
              script.push_back(watchCmd);
              watchCmd.clear();
            }
            if (script.size()==0)
            {
              mLogger.log("ERROR_SCRIPT_EMPTY", mScriptPath);
              error = ERROR_SCRIPT_EMPTY;
            }
            else
            {
              mWatchPaths.push_back(path);
              mScripts.push_back(script);
              script.clear();
              path = "";
              empty = true;
            }
          }
        }
        else if (success)
        {
          if (var == "Path")
          {
            mLogger.log("PATH", value);
            if (watchCmd.cmd.length() > 0)
            {
              script.push_back(watchCmd);
              watchCmd.clear();
            }
            if (path.length() > 0)
            {
              mWatchPaths.push_back(path);
              mScripts.push_back(script);
              script.clear();
            }
            path = value;
            empty = false;
          }
          else if (var.find("Cmd") == 0 && var.length() > 3 && isdigit(var.at(3)))
          {
            mLogger.log("CMD", value);
            if (watchCmd.cmd.length() > 0)
            {
              script.push_back(watchCmd);
              watchCmd.clear();
            }
            watchCmd.cmd = value;
            empty = false;
          }
          else if (var.find("CmdSuccess") == 0)
          {
            mLogger.log("CMDSUCCESS", value);
            std::stringstream stream;
            if (watchCmd.checkSuccess)
            {
              script.push_back(watchCmd);
              watchCmd.clear();
            }
            watchCmd.checkSuccess = true;
            stream << value;
            stream >> watchCmd.successValue;
            empty = false;
          }
          else if (var.find("CmdFailure") == 0)
          {
            mLogger.log("CMDFAILURE", value);
            std::stringstream stream;
            if (watchCmd.checkError)
            {
              script.push_back(watchCmd);
              watchCmd.clear();
            }
            watchCmd.checkError = true;
            stream << value;
            stream >> watchCmd.errorValue;
            empty = false;
          }
          else if (var.find("Error") == 0)
          {
            mLogger.log("ERROR", value);
            if (watchCmd.errorCmd.length() > 0)
            {
              script.push_back(watchCmd);
              watchCmd.clear();
            }
            watchCmd.errorCmd = value;
            empty = false;
          }
        }
      }
    }
    if (empty == false)
    {
      if (watchCmd.cmd.length() > 0)
      {
        script.push_back(watchCmd);
        watchCmd.clear();
      }
      if (script.size()==0)
      {
        mLogger.log("ERROR_SCRIPT_EMPTY", mScriptPath);
        error = ERROR_SCRIPT_EMPTY;
      }
      else
      {
        mWatchPaths.push_back(path);
        mScripts.push_back(script);
        script.clear();
        empty = true;
      }
    }
  }
  ReleaseMutex(mhWatchAlertMutex);
  return error;
}


WatchAlert* GitWatch::createWatchAlert(size_t id, bool withScript)
{
  WatchAlert * alert = NULL;
  WaitForSingleObject(mhWatchAlertMutex, INFINITE);
  if (id < mNotifyIDs.size())
  {
    size_t offset = mNotifyIDs[id];
    if (offset < mWatchPaths.size())
    {
      alert = new WatchAlert;
      alert->id = id;
      alert->offset = offset;
      alert->watchPath = mWatchPaths[offset];
      alert->hNotify = mNotifyHandles[id];
      alert->hTimerQueue = mhTimerQueue;
      alert->hTimer = mTimerHandles[offset];
      if (withScript)
      {
        alert->script = mScripts[offset];
      }
    }
  }
  ReleaseMutex(mhWatchAlertMutex);
  return alert;
}


void GitWatch::msgWait()
{
  mKeepGoing = true;

  reloadScript();
  refreshNotifyHandles();
  refreshTimers();

  mLogger.log("START_WATCH_THREAD");
  do
  {
    DWORD waitResultID = WaitForMultipleObjects((DWORD) mNotifyHandles.size(), mNotifyHandles.data(), FALSE, (DWORD) 1000 * 60 * 120); // wait max 120 minutes then retry
    if (waitResultID == WAIT_TIMEOUT)
    {
      closeNotifyHandles();
      refreshNotifyHandles();
      refreshTimers();
    }
    else if (waitResultID >= WAIT_OBJECT_0 && waitResultID < WAIT_OBJECT_0 + mNotifyHandles.size())
    {
      mLogger.log("WAIT_OBJECT RECEIVED");
      size_t id = waitResultID - WAIT_OBJECT_0;
      DWORD threadID = 0;
      // Received stop service message
      if (id == 0) 
      {
        mKeepGoing = false;
        break;
      }
      // Received refresh message
      else if (id == 1)
      {
        if (mKeepGoing == false) break;
        closeNotifyHandles();
        reloadScript();
        refreshNotifyHandles();
        refreshTimers();
      }
      else
      {
        WatchAlert * alert = createWatchAlert(id, false);
        if (alert) 
        {
          CreateThread(NULL, 0, WaitNotification, (LPVOID) alert, 0, &threadID);
          if (FindNextChangeNotification(alert->hNotify)==FALSE)
          {
            //FindCloseChangeNotification(hNotify);
            mLogger.log("ERROR_FIND_NEXT", alert->watchPath);
          }
        }
      } 
    }
    else if (waitResultID >= WAIT_ABANDONED && waitResultID < WAIT_ABANDONED + mNotifyHandles.size())
    {
      mLogger.log("ERROR_WAIT_ABANDONED");
      
      closeNotifyHandles();
      refreshNotifyHandles();
      refreshTimers();
    }
    else if (waitResultID == WAIT_FAILED)
    {
      mLogger.log("ERROR_WAIT_FAILED");
      closeNotifyHandles();
      refreshNotifyHandles();
      refreshTimers();
    }
  }
  while (mKeepGoing);

  mLogger.log("EXIT_WATCH_THREAD");
  closeNotifyHandles();
  killTimerQueue();
}


void GitWatch::refreshNotifyHandles()
{
#ifdef _MSC_VER
  struct __stat64 buf;
#else
  struct _stat64 buf;
#endif
  DWORD notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;

  WaitForSingleObject(mhWatchAlertMutex, INFINITE);
  
  mNotifyHandles.clear();
  mNotifyIDs.clear();

  mNotifyHandles.push_back(mhSvcStopEvent);
  mNotifyIDs.push_back(0);
  mNotifyHandles.push_back(mhRefreshMutex);
  mNotifyIDs.push_back(1);
  for (size_t id = 0; id < mWatchPaths.size(); id++)
  {
    std::string watchPath = mWatchPaths[id];
    int statResult = _stat64(watchPath.c_str(), &buf);
    if (statResult != 0) 
    {
      mLogger.log("ERROR_STAT", watchPath);
    }
    else 
    {
      HANDLE hNotify = FindFirstChangeNotificationA(watchPath.c_str(), TRUE, notifyFilter);
      if (hNotify == INVALID_HANDLE_VALUE) 
      {
        mLogger.log("ERROR_NOTIFY_HANDLE", watchPath);
      }
      else
      {
        std::stringstream strBuf;
        strBuf << "SET_WATCH: '" << watchPath << "': " << (size_t) hNotify;
        //mLogger.log(SET_WATCH, watchPath);
        mLogger.log(strBuf.str());
        mNotifyHandles.push_back(hNotify);
        mNotifyIDs.push_back(id);
      }
    }
  }
  ReleaseMutex(mhWatchAlertMutex);
}


void GitWatch::closeNotifyHandles()
{
  WaitForSingleObject(mhWatchAlertMutex, INFINITE);
  for (size_t i = 0; i < mNotifyHandles.size(); i++)
  {
    if (i < 2) continue;
    FindCloseChangeNotification(mNotifyHandles[i]);
    mNotifyHandles[i] = NULL;
  }
  ReleaseMutex(mhWatchAlertMutex);
}


void GitWatch::killTimerQueue()
{
  WaitForSingleObject(mhWatchAlertMutex, INFINITE);
  DeleteTimerQueueEx(mhTimerQueue, NULL);
  mhTimerQueue = NULL;
  ReleaseMutex(mhWatchAlertMutex);
}


void GitWatch::refreshTimers()
{
  WaitForSingleObject(mhWatchAlertMutex, INFINITE);
  if (mhTimerQueue)
  {
    DeleteTimerQueueEx(mhTimerQueue, NULL);
    mhTimerQueue = NULL;
  }
  mTimerHandles.clear();
  mhTimerQueue = CreateTimerQueue();
  
  for (size_t id = 2; id < mNotifyHandles.size(); id++)
  {
    HANDLE hNewTimer = NULL;
    CreateTimerQueueTimer(&hNewTimer, mhTimerQueue, (WAITORTIMERCALLBACK) MainTimerProc, (PVOID) id, 60 * 1000 * 15, 0, 0);
    mTimerHandles.push_back(hNewTimer);
  }
  ReleaseMutex(mhWatchAlertMutex);
}


DWORD WINAPI WaitNotification(LPVOID options)
{
  gitWatch.waitNotification((WatchAlert*) options);
  return 0;
}


void GitWatch::waitNotification(WatchAlert* alert)
{
  std::string watchPath = alert->watchPath;
  mLogger.log("CHANGE DETECTED; TIMER SET", watchPath);
  
  WaitForSingleObject(mhWatchAlertMutex, INFINITE);

  // Reset timer for fifteen minutes
  if (alert->hTimer)
  {
    DeleteTimerQueueTimer(alert->hTimerQueue, alert->hTimer, NULL);
  }
  mTimerHandles[alert->offset] = NULL;
  // Set Timer for 60 seconds
  HANDLE hNewTimer = NULL;
  BOOL timerCreated = CreateTimerQueueTimer(&hNewTimer, alert->hTimerQueue, (WAITORTIMERCALLBACK) MainTimerProc, (PVOID) alert->id , 1000, 0, 0);
  //FIXME TIMER
  //BOOL timerCreated = CreateTimerQueueTimer(&hNewTimer, alert->hTimerQueue, (WAITORTIMERCALLBACK) MainTimerProc, (PVOID) alert->id , 60 * 1000, 0, 0);
  mTimerHandles[alert->offset] = hNewTimer;

  ReleaseMutex(mhWatchAlertMutex);

  if (timerCreated == false)
  {
    mLogger.log("CREATE TIMER QUEUE TIMER FAILED\n");
  }
  delete alert;
}


VOID WINAPI MainTimerProc(PVOID options, BOOLEAN timerOrWaitFired)
{
  UNUSED(timerOrWaitFired);

  gitWatch.timerProc((size_t) options);
}


DWORD GitWatch::logExecProcess(std::string cmd, std::string description, DWORD *pExitCode)
{
  std::stringstream strBuf;
  std::string output;
  DWORD retValue = 0;
  DWORD exitCode = 0;

  strBuf << description << " EXECUTING: '" << cmd << "'"; 
  mLogger.log(strBuf.str());
  strBuf.str("");
  strBuf.clear();

  retValue = exec_process(cmd, &exitCode, &output);
  if (pExitCode)
  {
    *pExitCode = exitCode;
  }
  strBuf << description << " RETURNED: '" << cmd << "' EXIT CODE: " << exitCode << " RETURN VALUE: " << retValue;
  mLogger.log(strBuf.str());
  strBuf.str("");
  strBuf.clear();

  strBuf << description << " STDOUT/STDERR: ";
  mLogger.log(strBuf.str());
  if (output.length() > 0)
  {
    mLogger.log(output);
  }
  else
  {
    mLogger.log("<NO OUTPUT>");
  }
  return retValue;
}


void GitWatch::timerProc(size_t id)
{
  std::stringstream strBuf;

  strBuf << GetCurrentThreadId();
  std::string strThreadId = strBuf.str();
  mLogger.log("TIMER HIT; THREAD ID started", strThreadId);

  WatchAlert * alert = createWatchAlert(id, true);
  if (alert == NULL) 
  {
    mLogger.log("INVALID ID PASSED TO TIMERPROC");
    return;
  }
  std::string watchPath = alert->watchPath;

  WaitForSingleObject(mhWatchAlertMutex, INFINITE);

  // Reset timer for fifteen minutes
  DeleteTimerQueueTimer(alert->hTimerQueue, alert->hTimer, NULL);

  mTimerHandles[alert->offset] = NULL;
  HANDLE hNewTimer = NULL;

  BOOL timerCreated = CreateTimerQueueTimer(&hNewTimer, alert->hTimerQueue, (WAITORTIMERCALLBACK) MainTimerProc, (PVOID) id, 60 * 1000 * 5, 0, 0);
  mTimerHandles[alert->offset] = hNewTimer;

  ReleaseMutex(mhWatchAlertMutex);

  if (timerCreated == false)
  {
    mLogger.log("CREATE TIMER QUEUE TIMER FAILED\n");
  }
  bool runCmds = false;
  mLogger.log("SETTING CURRENT DIR...", watchPath);
  if (SetCurrentDirectory(watchPath.c_str()))
  {
    std::size_t notInOld = 0;
    std::size_t notInNew = 0;
    bool dirDiffs = false;
    mLogger.log("SET CURRENT DIR, CHECKING FOR DIFFERENCES...", watchPath);
    DirEntry *pDirListNew = NULL;
    bool walkSuccess = DirWalk(watchPath.c_str(), &pDirListNew);

    WaitForSingleObject(mhDirListMutex, INFINITE);

    if (walkSuccess && mpDirListOld && pDirListNew)
    {
      NotFound *pNotFoundOld = DirDiff(pDirListNew, mpDirListOld, false, &notInOld);
      NotFound *pNotFoundNew = DirDiff(mpDirListOld, pDirListNew, true, &notInNew);
      FreeNotFoundList(pNotFoundOld);
      FreeNotFoundList(pNotFoundNew);
    }
    else if (walkSuccess)
    {
      runCmds = true;
    }
    if (walkSuccess == false || pDirListNew == NULL)
    {
      mLogger.log("FAILED TO EXAMINE/WALK THROUGH DIRECTORY", watchPath);
    }

    if (mpDirListOld)
    {
      FreeDirEntryList(mpDirListOld);
    }
    mpDirListOld = pDirListNew;

    ReleaseMutex(mhDirListMutex);

    if (notInOld > 0 || notInNew > 0)
    {
      runCmds = true;
      dirDiffs = true;
    }
    if (runCmds)
    {
      if (dirDiffs)
      {
        mLogger.log("--YES-- DIRECTORY DIFFERENCES DETECTED!", watchPath);
      }
      else
      {
        mLogger.log("INITIAL STARTUP; RUNNING CMDS!", watchPath);
      }
    }
    else if (walkSuccess)
    {
      mLogger.log("DIRECTORY DIFFERENCES --NOT-- DETECTED!", watchPath);
    }
  }
  else
  {
    mLogger.log("FAILED SET CURRENT DIR", watchPath);
  }

  if (runCmds)
  {
    std::string output;
    DWORD exitCode = 0;
    DWORD retValue = 0;
    bool error = false;

    for (size_t i = 0; i < alert->script.size() && error == false; i++)
    {
      WatchCmd *pWatchCmd = &alert->script[i];
      exitCode = 0;
      retValue = logExecProcess(pWatchCmd->cmd, "PROCESS EXEC", &exitCode);
      if (pWatchCmd->checkSuccess && (pWatchCmd->successValue != exitCode || retValue != 0))
      {
        mLogger.log("CHECK SUCCESS: ERROR HIT!");
        logExecProcess(pWatchCmd->errorCmd, "ERROR CMD", NULL);
        error = true;
      }
      if (pWatchCmd->checkError && (pWatchCmd->errorValue == exitCode || retValue != 0))
      {
        mLogger.log("CHECK ERROR: ERROR HIT!");
        logExecProcess(pWatchCmd->errorCmd, "ERROR CMD", NULL);
        error = true;
      }
    }
  }
  delete alert;
}


void GitWatch::refresh()
{
  ReleaseSemaphore(mhRefreshMutex, 1, NULL);
}


void GitWatch::quit()
{
  mKeepGoing = false;
  ReleaseSemaphore(mhRefreshMutex, 1, NULL);
}
 

void readPipeOutput(HANDLE hPipe, std::string *pOutText)
{
  char *pBuff = NULL;
  DWORD bytesRead = 0;
  BOOL readSuccess = FALSE;

  pBuff = new char[8192];
  if (pBuff) 
  {
    do
    {
      bytesRead = 0;
      ZeroMemory(pBuff, 8192);
      readSuccess = ReadFile(hPipe, pBuff, 8190, &bytesRead, NULL);
      if (bytesRead == 0)
      {
        readSuccess = FALSE;
      }
      else
      {
        pOutText->append(pBuff);
      }
    } while (readSuccess);
    delete[] pBuff;
  }
}


DWORD exec_process(std::string cmdline, DWORD *pExitCode, std::string *pOutText)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  LPSTR cmdlinePtr = (LPSTR) cmdline.c_str();
  HANDLE hChildStdOutRd = NULL;
  HANDLE hChildStdOutWr = NULL;
  HANDLE hChildStdErrRd = NULL;
  HANDLE hChildStdErrWr = NULL;
  HANDLE hChildStdInRd = NULL;
  HANDLE hChildStdInWr = NULL;
  SECURITY_ATTRIBUTES saAttr;
  BOOL setupRedirect = FALSE;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  *pExitCode = 0;

  if (pOutText)
  {
    ZeroMemory(&saAttr, sizeof(saAttr));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT. 
    CreatePipe(&hChildStdOutRd, &hChildStdOutWr, &saAttr, 0);

    // Create a pipe for the child process's STDERR. 
    CreatePipe(&hChildStdErrRd, &hChildStdErrWr, &saAttr, 0);

    // Create a pipe for the child process's STDIN.
    CreatePipe(&hChildStdInRd, &hChildStdInWr, &saAttr, 0);

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (hChildStdOutRd)
    {
      setupRedirect = SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0);
    }

    // Ensure the read handle to the pipe for STDERR is not inherited.
    if (setupRedirect && hChildStdErrRd)
    {
      setupRedirect = SetHandleInformation(hChildStdErrRd, HANDLE_FLAG_INHERIT, 0);
    }

    // Ensure the write handle to the pipe for STDIN is not inherited.
    if (setupRedirect && hChildStdInWr)
    {
      setupRedirect = SetHandleInformation(hChildStdInWr, HANDLE_FLAG_INHERIT, 0);
    }
  }

  if (setupRedirect)
  {
    si.hStdError = hChildStdErrWr;
    si.hStdOutput = hChildStdOutWr;
    si.hStdInput = hChildStdInRd;
    si.dwFlags |= STARTF_USESTDHANDLES;
  }
  // Start the child process. 
  BOOL ok = CreateProcessA(NULL,   // No module name (use command line)
                           cmdlinePtr,     // Command line
                           NULL,           // Process handle not inheritable
                           NULL,           // Thread handle not inheritable
                           TRUE,           // Set handle inheritance to TRUE
                           0,              // No creation flags
                           NULL,           // Use parent's environment block
                           NULL,           // Use parent's starting directory 
                           &si,            // Pointer to STARTUPINFO structure
                           &pi);           // Pointer to PROCESS_INFORMATION structure
  
  //printf("CreateProcess failed (%d).\n", GetLastError());
  DWORD retValue = 0;

  if (pOutText)
  {
    CloseHandle(hChildStdOutWr);
    CloseHandle(hChildStdErrWr);
    CloseHandle(hChildStdInRd);
  }
  // Wait until child process exits.
  if (ok)
  {
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, pExitCode);

    if (setupRedirect)
    {
      readPipeOutput(hChildStdOutRd, pOutText);
      readPipeOutput(hChildStdErrRd, pOutText);
    }

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  else
  {
    retValue = GetLastError();
  }
  if (pOutText)
  {
    CloseHandle(hChildStdOutRd);
    CloseHandle(hChildStdErrRd);
    CloseHandle(hChildStdInWr);
  }
  return retValue;
}


GitWatch::~GitWatch()
{
  if (mhDirListMutex && mpDirListOld)
  {
    WaitForSingleObject(mhDirListMutex, INFINITE);
    FreeDirEntryList(mpDirListOld);
    mpDirListOld = NULL;
    ReleaseMutex(mhDirListMutex);
    mhDirListMutex = NULL;
  }
}
