cmake_minimum_required(VERSION 3.20)

project(UniversalBorderlessUpscaler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows SDK secimini CMake'e birak (sabit bir surum dayatmiyoruz).
# Bu, farkli makinelerde 10.0.19041, 10.0.22621, 10.0.26100 gibi
# SDK'larin otomatik secilmesini saglar.
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)
endif()

if(MSVC)
    # CRT'yi STATIK bagla (/MT): UBU.exe hicbir Visual C++ Redistributable
    # kurulumuna ihtiyac duymadan tek basina calisir (bagimsiz/portable exe).
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    add_compile_definitions(
        _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
    )
    add_compile_options(
        /EHsc
        /W3
        /permissive-
        /utf-8
    )
endif()

set(SOURCES
    src/Common.h
    src/ShaderSources.h
    src/D3D11Renderer.h
    src/D3D11Renderer.cpp
    src/CaptureManager.h
    src/CaptureManager.cpp
    src/OverlayWindow.h
    src/OverlayWindow.cpp
    src/UpscalerApp.h
    src/UpscalerApp.cpp
    src/Main.cpp
)

set(SHADERS
    src/Shaders/MainShaders.hlsl
)

# HLSL'leri derleme, sadece IDE'de gorunur tut
set_source_files_properties(${SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(UBU WIN32 ${SOURCES} ${SHADERS})

target_include_directories(UBU PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(UBU PRIVATE
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    dwmapi.lib
    user32.lib
    gdi32.lib
    shell32.lib
)

# Post-build: UBU.exe'yi proje kokune kopyala (kolay calistirma icin)
add_custom_command(TARGET UBU POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:UBU>
        "${CMAKE_SOURCE_DIR}/UBU.exe"
    COMMENT "Copying UBU.exe to project root..."
)