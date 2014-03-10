Dependencies
============
Some dependencies (the small ones) are automatically built by the
third_party/fetch_and_build.sh script :

- figtree
- google mock
- glog

Additionally, the following libraries are required :

- OpenCV

Build instructions
------------------

  cd third_party
  ./fetch_and_build.sh
  cd ..
  ./gen_ninja.sh
  ./build.sh

Use the run script to run a binary (it sets up the correct LD_LIBRARY_PATH) :

  ./run.sh out/Default/tests
  ./run.sh out/Default/interactive

References
==========
[Bai09] "Geodesic Matting: A Framework for Fast Interactive Image
         and Video Segmentation and Matting", Int J Comput Vis'09
