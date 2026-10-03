find_package(Qt6 6.12 REQUIRED COMPONENTS Test)

# Each suite links the real QML module (tyli_ui + its plugin), so the smoke test
# loads the same screens as the app.
function(tyli_add_test name)
    qt_add_executable(${name} ${name}.cpp test_support.hpp)
    target_link_libraries(${name} PRIVATE tyli_ui tyli_uiplugin Qt6::Test)
    target_compile_definitions(${name} PRIVATE TYLI_SCREENSHOT_DIR="${CMAKE_BINARY_DIR}/screenshots")
    if(MSVC)
        target_compile_options(${name} PRIVATE /utf-8)  # the tests hold non-ASCII strings ("Música ✓")
    endif()
    add_test(NAME ${name} COMMAND ${name})
    # No display on the VM (or in CI): render offscreen with the software renderer.
    # TYLI_PLAYER_OUTPUTS=null: the player decodes nothing to a sound card or a window.
    # QT_FORCE_STDERR_LOGGING: without a console window (the Windows CI runner), Qt Test and
    # qWarning write to the debugger (OutputDebugString), so a failure showed no output at all.
    set_tests_properties(${name} PROPERTIES ENVIRONMENT
                         "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_FORCE_STDERR_LOGGING=1;TYLI_PLAYER_OUTPUTS=null")
endfunction()

tyli_add_test(tst_library_bridge)
tyli_add_test(tst_copy_grouper)
tyli_add_test(tst_media_models)
tyli_add_test(tst_player_bridge)
tyli_add_test(tst_qml_smoke)
