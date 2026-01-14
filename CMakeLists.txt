cmake_minimum_required(VERSION 3.15)
project(jls LANGUAGES C)

# Сборка по умолчанию- Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Проверка наличия в системе doxygen
find_program(DOXYGEN doxygen)
if (DOXYGEN)
    message(STATUS "Doxygen found: ${DOXYGEN}")
else()
    message(STATUS "Doxygen not found. HTML documentation is unavailable")
    message(STATUS "Command to install this package:\n\t sudo apt install doxygen")
endif()

# Проверка наличия в системе valgrind
find_program(VALGRIND valgrind)
if (VALGRIND)
    message(STATUS "Valgrind found: ${VALGRIND}")
else()
    message(STATUS "Valgrind not found. Memory-leak tests is unavailable")
    message(STATUS "Command to install this package:\n\t sudo apt install valgrind")
endif()

# Проверка наличия в системе meld
find_program(MELD meld)
if (MELD)
    message(STATUS "Meld found: ${MELD}")
else()
    message(STATUS "Meld not found. Failed Comapre tests will not be visualized")
    message(STATUS "Command to install this package:\n\t sudo apt install meld")
endif()

# Проверка наличия в системе time
find_program(TIME /usr/bin/time)
if (TIME)
    message(STATUS "/usr/bin/time found: ${TIME}")
else()
    message(STATUS "/usr/bin/time not found. Unable to calculate ${PROJECT_NAME} and ls -l uptime and RAM consumption")
    message(STATUS "Command to install this package:\n\t sudo apt install time")
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
        RUNTIME DESTINATION bin
        )

# Настройка правила uninstall
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND}
        -D MANIFEST_FILE=${CMAKE_BINARY_DIR}/install_manifest.txt
        -P ${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake
    )

# Настройка правила создания документации
if (DOXYGEN)
    add_custom_target(doc
        COMMAND ${DOXYGEN} Doxyfile > ${CMAKE_BINARY_DIR}/doxygen.log 2>&1
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc
        COMMENT "Generating HTML documentation. Log file: ${CMAKE_BINARY_DIR}/doxygen.log"
        VERBATIM
    )
endif()

# Настройка правил тестирования
add_custom_target(tests
    COMMAND bash -c "${CMAKE_SOURCE_DIR}/scripts/makeTests ${CMAKE_BINARY_DIR}/${PROJECT_NAME}"
)
