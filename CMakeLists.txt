project(vfs.sftp)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Boost REQUIRED)
find_package(kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(SSH REQUIRED)

include_directories(${KODI_INCLUDE_DIR}
                    ${KODIPLATFORM_INCLUDE_DIRS}
                    ${SSH_INCLUDE_DIRS})

set(SFTP_SOURCES src/SFTPSession.cpp
                 src/SFTPFile.cpp)

set(DEPLIBS ${KODIPLATFORM_LIBRARIES}
            ${SSH_LIBRARIES})

add_definitions( -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)

build_addon(vfs.sftp SFTP DEPLIBS)
