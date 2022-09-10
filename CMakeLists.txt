cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12 CACHE STRING "")

set(vst3sdk_SOURCE_DIR R:/VST_SDK/vst3sdk)
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(NanoSynth
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 1.0.0.0 
    DESCRIPTION "NanoSynth VST 3 Plug-in"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(NanoSynth
    source/version.h
    source/NanoSynth_cids.h
    source/NanoSynth_processor.h
    source/NanoSynth_processor.cpp
    source/NanoSynth_controller.h
    source/NanoSynth_controller.cpp
    source/NanoSynth_entry.cpp
)

#- VSTGUI Wanted ----
if(SMTG_ADD_VSTGUI)
    target_sources(NanoSynth
        PRIVATE
            resource/NanoSynth_editor.uidesc
    )
    target_link_libraries(NanoSynth
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(NanoSynth
        RESOURCES
            "resource/NanoSynth_editor.uidesc"
    )
endif(SMTG_ADD_VSTGUI)
# -------------------

smtg_target_add_plugin_snapshots (NanoSynth
    RESOURCES
        resource/CD89DB866EE0523796F5D7A5BAAF60DD_snapshot.png
        resource/CD89DB866EE0523796F5D7A5BAAF60DD_snapshot_2.0x.png
)

target_link_libraries(NanoSynth
    PRIVATE
        sdk
)

smtg_target_configure_version_file(NanoSynth)

if(SMTG_MAC)
    smtg_target_set_bundle(NanoSynth
        BUNDLE_IDENTIFIER net.quero.nanosynth
        COMPANY_NAME "quero"
    )
    smtg_target_set_debug_executable(NanoSynth
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(NanoSynth PRIVATE 
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT NanoSynth)

        smtg_target_set_debug_executable(NanoSynth
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)
