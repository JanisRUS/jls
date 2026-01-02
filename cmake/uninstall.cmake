if(NOT DEFINED MANIFEST_FILE)
    message(FATAL_ERROR "MANIFEST_FILE not set")
endif()

if(NOT EXISTS "${MANIFEST_FILE}")
    message(FATAL_ERROR "Cannot find install manifest: ${MANIFEST_FILE}")
endif()

file(READ "${MANIFEST_FILE}" manifest)
string(REPLACE "\n" ";" manifest_files "${manifest}")

foreach(file ${manifest_files})
    if(EXISTS "${file}" OR IS_SYMLINK "${file}")
        message(STATUS "Removing ${file}")
        file(REMOVE "${file}")
    else()
        message(STATUS "Already removed: ${file}")
    endif()
endforeach()