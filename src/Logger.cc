#include "GitWatchUtil.h"
#include "logger.h"

Logger::Logger()
{
	mhLogMutex = CreateMutex(NULL, FALSE, NULL);
	mIsOpen = false;
}


bool Logger::open(std::string path)
{
	mLogFile.open(path.c_str(), std::fstream::app | std::fstream::out);
	mIsOpen = mLogFile.is_open();
	return mIsOpen;
}


void Logger::log(std::string item, std::string path)
{
	WaitForSingleObject(mhLogMutex, INFINITE);
	if (mIsOpen) 
  {
    mLogFile << currentDateTime() << " " << item;
    if (path.length() > 0)
    {
      mLogFile << ": '" << path << "'";
    }
    mLogFile << std::endl;
  }
	ReleaseMutex(mhLogMutex);
}


Logger::~Logger()
{
	CloseHandle(mhLogMutex);
	mIsOpen = false;
	mLogFile.close();
}
