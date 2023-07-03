#ifndef __GITWATCH_H
#define __GITWATCH_H

#include <windows.h>
#include <wincred.h>
#include <tchar.h>
#include <strsafe.h>
//#include "internal.h"
// C RunTime Header Files
//#include <cmalloc>
//#include <cmemory>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fileapi.h>
#include "DirDiff.h"
#include "logger.h"
#include "GitWatchError.h"

#ifndef _MSC_VER
#include <sys/stat.h>
#endif


DWORD WINAPI notifyThread(LPVOID options);
DWORD WINAPI WaitNotification(LPVOID options);
DWORD WINAPI TimerNotification(LPVOID options);
VOID WINAPI MainTimerProc(PVOID options, BOOLEAN timerOrWaitFired);

void captureOutput(HANDLE hPipe, std::string *pOutText);
DWORD exec_process(std::string cmdline, DWORD *pExitCode, std::string *pOutText);
void chomp(std::string &text);

class WatchCmd {
public:
  std::string cmd;
  std::string errorCmd;
  DWORD successValue;
  bool checkSuccess;
  DWORD errorValue;
  bool checkError;
public:
  WatchCmd();
  void clear();
};


struct WatchAlert {
  size_t id;
  size_t offset;
  std::string watchPath;
  HANDLE hNotify;
  HANDLE hTimerQueue;
  HANDLE hTimer;
  std::vector<WatchCmd> script;
};


class GitWatch {
protected:
  static std::string mAppName;
  //static std::string dlgClassName;
  std::string mConfigPath;
  std::string mConfigDir;
  std::string mScriptPath;
  std::string mLogPath;
  Logger mLogger;
  bool mKeepGoing;
  HANDLE mhSvcStopEvent, mhRefreshMutex, mhWatchAlertMutex;
  HANDLE mhDirListMutex;
  std::vector<std::string> mWatchPaths;
  std::vector<std::vector<WatchCmd>> mScripts;
  std::vector<HANDLE> mNotifyHandles;
  std::vector<size_t> mNotifyIDs;
  HANDLE mhTimerQueue;
  std::vector<HANDLE> mTimerHandles;
  DirEntry *mpDirListOld;
public:
  ~GitWatch();
  int init(HANDLE hSvcStopEvent);
  int reloadWatchPaths();
  int reloadScript();
  void msgWait();
  void closeNotifyHandles();
  void refreshNotifyHandles();
  void refreshTimers();
  void killTimerQueue();
  void waitNotification(WatchAlert*);
  void timerProc(size_t id);
  void refresh();
  WatchAlert * createWatchAlert(size_t id, bool copyScript);
  void quit();
  void log(UINT message) { std::stringstream strBuf; strBuf << message; mLogger.log("MESSAGE", strBuf.str()); }
  DWORD logExecProcess(std::string cmd, std::string description, DWORD *pExitCode);
};



extern GitWatch gitWatch;

#endif
