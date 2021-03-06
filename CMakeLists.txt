# Icinga 2
# Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

cmake_minimum_required(VERSION 2.6)
set(BOOST_MIN_VERSION "1.41.0")

project(icinga2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third-party/cmake")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()

option(ICINGA2_WITH_MYSQL "Build the MySQL IDO module" ON)
option(ICINGA2_WITH_PGSQL "Build the PostgreSQL IDO module" ON)
option(ICINGA2_WITH_CHECKER "Build the checker module" ON)
option(ICINGA2_WITH_COMPAT "Build the compat module" ON)
option(ICINGA2_WITH_DEMO "Build the demo module" OFF)
option(ICINGA2_WITH_HELLO "Build the hello module" OFF)
option(ICINGA2_WITH_LIVESTATUS "Build the Livestatus module" ON)
option(ICINGA2_WITH_NOTIFICATION "Build the notification module" ON)
option(ICINGA2_WITH_PERFDATA "Build the perfdata module" ON)
option(ICINGA2_WITH_STUDIO "Build the Icinga Studio application" OFF)
option(ICINGA2_WITH_TESTS "Run unit tests" ON)

file(STRINGS icinga2.spec VERSION_LINE REGEX "^Version: ")
string(REPLACE "Version: " "" ICINGA2_VERSION ${VERSION_LINE})

include(GNUInstallDirs)
include(InstallConfig)

set(ICINGA2_USER "icinga" CACHE STRING "Icinga 2 user")
set(ICINGA2_GROUP "icinga" CACHE STRING "Icinga 2 group")
set(ICINGA2_COMMAND_GROUP "icingacmd" CACHE STRING "Icinga 2 command group")
set(ICINGA2_RUNDIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/run" CACHE STRING "/run directory")
set(ICINGA2_PLUGINDIR "/usr/lib/nagios/plugins" CACHE STRING "Path for the check plugins")
set(ICINGA2_GIT_VERSION_INFO ON CACHE BOOL "Whether to use git describe")
set(ICINGA2_UNITY_BUILD ON CACHE BOOL "Whether to perform a unity build")
set(ICINGA2_LTO_BUILD OFF CACHE BOOL "Whether to use LTO")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/COPYING" ICINGA2_LICENSE_GPL)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.Exceptions" ICINGA2_LICENSE_ADDITIONS)
set(ICINGA2_LICENSE "${ICINGA2_LICENSE_GPL}\n\n---\n\n${ICINGA2_LICENSE_ADDITIONS}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt" ${ICINGA2_LICENSE})

include(GetGitRevisionDescription)
git_describe(GIT_VERSION --tags)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icinga-version.h.force)
  configure_file(icinga-version.h.force ${CMAKE_CURRENT_BINARY_DIR}/icinga-version.h COPYONLY)
else()
  if(NOT ICINGA2_GIT_VERSION_INFO OR GIT_VERSION MATCHES "-NOTFOUND$")
    file(STRINGS icinga2.spec SPEC_VERSION REGEX "^Version:")
    string(LENGTH "${SPEC_VERSION}" SPEC_VERSION_LENGTH)
    math(EXPR SPEC_VERSION_LENGTH "${SPEC_VERSION_LENGTH} - 9")
    string(SUBSTRING ${SPEC_VERSION} 9 ${SPEC_VERSION_LENGTH} SPEC_VERSION)

    file(STRINGS icinga2.spec SPEC_REVISION REGEX "^%define revision ")
    string(LENGTH "${SPEC_REVISION}" SPEC_REVISION_LENGTH)
    math(EXPR SPEC_REVISION_LENGTH "${SPEC_REVISION_LENGTH} - 17")
    string(SUBSTRING ${SPEC_REVISION} 17 ${SPEC_REVISION_LENGTH} SPEC_REVISION)

    set(GIT_VERSION "r${SPEC_VERSION}-${SPEC_REVISION}")
  endif()
  configure_file(icinga-version.h.cmake icinga-version.h)
endif()

if(WIN32)
  set(Boost_USE_STATIC_LIBS ON)
  add_definitions(-DBOOST_ALL_NO_LIB)
endif()

find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS thread system program_options regex REQUIRED)

link_directories(${Boost_LIBRARY_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(YAJL)

if(NOT YAJL_FOUND)
  include_directories(${icinga2_BINARY_DIR}/third-party/yajl/include)
  link_directories(${icinga2_BINARY_DIR}/third-party/yajl)
  set(YAJL_LIBRARIES "yajl")
endif()

find_package(Editline)
set(HAVE_EDITLINE "${EDITLINE_FOUND}")

find_package(Termcap)
set(HAVE_TERMCAP "${TERMCAP_FOUND}")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/lib
)

#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

if(APPLE)
  set(CMAKE_INSTALL_NAME_DIR "@executable_path/..")
  set(CMAKE_MACOSX_RPATH 0)
endif(APPLE)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -g")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "SunPro")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mt")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mt -library=stlport4")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_SYSTEM_NAME MATCHES AIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -g -lpthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -lpthread")
  elseif(CMAKE_SYSTEM_NAME MATCHES "kOpenBSD.*|OpenBSD.*")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pthread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lpthread")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  endif()

  if(NOT CMAKE_SYSTEM_NAME MATCHES AIX AND NOT CMAKE_SYSTEM_NAME MATCHES OpenBSD AND NOT CMAKE_SYSTEM_NAME MATCHES SunOS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,--no-export-dynamic -Bsymbolic-functions -Wl,--dynamic-list-cpp-typeinfo -Wl,--dynamic-list-data")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--no-export-dynamic -Bsymbolic-functions -Wl,--dynamic-list-cpp-typeinfo -Wl,--dynamic-list-data")
  endif()
endif()

if(ICINGA2_LTO_BUILD)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto -fno-fat-lto-objects")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -fno-fat-lto-objects")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")
endif()

include(CheckCCompilerFlag)

check_c_compiler_flag(-fvisibility-inlines-hidden HAVE_VISIBILITY_INLINES_HIDDEN)

if(HAVE_VISIBILITY_INLINES_HIDDEN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility-inlines-hidden")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/Bin/${CMAKE_BUILD_TYPE} CACHE PATH "Library output path")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/Bin/${CMAKE_BUILD_TYPE} CACHE PATH "Executable output path")

include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFileCXX)

check_symbol_exists(__COUNTER__ "" HAVE_COUNTER_MACRO)

if(NOT HAVE_COUNTER_MACRO AND ICINGA2_UNITY_BUILD)
  message(STATUS "Your C/C++ compiler does not support the __COUNTER__ macro. Disabling unity build.")
  set(ICINGA2_UNITY_BUILD FALSE)
endif()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DI2_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DI2_DEBUG")

check_function_exists(vfork HAVE_VFORK)
check_function_exists(backtrace_symbols HAVE_BACKTRACE_SYMBOLS)
check_function_exists(pipe2 HAVE_PIPE2)
check_function_exists(nice HAVE_NICE)
check_library_exists(dl dladdr "dlfcn.h" HAVE_DLADDR)
check_library_exists(execinfo backtrace_symbols "" HAVE_LIBEXECINFO)
check_include_file_cxx(cxxabi.h HAVE_CXXABI_H)

if(HAVE_LIBEXECINFO)
  set(HAVE_BACKTRACE_SYMBOLS TRUE)
endif()

if(UNIX)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-std=c++11" CXX_FLAG_CXX11)
  check_cxx_compiler_flag("-std=c++0x" CXX_FLAG_CXX0X)
  if(CXX_FLAG_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  elseif(CXX_FLAG_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  endif()

  check_cxx_compiler_flag("-Wno-inconsistent-missing-override" CXX_FLAG_NO_OVERRIDE_WARNING)
  if(CXX_FLAG_NO_OVERRIDE_WARNING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")
  endif()
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("class Base { public: virtual void test(void) { } }; class Derived : public Base { virtual void test(void) override { } }; int main(){}" CXX_FEATURE_OVERRIDE)

if(NOT CXX_FEATURE_OVERRIDE)
  add_definitions("-Doverride=")
endif()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h ESCAPE_QUOTES)

install(
  FILES README.md COPYING COPYING.Exceptions AUTHORS ChangeLog NEWS
  DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

include(CTest)
enable_testing()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(third-party)
add_subdirectory(tools)
add_subdirectory(lib)
add_subdirectory(icinga-app)
add_subdirectory(etc)
add_subdirectory(itl)
add_subdirectory(doc)
add_subdirectory(agent)
add_subdirectory(plugins)

if(ICINGA2_WITH_STUDIO)
  add_subdirectory(icinga-studio)
endif()

if(ICINGA2_WITH_TESTS)
  add_subdirectory(test)
endif()

set(CPACK_PACKAGE_NAME "Icinga2")
set(CPACK_PACKAGE_VENDOR "Icinga Development Team")
set(CPACK_PACKAGE_VERSION ${ICINGA2_VERSION})
set(CPACK_NSIS_DISPLAY_NAME "Icinga 2")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ICINGA2")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icinga-app\\\\icinga.ico")
set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icinga-app\\\\icinga.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/icinga-app\\\\icinga.ico")
set(CPACK_NSIS_INSTALLED_ICON_NAME "sbin\\\\icinga2.exe")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")

set(CPACK_NSIS_EXECUTABLES_DIRECTORY "sbin")
set(CPACK_PACKAGE_EXECUTABLES "Icinga2SetupAgent;Icinga 2 Agent Wizard;icinga-studio;Icinga Studio")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "Icinga2SetupAgent")
set(CPACK_NSIS_EXTRA_PREINSTALL_COMMANDS "nsExec::Exec 'net stop icinga2'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nnsExec::Exec 'icacls \\\"$INSTDIR\\\" /grant *S-1-5-20:(oi)(ci)m'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nnsExec::Exec 'icacls \\\"$INSTDIR\\\\etc\\\" /inheritance:r /grant:r *S-1-5-20:(oi)(ci)m /grant:r *S-1-5-32-544:(oi)(ci)f'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\etc\\\\icinga2\\\\pki'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\cache\\\\icinga2'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\lib\\\\icinga2\\\\pki'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\lib\\\\icinga2\\\\agent\\\\inventory'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\lib\\\\icinga2\\\\api\\\\config'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\lib\\\\icinga2\\\\api\\\\log'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\lib\\\\icinga2\\\\api\\\\zones'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\log\\\\icinga2\\\\compat\\\\archive'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\log\\\\icinga2\\\\crash'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\run\\\\icinga2\\\\cmd'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\spool\\\\icinga2\\\\perfdata'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nCreateDirectory '$INSTDIR\\\\var\\\\spool\\\\icinga2\\\\tmp'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nnsExec::Exec '\\\"$INSTDIR\\\\sbin\\\\icinga2\\\" --scm-install daemon'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}\nnsExec::Exec 'net start icinga2'")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "nsExec::Exec '\\\"$INSTDIR\\\\sbin\\\\icinga2\\\" --scm-uninstall'")

set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
include(InstallRequiredSystemLibraries)

if(WIN32)
  set(NSCP_SHA256SUM "")
  if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/NSCP-Win32.msi)
    file(SHA256 ${CMAKE_CURRENT_BINARY_DIR}/NSCP-Win32.msi NSCP_SHA256SUM)
  endif()
    
  if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/NSCP-Win32.msi OR NOT ${NSCP_SHA256SUM} STREQUAL "226cef0154daf1f9b93d0e168068e723e2be407548e5c4772557401d87535b48")
    file(DOWNLOAD https://github.com/mickem/nscp/releases/download/0.4.3.143/NSCP-0.4.3.143-Win32.msi ${CMAKE_CURRENT_BINARY_DIR}/NSCP-Win32.msi SHOW_PROGRESS)
  endif()

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/NSCP-Win32.msi DESTINATION ${CMAKE_INSTALL_SBINDIR})

  install(
    PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
      ${OPENSSL_INCLUDE_DIR}/../bin/libeay32.dll ${OPENSSL_INCLUDE_DIR}/../bin/ssleay32.dll
    DESTINATION ${CMAKE_INSTALL_SBINDIR}
  )
endif()

include(CPack)
