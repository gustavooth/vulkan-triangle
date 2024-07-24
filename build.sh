echo "Compiling shaders..."

glslc shader/triangle.vert -o app/shader/triangle.vert.spv
glslc shader/triangle.frag -o app/shader/triangle.frag.spv

echo "Compiling triangle..."

buildf="build"
if [ ! -d "$buildf" ]; then
    mkdir "$buildf"
fi

cd $buildf
cmake -G Ninja ..
ninja