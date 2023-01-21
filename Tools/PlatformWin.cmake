add_subdirectory(ImageDiff)

if (ENABLE_WEBKIT OR ENABLE_WEBKIT_LEGACY)
    if (USE_WINUI3)
        add_subdirectory(MiniBrowser/winui3)
    else ()
        add_subdirectory(MiniBrowser/win)
    endif ()
endif ()

#add_subdirectory(TestRunnerShared)

if (ENABLE_WEBKIT_LEGACY)
#    add_subdirectory(DumpRenderTree)
endif ()

if (ENABLE_WEBKIT)
#    add_subdirectory(WebKitTestRunner)
endif ()
