# Locates the X-Plane SDK headers and platform import libraries.
# Sets XPLMSDK_INCLUDE_DIRS and XPLMSDK_LIBRARIES on success.

find_path(XPLMSDK_INCLUDE_DIR
    NAMES XPLMDefs.h
    HINTS
        "${XPLMSDK_ROOT}/XPLM"
        "${CMAKE_SOURCE_DIR}/sdk/XPLM"
    DOC "X-Plane SDK XPLM headers"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XPLMSDK DEFAULT_MSG XPLMSDK_INCLUDE_DIR)

if(XPLMSDK_FOUND)
    # XPLMSDK_INCLUDE_DIR is sdk/XPLM.
    # Headers are included as <XPLM/XPLMFoo.h> so the search root must be sdk/ (the parent).
    get_filename_component(XPLMSDK_ROOT_DIR "${XPLMSDK_INCLUDE_DIR}" DIRECTORY)
    set(XPLMSDK_INCLUDE_DIRS "${XPLMSDK_ROOT_DIR}")

    # XPLMSDK_INCLUDE_DIR is sdk/XPLM — one level up is sdk/, so ../Libraries is correct.
    if(WIN32)
        if(MSVC)
            set(XPLMSDK_LIBRARIES
                "${XPLMSDK_INCLUDE_DIR}/../Libraries/Win/XPLM_64.lib"
                "${XPLMSDK_INCLUDE_DIR}/../Libraries/Win/XPWidgets_64.lib"
            )
        else()
            # MinGW: use the dlltool-generated MinGW import libraries (.a)
            set(XPLMSDK_LIBRARIES
                "${XPLMSDK_INCLUDE_DIR}/../Libraries/Win/libXPLM_64.a"
                "${XPLMSDK_INCLUDE_DIR}/../Libraries/Win/libXPWidgets_64.a"
            )
        endif()
    elseif(APPLE)
        set(XPLMSDK_LIBRARIES
            "${XPLMSDK_INCLUDE_DIR}/../Libraries/Mac/XPLM.framework"
            "${XPLMSDK_INCLUDE_DIR}/../Libraries/Mac/XPWidgets.framework"
        )
    else()
        # Linux: symbols are resolved at runtime; no link-time library needed.
        set(XPLMSDK_LIBRARIES "")
        set(XPLMSDK_LINK_FLAGS "-Wl,--allow-shlib-undefined")
    endif()
endif()
