#ifndef __LOGGER_H
#define __LOGGER_H
#include <windows.h>
#include <fstream>
#include <string>

class Logger {
protected:
  HANDLE mhLogMutex;
  std::ofstream mLogFile;
  bool mIsOpen;
public:
  bool open(std::string);
  void log(std::string, std::string name = "");
  Logger();
  ~Logger();
};

#endif
