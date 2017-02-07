CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(edcltool)
SET(PROJECT_VERSION   0.2.3)
SET(AURA_API_VERSION  1)

enable_testing()

SET(SRCS
  src/chips.c
  src/edcl.c
  src/elf.c
  src/platform-linux.c
)


FILE(GLOB_RECURSE LUALIBS lib/*.lua)


FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(ELF REQUIRED lua5.2)
PKG_CHECK_MODULES(LUA REQUIRED libelf)

SET(CMAKE_C_FLAGS "-Wall -fPIC -DENABLE_ELFLOAD -DLUAEXT_PREFIX='\"${CMAKE_INSTALL_PREFIX}/share/edcltool/${CMAKE_LIBRARY_PATH}/lib\"'")

INCLUDE_DIRECTORIES(
    ${ELF_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/
)

ADD_LIBRARY(edcl OBJECT ${SRCS})

ADD_LIBRARY(edclstatic STATIC $<TARGET_OBJECTS:edcl>)
SET_TARGET_PROPERTIES(edclstatic PROPERTIES OUTPUT_NAME edcl)
TARGET_LINK_LIBRARIES(edclstatic
  ${ELF_LIBRARIES}
  ${LUA_LIBRARIES}
)

ADD_LIBRARY(edclshared SHARED $<TARGET_OBJECTS:edcl>)
SET_TARGET_PROPERTIES(edclshared PROPERTIES OUTPUT_NAME edcl)
TARGET_LINK_LIBRARIES(edclshared
  ${ELF_LIBRARIES}
  ${LUA_LIBRARIES}
)

SET_TARGET_PROPERTIES(edclshared PROPERTIES SOVERSION ${PROJECT_VERSION}
  VERSION ${AURA_API_VERSION})


ADD_EXECUTABLE(edcldump src/edcl_dump.c)
TARGET_LINK_LIBRARIES(edcldump
  edclshared
)

ADD_EXECUTABLE(edcltest src/edcl_test.c)
TARGET_LINK_LIBRARIES(edcltest
  edclshared
)

ADD_EXECUTABLE(edclwrite src/edcl_write.c)
TARGET_LINK_LIBRARIES(edclwrite
  edclshared
)

ADD_EXECUTABLE(edcltool src/edcltool.c)
TARGET_LINK_LIBRARIES(edcltool
  ${ELF_LIBRARIES}
  ${LUA_LIBRARIES}
  edclshared
)

INSTALL(TARGETS edcldump
  DESTINATION bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
  )

INSTALL(TARGETS edclwrite
  DESTINATION bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
  )

INSTALL(TARGETS edcltool
  DESTINATION bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
  )

INSTALL(TARGETS edcltest
  DESTINATION bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE SETUID
  )

INSTALL(TARGETS edclshared LIBRARY
        DESTINATION lib/${CMAKE_LIBRARY_PATH})

INSTALL(TARGETS edclstatic ARCHIVE
        DESTINATION lib/${CMAKE_LIBRARY_PATH})

INSTALL(FILES ${LUALIBS}
       DESTINATION share/edcltool/${CMAKE_LIBRARY_PATH}/lib/)



         macro(generate_clang_complete)
         message(STATUS "Generarating ${CMAKE_SOURCE_DIR}/.clang_complete")
         get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
         file(WRITE ${CMAKE_SOURCE_DIR}/.clang_complete "")
         foreach(dir ${dirs})
           file(APPEND ${CMAKE_SOURCE_DIR}/.clang_complete "-I${dir}\n")
         endforeach()
         string(REPLACE "'" "" CMAKE_C_FLAGS_SPLIT ${CMAKE_C_FLAGS})
         string(REPLACE " " ";" CMAKE_C_FLAGS_SPLIT ${CMAKE_C_FLAGS_SPLIT})
         foreach(flag ${CMAKE_C_FLAGS_SPLIT})
         # FixMe: Ugly hack
           if (NOT "${flag}" MATCHES "-Wall")
             if (NOT "${flag}" MATCHES "-g")
               if (NOT "${flag}" MATCHES "-fPIC")
                 file(APPEND ${CMAKE_SOURCE_DIR}/.clang_complete "${flag}\n")
               endif()
             endif()
           endif()
         endforeach()
       endmacro()

generate_clang_complete()
