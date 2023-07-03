#include <iostream>
#include <string>
#include "DirDiff.h"
#include "MemoryLeaks.h"

int main(int argc, char* argv[])
{
  DirEntry *pHeadDirA = NULL;
  DirEntry *pHeadDirB = NULL;
  NotFound *pNotFoundHeadA = NULL;
  NotFound *pNotFoundHeadB = NULL;
  if (argc > 1)
  {
    char stuff[32];
    pHeadDirA = DirWalk(argv[1]);
    std::cout << "Sleeping 20 seconds..." << std::endl;
    Sleep(1000 * 20);
    pHeadDirB = DirWalk(argv[1]);

    std::size_t notCtA = 0;
    std::size_t notCtB = 0;
    pNotFoundHeadA = DirDiff(pHeadDirB, pHeadDirA, false, &notCtA);
    pNotFoundHeadB = DirDiff(pHeadDirA, pHeadDirB, true, &notCtB);

    std::cout << "Files Changed:";
    std::cout << std::endl;
    coutNotFound(pNotFoundHeadA);

    std::cout << "Files/Dirs Added: ";
    std::cout << std::endl;
    coutNotFound(pNotFoundHeadB);

    std::cout << "Free Memory: pHeadDirA" << std::endl;
    FreeDirEntryList(pHeadDirA);
    std::cout << "Free Memory: pHeadDirB" << std::endl;
    FreeDirEntryList(pHeadDirB);
    std::cout << "saveMemoryLeaks()" << std::endl;
    saveMemoryLeaks();
    std::cout << "Free Memory: pNotFoundHeadA" << std::endl;
    FreeNotFoundList(pNotFoundHeadA);
    std::cout << "Free Memory: pNotFoundHeadB" << std::endl;
    FreeNotFoundList(pNotFoundHeadB);
  }
  return 0;
}
