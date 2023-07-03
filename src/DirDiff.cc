#include "DirDiff.h"
#include <vector>
#include <iostream>
#include "MemoryLeaks.h"

DirEntry* AdvanceDir(DirEntry *pTailDir)
{
  DirEntry* pNewTail = NULL;
  if (pTailDir == NULL)
  {
    // skip, return NULL
  }
  else if (pTailDir->pFirstDirIn)
  {
    pNewTail = pTailDir->pFirstDirIn;
  }
  else if (pTailDir->pNextDir)
  {
    pNewTail = pTailDir->pNextDir;
  }
  else
  {
    do
    {
      pTailDir = pTailDir->pUpDir;
      if (pTailDir == NULL)
      {
        break;
      }
      pNewTail = pTailDir->pNextDir;
    } while (pNewTail == NULL);
  }
  return pNewTail;
}


bool FindDir(DirEntry *pHeadDirB, DirEntry *pTailDirA, DirEntry **ptpFoundDirB, bool ignoreAttrib, bool *pNameMatch)
{
  std::stack<DirEntry*> dirStack;
  DirEntry *pDirEntryA = pTailDirA;
  if (pTailDirA == NULL || pHeadDirB == NULL)
  {
    *pNameMatch = false;
    *ptpFoundDirB = NULL;
    return false;
  }
  while (pDirEntryA->pUpDir)
  {
    dirStack.push(pDirEntryA);
    pDirEntryA = pDirEntryA->pUpDir;
  }
  if (dirStack.empty())
  {
    *pNameMatch = true;
    *ptpFoundDirB = pHeadDirB;
    return true;
  }
  DirEntry *pPreviousDirB = pHeadDirB;
  DirEntry *pDirEntryB = pHeadDirB->pFirstDirIn;
  bool dirNameMatch = false;
  while (dirStack.empty() == false && pDirEntryB)
  {
    pDirEntryA = dirStack.top();
    //std::cout << "DirAComp:  " << pDirEntryA->pThis->cFileName << std::endl;
    dirNameMatch = false;
    while (dirNameMatch == false && pDirEntryB)
    {
      //std::cout << "DirBComp:  " << pDirEntryB->pThis->cFileName << std::endl;
      if (strncmp(pDirEntryA->pThis->cFileName, pDirEntryB->pThis->cFileName, MAX_PATH) == 0)
      {
        //std::cout << "DirFound! : " << pDirEntryA->pThis->cFileName;
        dirNameMatch = true;
      }
      else
      {
        pDirEntryB = pDirEntryB->pNextDir;
      }
    }
    if (dirNameMatch)
    {
      pPreviousDirB = pDirEntryB;
      pDirEntryB = pDirEntryB->pFirstDirIn;
    }
    else
    {
      pDirEntryB = NULL;
    }
    dirStack.pop();
  }
  bool dirCompleteMatch = false;
  if (dirNameMatch)
  {
    *pNameMatch = true;
    *ptpFoundDirB = pPreviousDirB;
    if (ignoreAttrib || (
        pDirEntryA->pThis->dwFileAttributes == pPreviousDirB->pThis->dwFileAttributes))
    {
      dirCompleteMatch = true;
    }
  }
  else
  {
    *ptpFoundDirB = NULL;
  }
  return dirCompleteMatch;
}


bool FindFile(Entry *pFirstEntryInB, Entry *pEntryA, bool ignoreAttrib, bool *pNameMatch)
{
  //Entry *pEntryB = pFoundDirB->pFirstEntryIn;
  bool fileMatch = false;
  bool nameMatch = false;
  Entry *pEntryB = pFirstEntryInB;
  while (pEntryB && nameMatch == false)
  {
    //std::cout << "File2Comp:  " << pEntryB->cFileName << std::endl;
    if (strncmp(pEntryA->cFileName, pEntryB->cFileName, MAX_PATH) == 0)
    {
      //std::cout << "File2Comp: Name Match!" << std::endl;
      nameMatch = true;
      if (ignoreAttrib || (
          pEntryA->ftCreationTime.dwLowDateTime == pEntryB->ftCreationTime.dwLowDateTime &&
          pEntryA->ftCreationTime.dwHighDateTime == pEntryB->ftCreationTime.dwHighDateTime &&
          pEntryA->ftLastWriteTime.dwLowDateTime == pEntryB->ftLastWriteTime.dwLowDateTime &&
          pEntryA->ftLastWriteTime.dwHighDateTime == pEntryB->ftLastWriteTime.dwHighDateTime &&
          pEntryA->dwFileAttributes == pEntryB->dwFileAttributes &&
          pEntryA->nFileSizeHigh == pEntryB->nFileSizeHigh &&
          pEntryA->nFileSizeLow == pEntryB->nFileSizeLow))
      {
        fileMatch = true;
        //std::cout << "File2Comp: Attribute Match!" << std::endl;
      }
    }
    pEntryB = pEntryB->pNextEntry;
  }
  *pNameMatch = nameMatch;
  return fileMatch;
}


NotFound* DirDiff(DirEntry *pHeadDirB, DirEntry *pHeadDirA, bool ignoreAttrib, std::size_t *pNotMatchCount)
{
  NotFound *pNotFoundHead = NULL;
  NotFound *pNotFoundTail = NULL;
  std::size_t notMatch = 0;
  DirEntry *pTailDirA = pHeadDirA;
  if (pHeadDirB == NULL) 
  {
    *pNotMatchCount = 0;
    return NULL;
  }
  while (pTailDirA)
  {
    Entry *pEntryA = pTailDirA->pFirstEntryIn;
    Entry *pFirstEntryInB = NULL;
    if (pEntryA)
    {
      DirEntry *pFoundDirB = NULL;
      bool dirNameMatch = false;
      bool dirCompleteMatch = FindDir(pHeadDirB, pTailDirA, &pFoundDirB, ignoreAttrib, &dirNameMatch); // dirCompleteMatch is true if all attributes match
      if (dirNameMatch)
      {
        pFirstEntryInB = pFoundDirB->pFirstEntryIn;
      }
      if (dirCompleteMatch == false)
      {
        NotFound *pNotFound = track_new NotFound;
        pNotFound->pDirEntry = pTailDirA;
        pNotFound->pEntry = NULL;
        pNotFound->missing = (dirNameMatch == true ? false : true);
        pNotFound->pNextNotFound = NULL;
        if (pNotFoundTail == NULL)
        {
          pNotFoundHead = pNotFound;
        }
        else
        {
          pNotFoundTail->pNextNotFound = pNotFound;
        }
        pNotFoundTail = pNotFound;
        notMatch++;
      }
      do
      {
        //std::cout << "File1Comp:  " << pEntryA->cFileName << std::endl;
        bool fileCompleteMatch = false;
        bool fileNameMatch = false;
        if (dirNameMatch)
        {
          fileCompleteMatch = FindFile(pFirstEntryInB, pEntryA, ignoreAttrib, &fileNameMatch);
        }
        if (fileCompleteMatch==false) 
        {
          NotFound *pNotFound = track_new NotFound;
          pNotFound->pDirEntry = pTailDirA;
          pNotFound->pEntry = pEntryA;
          pNotFound->missing = (fileNameMatch == true ? false : true);
          pNotFound->pNextNotFound = NULL;
          if (pNotFoundTail == NULL)
          {
            pNotFoundHead = pNotFound;
          }
          else
          {
            pNotFoundTail->pNextNotFound = pNotFound;
          }
          pNotFoundTail = pNotFound;
          notMatch++;
          //std::cout << "File1Comp: Not Match: " << pEntryA->cFileName << std::endl;
        }
        pEntryA = pEntryA->pNextEntry;
      } while (pEntryA);
    }
    pTailDirA = AdvanceDir(pTailDirA);
  } while (pTailDirA);

  *pNotMatchCount = notMatch;
  return pNotFoundHead;
}


bool DirWalk(LPCSTR root, DirEntry **ptpHeadDir)
{
  WIN32_FIND_DATAA fd;
  BY_HANDLE_FILE_INFORMATION bhfi;
  HANDLE hFind;
  std::string fileName;
  std::string rootPath = root;
  std::string regexPath;
  std::string fullPath;
  DirEntry *pHeadDir = NULL;
  DirEntry *pTailDir = NULL;
  bool success = true;

  if (rootPath.length() > 0 && 
      rootPath.at(rootPath.length()-1) != '\\' &&
      rootPath.at(rootPath.length()-1) != '/')
  {
    rootPath.append("\\");
  }
  do
  {
    fullPath = rootPath;
    fullPath.append(stringifyDir(pTailDir));
    regexPath = fullPath;
    regexPath.append("*");
    //std::cout << "Searching: " << fullPath << std::endl;
    memset(&fd, 0, sizeof(fd));
    hFind = FindFirstFile(regexPath.c_str(), &fd);
    if (hFind == NULL) 
    {
      success = false;
      break;
    }

    DirEntry *pPreviousDir = NULL;
    Entry *pPreviousEntry = NULL;

    if (pTailDir == NULL)
    {
      pHeadDir = track_new DirEntry;
      pHeadDir->pThis = NULL;
      pHeadDir->pFirstEntryIn = NULL;
      pHeadDir->pNextDir = NULL;
      pHeadDir->pUpDir = NULL;
      pHeadDir->pFirstDirIn = NULL;
      pTailDir = pHeadDir;
    }
    do
    {
      BOOL readFileInfo = FALSE;
      if (fd.cFileName[0] == '.' && fd.cFileName[1] == 0) continue;
      if (fd.cFileName[0] == '.' && fd.cFileName[1] == '.' && fd.cFileName[2] == 0) continue;
      Entry *pEntry = track_new Entry;
      memset(pEntry, 0, sizeof(Entry));
      fileName = fullPath;
      fileName.append(fd.cFileName);
      HANDLE hFile = CreateFile(fileName.c_str(), 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL); 
      if (hFile != INVALID_HANDLE_VALUE) 
      {
        memset(&bhfi, 0, sizeof(bhfi));
        readFileInfo = GetFileInformationByHandle(hFile, &bhfi);
        CloseHandle(hFile);
      }
      if (readFileInfo) 
      {
        pEntry->dwFileAttributes = bhfi.dwFileAttributes;
        pEntry->ftCreationTime = bhfi.ftCreationTime;
        pEntry->ftLastWriteTime = bhfi.ftLastWriteTime;
        pEntry->nFileSizeHigh = bhfi.nFileSizeHigh;
        pEntry->nFileSizeLow = bhfi.nFileSizeLow;
      }
      else
      {
        pEntry->dwFileAttributes = fd.dwFileAttributes;
        pEntry->ftCreationTime = fd.ftCreationTime;
        pEntry->ftLastWriteTime = fd.ftLastWriteTime;
        pEntry->nFileSizeHigh = fd.nFileSizeHigh;
        pEntry->nFileSizeLow = fd.nFileSizeLow;
      }
      strncpy_s(pEntry->cFileName, MAX_PATH, fd.cFileName, MAX_PATH);
      //std::cout << pEntry->cFileName;

      if (pEntry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        DirEntry *pNextDir = track_new DirEntry;
        //std::cout << "\\" << std::endl;
        pNextDir->pThis = pEntry;
        pNextDir->pFirstEntryIn = NULL;
        pNextDir->pFirstDirIn = NULL;
        pNextDir->pNextDir = NULL;
        pNextDir->pUpDir = pTailDir;
        if (pPreviousDir)
        {
          pPreviousDir->pNextDir = pNextDir;
        }
        else
        {
          pTailDir->pFirstDirIn = pNextDir;
        }
        pPreviousDir = pNextDir;
      }
      else
      {
        //std::cout << std::endl;
        if (pPreviousEntry)
        {
          pPreviousEntry->pNextEntry = pEntry;
        }
        if (pTailDir->pFirstEntryIn == NULL)
        {
          pTailDir->pFirstEntryIn = pEntry;
        }
        pPreviousEntry = pEntry;
      }
      memset(&fd, 0, sizeof(fd));
    } while (FindNextFile(hFind, &fd));
    if (hFind)
    {
      FindClose(hFind);
    }
    pTailDir = AdvanceDir(pTailDir);
  } while (pTailDir);
  
  *ptpHeadDir = pHeadDir;
  return success;
}


std::string stringifyDir(DirEntry *pDirEntry)
{
  std::string str;
  std::stack<DirEntry*> dirStack;

  while (pDirEntry)
  {
    if (pDirEntry->pThis)
    {
      dirStack.push(pDirEntry);
    }
    pDirEntry = pDirEntry->pUpDir;
  }
  while (dirStack.empty() == false)
  {
    pDirEntry = dirStack.top();
    str.append(pDirEntry->pThis->cFileName);
    str.append("\\");
    dirStack.pop();
  }
  return str;
}


void coutNotFound(NotFound *pNotFoundHead)
{
  NotFound *pNotFound = pNotFoundHead;
  while (pNotFound)
  {
    std::cout << stringifyDir(pNotFound->pDirEntry);
    if (pNotFound->pEntry)
    {
      std::cout << pNotFound->pEntry->cFileName;
    }
    if (pNotFound->missing)
    {
      std::cout << " (Missing) ";
    }
    else
    {
      std::cout << " (Attrib Diff) ";
    }
    std::cout << std::endl;
    pNotFound = pNotFound->pNextNotFound;
  }
}


void coutDirHead(DirEntry *pDirEntryHead)
{
  DirEntry *pTailDir = pDirEntryHead;
  while (pTailDir)
  {
    std::string fullDir = stringifyDir(pTailDir);
    std::cout << fullDir << std::endl;
    Entry *pEntry = pTailDir->pFirstEntryIn;
    while (pEntry)
    {
      std::cout << fullDir << pEntry->cFileName << std::endl;
      pEntry = pEntry->pNextEntry;
    }
    pTailDir = AdvanceDir(pTailDir);
  }
}


void FreeDirEntryList(DirEntry *pDirEntryHead)
{
  DirEntry *pTailDir = pDirEntryHead;
  std::stack<DirEntry*> deletePtrStack;

  while (pTailDir)
  {
    deletePtrStack.push(pTailDir);
    if (pTailDir->pThis)
    {
      delete pTailDir->pThis;
      pTailDir->pThis = NULL;
    }
    Entry *pEntry = pTailDir->pFirstEntryIn;
    pTailDir->pFirstEntryIn = NULL;
    while (pEntry)
    {
      Entry* pOld = pEntry;
      pEntry = pEntry->pNextEntry;
      pOld->pNextEntry = NULL;
      delete pOld;
    }
    pTailDir = AdvanceDir(pTailDir);
  }
  while (deletePtrStack.empty() == false)
  {
    DirEntry *pDirEntry = deletePtrStack.top();
    memset(pDirEntry, 0, sizeof(DirEntry));
    delete pDirEntry;
    deletePtrStack.pop();
  }
}


void FreeNotFoundList(NotFound *pNotFound)
{
  NotFound *pOldNotFound;

  while (pNotFound)
  {
    pOldNotFound = pNotFound;
    pNotFound = pNotFound->pNextNotFound;
    pOldNotFound->pNextNotFound = NULL;
    delete pOldNotFound;
  }
}
