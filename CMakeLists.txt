# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
execute_process(
  COMMAND xcrun --show-sdk-path
  OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND xcode-select -p
  OUTPUT_VARIABLE CMAKE_XCODE_TOOLCHAIN_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

project(MediaProcessor)

# 获得版本号
file(READ "version" VERSION_STRING)
string(STRIP "${VERSION_STRING}" VERSION_STRING)
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" VERSION_MATCH "${VERSION_STRING}")
set(MAJOR_VERSION "${CMAKE_MATCH_1}")
set(MIDDLE_VERSION "${CMAKE_MATCH_2}")
set(MINOR_VERSION "${CMAKE_MATCH_3}")
add_definitions(-DMAJOR_VERSION=${MAJOR_VERSION})
add_definitions(-DMIDDLE_VERSION=${MIDDLE_VERSION})
add_definitions(-DMINOR_VERSION=${MINOR_VERSION})

# 获得GIT信息
execute_process(
  COMMAND git log --format="[sha1]:%h [author]:%cn [time]:%ci [commit]:%s [branch]:%d" -1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE VERSION_SHA1
  OUTPUT_STRIP_TRAILING_WHITESPACE  # 去掉末尾换行符
  ERROR_QUIET                      # 静默错误（如非 git 仓库）
)
add_compile_definitions(GIT_SHA1="${VERSION_SHA1}")

# 配置
add_definitions(-DSPDLOG_USE_STD_FORMAT)
include(${PROJECT_SOURCE_DIR}/option.cmake)
include(${PROJECT_SOURCE_DIR}/flags.cmake)

# 加入代码
set(CORE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/core)
set(ACODEC_SOURCE_DIR ${PROJECT_SOURCE_DIR}/acodecs)
set(VCODEC_SOURCE_DIR ${PROJECT_SOURCE_DIR}/vcodecs)
set(THIRD_PARTY_SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(UTILS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/utils)
set(FILTER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/filter)

include(${ACODEC_SOURCE_DIR}/acodecs.cmake)
include(${CORE_SOURCE_DIR}/core.cmake)
include(${THIRD_PARTY_SOURCE_DIR}/thirdparty.cmake)
# include(${UTILS_SOURCE_DIR}/utils.cmake)

# 编译
include_directories(${MP_INCLUDE_DIR})
link_directories(${MP_LINK_DIRECTORIES})

message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")
message(STATUS "----------------------------------------")
message(STATUS "CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Debug Flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "Release Flags: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "========================================")

# 创建静态库
add_library(mediaprocessor STATIC ${MP_SOURCE_FILES})
target_compile_definitions(mediaprocessor PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)
target_link_libraries(mediaprocessor ${MP_LINK_LIBRARIES})
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # 创建共享库
  add_library(mediaprocessor_shared SHARED ${MP_SOURCE_FILES})
  set_target_properties(mediaprocessor_shared PROPERTIES OUTPUT_NAME "mediaprocessor")
  target_link_libraries(mediaprocessor_shared ${MP_LINK_DIRECTORIES})
endif()

if(BUILD_MPROC_TOOLS)
  add_subdirectory(${PROJECT_SOURCE_DIR}/tools)
endif()
