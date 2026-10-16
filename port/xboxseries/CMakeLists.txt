cmake_minimum_required(VERSION 3.24)
set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0.26100.0 CACHE STRING "Xbox UWP SDK")
if(EXISTS "$ENV{VCToolsInstallDir}/bin/Hostx64/x64/link.exe")
  set(CMAKE_LINKER "$ENV{VCToolsInstallDir}/bin/Hostx64/x64/link.exe" CACHE FILEPATH "UWP metadata linker" FORCE)
endif()
project(OpenCEUWP LANGUAGES C CXX ASM_MASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(UWP_DEPS "C:/bsuwp/_deps/uwpdep-src" CACHE PATH "libuwp/Mesa/zlib UWP bundle")
set(UWP_SDL3_ROOT "" CACHE PATH "SternXD SDL3-UWP install root")
foreach(sdl_file bin/SDL3.dll lib/SDL3.lib include/SDL3/SDL.h cmake/SDL3Config.cmake)
  if(NOT EXISTS "${UWP_SDL3_ROOT}/${sdl_file}")
    message(FATAL_ERROR "UWP_SDL3_ROOT is missing ${sdl_file}. Run tools/setup_uwp.ps1.")
  endif()
endforeach()
set(SDL3_DIR "${UWP_SDL3_ROOT}/cmake" CACHE PATH "SDL3 UWP CMake package" FORCE)
find_package(SDL3 CONFIG REQUIRED)
file(READ "${UWP_SDL3_ROOT}/include/SDL3/SDL_video.h" SDL3_VIDEO_HEADER)
if(NOT SDL3_VIDEO_HEADER MATCHES "SDL_PROP_WINDOW_WINRT_WINDOW_POINTER")
  message(FATAL_ERROR "SDL3 must be the SternXD UWP fork, not a desktop SDL3 build")
endif()
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED}")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(OUTPUT "${GENERATED}/host_gl.c"
  COMMAND "${Python3_EXECUTABLE}" "${REPO_ROOT}/tools/xbox_host_gl.py"
    "${REPO_ROOT}/build/uwp/guest/gen/guest_gl.c" "${GENERATED}/host_gl.c"
  DEPENDS "${REPO_ROOT}/tools/xbox_host_gl.py" "${REPO_ROOT}/build/uwp/guest/gen/guest_gl.c")
add_custom_command(OUTPUT "${GENERATED}/host_posix.c"
  COMMAND "${Python3_EXECUTABLE}" "${REPO_ROOT}/tools/xbox_host_posix.py"
    "${REPO_ROOT}/build/uwp/guest/gen/guest_posix.c" "${GENERATED}/host_posix.c"
  DEPENDS "${REPO_ROOT}/tools/xbox_host_posix.py" "${REPO_ROOT}/build/uwp/guest/gen/guest_posix.c")

set(HOST_SOURCES
  host/host_browser.cpp
  host/host_main.cpp host/xiso_installer.cpp host/setup_ui.cpp host/host_loader.c host/host_memory.c host/host_thread.c host/host_syscall.c host/host_sdl.c
  host/host_posix_misc.c host/guest_call.asm
  "${REPO_ROOT}/build/uwp/host/host_import_table.c"
  "${REPO_ROOT}/port/windows/src/win32_files.c"
  "${REPO_ROOT}/port/windows/src/win32_net.c"
  "${GENERATED}/host_gl.c" "${GENERATED}/host_posix.c")

set(IMGUI "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
list(APPEND HOST_SOURCES
  "${IMGUI}/imgui.cpp" "${IMGUI}/imgui_draw.cpp" "${IMGUI}/imgui_tables.cpp" "${IMGUI}/imgui_widgets.cpp"
  "${IMGUI}/backends/imgui_impl_sdl3.cpp" "${IMGUI}/backends/imgui_impl_opengl3.cpp")

set(WINRT_ENTRY "${CMAKE_CURRENT_BINARY_DIR}/winrt_main.obj")
add_custom_command(OUTPUT "${WINRT_ENTRY}"
  COMMAND cl.exe /nologo /TP /ZW /EHsc /DWINAPI_FAMILY=WINAPI_FAMILY_APP /DNOMINMAX
    /I"${UWP_SDL3_ROOT}/include" /Fo"${WINRT_ENTRY}" /c "${CMAKE_CURRENT_SOURCE_DIR}/host/winrt_main.cpp"
  DEPENDS host/winrt_main.cpp)
set_source_files_properties("${WINRT_ENTRY}" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

add_executable(OpenCEUWP WIN32 ${HOST_SOURCES} "${WINRT_ENTRY}")
set_target_properties(OpenCEUWP PROPERTIES OUTPUT_NAME "Warthog")
set_property(TARGET OpenCEUWP APPEND PROPERTY LINK_DEPENDS "${REPO_ROOT}/build/uwp/halo_guest.elf")
target_compile_definitions(OpenCEUWP PRIVATE WINAPI_FAMILY=WINAPI_FAMILY_APP SDL_PLATFORM_WINRT=1 NOMINMAX
  _CRT_SECURE_NO_WARNINGS HALO_GUEST_IMAGE_BASE=0x70000000u)
target_compile_options(OpenCEUWP PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/W3>
  "$<$<COMPILE_LANGUAGE:C,CXX>:/clang:-ffile-prefix-map=${REPO_ROOT}=warthog>"
  "$<$<COMPILE_LANGUAGE:C,CXX>:/clang:-ffile-prefix-map=${CMAKE_CURRENT_BINARY_DIR}=build>")
target_include_directories(OpenCEUWP PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/host"
  "${REPO_ROOT}/port/android/include"
  "${REPO_ROOT}/port/linux/src"
  "${REPO_ROOT}/source/memory/zlib"
  "${IMGUI}" "${IMGUI}/backends"
  "${UWP_DEPS}/x64/include" "${UWP_SDL3_ROOT}/include")
target_link_directories(OpenCEUWP BEFORE PRIVATE "$ENV{VCToolsInstallDir}/lib/x64/store")
target_link_libraries(OpenCEUWP PRIVATE WindowsApp bcrypt ws2_32 iphlpapi
  SDL3::SDL3 "${UWP_DEPS}/x64/lib/libuwp.lib")
target_link_options(OpenCEUWP PRIVATE /APPCONTAINER /WINMD
  "/WINMDFILE:${CMAKE_CURRENT_BINARY_DIR}/OpenCEUWP.winmd")

foreach(runtime libuwp.dll opengl32.dll libgallium_wgl.dll dxil.dll z-1.dll)
  add_custom_command(TARGET OpenCEUWP POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${UWP_DEPS}/x64/bin/${runtime}" "$<TARGET_FILE_DIR:OpenCEUWP>/${runtime}")
endforeach()
add_custom_command(TARGET OpenCEUWP POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${UWP_SDL3_ROOT}/bin/SDL3.dll" "$<TARGET_FILE_DIR:OpenCEUWP>/SDL3.dll")
add_custom_command(TARGET OpenCEUWP POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${UWP_SDL3_ROOT}/licenses/SDL3/LICENSE.txt" "$<TARGET_FILE_DIR:OpenCEUWP>/sdl3-LICENSE.txt")
add_custom_command(TARGET OpenCEUWP POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
  "${REPO_ROOT}/build/uwp/halo_guest.elf" "$<TARGET_FILE_DIR:OpenCEUWP>/halo_guest.elf")
