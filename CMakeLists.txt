cmake_minimum_required(VERSION 3.11)

#
# Project details
#

project(
  "Project"
  VERSION 0.1.0 
  LANGUAGES CXX
)

#
# Set project options
#

include(cmake/StandardSettings.cmake)
include(cmake/Utils.cmake)
message(STATUS "Started CMake for ${PROJECT_NAME} v${PROJECT_VERSION}...\n")

#
# Setup alternative names
#

if(${PROJECT_NAME}_USE_ALT_NAMES)
	string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
	string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
else()
	set(PROJECT_NAME_LOWERCASE ${PROJECT_NAME})
	set(PROJECT_NAME_UPPERCASE ${PROJECT_NAME})
endif()

#
# Prevent building in the source directory
#

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.\n")
endif()

#
# Enable package managers
#

include(cmake/Conan.cmake)
include(cmake/Vcpkg.cmake)

#
# Create library, setup header and source files
#

# Find all headers and implementation files
include(cmake/SourcesAndHeaders.cmake)

if(${PROJECT_NAME}_BUILD_EXECUTABLE)
  add_executable(${PROJECT_NAME} ${exe_sources})
  add_library(${PROJECT_NAME}_LIB ${headers} ${sources})
elseif(${PROJECT_NAME}_BUILD_HEADERS_ONLY)
  add_library(${PROJECT_NAME} INTERFACE)
else()
  add_library(
    ${PROJECT_NAME}
    ${headers}
    ${sources}
  )
endif()

verbose_message("Found the following header files:")
verbose_message(${headers})
verbose_message("Found the following source files:")
if(${PROJECT_NAME}_BUILD_EXECUTABLE)
	verbose_message(${exe_sources})
else()
	verbose_message(${sources})
endif()
message(STATUS "Added all header and implementation files.\n")

#
# Set the project standard and warnings
#

if(${PROJECT_NAME}_BUILD_HEADERS_ONLY)
  target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
else()
  target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
endif()
include(cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})

verbose_message("Applied compiler warnings. Using standard ${CXX_STANDARD}.\n")

#
# Enable Doxygen
#

include(cmake/Doxygen.cmake)

#
# Set the build/user include directories
#

# Allow usage of header files in the `src` directory, but only for utilities
if(${PROJECT_NAME}_BUILD_HEADERS_ONLY)
  target_include_directories(
    ${PROJECT_NAME}
    INTERFACE 
      $<INSTALL_INTERFACE:include>    
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
else()
  target_include_directories(
    ${PROJECT_NAME}
    PUBLIC 
      $<INSTALL_INTERFACE:include>    
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
	if(${PROJECT_NAME}_BUILD_EXECUTABLE)
		target_include_directories(
			${PROJECT_NAME}_LIB
			PUBLIC 
				$<INSTALL_INTERFACE:include>    
				$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
			PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/src
		)
	endif()
endif()

message(STATUS "Finished setting up include directories.")

#
# Model project dependencies 
#

find_package(BZip2 REQUIRED)
find_package(ZLIB)
find_package(Boost 1.62 REQUIRED COMPONENTS
             date_time filesystem iostreams)



#find_package(LibArchive REQUIRED "libraries/libarchive")
find_path(LibArchive_INCLUDE_DIR
  NAMES archive.h
  PATHS
  "/home/criley38/.local/include"
  DOC "libarchive include directory"
  )

find_library(LibArchive_LIBRARY
  NAMES archive libarchive
  PATHS
  "/home/criley38/.local/lib"
  DOC "libarchive library"
  )

mark_as_advanced(LibArchive_INCLUDE_DIR LibArchive_LIBRARY)
message(${LibArchive_LIBRARY})
message(${LibArchive_INCLUDE_DIR})
# Extract the version number from the header.
if(LibArchive_INCLUDE_DIR AND EXISTS "${LibArchive_INCLUDE_DIR}/archive.h")
  # The version string appears in one of three known formats in the header:
  #  #define ARCHIVE_LIBRARY_VERSION "libarchive 2.4.12"
  #  #define ARCHIVE_VERSION_STRING "libarchive 2.8.4"
  #  #define ARCHIVE_VERSION_ONLY_STRING "3.2.0"
  # Match any format.
  set(_LibArchive_VERSION_REGEX "^#define[ \t]+ARCHIVE[_A-Z]+VERSION[_A-Z]*[ \t]+\"(libarchive +)?([0-9]+)\\.([0-9]+)\\.([0-9]+)[^\"]*\".*$")
  file(STRINGS "${LibArchive_INCLUDE_DIR}/archive.h" _LibArchive_VERSION_STRING LIMIT_COUNT 1 REGEX "${_LibArchive_VERSION_REGEX}")
  if(_LibArchive_VERSION_STRING)
    string(REGEX REPLACE "${_LibArchive_VERSION_REGEX}" "\\2.\\3.\\4" LibArchive_VERSION "${_LibArchive_VERSION_STRING}")
  endif()
  unset(_LibArchive_VERSION_REGEX)
  unset(_LibArchive_VERSION_STRING)
endif()

find_package_handle_standard_args(LibArchive
                                  REQUIRED_VARS LibArchive_LIBRARY LibArchive_INCLUDE_DIR
                                  VERSION_VAR LibArchive_VERSION
  )
unset(LIBARCHIVE_FOUND)

if(LibArchive_FOUND)
  set(LibArchive_INCLUDE_DIRS ${LibArchive_INCLUDE_DIR})
  set(LibArchive_LIBRARIES    ${LibArchive_LIBRARY})

  if (NOT TARGET LibArchive::LibArchive)
    add_library(LibArchive::LibArchive UNKNOWN IMPORTED)
    set_target_properties(LibArchive::LibArchive PROPERTIES
      IMPORTED_LOCATION "${LibArchive_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${LibArchive_INCLUDE_DIR}")
  endif ()
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${LibArchive_INCLUDE_DIR})
include_directories("/home/criley38/filter-data-cpp/libraries/gzip-hpp/include")
# Identify and link with the specific "packages" the project uses
#find_package(package_name package_version REQUIRED package_type [other_options])
#target_link_libraries(
#  ${PROJECT_NAME}
#  PUBLIC
#    dependency1 ...
#  PRIVATE
#    dependency2 ...
#    ${PROJECT_NAME}_PROJECT_OPTIONS        
#    ${PROJECT_NAME}_PROJECT_WARNINGS        
#)
target_link_libraries(${PROJECT_NAME} bz2)
target_link_libraries(${PROJECT_NAME} Boost::iostreams)
target_link_libraries(${PROJECT_NAME} ${LibArchive_LIBRARY})
target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
verbose_message("Successfully added all dependencies and linked against them.")

#
# Provide alias to library for 
#

if(${PROJECT_NAME}_BUILD_EXECUTABLE)
  add_executable(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
else()
  add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
endif()

verbose_message("Project is now aliased as ${PROJECT_NAME}::${PROJECT_NAME}.\n")

#
# Format the project using the `clang-format` target (i.e: cmake --build build --target clang-format)
#

add_clang_format_target()

#
# Install library for easy downstream inclusion
#

include(GNUInstallDirs)
install(
  TARGETS
    ${PROJECT_NAME}
  EXPORT
    ${PROJECT_NAME}Targets
  LIBRARY DESTINATION
    ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION
    ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION
    include
  PUBLIC_HEADER DESTINATION
    include
)

install(
  EXPORT 
    ${PROJECT_NAME}Targets
  FILE
    ${PROJECT_NAME}Targets.cmake
  NAMESPACE
    ${PROJECT_NAME}::
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

#
# Add version header
#

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/version.hpp.in
    include/${PROJECT_NAME_LOWERCASE}/version.hpp
    @ONLY
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME_LOWERCASE}/version.hpp
  DESTINATION
    include/${PROJECT_NAME_LOWERCASE}
)

#
# Install the `include` directory
#

install(
  DIRECTORY
    include/${PROJECT_NAME_LOWERCASE}
  DESTINATION
    include
)

verbose_message("Install targets succesfully build. Install with `cmake --build <build_directory> --target install --config <build_config>`.")

#
# Quick `ConfigVersion.cmake` creation
#

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${PROJECT_NAME}ConfigVersion.cmake
  VERSION
    ${PROJECT_VERSION}
  COMPATIBILITY
    SameMajorVersion
)

configure_package_config_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION 
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

#
# Generate export header if specified
#

if(${PROJECT_NAME}_GENERATE_EXPORT_HEADER)
  include(GenerateExportHeader)
  generate_export_header(${PROJECT_NAME})
  install(
    FILES
      ${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWERCASE}_export.h 
    DESTINATION
      include
  )

message(STATUS "Generated the export header `${PROJECT_NAME_LOWERCASE}_export.h` and installed it.")
endif()

message(STATUS "Finished building requirements for installing the package.\n")

