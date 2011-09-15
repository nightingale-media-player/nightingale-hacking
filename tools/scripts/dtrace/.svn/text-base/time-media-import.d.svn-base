#!/usr/sbin/dtrace -Zs
#pragma D option switchrate=10
/*
 Monitors time to import media files, providing a breakdown for file scanning, 
 library item creation, and metadata scanning.

 Run with "sudo ./time-media-import.d -p<PID>"
*/

dtrace:::BEGIN
{
  printf("Target pid: %d\n", $target);
}


pid$target::*sbFileScan??:entry
{
  printf("\nFile Scan Started\n");
  fileStart = timestamp;
}


pid$target::sbLocalDatabaseLibrary*BatchCreateMediaItemsAsync*:entry
{
  fileTime = timestamp - fileStart;
  fileTimeInSeconds = fileTime / 1000000000;  
  printf("\nFileScan Finished - %d seconds, %d nanoseconds\n", fileTimeInSeconds, fileTime);

  printf("\nItem Create/Hashing Started\n");
  batchCreateStart = timestamp;
}
pid$target::sbLocalDatabaseLibrary*CompleteBatchCreateMediaItems*:return
{
  batchCreateTime = timestamp - batchCreateStart;
  batchCreateTimeInSeconds = batchCreateTime / 1000000000;  
  printf("\nItem Create/Hashing Finished - %d seconds, %d nanoseconds\n", batchCreateTimeInSeconds, batchCreateTime);
}


pid$target::sbMetadataJob??Init(*:entry
{
  printf("\nJob Started");
  jobStart = timestamp;
}
pid$target::sbMetadataJob*FinishJob*:return
{
  jobComplete = timestamp;
  jobTime = jobComplete - jobStart;
  jobTimeInSeconds = jobTime / 1000000000;  
  
  totalTime = jobComplete - fileStart;
  totalTimeInSeconds = totalTime / 1000000000;  
  
  printf("\nJob Finished - %d seconds, %d nanoseconds\n", jobTimeInSeconds, jobTime);
  printf("\nComplete import time - %d seconds, %d nanoseconds\n", totalTimeInSeconds, totalTime);
}

