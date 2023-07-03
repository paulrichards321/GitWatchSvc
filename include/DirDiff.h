#ifndef __DIRDIFF_H
#define __DIRDIFF_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>
#include <stack>

struct Entry;
struct DirEntry;
struct NotFound;

struct Entry
{
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastWriteTime;
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
  CHAR cFileName[MAX_PATH];
  Entry* pNextEntry;
};

struct DirEntry
{
  Entry *pThis;
  Entry *pFirstEntryIn;
  DirEntry *pFirstDirIn;
  DirEntry *pNextDir;
  DirEntry *pUpDir;
};

struct NotFound
{
  DirEntry* pDirEntry;
  Entry* pEntry;
  bool missing;
  NotFound *pNextNotFound;
};

bool DirWalk(LPCSTR root, DirEntry **ptpHeadDir);
DirEntry* AdvanceDir(DirEntry *pTailDir);
NotFound* DirDiff(DirEntry *pHeadDirB, DirEntry *pHeadDirA, bool ignoreAttrib, std::size_t *pNotMatchCount);
bool FindDir(DirEntry *pHeadDirB, DirEntry *pTailDirA, DirEntry **ptpFoundDirB, bool ignoreAttrib, bool *pDirNameMatch);
bool FindFile(Entry *pFirstEntryInB, Entry *pEntryA, bool ignoreAttrib, bool *pNameMatch);
void FreeDirEntryList(DirEntry *pDirEntryHead);
void FreeNotFoundList(NotFound *pNotFoundHead);
std::string stringifyDir(DirEntry *pDirEntry);
void coutNotFound(NotFound *pNotFoundHead);
void coutDirHead(DirEntry *pDirEntryHead);

#endif
