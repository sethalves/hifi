
Instructions for adding BinaryFaceHMD Tracking to Interface on Windows
Jungwoon Park, 15 Nov 2016.

1.  Copy the SDK folders (include, models, windows) into the interface/externals/binaryfacehmd folder. This readme.txt should be there as well.

   You may optionally choose to copy the SDK folders to a location outside the repository (so you can re-use with different 
   checkouts and different projects). If so, set the ENV variable "HIFI_LIB_DIR" to a directory containing a subfolder 
   "binaryfacehmd" that contains the folders mentioned above.

3. Clear your build directory, run cmake and build, and you should be all set.
