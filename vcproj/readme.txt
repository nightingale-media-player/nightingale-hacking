

  This directory hosts locally generated Visual Studio IDE projects that reflect
  the Songbird source tree. 

  Please note that these projects WILL *NOT* BUILD! They are only intended for users
  of Visual Studio as a convenient way to access any of the project's files from
  within the IDE.

  To create, or update a Visual Studio project, use the following command line:

  make-project.sh VCVERSION

  where VCVERSION is 7, 8, or 9 depending on which version of the Visual C++ Ide
  you want to build projects for.

  eg.

  make-project.sh 8

  will create or update a VC8 (Visual Studio 2005) project file.


  Note: For VC8 and above, you have to make sure that the Solution Explorer Mode is
  set to "Show only files in project". For VC8, this setting is found under Tools 
  -> Options -> Projects and Solutions -> VC++ Project Settings.