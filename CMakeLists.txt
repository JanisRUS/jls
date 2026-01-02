cmake_minimum_required(VERSION 3.15)
project(jls LANGUAGES C)

# Сборка по умолчанию- Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Проверка наличия в системе doxygen
find_program(DOXYGEN doxygen)
if(DOXYGEN)
    message(STATUS "Doxygen found: ${DOXYGEN}")
else()
    message(STATUS "Doxygen not found. HTML documentation is unavailable")
endif()

# Проверка наличия в системе valgrind
find_program(VALGRIND valgrind)
if(VALGRIND)
    message(STATUS "Valgrind found: ${VALGRIND}")
else()
    message(STATUS "Doxygen not found. Memory-leak tests is unavailable")
endif()

# Основные настройки проекта
set(SOURCES_DIR "src")
set(HEADERS_DIR "include")
file(GLOB SOURCES "${SOURCES_DIR}/*.c")
file(GLOB HEADERS "${HEADERS_DIR}/*.h")
set(ALL_FILES ${SOURCES} ${HEADERS})

add_executable(${PROJECT_NAME} ${ALL_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${HEADERS_DIR})

# Настройка правила install
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin)

# Настройка правила uninstall
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND}
        -D MANIFEST_FILE=${CMAKE_BINARY_DIR}/install_manifest.txt
        -P ${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake)

# Настройка правила создания документации
if(DOXYGEN)
    add_custom_target(doc
        COMMAND ${DOXYGEN} Doxyfile > ${CMAKE_BINARY_DIR}/doxygen.log 2>&1
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc
        COMMENT "Generating HTML documentation. Log file: ${CMAKE_BINARY_DIR}/doxygen.log"
        VERBATIM)
endif()

# Настройка тестирования
enable_testing()

add_test(NAME Root COMMAND ./${PROJECT_NAME} /)

if(VALGRIND)
    add_test(NAME RootValgrind COMMAND ${VALGRIND} --leak-check=full --error-exitcode=1 ./${TARGET_FILE} /)
endif()
