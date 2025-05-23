# credit goes to https://github.com/Lauriethefish for the original version
# also https://github.com/DanTheMan827 helped make it recurse directories

# Directory where our arbitrary asset files are stored
set(ASSETS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Directory to save the object files generated by llvm-objcopy
set(ASSET_BINARIES_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/binaryAssets)

# Directory to save the prepended files to
set(PREPENDED_ASSETS_DIR ${CMAKE_CURRENT_BINARY_DIR}/prependedAssets)
set(ASSET_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/assets.hpp")

if(EXISTS ${ASSETS_DIRECTORY})
    # Create necessary directories.
    file(MAKE_DIRECTORY ${ASSET_BINARIES_DIRECTORY})
    file(MAKE_DIRECTORY ${PREPENDED_ASSETS_DIR})

    # Recursively find all files (with relative paths) in the assets directory.
    file(
        GLOB_RECURSE ASSETS
        RELATIVE ${ASSETS_DIRECTORY}
        "${ASSETS_DIRECTORY}/*"
    )
    set(BINARY_ASSET_FILES "")

    set(EXTERN_DECLARATIONS "")
    set(ASSET_DECLARATIONS "")

    foreach(REL_FILE IN LISTS ASSETS)
        # Full path to the source asset.
        set(SRC_FILE "${ASSETS_DIRECTORY}/${REL_FILE}")

        # Skip directories.
        if(IS_DIRECTORY ${SRC_FILE})
            continue()
        endif()

        # Get the file size.
        file(SIZE "${SRC_FILE}" FILE_SIZE)

        message("-- Including asset: ${REL_FILE}")

        # Create matching subdirectories in the prepended assets folder.
        get_filename_component(REL_DIR ${REL_FILE} DIRECTORY)
        if(NOT REL_DIR STREQUAL "")
            file(MAKE_DIRECTORY "${PREPENDED_ASSETS_DIR}/${REL_DIR}")
        endif()

        # Create a prepended copy of the asset (with 32 extra bytes at the beginning).
        add_custom_command(
            OUTPUT "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo_append "                                " >
                    "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            COMMAND ${CMAKE_COMMAND} -E cat "${SRC_FILE}" >> "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            COMMAND ${CMAKE_COMMAND} -E echo_append " " >> "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            DEPENDS "${SRC_FILE}"
        )

        # Create matching subdirectories in the binary assets folder.
        if(NOT REL_DIR STREQUAL "")
            file(MAKE_DIRECTORY "${ASSET_BINARIES_DIRECTORY}/${REL_DIR}")
        endif()

        # Define the output object file (with a .o extension appended).
        set(OUTPUT_OBJ "${ASSET_BINARIES_DIRECTORY}/${REL_FILE}.o")

        # Use llvm-objcopy to create an object file that embeds the asset.
        add_custom_command(
            OUTPUT "${OUTPUT_OBJ}"
            COMMAND ${CMAKE_OBJCOPY} "${REL_FILE}" "${OUTPUT_OBJ}" --input-target binary
                    --output-target elf64-aarch64 --set-section-flags binary=strings
            DEPENDS "${PREPENDED_ASSETS_DIR}/${REL_FILE}"
            WORKING_DIRECTORY "${PREPENDED_ASSETS_DIR}"
        )
        list(APPEND BINARY_ASSET_FILES "${OUTPUT_OBJ}")

        # Generate a mangled symbol name by replacing non-alphanumeric characters with underscores.
        string(REGEX REPLACE "[^a-zA-Z0-9]" "_" FULL_SYMBOL ${REL_FILE})

        # Compute a local asset name from the file name (sanitized).
        get_filename_component(FILE_NAME ${REL_FILE} NAME)
        string(REGEX REPLACE "[^a-zA-Z0-9]" "_" LOCAL_NAME ${FILE_NAME})

        # Append the extern declarations for this asset inside Externs.
        set(EXTERN_DECLARATIONS
            "${EXTERN_DECLARATIONS}extern \"C\" uint8_t _binary_${FULL_SYMBOL}_start[];
        "
        )
        set(EXTERN_DECLARATIONS
            "${EXTERN_DECLARATIONS}extern \"C\" uint8_t _binary_${FULL_SYMBOL}_end[];
        "
        )

        # Set the indentation for the asset declaration code.
        set(NAMESPACE "")

        # Build the asset declaration code directly.
        # If the asset is in a subfolder, wrap it in nested namespaces.
        if(NOT REL_DIR STREQUAL "")
            string(REPLACE "/" ";" DIR_LIST ${REL_DIR})

            foreach(DIR IN LISTS DIR_LIST)
                string(REGEX REPLACE "[^a-zA-Z0-9]" "_" NAMESPACE_PART ${DIR})
                if(NAMESPACE STREQUAL "")
                    set(NAMESPACE ${NAMESPACE_PART})
                else()
                    set(NAMESPACE "${NAMESPACE}::${NAMESPACE_PART}")
                endif()
            endforeach()
        endif()

        if(NAMESPACE STREQUAL "")
            set(ASSET_DECLARATIONS
                "${ASSET_DECLARATIONS}// ${REL_FILE}
    DECLARE_ASSET(${LOCAL_NAME}, ${FULL_SYMBOL});
    "
            )
        else()
            set(ASSET_DECLARATIONS
                "${ASSET_DECLARATIONS}// ${REL_FILE}
    DECLARE_ASSET_NS(${NAMESPACE}, ${LOCAL_NAME}, ${FULL_SYMBOL});
    "
            )
        endif()
    endforeach()

    # Trim excess whitespace from the asset declarations.
    string(STRIP "${ASSET_DECLARATIONS}" ASSET_DECLARATIONS)

    # Trim excess whitespace from the extern declarations.
    string(STRIP "${EXTERN_DECLARATIONS}" EXTERN_DECLARATIONS)

    set(ASSET_HEADER_DATA
        "#pragma once

#include \"metacore/shared/assets.hpp\"

#define DECLARE_ASSET(name, binary)       \\
    const IncludedAsset name {            \\
        Externs::_binary_##binary##_start, \\
        Externs::_binary_##binary##_end    \\
    };

#define DECLARE_ASSET_NS(namespaze, name, binary) \\
    namespace namespaze { DECLARE_ASSET(name, binary) }

namespace IncludedAssets {
    namespace Externs {
        ${EXTERN_DECLARATIONS}
    }

    ${ASSET_DECLARATIONS}
}
"
    )
    list(LENGTH BINARY_ASSET_FILES COUNT)
    if(${COUNT} GREATER 0)
        # Check if the output file already exists.
        if(EXISTS ${ASSET_HEADER_PATH})
            file(READ ${ASSET_HEADER_PATH} CURRENT_ASSET_HEADER)
        else()
            set(CURRENT_ASSET_HEADER "")
        endif()

        # If the contents of the asset header have changed, write the new contents.
        if(NOT "${CURRENT_ASSET_HEADER}" STREQUAL "${ASSET_HEADER_DATA}")
            message("-- Writing '${ASSET_HEADER_PATH}'")
            file(WRITE ${ASSET_HEADER_PATH} "${ASSET_HEADER_DATA}")
        else()
            message("-- '${ASSET_HEADER_PATH}' is up to date.")
        endif()

        # Create an object library for the asset files so they are linked into your final binary.
        add_library(asset_files OBJECT ${BINARY_ASSET_FILES})
        set_target_properties(asset_files PROPERTIES LINKER_LANGUAGE CXX)
        target_link_libraries(${COMPILE_ID} PRIVATE asset_files ${BINARY_ASSET_FILES})
    else()
        if(EXISTS ${ASSET_HEADER_PATH})
            message(
                "-- Removing '${ASSET_HEADER_PATH}' as no assets have been found in '${ASSETS_DIRECTORY}'"
            )
            file(REMOVE ${ASSET_HEADER_PATH})
        endif()
    endif()
else()
    if(EXISTS ${ASSET_HEADER_PATH})
        message(
            "-- Removing '${ASSET_HEADER_PATH}' as the directory '${ASSETS_DIRECTORY}' does not exist"
        )
        file(REMOVE ${ASSET_HEADER_PATH})
    endif()
endif()
