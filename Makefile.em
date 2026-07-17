CXX = em++
CXXFLAGS = -std=c++20 -s USE_SDL_MIXER=2 -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_LIBJPEG=1 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png","jpg"]' -s NO_DISABLE_EXCEPTION_CATCHING 
LIBS_PATH = $(HOME)/emscripten-libs
ZLIB_INCLUDE = -s USE_ZLIB=1
PNG_INCLUDE = -s USE_LIBPNG=1
MX_INCLUDE = -I$(LIBS_PATH)/mx2/include -I/usr/include/glm
ZLIB_LIB = -s USE_ZLIB=1
PNG_LIB = -s USE_LIBPNG=1
LIBMX_LIB = $(LIBS_PATH)/mx2/lib/libmx.a 
PRELOAD = --preload-file data
ASYNC_FLAGS= -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536
SOURCES = graphics.cpp
OBJECTS = $(SOURCES:.cpp=.o)
OUTPUT = MX_app.html
SHADER_CACHE_SCRIPT = cache_webgl_shaders.pl
SHADER_INDEX = data/shaders/index.txt
SHADER_SOURCES = $(filter-out data/shaders/fractal _noise.glsl,$(wildcard data/shaders/*.glsl)) data/shaders/fractal\ _noise.glsl
SHADER_CACHE_INDEX = data/shaders/webgl_cache/index.txt

.PHONY: all clean install

all: $(SHADER_CACHE_INDEX) $(OUTPUT)

$(SHADER_CACHE_INDEX): $(SHADER_CACHE_SCRIPT) $(SHADER_INDEX) $(SHADER_SOURCES)
	perl $(SHADER_CACHE_SCRIPT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(MX_INCLUDE) $(ZLIB_INCLUDE) $(PNG_INCLUDE) -c $< -o $@

$(OUTPUT): $(OBJECTS) $(SHADER_CACHE_INDEX)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(OUTPUT) $(PRELOAD)  -s USE_SDL=2 -s USE_LIBJPEG=1 -s USE_SDL_IMAGE=2 -s USE_SDL_MIXER=2 -s SDL2_IMAGE_FORMATS='["png","jpg"]' -s USE_SDL_TTF=2 $(LIBMX_LIB) $(PNG_LIB) $(ZLIB_LIB) -s ALLOW_MEMORY_GROWTH -s ASSERTIONS -s ENVIRONMENT=web -s USE_WEBGL2=1 -s FULL_ES3 -s USE_SDL_MIXER=2 -lembind -s EXPORTED_RUNTIME_METHODS=['HEAPU8','FS'] -s EXPORTED_FUNCTIONS=['_malloc','_free','_main'] -s OFFSCREEN_FRAMEBUFFER=1 $(ASYNC_FLAGS)

clean:
	rm -f *.o $(OUTPUT) *.wasm *.js *.data
