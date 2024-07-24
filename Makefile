CFLAGS = -std=c++17 -I. -I$(VULKAN_SDK_PATH)/include -Iimgui/ 
ifeq ($(DEBUG),1)
	CFLAGS := -g3 $(CFLAGS)
endif
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib \
			 $(shell pkgconf --static --libs glfw3) \
			 -lvulkan 

vertSources = $(shell find ./shaders -type f -name "*.vert")
vertObjFiles = $(patsubst %.vert, obj/%.vert.spv, $(vertSources))
fragSources = $(shell find ./shaders -type f -name "*.frag")
fragObjFiles = $(patsubst %.frag, obj/%.frag.spv, $(fragSources))
compSources = $(shell find ./shaders -type f -name "*.comp")
compObjFiles = $(patsubst %.comp, obj/%.comp.spv, $(compSources))
tescSources = $(shell find ./shaders -type f -name "*.tesc")
tescObjFiles = $(patsubst %.tesc, obj/%.tesc.spv, $(tescSources))
teseSources = $(shell find ./shaders -type f -name "*.tese")
teseObjFiles = $(patsubst %.tese, obj/%.tese.spv, $(teseSources))
SRCS = $(shell find -type f -name "*.cpp" -not -path "*/mains/*")
OBJS = $(patsubst ./%.cpp, obj/%.o, $(SRCS))
MAINOUTS = FirstApp SecondApp

$(MAINOUTS): $(OBJS) $(vertObjFiles) $(fragObjFiles) $(compObjFiles) $(tescObjFiles) $(teseObjFiles)
	@mkdir -p bin
	@mkdir -p obj/mains
	g++ $(CFLAGS) -c mains/$@.cpp -o obj/mains/$@.o
	g++ $(CFLAGS) -o bin/$@ obj/mains/$@.o $(OBJS) $(LDFLAGS)
obj/%.o: %.cpp
	@mkdir -p $(@D)
	g++ $(CFLAGS) -c $< -o $@ 

obj/%.spv: %
	@mkdir -p $(@D)
	glslc $< -o $@

.PHONY: test clean

test1: FirstApp
	bin/FirstApp $(ARGS)

test2: SecondApp
	bin/SecondApp $(ARGS)

clean:
	rm -rf bin/
	rm -f shaders/*.spv
	rm -rf obj/
