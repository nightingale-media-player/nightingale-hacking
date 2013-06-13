#include <string>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include "UnityProxy.h"


bool UnityProxy::isUnityRunning()
{
		bool haveUnity = false;
		bool haveSound = false;

		std::string PROCESS_INDICATOR_SOUND = "indicator-sound-service";
		std::string PROCESS_UNITY = "unity-panel-service";
		std::string clPath, clStr, dName, procName;
    char tmp[1024];
    struct dirent* de;
    DIR* pdir;
    FILE* clFile;

    // read /proc
    pdir = opendir("/proc");
    if (pdir == NULL) exit(EXIT_FAILURE);

    for (de = readdir(pdir); de != NULL && (!haveUnity || !haveSound); de = readdir(pdir)) {
        // only read process directories
        if (de->d_type == DT_DIR) {
        		dName = de->d_name;
            if (dName.find_first_not_of("0123456789") == std::string::npos) {
								clPath.assign("/proc/");
								clPath.append(de->d_name);
								clPath.append("/cmdline");

								// open /proc/*/cmdline and read it's contents
								clFile = fopen(clPath.c_str(), "r");
								if (clFile != NULL) {
										fscanf(clFile, "%s", tmp);
										fclose(clFile);
										clStr = tmp;
										// get base of process name
										procName = clStr.substr(clStr.find_last_of("/") + 1);
										if (!haveSound)
										    haveSound = (procName == PROCESS_INDICATOR_SOUND) ? true : false;
										if (!haveUnity) {
										    haveUnity = (procName == PROCESS_UNITY) ? true : false;
										}
								}
            }
        }
    }
    closedir(pdir);

    // make sure we have both of them
    return (haveUnity && haveSound);
}
