#ifndef __GITWATCH_UTIL_H
#define __GITWATCH_UTIL_H

#include <windows.h>
#include <string>

void chomp(std::string &text);
std::string currentDateTime();
std::string getLastErrorAsString(DWORD errorCode);
std::string getDlgItemString(HWND hDlg, int id);
std::string stripNewLines(std::string text);
bool scanIniLine(std::string& line, std::string& section, std::string& var, std::string& value, bool trim);

#ifndef UNUSED
#define UNUSED(arg) (void) arg
#endif

#endif
