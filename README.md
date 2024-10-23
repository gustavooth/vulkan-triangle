# Vulkan Triangle

<img src="images/screenshot.png?raw=true" alt="Screenshot" height=300/>

## Build setup

If you use vscode, make sure the files `.vscode/c_cpp_properties.json` `CMakeLists.txt` are configured correctly.

### Requirements for Linux

Install these via package manager:

 - `git`
 - `make`
 - `gdb` // for debugging
 - `vulkan-devel`

 ### Requirements for Windows

 - Git: https://git-scm.com/download/windows
 - Clang: https://releases.llvm.org
 - Make: https://gnuwin32.sourceforge.net/packages/make.htm
 - msvc: install Visual Studio
 - Windows SDK: install Visual Studio:
 - Vulkan SDK: https://vulkan.lunarg.com/sdk/home

## Build

 - Run command  `make -f Makefile.linux.mak`.

## Run

 - Run command  `make -f Makefile.linux.mak run`.
