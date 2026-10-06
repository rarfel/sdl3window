# SDL3 Window
Simple SDL3 window written in c++, a template for future projects.  
It contains two bash scripts, one for the first build `setup.sh` and the other to run `make.sh`, simply type in the terminal `sh setup.sh` to create the build directory, and after building `sh make.sh` whenever you want to run it.  
## vendor
The vendor directory holds all the external dependencies the code needs to run, and all of then have to get cloned from source to work.  
There is a bash script to clone all the dependencies from source (`sh dependencies.sh`), but if you dont want the latest version, you can go to the sources yourself and download the source code into vendor. Just be sure to delete the empty folder so it can clone properly.  
### ImGui
The ImGui folder contains both the normal SDL3 renders, vulkan and opengl backends, most of the backends behave similary, so the helper functions in the `imguiMenu.cpp` accept the render implementation directly and only simple modifications are needed to make it work.
### Vulkan
I created this project to learn the vulkan API, I'm currently following <a href="https://www.youtube.com/watch?v=DC9FBRQKNck">this</a> tutorial.  
## debug
If sometime during development you need to debug the code, you can use `sh debug.sh` to create a debug folder, it uses gdb, so be aware of that.  
