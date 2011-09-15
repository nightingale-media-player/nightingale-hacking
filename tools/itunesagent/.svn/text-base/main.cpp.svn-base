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

#include "sbiTunesAgentProcessor.h"
#include <iostream>
#include <sstream>

/**
 * Standard C main function
 */
int main(int argc, char * argv[]) {
  sbiTunesAgentProcessorPtr processor(sbCreatesbiTunesAgentProcessor());
  // Were we asked to unregister from "run"
  if (argc > 1 && std::string(argv[1]) == "--unregister") {
    sbError error = processor->UnregisterForStartOnLogin();
    return error ? -1 : 0;
  }
  
  // Kill any of our processes
  if (argc > 1 && (std::string(argv[1]) == "--roundhouse" ||
                        std::string(argv[1]) == "--kill")) {
    sbError error = processor->KillAllAgents();
    return error ? -1 : 0;
  }

  // Don't start duplicate copies of the agent.
  if (processor->GetIsAgentRunning()) {
    return 0;
  }

  int index = 1;
 
  while (index < argc) {
    std::string flag = argv[index];

    // Set the batch size
    if (argc > index+1 && flag == "--batch-size") {
      unsigned int batchSize = sbiTunesAgentProcessor::BATCH_SIZE;
      // Convert the string arg to an integer
      std::istringstream parser(argv[++index]);
      parser >> batchSize;
      processor->SetBatchSize(batchSize);
    }
    // Register the profile
    else if (argc > index+1 && flag == "--profile") {
      std::string const profile = argv[++index];
      processor->RegisterProfile(profile);
    }

    index++;
  }
  
  // Register the app with the run startup key
  sbError error = processor->RegisterForStartOnLogin();
  if (error) {
    // Handle the error and return if told to stop
    if (!processor->ErrorHandler(error)) {
      return -1;
    }
  }
  
  if (processor->TaskFileExists()) {
    error = processor->WaitForiTunes();
    if (!error) {
      error = processor->ProcessTaskFile();
    }
  }
  return error ? -1 : 0;
}
