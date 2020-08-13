cmake_minimum_required(VERSION 3.12)

project(cosemclient VERSION 1.0
                  DESCRIPTION "Cosem client command line utility"
                  LANGUAGES C CXX)

set(TOP_DIR ${PROJECT_SOURCE_DIR}/..)

set(TARGET cosemclient)

add_executable(${TARGET})

target_compile_definitions(${TARGET} PRIVATE
    COSEM_CLIENT_VER=\"1.2.0\"
)

if(WIN32)
  target_compile_definitions(${TARGET} PRIVATE
    USE_WINDOWS_OS
  )
elseif(UNIX)
  target_compile_definitions(${TARGET} PRIVATE
    USE_UNIX_OS
  )
endif()

if(MSVC)
  target_compile_definitions(${TARGET} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_SECURE_NO_DEPRECATE
  )
elseif(MINGW)
  target_compile_definitions(${TARGET} PRIVATE
    STRSAFE_NO_DEPRECATE
  )
elseif(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(
        -Wno-unused-function
        -Wno-unused-result
  )
endif()

target_sources( ${TARGET} PRIVATE
  src/client_config.h
  src/cosem_client_hal.c
  src/main.cpp
  lib/AxdrPrinter.cpp
  lib/AxdrPrinter.h
  lib/Configuration.cpp
  lib/Configuration.h
  lib/CosemClient.cpp
  lib/CosemClient.h
  lib/Transport.cpp
  lib/Transport.h
  lib/Util.cpp
  lib/Util.h
)

target_include_directories(${TARGET} PRIVATE
    lib 
    ${TOP_DIR}/hdlc
    ${TOP_DIR}/src
    ${TOP_DIR}/share/crypto
    ${TOP_DIR}/share/ip
    ${TOP_DIR}/share/serial
    ${TOP_DIR}/share/util
    ${TOP_DIR}/json
)

target_link_libraries(${TARGET} PRIVATE
  cosemlib
)

if(WIN32)
  target_link_libraries(${TARGET} PRIVATE
     ws2_32
  )    
elseif(UNIX)
  target_link_libraries(${TARGET} PRIVATE
     pthread
  )
endif()
