cmake_minimum_required(VERSION 3.15...4.2) # for CMake 3.15+ features, but keep it compatible with older versions
project(WeatherSound VERSION 0.0.1)
# JUCE의 안전 장치를 전역 수준에서 강제로 끔
add_compile_definitions(JUCE_VST3_CAN_REPLACE_VST2=0)

add_subdirectory(JUCE)

juce_add_plugin(WeatherSound
    COMPANY_NAME "Jihoaudio"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_CODE "Slft"
    FORMATS VST3 AU Standalone
    VST3_CAN_REPLACE_VST2 FALSE
)

# 소스 파일 등록 (.jucer에 있던 파일들)
target_sources(WeatherSound PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SimpleFilterChannel.cpp
    Source/SimpleFilterChannel.h
    Source/dsp/FMSynth.cpp
    Source/dsp/FMSynth.h
    Source/dsp/dspUtils.h

)

set(WEBVIEW_FILES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ui/dist")
execute_process(
    COMMAND npm run build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ui
)
add_custom_target(BuildUI ALL
    COMMAND npm run build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ui
    BYPRODUCTS ${WEBVIEW_FILES_SOURCE_DIR}
)
add_dependencies(WeatherSound BuildUI)

set(TARGET_WEBVIEW_FILES_ZIP_PATH "${CMAKE_BINARY_DIR}/webview_files.zip")
get_filename_component(PUBLIC_PATH "${WEBVIEW_FILES_SOURCE_DIR}" ABSOLUTE)
file(
    ARCHIVE_CREATE
    OUTPUT
    ${TARGET_WEBVIEW_FILES_ZIP_PATH}
    PATHS
    "${PUBLIC_PATH}"
    FORMAT
    zip
    VERBOSE
)

# Pass the prefix of the zipped files (e.g., "public/") to C++
# cmake_path(GET PUBLIC_PATH FILENAME ZIPPED_FILES_PREFIX)
# target_compile_definitions(${PROJECT_NAME} PRIVATE ZIPPED_FILES_PREFIX="${ZIPPED_FILES_PREFIX}/")

juce_add_binary_data(WebViewFiles
    HEADER_NAME WebViewFiles.h
    NAMESPACE webview_files
    SOURCES ${TARGET_WEBVIEW_FILES_ZIP_PATH}
)

# 사용 중인 JUCE 모듈 링크
target_link_libraries(WeatherSound PRIVATE WebViewFiles
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_core
    juce::juce_data_structures
)

# C++ 표준 설정
target_compile_features(WeatherSound PRIVATE cxx_std_20)

# 컴파일러 설정 (Clang 17의 엄격한 경고 해결)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(WeatherSound PRIVATE -Wno-error -Wno-reserved-identifier)
endif()