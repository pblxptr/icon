# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/eblxptr/Drafts/zmq_boost

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/eblxptr/Drafts/zmq_boost/build

# Include any dependencies generated for this target.
include CMakeFiles/zmq_boost.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/zmq_boost.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/zmq_boost.dir/flags.make

CMakeFiles/zmq_boost.dir/main.cpp.o: CMakeFiles/zmq_boost.dir/flags.make
CMakeFiles/zmq_boost.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/eblxptr/Drafts/zmq_boost/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/zmq_boost.dir/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/zmq_boost.dir/main.cpp.o -c /home/eblxptr/Drafts/zmq_boost/main.cpp

CMakeFiles/zmq_boost.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/zmq_boost.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/eblxptr/Drafts/zmq_boost/main.cpp > CMakeFiles/zmq_boost.dir/main.cpp.i

CMakeFiles/zmq_boost.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/zmq_boost.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/eblxptr/Drafts/zmq_boost/main.cpp -o CMakeFiles/zmq_boost.dir/main.cpp.s

# Object files for target zmq_boost
zmq_boost_OBJECTS = \
"CMakeFiles/zmq_boost.dir/main.cpp.o"

# External object files for target zmq_boost
zmq_boost_EXTERNAL_OBJECTS =

zmq_boost: CMakeFiles/zmq_boost.dir/main.cpp.o
zmq_boost: CMakeFiles/zmq_boost.dir/build.make
zmq_boost: CMakeFiles/zmq_boost.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/eblxptr/Drafts/zmq_boost/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable zmq_boost"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/zmq_boost.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/zmq_boost.dir/build: zmq_boost

.PHONY : CMakeFiles/zmq_boost.dir/build

CMakeFiles/zmq_boost.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/zmq_boost.dir/cmake_clean.cmake
.PHONY : CMakeFiles/zmq_boost.dir/clean

CMakeFiles/zmq_boost.dir/depend:
	cd /home/eblxptr/Drafts/zmq_boost/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/eblxptr/Drafts/zmq_boost /home/eblxptr/Drafts/zmq_boost /home/eblxptr/Drafts/zmq_boost/build /home/eblxptr/Drafts/zmq_boost/build /home/eblxptr/Drafts/zmq_boost/build/CMakeFiles/zmq_boost.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/zmq_boost.dir/depend

