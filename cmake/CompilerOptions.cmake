function(target_apply_compiler_options target)
    target_compile_features(${target} PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4 /WX-
            /wd4100  # unreferenced formal parameter
            /wd4996  # deprecated POSIX names
        )
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wno-unused-parameter
            -fvisibility=hidden
        )
    endif()

    # X-Plane SDK platform defines
    if(WIN32)
        target_compile_definitions(${target} PRIVATE IBM=1)
    elseif(APPLE)
        target_compile_definitions(${target} PRIVATE APL=1)
    else()
        target_compile_definitions(${target} PRIVATE LIN=1)
    endif()

    # Enable all XPLM API tiers up to 4.3.0
    target_compile_definitions(${target} PRIVATE
        XPLM200=1 XPLM210=1 XPLM300=1 XPLM301=1 XPLM400=1 XPLM410=1 XPLM430=1
    )
endfunction()
