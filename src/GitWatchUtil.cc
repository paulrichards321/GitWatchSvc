#include <windows.h>
#include "GitWatchUtil.h"
#include <ctime>
#include <algorithm>

void chomp(std::string &text)
{
  // strip leading spaces
  size_t leadingSpaces = 0;
  for (size_t i = 0; i < text.length() && text.at(i) == ' '; i++)
  {
    leadingSpaces++;
  }
  if (leadingSpaces)
  {
    text.erase(0, leadingSpaces);
  }
    
  // strip trailing spaces
  size_t trailingSpaces = 0;
  if (text.length() > 0)
  {
    for (size_t i = text.length() - 1; i > 0 && text.at(i) == ' '; i--)
    {
      trailingSpaces++;
    }
    if (trailingSpaces)
    {
      text.erase(text.length()-trailingSpaces, trailingSpaces);
    }
  }
  // strip any sort of newline
  for (size_t i = 0; i < text.length(); i++)
  {
    if (text.at(i) == 0x0D || text.at(i) == 0x0A)
    {
      text.erase(i);
      i--;
    }
  }
}


std::string currentDateTime(void)
{
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];

	//tstruct = *localtime(&now);
	localtime_s(&tstruct, &now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

	return buf;
}


std::string getLastErrorAsString(DWORD errorMessageID)
{
  // Get the error message ID, if any.
  if (errorMessageID == 0) {
    //return std::string();
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


bool scanIniLine(std::string& line, std::string& section, std::string& var, std::string& value, bool trim)
{
  std::size_t start_var = std::string::npos;
  std::size_t end_var = std::string::npos;
  std::size_t equals_pos = std::string::npos;
  std::size_t start_val = std::string::npos;
  std::size_t end_val = std::string::npos;
  std::size_t bracket_first = std::string::npos;
  std::size_t bracket_second = std::string::npos;
  bool in_dbl_quote = false;
  bool used_dbl_quote = false;
  bool found_val = false;

  char key = 0;
  // option1=z\n
  std::size_t pos = 0;
  for (; pos < line.length(); pos++)
  {
    key = line.at(pos);
    if (key == ' ' || key == '\t') 
    {
      if (in_dbl_quote)
      {
        if (start_val == std::string::npos)
        {
          start_val = pos;
        }
      }
      else if (start_var != std::string::npos && end_var == std::string::npos)
      {
        end_var = pos-1;
      }
    }
    else if (key == '\"')
    {
      if (in_dbl_quote)
      {
        in_dbl_quote = false;
        if (start_val != std::string::npos)
        {
          end_val = pos-1;
        }
        used_dbl_quote = true;
      }
      else if (start_val != std::string::npos)
      {
      }
      else
      {
        in_dbl_quote = true;
      }
    }
    else if (key == '\r' || key == '\n')
    {
      if (end_val == std::string::npos && start_val != std::string::npos)
      {
        end_val = pos-1;
      }
      break;
    }
    else if (key == '=' && equals_pos == std::string::npos)
    {
      if (start_var != std::string::npos && end_var == std::string::npos)
      {
        end_var = pos-1;
      }
      equals_pos = pos;
    }
    else if (key == '[' && start_var == std::string::npos)
    {
      bracket_first = pos;
    }
    else if (key == ']' && start_var == std::string::npos)
    {
      bracket_second = pos;
    }
    else if (start_var == std::string::npos)
    {
      start_var = pos;
    }
    else if (equals_pos != std::string::npos && start_val == std::string::npos)
    {
      start_val = pos;
    }
  }
  if (start_var != std::string::npos && start_val != std::string::npos)
  {
    if (end_val == std::string::npos)
    {
      end_val = pos - 1;
    }
    if (trim && used_dbl_quote == false)
    {
      key = line.at(end_val);
      while (end_val >= start_val && (key == ' ' || key == '\t'))
      {
        end_val--;
        key = line.at(end_val);
      }
    }
    section = "";
    var = line.substr(start_var, end_var - start_var + 1); 
    value = line.substr(start_val, end_val - start_val + 1);
    found_val = true;
  } 
  else if (bracket_first != std::string::npos && bracket_second != std::string::npos && bracket_second > bracket_first)
  {
    section = line.substr(bracket_first+1, bracket_second - bracket_first - 1);
    found_val = true;
    var = "";
    value = "";
  }
  else
  {
    section = "";
    var = "";
    value = "";
  }
  return found_val;
}    
