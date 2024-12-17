cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)
cmake_policy(VERSION 3.14.7...3.28.1)

# Debug command
# cmake .. -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles"

set(PROJECT_NAME gourdin)

project(${PROJECT_NAME})

set(EXTERNAL_DIR ${CMAKE_BINARY_DIR}/external)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Make sure all configurations output libraries in the same directory instead of in sub-directories per config
# This allows easier linking from the Xcode iOS project
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

if (IOS)
  # Disable ImageIO for SDL2_image as it's not available for iOS, just OS X, and will fail at link time
  set(SDL2IMAGE_BACKEND_IMAGEIO 0)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release
      RelWithDebInfo MinSizeRel."
      FORCE)
endif()

if (NOT ANDROID AND NOT IOS)
  # handling warnings
  if(MSVC)
    add_definitions( -DNOMINMAX )
    # Force to always compile with W4
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
      string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
  elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # Update if necessary
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-switch -pedantic")
  endif()

  set (CMAKE_CXX_STANDARD 11)
  set(EXECUTABLE_OUTPUT_PATH bin/${CMAKE_BUILD_TYPE})

  # Move the built shared libraries to the executable location
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${EXECUTABLE_OUTPUT_PATH})

  find_package(OpenGL)
endif()

################################################################################
### Fetching external content
################################################################################

include(FetchContent)

if (WIN32)
  SET(BUILD_UTILS OFF)
  FetchContent_Declare(
    GLEW
    URL            https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.zip
    SOURCE_DIR     ${EXTERNAL_DIR}/glew
  )
  FetchContent_GetProperties(glew)
  if(NOT glew_POPULATED)
    FetchContent_Populate(glew)
    add_subdirectory(${EXTERNAL_DIR}/glew/build/cmake)
  endif()
endif()

# We need to get the version directly from brew to load PNG files correctly
if (IOS OR NOT APPLE)
  FetchContent_Declare(
    SDL
    URL            https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.28.5.zip
    SOURCE_DIR     ${EXTERNAL_DIR}/SDL2
  )

  FetchContent_Declare(
    SDL_IMAGE
    URL            https://github.com/libsdl-org/SDL_image/archive/release-2.8.1.zip
    SOURCE_DIR     ${EXTERNAL_DIR}/SDL2_image
  )
endif()

FetchContent_Declare(
  LIB_SDL2_PP
  URL            https://github.com/libSDL2pp/libSDL2pp/archive/0.16.0.zip
  SOURCE_DIR     ${EXTERNAL_DIR}/libSDL2pp
)

FetchContent_Declare(
  GLM
  URL            https://github.com/g-truc/glm/releases/download/0.9.9.7/glm-0.9.9.7.zip
  SOURCE_DIR     ${EXTERNAL_DIR}/glm
)

# Build SDL2 and SDL2_image with CMake
if (IOS OR NOT APPLE)
  # Do not build the static version of SDL
  set(BUILD_SHARED_LIBS ON)

  set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
  set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
  set(SDL2IMAGE_DEPS_SHARED OFF CACHE BOOL "Building png and zlib statically to avoid linking problems on android" FORCE)
  set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
  set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)

  # Make sure debug libs have the same name as release ones
  set(SDL_CMAKE_DEBUG_POSTFIX "")
  set(SDL2IMAGE_DEBUG_POSTFIX "")

  # SDL2pp needs defining where the SDL2 libraries are
  set(SDL2_LIBRARY SDL2)
  set(SDL2MAIN_LIBRARY SDL2main)
  set(SDL2_INCLUDE_DIR ${EXTERNAL_DIR}/SDL2/include)

  set(SDL2_IMAGE_LIBRARY SDL2_image)
  set(SDL2_IMAGE_INCLUDE_DIR ${EXTERNAL_DIR}/SDL2_image/include)

else()
  find_package(PkgConfig REQUIRED)

  # Hack needed for linux because the pkg-config module is lower case for some reason
  if (UNIX AND NOT APPLE)
    pkg_check_modules(sdl2 REQUIRED sdl2 SDL2_image)
  else()
    pkg_check_modules(SDL2 REQUIRED SDL2 SDL2_image)
  endif()
endif()

set(SDL2PP_WITH_IMAGE ON)

# Fixing the build of SDL on VS2019
if (MSVC)
  list(APPEND EXTRA_LIBS vcruntime)
endif()


if (IOS OR NOT APPLE)
  FetchContent_MakeAvailable(SDL SDL_IMAGE)

  # Making sure libraries don't generate symlinks
  set_property(TARGET SDL2 PROPERTY VERSION)
  set_property(TARGET SDL2 PROPERTY SOVERSION)
  set_property(TARGET SDL2_image PROPERTY VERSION)
  set_property(TARGET SDL2_image PROPERTY SOVERSION)
endif()
FetchContent_MakeAvailable(LIB_SDL2_PP)

# Don't add GLM as a subdirectory, since we just want to include it
FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
  FetchContent_Populate(glm)
endif()

# Copying needed files for the android project
if (ANDROID)
  # SDL activity
  set(SDL_ACTIVITY_FOLDER app/src/main/java/org/libsdl/app/)
  file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/android/${SDL_ACTIVITY_FOLDER})
  file(COPY ${EXTERNAL_DIR}/SDL2/android-project/${SDL_ACTIVITY_FOLDER} DESTINATION ${CMAKE_SOURCE_DIR}/android/${SDL_ACTIVITY_FOLDER})

  # Assets
  file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/res)
  file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_SOURCE_DIR}/android/app/src/main/assets)
  # Shaders
  file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/src/shaders)
  file(COPY ${CMAKE_SOURCE_DIR}/src/shaders DESTINATION ${CMAKE_SOURCE_DIR}/android/app/src/main/assets/src)
endif()

################################################################################
### End fetching external content
################################################################################

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(INCLUDE_DIRS
  ${OPENGL_INCLUDE_DIRS}
  ${EXTERNAL_DIR}/glm
  ${EXTERNAL_DIR}/glew/include
  ${SDL2PP_INCLUDE_DIRS}
)


# Find sources recursively
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "src/*.h")
file(GLOB_RECURSE DEBUG_SOURCE_FILES "src/*.cc")
file(GLOB_RECURSE DEBUG_HEADER_FILES "src/*.hpp")

# Add the folders with headers to the include path
foreach (_headerFile ${HEADER_FILES})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND INCLUDE_DIRS ${_dir})
endforeach()

# Only add debug classes if we are in debug mode
if ( CMAKE_BUILD_TYPE MATCHES Debug OR MSVC)
  foreach (_headerFile ${DEBUG_HEADER_FILES})
      get_filename_component(_dir ${_headerFile} PATH)
      list (APPEND INCLUDE_DIRS ${_dir})
  endforeach()
endif()

list(REMOVE_DUPLICATES INCLUDE_DIRS)

# Add external libraries added manually to the build workflow
file(GLOB_RECURSE EXTERNAL_SOURCE_FILES "external_manual/*.cpp")
list(APPEND SOURCE_FILES ${EXTERNAL_SOURCE_FILES})

if ( CMAKE_BUILD_TYPE MATCHES Debug OR MSVC)
  list(APPEND SOURCE_FILES ${DEBUG_SOURCE_FILES})
  list(APPEND HEADER_FILES ${DEBUG_HEADER_FILES})
endif()

foreach (_sourceFile ${EXTERNAL_SOURCE_FILES})
    get_filename_component(_dir ${_sourceFile} PATH)
    list (APPEND INCLUDE_DIRS ${_dir})
endforeach()

# Add font header files
list (APPEND INCLUDE_DIRS "external_manual/fonts/")

include_directories(${INCLUDE_DIRS})

if (IOS) 
  # Remove main.cpp as it's manually added to the XCode project directly as the entry point
  list(REMOVE_ITEM SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/main.cpp)
endif()

if(ANDROID OR IOS)
  add_library( ${PROJECT_NAME} SHARED ${SOURCE_FILES} )

  target_link_libraries(${PROJECT_NAME}
    GLESv3
    log
    ${SDL2_LIBRARY}
    ${SDL2_IMAGE_LIBRARY}
    ${SDL2PP_LIBRARIES}
  )

else()
  add_executable(${PROJECT_NAME} ${SOURCE_FILES})

  if (WIN32)
    target_link_libraries(${PROJECT_NAME}
      ${OPENGL_LIBRARIES}
      glew_s
      ${SDL2_LIBRARY}
      ${SDL2_IMAGE_LIBRARY}
      ${SDL2PP_LIBRARIES}
    )

    if (MSVC) # Setup the VS solution file correctly
      set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
      set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    endif()
  else()
    target_link_libraries(${PROJECT_NAME}
      ${OPENGL_LIBRARIES}
      ${SDL2_LIBRARY}
      ${SDL2_IMAGE_LIBRARY}
      ${SDL2PP_LIBRARIES}
    )
  endif()
endif()

# Generate file for clang-autocomplete and clang-linter in atom

message(STATUS "Generating ${CMAKE_SOURCE_DIR}/.clang_complete")
file(WRITE ${CMAKE_SOURCE_DIR}/.clang_complete "")
foreach(dir ${INCLUDE_DIRS})
  file(APPEND ${CMAKE_SOURCE_DIR}/.clang_complete "-I${dir}\n")
endforeach()

if (${CMAKE_CXX_FLAGS})
  string(REPLACE "'" "" CMAKE_CXX_FLAGS_SPLIT ${CMAKE_CXX_FLAGS})
  string(REPLACE " " ";" CMAKE_CXX_FLAGS_SPLIT ${CMAKE_CXX_FLAGS_SPLIT})
  foreach(flag ${CMAKE_CXX_FLAGS_SPLIT})
    file(APPEND ${CMAKE_SOURCE_DIR}/.clang_complete "${flag}\n")
  endforeach()
endif()
