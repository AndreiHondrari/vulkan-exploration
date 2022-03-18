export SHADERS_LOCATION=shaders

glslc $SHADERS_LOCATION/shader.vert -o $SHADERS_LOCATION/vert.spv
glslc $SHADERS_LOCATION/shader.frag -o $SHADERS_LOCATION/frag.spv
