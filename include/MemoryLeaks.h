#ifndef __MEMORYLEAKS_H
#define __MEMORYLEAKS_H

#include <new>

struct MemList;

long long int saveMemoryLeaks();

struct MemList
{
  std::size_t size;
  int line;
  const char *filename;
  void *ptr;
  MemList *pNext;
};

#if defined(DEBUG) || defined(_DEBUG)
void *operator new(std::size_t size);
void *operator new(std::size_t size, int line, const char* file);
void operator delete(void *ptr) noexcept;
void operator delete(void *ptr, const std::nothrow_t& nothrow_constant) noexcept;
void operator delete(void *ptr, std::size_t size) noexcept;
void operator delete(void *ptr, std::size_t size, const std::nothrow_t& nothrow_constant) noexcept;

#define track_new new (__LINE__, __FILE__)

#else

#define track_new new

#endif

#endif
