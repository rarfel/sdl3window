# SDL3 Window
Simple SDL3 window written in c++, a template for future projects.  
It contains two bash scripts, one for the first build `setup.sh` and the other to run `make.sh`, simply type in the terminal `sh setup.sh` to create the build directory, and after building `sh make.sh` whenever you want to run it.  
## vendor
The vendor directory holds all the external dependencies the code needs to run, and all of then have to get cloned from source to work.  
There is a bash script to clone all the dependencies from source (`sh dependencies.sh`), but if you dont want the latest version, you can go to the sources yourself and download the source code into vendor. Just be sure to delete the empty folder so it can clone properly.
