CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(edcltool)
SET(PROJECT_VERSION   0.2.5)
SET(EDCL_API_VERSION  1)

set(CPACK_GENERATOR ZIP)
SET(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

enable_testing()
include(ExternalProject)
include(CPack)

SET(SRCS
  src/chips.c
  src/edcl.c
  src/elf.c
)

if (LINUX)
    LIST(APPEND SRCS src/platform-linux.c)
elseif(WIN32)
    LIST(APPEND SRCS src/platform-windows.c)
endif()

FILE(GLOB_RECURSE LUALIBS lib/*.lua)

ADD_LIBRARY(edcl OBJECT ${SRCS})

SET(CMAKE_C_FLAGS "-Wall -DENABLE_ELFLOAD")

if (LINUX)
  FIND_PACKAGE(PkgConfig)
  PKG_CHECK_MODULES(ELF REQUIRED lua5.2)
  PKG_CHECK_MODULES(LUA REQUIRED libelf)
  target_compile_definitions(edcl PUBLIC -fPIC)
  set(PLATFORM_BINDIR bin)
  set(PLATFORM_LIBDIR share/edcltool/${CMAKE_LIBRARY_PATH}/lib/)
elseif(WIN32)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DEDCL_WINDOWS")
  set(PLATFORM_BINDIR "dist")
  set(PLATFORM_LIBDIR "dist/lib")
  ExternalProject_Add(libelf
    URL http://www.mr511.de/software/libelf-0.8.13.tar.gz
    CONFIGURE_COMMAND sh ../libelf/configure --prefix="binary"
    BUILD_COMMAND make
    INSTALL_COMMAND make install
  )
  set(ELF_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/libelf-prefix/src/libelf-build/lib/binary/include)

  ExternalProject_Add(lua
    URL https://www.lua.org/ftp/lua-5.2.4.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make mingw
    INSTALL_COMMAND ""
  )
  set(ELF_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/libelf-prefix/src/libelf-build/lib/binary/include)
  set(LUA_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/lua-prefix/src/lua/src)

  target_include_directories(edcl PUBLIC ${CMAKE_SOURCE_DIR}/windeps/WpdPack/Include)
  set(ELF_LIBRARIES ${CMAKE_BINARY_DIR}/libelf-prefix/src/libelf-build/lib/binary/lib/libelf.a)
  set(LUA_LIBRARIES ${CMAKE_BINARY_DIR}/lua-prefix/src/lua/src/liblua.a)

  ADD_CUSTOM_TARGET(icon
      COMMAND windres -I${CMAKE_SOURCE_DIR} -i${CMAKE_SOURCE_DIR}/edcl.rc
      -o ${CMAKE_BINARY_DIR}/icon.o )

  set(PCAP_LIBRARIES
    -lwsock32
    ${CMAKE_SOURCE_DIR}/windeps/WpdPack/Lib/libpacket.a
    ${CMAKE_SOURCE_DIR}/windeps/WpdPack/Lib/libwpcap.a
  )

  add_dependencies(edcl lua libelf icon)
  set(ICON     ${CMAKE_BINARY_DIR}/icon.o)

endif()


if (NOT EDCL_PORTABLE)
  set(EDCLTOLINK edclshared)
  if (LINUX)
    target_compile_options(edcl PUBLIC -DLUAEXT_PREFIX='\"${CMAKE_INSTALL_PREFIX}/share/edcltool/${CMAKE_LIBRARY_PATH}/lib\"')
  elseif(WIN32)
    #We don't support shared link on win32
    set(EDCLTOLINK edclstatic)
    target_compile_options(edcl PUBLIC "-DLUAEXT_PREFIX=$ENV{PROGRAMFILES}\\edcltool\\lib")
  endif()
else()
  set(EDCLTOLINK edclstatic)
  target_compile_options(edcl PUBLIC "-DLUAEXT_PREFIX=.\\lib")
endif()



INCLUDE_DIRECTORIES(
    ${ELF_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/
)



ADD_LIBRARY(edclstatic STATIC $<TARGET_OBJECTS:edcl>)
SET_TARGET_PROPERTIES(edclstatic PROPERTIES OUTPUT_NAME edcl)
TARGET_LINK_LIBRARIES(edclstatic
  ${ELF_LIBRARIES}
  ${LUA_LIBRARIES}
  ${PCAP_LIBRARIES}
)

ADD_LIBRARY(edclshared SHARED $<TARGET_OBJECTS:edcl> )
SET_TARGET_PROPERTIES(edclshared PROPERTIES OUTPUT_NAME edcl)
TARGET_LINK_LIBRARIES(edclshared
  ${ELF_LIBRARIES}
  ${LUA_LIBRARIES}
  ${PCAP_LIBRARIES}
)

SET_TARGET_PROPERTIES(edclshared PROPERTIES SOVERSION ${PROJECT_VERSION}
  VERSION ${EDCL_API_VERSION})


ADD_EXECUTABLE(edcldump src/edcl_dump.c ${RES_FILES})
TARGET_LINK_LIBRARIES(edcldump
  ${EDCLTOLINK}
)

ADD_EXECUTABLE(edcltest src/edcl_test.c ${RES_FILES})
TARGET_LINK_LIBRARIES(edcltest
  ${EDCLTOLINK}
)

ADD_EXECUTABLE(edclwrite src/edcl_write.c ${RES_FILES})
TARGET_LINK_LIBRARIES(edclwrite
  ${EDCLTOLINK}
)

ADD_EXECUTABLE(edcltool src/edcltool.c)
TARGET_LINK_LIBRARIES(edcltool
  ${ELF_LIBRARIES}
  ${LUA_LIBRARIES}
  ${PCAP_LIBRARIES}
  ${EDCLTOLINK}
  ${ICON}
)

INSTALL(TARGETS edcldump
  DESTINATION ${PLATFORM_BINDIR}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
  )

INSTALL(TARGETS edclwrite
  DESTINATION ${PLATFORM_BINDIR}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
  )

INSTALL(TARGETS edcltool
  DESTINATION ${PLATFORM_BINDIR}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
  )

INSTALL(TARGETS edcltest
  DESTINATION ${PLATFORM_BINDIR}
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
  )

if (NOT WIN32)
INSTALL(TARGETS edclshared LIBRARY
        DESTINATION lib/${CMAKE_LIBRARY_PATH}
      )
endif()

INSTALL(TARGETS edclstatic ARCHIVE
        DESTINATION lib/${CMAKE_LIBRARY_PATH})

INSTALL(FILES ${LUALIBS}
      DESTINATION ${PLATFORM_LIBDIR}
)
