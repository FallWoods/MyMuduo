# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.31

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/fallwoods/CppProject/mymuduo

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/fallwoods/CppProject/mymuduo/build

# Include any dependencies generated for this target.
include example/CMakeFiles/echoServer.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include example/CMakeFiles/echoServer.dir/compiler_depend.make

# Include the progress variables for this target.
include example/CMakeFiles/echoServer.dir/progress.make

# Include the compile flags for this target's objects.
include example/CMakeFiles/echoServer.dir/flags.make

example/CMakeFiles/echoServer.dir/codegen:
.PHONY : example/CMakeFiles/echoServer.dir/codegen

example/CMakeFiles/echoServer.dir/echoServer.cc.o: example/CMakeFiles/echoServer.dir/flags.make
example/CMakeFiles/echoServer.dir/echoServer.cc.o: /home/fallwoods/CppProject/mymuduo/example/echoServer.cc
example/CMakeFiles/echoServer.dir/echoServer.cc.o: example/CMakeFiles/echoServer.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/fallwoods/CppProject/mymuduo/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object example/CMakeFiles/echoServer.dir/echoServer.cc.o"
	cd /home/fallwoods/CppProject/mymuduo/build/example && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT example/CMakeFiles/echoServer.dir/echoServer.cc.o -MF CMakeFiles/echoServer.dir/echoServer.cc.o.d -o CMakeFiles/echoServer.dir/echoServer.cc.o -c /home/fallwoods/CppProject/mymuduo/example/echoServer.cc

example/CMakeFiles/echoServer.dir/echoServer.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/echoServer.dir/echoServer.cc.i"
	cd /home/fallwoods/CppProject/mymuduo/build/example && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/fallwoods/CppProject/mymuduo/example/echoServer.cc > CMakeFiles/echoServer.dir/echoServer.cc.i

example/CMakeFiles/echoServer.dir/echoServer.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/echoServer.dir/echoServer.cc.s"
	cd /home/fallwoods/CppProject/mymuduo/build/example && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/fallwoods/CppProject/mymuduo/example/echoServer.cc -o CMakeFiles/echoServer.dir/echoServer.cc.s

# Object files for target echoServer
echoServer_OBJECTS = \
"CMakeFiles/echoServer.dir/echoServer.cc.o"

# External object files for target echoServer
echoServer_EXTERNAL_OBJECTS =

/home/fallwoods/CppProject/mymuduo/example/echoServer: example/CMakeFiles/echoServer.dir/echoServer.cc.o
/home/fallwoods/CppProject/mymuduo/example/echoServer: example/CMakeFiles/echoServer.dir/build.make
/home/fallwoods/CppProject/mymuduo/example/echoServer: /home/fallwoods/CppProject/mymuduo/lib/libtiny_network.so
/home/fallwoods/CppProject/mymuduo/example/echoServer: example/CMakeFiles/echoServer.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/fallwoods/CppProject/mymuduo/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable /home/fallwoods/CppProject/mymuduo/example/echoServer"
	cd /home/fallwoods/CppProject/mymuduo/build/example && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/echoServer.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
example/CMakeFiles/echoServer.dir/build: /home/fallwoods/CppProject/mymuduo/example/echoServer
.PHONY : example/CMakeFiles/echoServer.dir/build

example/CMakeFiles/echoServer.dir/clean:
	cd /home/fallwoods/CppProject/mymuduo/build/example && $(CMAKE_COMMAND) -P CMakeFiles/echoServer.dir/cmake_clean.cmake
.PHONY : example/CMakeFiles/echoServer.dir/clean

example/CMakeFiles/echoServer.dir/depend:
	cd /home/fallwoods/CppProject/mymuduo/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/fallwoods/CppProject/mymuduo /home/fallwoods/CppProject/mymuduo/example /home/fallwoods/CppProject/mymuduo/build /home/fallwoods/CppProject/mymuduo/build/example /home/fallwoods/CppProject/mymuduo/build/example/CMakeFiles/echoServer.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : example/CMakeFiles/echoServer.dir/depend

