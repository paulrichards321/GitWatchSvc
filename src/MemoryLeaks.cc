#include <windows.h>
#include <stdio.h>
#include "MemoryLeaks.h"

long long int gMemListPtrs = 0; 
int gDebugLevel = 1;

MemList *pMemListHead = NULL;
MemList *pMemListTail = NULL;

#if defined(DEBUG) || defined(_DEBUG)

void *operator new(std::size_t size)
{
  void *ptr = malloc(size);
  if (ptr)
  {
    MemList *pMemListItem = (MemList*) malloc(sizeof(MemList));
    if (pMemListItem == NULL)
    {
      throw std::bad_alloc();
    }
    if (pMemListTail == NULL)
    {
      pMemListHead = pMemListItem;
      pMemListTail = pMemListItem;
    }
    else
    {
      pMemListTail->pNext = pMemListItem;
      pMemListTail = pMemListItem;
    }
    pMemListItem->size = size;
    pMemListItem->line = 0;
    pMemListItem->filename = "";
    pMemListItem->ptr = ptr;
    pMemListItem->pNext = NULL;
    gMemListPtrs++;
  }
  else
  {
    throw std::bad_alloc();
  }
  return ptr;
}


void *operator new(std::size_t size, int line, const char *filename)
{
  void *ptr = malloc(size);
  if (ptr)
  {
    MemList *pMemListItem = (MemList*) malloc(sizeof(MemList));
    if (pMemListItem == NULL)
    {
      throw std::bad_alloc();
    }
    if (pMemListTail == NULL)
    {
      pMemListHead = pMemListItem;
      pMemListTail = pMemListItem;
    }
    else
    {
      pMemListTail->pNext = pMemListItem;
      pMemListTail = pMemListItem;
    }
    pMemListItem->size = size;
    pMemListItem->line = line;
    pMemListItem->filename = filename;
    pMemListItem->ptr = ptr;
    pMemListItem->pNext = NULL;
    gMemListPtrs++;
    if (gDebugLevel > 1)
    {
      FILE *fp = fopen("memleak.txt", "a");
      if (fp)
      {
        fprintf(fp, "new() File: %s  Line: %i  Size: %lld  Address: %p\n", 
                filename, line, (long long int) size, ptr);
        fclose(fp);
      }
    }
  }
  else
  {
    throw std::bad_alloc();
  }
  return ptr;
}


void operator delete(void *ptr) noexcept
{
  MemList *pMemListItem = pMemListHead;
  MemList *pPreviousItem = NULL;
  bool found = false;
  while (pMemListItem && found == false)
  {
    if (pMemListItem->ptr == ptr)
    {
      found = true;
      free(ptr);
      if (pPreviousItem)
      {
        pPreviousItem->pNext = pMemListItem->pNext;
      }
      else
      {
        pMemListHead = pMemListItem->pNext;
      }
      if (pMemListTail == pMemListItem)
      {
        pMemListTail = pPreviousItem;
      }
      if (gDebugLevel > 1)
      {
        FILE *fp = fopen("memleak.txt", "a");
        if (fp)
        {
          fprintf(fp, "delete() File: %s  Line: %i  Size: %lli  Address: %p\n", 
                  pMemListItem->filename, pMemListItem->line, 
                  (long long int) pMemListItem->size, ptr);
          fclose(fp);
        }
      }
      free(pMemListItem);
      gMemListPtrs--;
      pMemListItem = NULL;
    }
    else
    {
      pPreviousItem = pMemListItem;
      pMemListItem = pMemListItem->pNext;
    }
  }
  if (found == false && gDebugLevel > 1)
  {
    FILE *fp = fopen("memleak.txt", "a");
    if (fp)
    {
      fprintf(fp, "delete() Unknown delete for Address: %p\n", ptr);
      fclose(fp);
    }
  }
}


void operator delete (void *ptr, const std::nothrow_t& nothrow_constant) noexcept
{
  MemList *pMemListItem = pMemListHead;
  MemList *pPreviousItem = NULL;
  bool found = false;
  while (pMemListItem && found == false)
  {
    if (pMemListItem->ptr == ptr)
    {
      found = true;
      free(ptr);
      if (pPreviousItem)
      {
        pPreviousItem->pNext = pMemListItem->pNext;
      }
      else
      {
        pMemListHead = pMemListItem->pNext;
      }
      if (pMemListTail == pMemListItem)
      {
        pMemListTail = pPreviousItem;
      }
      if (gDebugLevel > 1)
      {
        FILE *fp = fopen("memleak.txt", "a");
        if (fp)
        {
          fprintf(fp, "delete() File: %s  Line: %i  Size: %lli  Address: %p\n", 
                  pMemListItem->filename, pMemListItem->line, 
                  (long long int) pMemListItem->size, ptr);
          fclose(fp);
        }
      }
      free(pMemListItem);
      gMemListPtrs--;
      pMemListItem = NULL;
    }
    else
    {
      pPreviousItem = pMemListItem;
      pMemListItem = pMemListItem->pNext;
    }
  }
  if (found == false && gDebugLevel > 1)
  {
    FILE *fp = fopen("memleak.txt", "a");
    if (fp)
    {
      fprintf(fp, "delete() Unknown delete for Address: %p\n", ptr);
      fclose(fp);
    }
  }
}


void operator delete(void *ptr, std::size_t size) noexcept
{
  MemList *pMemListItem = pMemListHead;
  MemList *pPreviousItem = NULL;
  bool found = false;
  while (pMemListItem && found == false)
  {
    if (pMemListItem->ptr == ptr)
    {
      found = true;
      free(ptr);
      if (pPreviousItem)
      {
        pPreviousItem->pNext = pMemListItem->pNext;
      }
      else
      {
        pMemListHead = pMemListItem->pNext;
      }
      if (pMemListTail == pMemListItem)
      {
        pMemListTail = pPreviousItem;
      }
      if (gDebugLevel > 1)
      {
        FILE *fp = fopen("memleak.txt", "a");
        if (fp)
        {
          fprintf(fp, "delete() File: %s  Line: %i  Size: %lli  Address: %p\n", 
                  pMemListItem->filename, pMemListItem->line, 
                  (long long int) pMemListItem->size, ptr);
          fclose(fp);
        }
      }
      free(pMemListItem);
      gMemListPtrs--;
      pMemListItem = NULL;
    }
    else
    {
      pPreviousItem = pMemListItem;
      pMemListItem = pMemListItem->pNext;
    }
  }
  if (found == false && gDebugLevel > 1)
  {
    FILE *fp = fopen("memleak.txt", "a");
    if (fp)
    {
      fprintf(fp, "delete() Unknown delete for Address: %p  Size: %lli\n", ptr, (long long int) size);
      fclose(fp);
    }
  }
}


void operator delete(void *ptr, std::size_t size, const std::nothrow_t& nothrow_constant) noexcept
{
  MemList *pMemListItem = pMemListHead;
  MemList *pPreviousItem = NULL;
  MemList *pNextItem = NULL;
  bool found = false;
  while (pMemListItem && found == false)
  {
    if (pMemListItem->ptr == ptr)
    {
      found = true;
      free(ptr);
      if (pPreviousItem)
      {
        pPreviousItem->pNext = pMemListItem->pNext;
      }
      else
      {
        pMemListHead = pMemListItem->pNext;
      }
      if (pMemListTail == pMemListItem)
      {
        pMemListTail = pPreviousItem;
      }
      if (gDebugLevel > 1)
      {
        FILE *fp = fopen("memleak.txt", "a");
        if (fp)
        {
          fprintf(fp, "delete() File: %s  Line: %i  Size: %lli  Address: %p\n", 
                  pMemListItem->filename, pMemListItem->line, 
                  (long long int) pMemListItem->size, ptr);
          fclose(fp);
        }
      }
      free(pMemListItem);
      gMemListPtrs--;
      pMemListItem = NULL;
    }
    else
    {
      pPreviousItem = pMemListItem;
      pMemListItem = pMemListItem->pNext;
    }
  }
  if (found == false && gDebugLevel > 1)
  {
    FILE *fp = fopen("memleak.txt", "a");
    if (fp)
    {
      fprintf(fp, "delete() Unknown delete for Address: %p  Size: %lli\n", ptr, (long long int) size);
      fclose(fp);
    }
  }
}


long long int saveMemoryLeaks()
{
  MemList *pMemListItem = pMemListHead;
  FILE *fp;
  fp = fopen("memleak.txt", "a");
  if (fp)
  {
    fprintf(fp, "Showing Memory Leaks:\n");
  }
  if (gMemListPtrs == 0)
  {
    OutputDebugString("No memory leaks detected.");
    if (fp)
    {
      fprintf(fp, "No memory leaks detected.\n");
      fclose(fp);
    }
    return 0;
  }
  OutputDebugString("Memory leaks detected. Check memleak.txt.");
  while (pMemListItem)
  {
    if (fp)
    {
      fprintf(fp, "Leak: File: %s  Line: %i  Size: %lli  Address: %p\n", 
              pMemListItem->filename, pMemListItem->line, 
              (long long int) pMemListItem->size, pMemListItem->ptr);
    }
    pMemListItem = pMemListItem->pNext;
  }
  if (fp)
  {
    fclose(fp);
  }
  return gMemListPtrs;
}


#else


long long int saveMemoryLeaks()
{
  return 0;
}


#endif
