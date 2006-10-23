
function run_test () {
  do_load_songbird();
  dump("Hello World\n");

  // This doesn't work yet. Songbird's component interfaces aren't getting added to xpti.dat 
  //const testSB_NewDataRemote = new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1", "sbIDataRemote", "init");
  //var dr = testSB_NewDataRemote("foo", null);
}

