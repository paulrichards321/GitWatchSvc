// GitWatch.cpp : Defines the entry point for the application.
//
// header.h : include file for standard system include files,
// or project specific include files
//
// Windows Header Files
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <strsafe.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <fileapi.h>
#include "logger.h"
#include "resource.h"
#include "GitWatchError.h"
#include "GitWatchUtil.h" 
#include "GitWatchSvc.h"
#include "GitWatchSvcHelper.h"

#ifndef _MSC_VER
#include <sys/stat.h>
#endif

#define C_PAGES 3

INT_PTR APIENTRY MainDlgProc(HWND hMainDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY ChildDlgProc(HWND hChildDlg, UINT message, WPARAM wParam, LPARAM lParam) ;
DWORD exec_process(std::string cmdline, DWORD *pExitCode);

#ifndef DLGTEMPLATEEX

typedef struct {
  WORD      dlgVer;
  WORD      signature;
  DWORD     helpID;
  DWORD     exStyle;
  DWORD     style;
  WORD      cDlgItems;
  short     x;
  short     y;
  short     cx;
  short     cy;
} DLGTEMPLATEEX;

#endif

std::string getDlgItemString(HWND hDlg, int id)
{
  std::string text;
  char buf[4096];
  buf[0] = 0;
  GetDlgItemText(hDlg, id, buf, sizeof(buf));
  text = buf;
  return text;
}

class GitWatchGui
{
private:
  HINSTANCE mhInst;
  static std::string mAppName;
  HICON mhSmallIcon, mhRegularIcon;
  HWND mhMainDlg;
  HWND mhDisplay;
  HWND mhTab;       // tab control 
  RECT mRcDisplay;     // display rectangle for the tab control 
  DLGTEMPLATEEX *mApRes[C_PAGES]; 
  static char mTabTitles[3][32];
  NOTIFYICONDATA mNid;
  std::string mLogPath;
  std::string mConfigDir;
  std::string mConfigPath;
  std::vector<std::string> mWatchPaths;
  Logger mLogger;
  bool mFirstShown;
public:
  int init(HINSTANCE hInstance, int cmdShow, int argc, char* argv[]);
  int mainDlgInit(HWND hDlg);
  DLGTEMPLATEEX* doLockDlgRes(LPCTSTR lpszResName);
  void selChanged();
  void selChanging();
  int getCurrentTabID();
  void childDlgInit(HWND hDlg);
  INT_PTR showWindow(HWND hDlg, WPARAM wParam, LPARAM lParam);
  int reloadWatchPaths();
  int saveWatchPaths();
  bool saveSettings(int id);
  bool refresh();
  BOOL buttonPress(HWND hDlg, int tabId, int buttonId);
  void log(std::string msg) { mLogger.log(msg); }
  void quit() {     
    Shell_NotifyIcon(NIM_DELETE, &mNid);  
    PostQuitMessage(0);
  }
} gitWatch;

std::string GitWatchGui::mAppName = "GitWatch";
char GitWatchGui::mTabTitles[3][32] = { "Service", "Watch", "Git Commands" };


extern GitWatchSvc gitWatchSvc;
extern GitWatchSvcHelper gitWatchSvcHelper;



int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR     lpCmdLine,
                     _In_ int       nCmdShow)
{
  UNUSED(hPrevInstance);
  UNUSED(lpCmdLine);

  return gitWatch.init(hInstance, nCmdShow, __argc, __argv);
}


int GitWatchGui::init(HINSTANCE hInstance, int cmdShow, int argc, char* argv[])
{
  UNUSED(cmdShow);

  std::string tempPath;
  bool pathProvided = false;

  mhInst = hInstance;

  if (argc > 1)
  {
    tempPath = argv[1];
    size_t slashPos = tempPath.rfind("\\");
    if (slashPos != std::string::npos)
    {
      pathProvided = true;
    }
  }

  if (pathProvided == false)
  {
    char name[8192];
    GetModuleFileNameA(NULL, name, sizeof(name));
    tempPath = name;
    size_t slashPos = tempPath.rfind("\\");
    tempPath = tempPath.substr(0, slashPos);
  }
  else
  {
    size_t lastPos;
    int ate = 0;
    for (lastPos = tempPath.size(); lastPos > 0; lastPos--)
    {
      if (tempPath.at(lastPos) == '\\' || tempPath.at(lastPos) == '/')
      {
        ate++;
      }
      else
      {
        break;
      }
    }
    if (ate > 0)
    {
      tempPath = tempPath.substr(0, lastPos+1);
    }
  }
  mConfigDir = tempPath;
  mConfigPath = tempPath;
  mConfigPath.append("\\GitWatch.ini");
  mLogPath = tempPath;
  mLogPath.append("\\GitWatch.log");
  
  mLogger.open(mLogPath);
  mLogger.log("GUI_APP_START");

  INITCOMMONCONTROLSEX iccex;

  // Initialize common controls.
  iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  iccex.dwICC = ICC_TAB_CLASSES;
  InitCommonControlsEx(&iccex); 

  mhSmallIcon = LoadIcon(mhInst, MAKEINTRESOURCE(IDI_ICON1));
  mhRegularIcon = LoadIcon(mhInst, MAKEINTRESOURCE(IDI_ICON2));

  mhMainDlg = CreateDialog(mhInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC) MainDlgProc);
  ShowWindow(mhMainDlg, SW_SHOW);
  UpdateWindow(mhMainDlg);

  mNid.cbSize = sizeof(mNid);
  mNid.hWnd = mhMainDlg;
  mNid.hIcon = mhSmallIcon;
  mNid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
  mNid.uVersion = NOTIFYICON_VERSION_4;
  mNid.uCallbackMessage = WM_USER;

  StringCchCopy(mNid.szTip, ARRAYSIZE(mNid.szTip), mAppName.c_str());

// Show the notification.
  Shell_NotifyIcon(NIM_ADD, &mNid);  
  Shell_NotifyIcon(NIM_SETVERSION, &mNid);

  BOOL bRet;
  MSG msg;
  while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) 
  { 
    if (bRet == -1)
    {
      // Handle the error and possibly exit
    }
    else // if (!IsWindow(mhMainDlg) || !IsDialogMessage(mhMainDlg, &msg)) 
    { 
      TranslateMessage(&msg); 
      DispatchMessage(&msg); 
    } 
  } 
  return 0;
} 


int GitWatchGui::mainDlgInit(HWND hDlg)
{
  if (mFirstShown) return 0;

  DWORD dwDlgBase = GetDialogBaseUnits();
  int cxMargin = LOWORD(dwDlgBase) / 4; 
  int cyMargin = HIWORD(dwDlgBase) / 8; 

  TCITEM tie = {}; 
  int i; 

  // Create the tab control.
  mhMainDlg = hDlg;
  mhTab = GetDlgItem(hDlg, IDC_TAB1); 
  if (mhTab == NULL) 
  {
    MessageBox(0, "Failed to create or find tab control!", "Fatal Error", MB_OK);
    return 1;
  }
 
  // Add a tab for each of the three child dialog boxes. 
  tie.mask = TCIF_TEXT; 
  tie.iImage = -1; 

  tie.pszText = mTabTitles[0]; 
  TabCtrl_InsertItem(mhTab, 0, &tie); 
  tie.pszText = mTabTitles[1]; 
  TabCtrl_InsertItem(mhTab, 1, &tie); 
  tie.pszText = mTabTitles[2]; 
  TabCtrl_InsertItem(mhTab, 2, &tie); 

  // Lock the resources for the three child dialog boxes. 
  mApRes[0] = doLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG2)); 
  mApRes[1] = doLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG3)); 
  mApRes[2] = doLockDlgRes(MAKEINTRESOURCE(IDD_DIALOG4)); 

  // Determine a bounding rectangle that is large enough to 
  // contain the largest child dialog box. 
  RECT rcTab;
  SetRectEmpty(&rcTab); 
  for (i = 0; i < C_PAGES; i++) 
  { 
    if (mApRes[i]->cx > rcTab.right) 
      rcTab.right = mApRes[i]->cx; 
    if (mApRes[i]->cy > rcTab.bottom) 
      rcTab.bottom = mApRes[i]->cy; 
  }

  // Map the rectangle from dialog box units to pixels.
  MapDialogRect(mhMainDlg, &rcTab);
    
  // Calculate how large to make the tab control, so 
  // the display area can accommodate all the child dialog boxes. 
  TabCtrl_AdjustRect(mhTab, TRUE, &rcTab); 
  OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top); 
 
  // Calculate the display rectangle. 
  CopyRect(&mRcDisplay, &rcTab); 
  TabCtrl_AdjustRect(mhTab, FALSE, &mRcDisplay); 
 
  // Set the size and position of the tab control, buttons, 
  // and dialog box. 
  SetWindowPos(mhTab, NULL, rcTab.left, rcTab.top, 
               rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, 
               SWP_NOZORDER); 
 
  // Move the first button below the tab control. 
  HWND hwndButton = GetDlgItem(mhMainDlg, IDOK); 

  // Determine the size of the button. 
  RECT rcButton;
  GetWindowRect(hwndButton, &rcButton); 
  rcButton.right -= rcButton.left; 
  rcButton.bottom -= rcButton.top; 

  int leftPos = rcTab.left + ((rcTab.right - rcTab.left) / 2) - (rcButton.right / 2);
  int topPos = rcTab.bottom + cyMargin;
  SetWindowPos(hwndButton, NULL, leftPos, topPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER); 
 
  int rightPos = rcTab.right + cyMargin + (2 * GetSystemMetrics(SM_CXDLGFRAME));
  int bottomPos = rcTab.bottom + rcButton.bottom + (2 * cyMargin)
               + (2 * GetSystemMetrics(SM_CYDLGFRAME)) 
               + GetSystemMetrics(SM_CYCAPTION);
  // Size the dialog box. 
  SetWindowPos(mhMainDlg, NULL, 0, 0, rightPos, bottomPos, SWP_NOMOVE | SWP_NOZORDER); 
              
  // Simulate selection of the first item. 
  selChanged(); 

  return 0;
}


// Loads and locks a dialog box template resource. 
// Returns the address of the locked dialog box template resource. 
// lpszResName - name of the resource. 
//
DLGTEMPLATEEX* GitWatchGui::doLockDlgRes(LPCTSTR lpszResName) 
{ 
  HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG); 

  // Note that g_mhInst is the global instance handle
  HGLOBAL hglb = LoadResource(mhInst, hrsrc);  
  return (DLGTEMPLATEEX*) LockResource(hglb); 
} 

// Processes the TCN_SELCHANGE notification. 
// hwndDlg - handle to the parent dialog box. 
//
void GitWatchGui::selChanged() 
{ 
  // Get the index of the selected tab.
  int iSel = TabCtrl_GetCurSel(mhTab); 
 
  // Destroy the current child dialog box, if any. 
  if (mhDisplay != NULL) 
    DestroyWindow(mhDisplay); 
 
  // Create the new child dialog box. Note that g_hInst is the
  // global instance handle.
  mhDisplay = CreateDialogIndirect(mhInst, (DLGTEMPLATE*) mApRes[iSel], mhMainDlg, ChildDlgProc); 

  return;
} 


void GitWatchGui::selChanging() 
{ 
  // Get the index of the selected tab.
  int iSel = TabCtrl_GetCurSel(mhTab); 
 
  if (iSel == 0) // Service Selected
  {
    std::string userName, password;
    int checked = IsDlgButtonChecked(mhDisplay, IDC_CHECK1);
    if (checked == BST_CHECKED)
    {
      userName = getDlgItemString(mhDisplay, IDC_EDIT1);
      password = getDlgItemString(mhDisplay, IDC_EDIT2);
    }
  }
  else if (iSel == 1) // Watch Selected
  {
  }
  else if (iSel == 2) // Git Selected
  {
  }
  return;
} 


int GitWatchGui::getCurrentTabID()
{
  return (int) TabCtrl_GetCurSel(mhTab); 
}


INT_PTR APIENTRY MainDlgProc(HWND hMainDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
  switch (message) 
  { 
    case WM_INITDIALOG:
    {
      gitWatch.mainDlgInit(hMainDlg);
      return TRUE;
    }
    case WM_NOTIFY:
    {
      if (((LPNMHDR)lParam)->code == TCN_SELCHANGE)
      {
        gitWatch.selChanged();
        return TRUE;
      }
      else if (((LPNMHDR)lParam)->code == TCN_SELCHANGING)
      {
        gitWatch.selChanging();
        return TRUE;
      }
      break;
    }
    case WM_USER:
    {
      //int xPos = GET_X_LPARAM(lParam);
      //int yPos = GET_Y_LPARAM(lParam);
      if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU)
      {
        ShowWindow(hMainDlg, SW_NORMAL);
        return TRUE;
      }
      break;
    }
    case WM_COMMAND: 
    {
      return gitWatch.buttonPress(hMainDlg, -1, LOWORD(wParam));
    }
    case WM_SHOWWINDOW:
    {
      gitWatch.showWindow(hMainDlg, wParam, lParam);
      return FALSE;
    }
    case WM_CLOSE:
    {
      if (gitWatch.saveSettings(IDD_DIALOG1))
      {
        DestroyWindow(hMainDlg);
        gitWatch.refresh();
      }
      return TRUE;
    }
    case WM_DESTROY:
    {
      gitWatch.quit();
      return TRUE;
    }
  } 
  return FALSE; 
} 


INT_PTR GitWatchGui::showWindow(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
  UNUSED(lParam);

  if (mhDisplay && GetDlgCtrlID(mhDisplay) == 0)
  {
    if (wParam == TRUE) 
    { 
      std::string msg;
      gitWatchSvcHelper.checkRunning(msg, NULL);
      SetDlgItemText(hDlg, IDC_STATIC1, msg.c_str());
      MessageBox(0, msg.c_str(), "Message", MB_OK);
      std::string errMsg;
      gitWatchSvcHelper.closeSvcNotify();
      if (gitWatchSvcHelper.registerSvcNotify(GetDlgItem(hDlg, IDC_STATIC1), errMsg)==0)
      {
        MessageBox(0, errMsg.c_str(), "Service Watch", MB_OK);
      }
      else
      {
        MessageBox(0, errMsg.c_str(), "Service Watch", MB_OK);
      }
      return FALSE; 
    } 
    else
    {
      gitWatchSvcHelper.closeSvcNotify();
    }
  }
  return FALSE; 
}


INT_PTR APIENTRY ChildDlgProc(HWND hChildDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
  switch (message) 
  { 
    case WM_INITDIALOG:
    {
      gitWatch.childDlgInit(hChildDlg);
      return TRUE;
    }
    case WM_COMMAND:
    {
      return gitWatch.buttonPress(hChildDlg, gitWatch.getCurrentTabID(), LOWORD(wParam));
    }
    case WM_SHOWWINDOW:
    {
      break;
    }
    case WM_DESTROY:
    {
      if (GetDlgCtrlID(hChildDlg) == 0)
      {
        gitWatchSvcHelper.closeSvcNotify();
      }
      break;
    }
  }
  return FALSE;
}


// Positions the child dialog box to occupy the display area of the 
//   tab control. 
// hwndDlg - handle of the dialog box.
//
void GitWatchGui::childDlgInit(HWND hDlg) 
{ 
  SetWindowPos(hDlg, NULL, mRcDisplay.left,
               mRcDisplay.top,//-2,
               (mRcDisplay.right - mRcDisplay.left),
               (mRcDisplay.bottom - mRcDisplay.top),
               SWP_SHOWWINDOW);
  switch (GetDlgCtrlID(hDlg))
  {
    case 0:
    {
      break;
    }
  }
  return;
}


BOOL GitWatchGui::buttonPress(HWND hDlg, int tabId, int buttonId)
{
  if (tabId == -1)
  {
    switch (buttonId)
    {
      case IDOK:
      {
        if (saveSettings(getCurrentTabID()))
        {
          ShowWindow(mhMainDlg, SW_HIDE);
          gitWatch.refresh();
        }
        return TRUE;
      }
      case IDCANCEL:
      {
        DestroyWindow(mhMainDlg);
        return TRUE;
      }
    }
  }
  else if (tabId == 0)
  {
    /*
    PUSHBUTTON      "Stop Service",IDC_BUTTON1,19,39,54,15
    PUSHBUTTON      "Start Service",IDC_BUTTON2,76,39,54,15
    PUSHBUTTON      "Enable Disable",IDC_BUTTON3,150,39,60,15
    PUSHBUTTON      "Remove Service",IDC_BUTTON4,211,39,59,15
    */
    bool svcButton = false;
    int status = 0;
    std::string msg;
    switch (buttonId)
    {
      case IDC_BUTTON1:
      {
        svcButton = true;
        status = gitWatchSvcHelper.stop(msg);
        break;
      }
      case IDC_BUTTON2:
      {
        svcButton = true;
        status = gitWatchSvcHelper.start(msg);
        break;
      }
      case IDC_BUTTON3:
      {
        std::string username, password;
        svcButton = true;
        int checked = IsDlgButtonChecked(mhDisplay, IDC_CHECK1);
        if (checked == BST_CHECKED)
        {
          username = getDlgItemString(hDlg, IDC_EDIT1);
          password = getDlgItemString(hDlg, IDC_EDIT2);
        }
        status = gitWatchSvcHelper.install(username, password, msg);
        break;
      }
      case IDC_BUTTON4:
      {
        svcButton = true;
        status = gitWatchSvcHelper.deleteSvc(msg);
        break;
      }
    }
    if (svcButton)
    {
      //std::string msg2, errMsg;
      //gitWatchSvcHelper.checkRunning(msg2, NULL);
      SetDlgItemText(hDlg, IDC_STATIC1, msg.c_str());
      MessageBox(0, msg.c_str(), "Message", MB_OK);
    }
    return FALSE;
  }
  else if (tabId == 1)
  {
    /*
    switch (buttonId)
    {
      case IDC_BUTTON1:
      {
        std::string dir;
        if (gitWatch.createBrowseDlg(&dir)==IDOK)
        {
          std::string dir = gitWatch.createBrowseDlg();
        }
        break;
      }
      case IDC_BUTTON2:
    }
    */
    /*
    PUSHBUTTON      "Browse...",IDC_BUTTON1,466,40,50,14
    PUSHBUTTON      "Add",IDC_BUTTON2,466,59,50,14
    PUSHBUTTON      "Remove",IDC_BUTTON3,466,75,50,14
    EDITTEXT        IDC_EDIT1,18,118,444,14,ES_AUTOHSCROLL
    PUSHBUTTON      "Browse...",IDC_BUTTON4,466,118,50,14
    */
  }
  return FALSE;
}


bool GitWatchGui::saveSettings(int id)
{
  UNUSED(id);
  return true;
}


bool GitWatchGui::refresh()
{
  return true;
}


int GitWatchGui::reloadWatchPaths()
{
  std::ifstream fs;
  std::string text;
  int error = 0;

  //logfile.open("GitWatch.log", std::fstream::out);
  //if (logfile.is_open()==false) return ERROR_LOG_FAILED;

  fs.open(mConfigPath, std::fstream::in);
  if (fs.is_open()==false) 
  {
    mLogger.log("ERROR_CONFIG_FAILED_OPEN", mConfigPath);
    error = ERROR_CONFIG_FAILED_OPEN;
  }
  else
  {
    while (std::getline(fs, text))
    {
      chomp(text);
      mWatchPaths.push_back(text);
      mLogger.log("CONFIG READ PATH", text);
    }
    if (mWatchPaths.size()==0)
    {
      mLogger.log("ERROR_CONFIG_EMPTY", mConfigPath);
      error = ERROR_CONFIG_EMPTY;
    }
  }
  return error;
}
 

DWORD exec_process(std::string cmdline, DWORD *pExitCode)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  LPSTR cmdlinePtr = (LPSTR) cmdline.c_str();

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  *pExitCode = 0;
  // Start the child process. 
  BOOL ok = CreateProcessA(NULL,   // No module name (use command line)
                           cmdlinePtr,     // Command line
                           NULL,           // Process handle not inheritable
                           NULL,           // Thread handle not inheritable
                           FALSE,          // Set handle inheritance to FALSE
                           0,              // No creation flags
                           NULL,           // Use parent's environment block
                           NULL,           // Use parent's starting directory 
                           &si,            // Pointer to STARTUPINFO structure
                           &pi);           // Pointer to PROCESS_INFORMATION structure
  
  //printf("CreateProcess failed (%d).\n", GetLastError());
  DWORD retValue = 0;
  
  // Wait until child process exits.
  if (ok)
  {
    WaitForSingleObject(pi.hProcess, INFINITE);

    GetExitCodeProcess(pi.hProcess, pExitCode);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
  else
  {
    retValue = GetLastError();
  }
  return retValue;
}
