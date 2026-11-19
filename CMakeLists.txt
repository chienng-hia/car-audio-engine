cmake_minimum_required(VERSION 3.16)
project(CarAudioEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CAR_AUDIO_BUILD_TESTS "Build unit and integration tests" ON)

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Core library
add_library(car_audio_core STATIC
    src/common/types.cpp
    src/common/json.cpp
    src/common/wav_io.cpp
    src/config/audio_config.cpp
    src/focus/focus_manager.cpp
    src/routing/audio_router.cpp
    src/dsp/biquad_eq.cpp
    src/dsp/resampler.cpp
    src/dsp/gain_smoother.cpp
    src/dsp/spatial_balance.cpp
    src/dsp/sdvc.cpp
    src/dsp/limiter.cpp
    src/engine/audio_source.cpp
    src/engine/audio_sink.cpp
    src/engine/audio_engine.cpp
)

target_include_directories(car_audio_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# CLI Simulator & Scenario Runner
add_executable(car_audio_cli
    src/app/main.cpp
)
target_link_libraries(car_audio_cli PRIVATE car_audio_core)

# Tests
if(CAR_AUDIO_BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)

    function(add_car_audio_test target_name source_file)
        add_executable(${target_name} ${source_file})
        target_link_libraries(${target_name} PRIVATE car_audio_core)
        if(GTest_FOUND)
            target_link_libraries(${target_name} PRIVATE GTest::gtest_main)
            target_compile_definitions(${target_name} PRIVATE CAR_AUDIO_USE_GTEST=1)
        endif()
        add_test(NAME ${target_name} COMMAND ${target_name}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endfunction()

    add_car_audio_test(test_focus_manager tests/test_focus_manager.cpp)
    add_car_audio_test(test_dsp_pipeline tests/test_dsp_pipeline.cpp)
    add_car_audio_test(test_routing_and_config tests/test_routing_and_config.cpp)
    add_car_audio_test(test_engine_e2e tests/test_engine_e2e.cpp)
endif()
