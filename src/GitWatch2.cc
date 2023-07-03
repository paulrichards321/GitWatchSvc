#include <iostream>
#include <windows.h>
#include <string>

struct ExecProcessInfo
{
  HANDLE hChildStdOutRd;
  HANDLE hChildStdErrRd;
  std::string *pOutText;
};

std::string getLastErrorAsString(DWORD errorMessageID);
DWORD exec_process(std::string cmdline, DWORD *pExitCode, std::string *pOutText);
void captureOutput(HANDLE hPipe, std::string *pOutText);
DWORD WINAPI ReadFromPipeThread(LPVOID options);


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
  std::string proc1="c:\\msys64\\usr\\bin\\ls.exe";
  std::string proc2="c:\\msys64\\usr\\bin\\ls.exe -l asdjfklasd";
  std::string outText;
  DWORD exitCode = 0;
  DWORD retValue = 0;

  SetCurrentDirectory("e:\\src\\GitWatch\\src");

  std::cout << proc1 << std::endl;
  retValue = exec_process(proc1, &exitCode, &outText);
  std::cout << "exitCode: " << exitCode << " retValue: " << retValue << std::endl;
  std::cout << "outText: '" << outText << "'" << std::endl;
  exitCode = 0;

  std::cout << proc2 << std::endl;
  retValue = exec_process(proc2, &exitCode, &outText);
  std::cout << "exitCode: " << exitCode << " retValue: " << retValue << std::endl;
  std::cout << "outText: '" << outText << "'" << std::endl;

  return 0;
}


std::string getLastErrorAsString(DWORD errorMessageID)
{
  // Get the error message ID, if any.
  if (errorMessageID == 0) {
    return std::string();
  }
    
  LPSTR messageBuffer = nullptr;

  // Ask Win32 to give us the string version of that message ID.
  // The parameters we pass in, tell Win32 to create the buffer
  // that holds the message for us
  // (because we don't yet know how long the message string will be).
  size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                               FORMAT_MESSAGE_FROM_SYSTEM | 
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL, 
                               errorMessageID, 
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                               (LPSTR) &messageBuffer, 
                               0, 
                               NULL);
    
  // Copy the error message into a std::string.
  std::string message(messageBuffer, size);
    
  // Free the Win32's string's buffer.
  LocalFree(messageBuffer);
            
  return message;
}


void captureOutput(HANDLE hPipe, std::string *pOutText)
{
  char *pBuff = new char[8192];
  DWORD bytesRead = 0;
  BOOL readSuccess = FALSE;
  DWORD orgPipeState = 0;
  DWORD pipeState = 0;
  bool setState = false;
  DWORD outBytes = 0, inBytes = 0;
  
  /*
  GetNamedPipeInfo(hPipe, NULL, &outBytes, &inBytes, NULL);
  GetNamedPipeHandleState(hPipe, &orgPipeState, NULL, NULL, NULL, NULL, 0);
  pipeState = orgPipeState;
  if ((pipeState & PIPE_NOWAIT)==0) 
  {
    pipeState = pipeState | PIPE_NOWAIT;
    SetNamedPipeHandleState(hPipe, &pipeState, NULL, NULL);
    setState = true;
  }
  */
  std::cout << "READING BYTES..." << std::endl;
  /*
  if (outBytes || inBytes)
  {
    pBuff = new char[outBytes+2];
  }
  */
  if (pBuff) 
  {
    do
    {
      bytesRead = 0;
      ZeroMemory(pBuff, 8192);
      readSuccess = ReadFile(hPipe, pBuff, 8190, &bytesRead, NULL);
      DWORD errorCode = GetLastError();
      std::cout << "ErrorCode: '";
      std::cout << getLastErrorAsString(errorCode) << "'" << std::endl;
      if (bytesRead == 0)
      {
        std::cout << "NO BYTES READ FROM PIPE!" << std::endl;
        readSuccess = FALSE;
      }
      else
      {
        std::cout << "PIPE DATA START: '";
        std::cout << pBuff;
        std::cout << "' PIPE DATA END." << std::endl;
        pOutText->append(pBuff);
      }
    } while (readSuccess);
    delete[] pBuff;
  }
  /*
  if (setState)
  {
    SetNamedPipeHandleState(hPipe, &orgPipeState, NULL, NULL);
  }
  */
  /*
  pipeState = 0;
  GetNamedPipeHandleState(hPipe, &pipeState, NULL, NULL, NULL, NULL, 0);
  if ((pipeState & PIPE_NOWAIT)) 
  {
    pipeState = pipeState & PIPE_NOWAIT;
    SetNamedPipeHandleState(hPipe, &pipeState, NULL, NULL);
  }
  */
}


DWORD WINAPI ReadFromPipeThread(LPVOID options)
{
  ExecProcessInfo *epi = (ExecProcessInfo*) options;

  captureOutput(epi->hChildStdOutRd, epi->pOutText);
  captureOutput(epi->hChildStdErrRd, epi->pOutText);
  return 0;
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
  ExecProcessInfo epi;

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
    if (hChildStdErrRd)
    {
      setupRedirect = SetHandleInformation(hChildStdErrRd, HANDLE_FLAG_INHERIT, 0);
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (hChildStdInWr)
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

  // Wait until child process exits.
  if (ok)
  {
    DWORD threadID = 0;
    epi.hChildStdOutRd = hChildStdOutRd;
    epi.hChildStdErrRd = hChildStdErrRd;
    epi.pOutText = pOutText;

    CloseHandle(hChildStdOutWr);
    CloseHandle(hChildStdErrWr);
    CloseHandle(hChildStdInRd);

    //HANDLE hThread = CreateThread(NULL, 0, ReadFromPipeThread, (LPVOID) &epi, 0, &threadID);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, pExitCode);
    ReadFromPipeThread((LPVOID) &epi);

    /*
    DWORD outBytes = 0, inBytes = 0;
    std::stringstream stream2;
    std::string str2;

    outBytes = 0;
    inBytes = 0;
    GetNamedPipeInfo(hChildStdOutRd, NULL, &outBytes, &inBytes, NULL);
    stream2.str("");
    stream2 << "[1]Child Standard Output Rd, outBytes: " << outBytes << " inBytes: " << inBytes;
    << stream2.str());
    captureOutput(hChildStdOutRd, pOutText);
    */

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hChildStdOutRd);
/*
    if (hThread)
    {
      WaitForSingleObject(hThread, INFINITE);
    }
*/
    /*
    DWORD pipeState = 0;
    pipeState = 0;
    GetNamedPipeHandleState(hChildStdOutWr, &pipeState, NULL, NULL, NULL, NULL, 0);
    if ((pipeState & PIPE_NOWAIT)==0) 
    {
      pipeState = pipeState | PIPE_NOWAIT;
      SetNamedPipeHandleState(hChildStdOutWr, &pipeState, NULL, NULL);
    }
    */
    //captureOutput(hChildStdOutWr, pOutText);
  }
  else
  {
    retValue = GetLastError();
  }
  return retValue;
}
