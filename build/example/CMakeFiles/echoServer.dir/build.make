# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.27

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
CMAKE_COMMAND = /root/cmake/cmake-3.27.9-linux-x86_64/bin/cmake

# The command to remove a file.
RM = /root/cmake/cmake-3.27.9-linux-x86_64/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /root/CProject/mymuduo

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/CProject/mymuduo/build

# Include any dependencies generated for this target.
include example/CMakeFiles/echoServer.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include example/CMakeFiles/echoServer.dir/compiler_depend.make

# Include the progress variables for this target.
include example/CMakeFiles/echoServer.dir/progress.make

# Include the compile flags for this target's objects.
include example/CMakeFiles/echoServer.dir/flags.make

example/CMakeFiles/echoServer.dir/echoServer.cc.o: example/CMakeFiles/echoServer.dir/flags.make
example/CMakeFiles/echoServer.dir/echoServer.cc.o: /root/CProject/mymuduo/example/echoServer.cc
example/CMakeFiles/echoServer.dir/echoServer.cc.o: example/CMakeFiles/echoServer.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/root/CProject/mymuduo/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object example/CMakeFiles/echoServer.dir/echoServer.cc.o"
	cd /root/CProject/mymuduo/build/example && /opt/rh/devtoolset-8/root/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT example/CMakeFiles/echoServer.dir/echoServer.cc.o -MF CMakeFiles/echoServer.dir/echoServer.cc.o.d -o CMakeFiles/echoServer.dir/echoServer.cc.o -c /root/CProject/mymuduo/example/echoServer.cc

example/CMakeFiles/echoServer.dir/echoServer.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/echoServer.dir/echoServer.cc.i"
	cd /root/CProject/mymuduo/build/example && /opt/rh/devtoolset-8/root/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/CProject/mymuduo/example/echoServer.cc > CMakeFiles/echoServer.dir/echoServer.cc.i

example/CMakeFiles/echoServer.dir/echoServer.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/echoServer.dir/echoServer.cc.s"
	cd /root/CProject/mymuduo/build/example && /opt/rh/devtoolset-8/root/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/CProject/mymuduo/example/echoServer.cc -o CMakeFiles/echoServer.dir/echoServer.cc.s

# Object files for target echoServer
echoServer_OBJECTS = \
"CMakeFiles/echoServer.dir/echoServer.cc.o"

# External object files for target echoServer
echoServer_EXTERNAL_OBJECTS =

/root/CProject/mymuduo/example/echoServer: example/CMakeFiles/echoServer.dir/echoServer.cc.o
/root/CProject/mymuduo/example/echoServer: example/CMakeFiles/echoServer.dir/build.make
/root/CProject/mymuduo/example/echoServer: /root/CProject/mymuduo/lib/libtiny_network.so
/root/CProject/mymuduo/example/echoServer: example/CMakeFiles/echoServer.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/root/CProject/mymuduo/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable /root/CProject/mymuduo/example/echoServer"
	cd /root/CProject/mymuduo/build/example && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/echoServer.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
example/CMakeFiles/echoServer.dir/build: /root/CProject/mymuduo/example/echoServer
.PHONY : example/CMakeFiles/echoServer.dir/build

example/CMakeFiles/echoServer.dir/clean:
	cd /root/CProject/mymuduo/build/example && $(CMAKE_COMMAND) -P CMakeFiles/echoServer.dir/cmake_clean.cmake
.PHONY : example/CMakeFiles/echoServer.dir/clean

example/CMakeFiles/echoServer.dir/depend:
	cd /root/CProject/mymuduo/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/CProject/mymuduo /root/CProject/mymuduo/example /root/CProject/mymuduo/build /root/CProject/mymuduo/build/example /root/CProject/mymuduo/build/example/CMakeFiles/echoServer.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : example/CMakeFiles/echoServer.dir/depend

