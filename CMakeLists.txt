cmake_minimum_required(VERSION 3.1.0)

project(GitWatch)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
   set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

set(CMAKE_BUILD_TYPE Debug)

if (MSVC)
  set(CMAKE_VS_SDK_INCLUDE_DIRECTORIES ${CUSTOM_INCLUDE_DIRECTORIES})
else()
  set(CMAKE_CXX_FLAGS_DEBUG "-g -static -O0 -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-g -static -O3 -Wall -Wextra")
  # Include Directories
  foreach(LANG  C CXX)
    set(CMAKE_${LANG}_STANDARD_INCLUDE_DIRECTORIES ${CUSTOM_INCLUDE_DIRECTORIES})
  endforeach()
  find_library(comctl32
    NAMES libcomctl32.a
    PATHS ${win32_lib_dir}
  )
  set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -static")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

set ( GITWATCH_SVC_SRC
  src/GitWatch.cc 
  src/GitWatchSvc.cc
  src/GitWatchSvcHelper.cc
  src/GitWatchSvcEntry.cc
  src/GitWatchUtil.cc
  src/DirDiff.cc
  src/MemoryLeaks.cc
  src/Logger.cc
)

set ( GITWATCH_GUI_SRC
  src/GitWatchGui.cc
  src/GitWatchUtil.cc
  src/GitWatchSvc.cc
  src/GitWatchSvcHelper.cc
  src/Logger.cc
)

set ( MAIN_HEADERS
  include/GitWatch.h
  include/GitWatchUtil.h
  include/GitWatchSvcHelper.h
  include/GitWatchSvc.h
  include/MemoryLeaks.h
  include/DirDiff.h
  include/logger.h
)

set ( MAIN_RESOURCES
  resource/Resource.rc
  resource/icons/GitWatchSmall.ico
  resource/icons/GitWatchRegular.ico
)
 
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(GitWatchSvc ${GITWATCH_SVC_SRC} ${MAIN_HEADERS})
add_executable(GitWatchGui ${GITWATCH_GUI_SRC} ${MAIN_HEADERS} ${MAIN_RESOURCES})

target_link_libraries(GitWatchGui ${comctl32})
#target_link_libraries(GitWatchSvc)
