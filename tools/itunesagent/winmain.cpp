/*
 //
 // BEGIN SONGBIRD GPL
 //
 // This file is part of the Songbird web player.
 //
 // Copyright(c) 2005-2009 POTI, Inc.
 // http://songbirdnest.com
 //
 // This file may be licensed under the terms of of the
 // GNU General Public License Version 2 (the "GPL").
 //
 // Software distributed under the License is distributed
 // on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 // express or implied. See the GPL for the specific language
 // governing rights and limitations.
 //
 // You should have received a copy of the GPL along with this
 // program. If not, go to http://www.gnu.org/licenses/gpl.html
 // or write to the Free Software Foundation, Inc.,
 // 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 //
 // END SONGBIRD GPL
 //
 */

#include <algorithm>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>

#include "sbiTunesAgentAppWatcher.h"

/**
 * Win32 WinMain entry point used to forward to the generic main
 */

int main(int argc, char * argv[]);

/**
 * Retrieves the command line and presents it in a manner consumable by
 * standard c "main"
 */
class sbCommandLine 
{
public:
  /**
   * Retrieves and processes the command line
   */
  sbCommandLine();
  /**
   * Cleans up our allocated resources
   */
  ~sbCommandLine();
  /**
   * Whether the command line was properly parsed/retrieved
   */
  bool Parsed() const {
    return mParsed;
  }
  /**
   * The number of arguments in our command line
   */
  int ArgCount() const {
    return mArgCount;
  }
  char ** Args() {
    return mArgs;
  }
public:
  bool mParsed;
  char ** mArgs;
  int mArgCount;
};

sbCommandLine::sbCommandLine() : mParsed(false) {
  char buffer[4096];
  LPWSTR commandLine = GetCommandLineW();
  if (commandLine) {
    LPWSTR * commandLineArgs = CommandLineToArgvW(commandLine, &mArgCount);
    if (commandLineArgs) {
       mArgs = new char *[mArgCount];
       // Null out all pointers
       std::fill(mArgs, mArgs + mArgCount, static_cast<char*>(0));
       // Convert each arg from wide to multibyte
       for (int arg = 0; arg < mArgCount; ++arg) {
         size_t returnValue;
         errno_t error = wcstombs_s(&returnValue, 
                                    buffer,
                                    sizeof(buffer),
                                    commandLineArgs[arg],
                                    _TRUNCATE);
         if (error == 0) {
           mArgs[arg] = new char[returnValue];
           memcpy(mArgs[arg], buffer, returnValue);
         }
       }
       mParsed = true;
    }
    LocalFree(commandLineArgs);
  }
}

/**
 * Helper function to clean up the memory of each argument
 */
inline
void Destruct(char * arg)
{
  delete [] arg;
}

sbCommandLine::~sbCommandLine() {
  // Delete each of the args
  std::for_each(mArgs, mArgs + mArgCount, Destruct);  
  // Free the array itself
  delete [] mArgs;
}

/**
 * Win32 entry point
 */
int WINAPI WinMain(HINSTANCE, 
                   HINSTANCE, 
                   LPSTR, 
                   int) {
  // Get the commandline args
  sbCommandLine commandLine;

  int result = -1;
  // Since waiting for iTunes is a windows only issue, we'll do it
  // here
  return main(commandLine.ArgCount(), commandLine.Args());
}
