libseg
======
An interactive image foreground/background segmentation library.

For now, this is an implementation of geodesic matting of [Bai09]. It doesn't
implement the alpha matting part and just stops at the foreground/background
segmentation stage.

This library can be build for the Linux/OSX and for android.

Its only dependency is the FIGTree library[1]. A version adapted for android
(and still working on the desktop) is available here :
TODO: include figtree-android


Linux/OSX
---------
Linux/OSX build files and scripts are under unix/. Gyp (generate your project)
and ninja are used to build the projects.

Note that the Linux/OSX example programs require OpenCV. libseg itself
doesn't depend on OpenCV.

Build instruction :

  cd third_party/gmock-1.7.0
  ./configure
  make

  cd unix
  # Point FIGTREE to figtree-android
  export FIGTREE=../../figtree-android ./gen_ninja.sh
  ./build.sh

Android
-------
Android build files are under jni/.

Build instructions
------------------

Use the run script to run a binary (it sets up the correct LD_LIBRARY_PATH) :

  ./run.sh out/Default/tests
  ./run.sh out/Default/interactive

References
==========
[1] http://www.umiacs.umd.edu/~morariu/figtree/

[Bai09] "Geodesic Matting: A Framework for Fast Interactive Image
         and Video Segmentation and Matting", Int J Comput Vis'09
