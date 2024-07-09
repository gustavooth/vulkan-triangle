@echo off

echo Compiling shaders...

glslc shader/triangle.vert -o app/shader/triangle.vert.spv
glslc shader/triangle.frag -o app/shader/triangle.frag.spv

echo Compiling triangle...

set "buildf=build"
if not exist "%buildf%" (
    mkdir "%buildf%"
)

cd %buildf%
cmake -G Ninja ..
ninja