cmake_minimum_required(VERSION 3.23)
project(ClipboardSync LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(ClipboardSync
    main.cpp
    ClipboardManager.h   ClipboardManager.cpp
    ConfigDialog.h       ConfigDialog.cpp
    ConfigManager.h      ConfigManager.cpp
    CryptoEngine.h       CryptoEngine.cpp
    DatagramProcessor.h  DatagramProcessor.cpp
    WakeNotifier.h
    thirdparty/tiny-AES-c/aes.c
    resources.qrc
)

if(APPLE)
    enable_language(OBJCXX)
    target_sources(ClipboardSync PRIVATE WakeNotifier.mm)
else()
    target_sources(ClipboardSync PRIVATE WakeNotifier.cpp)
endif()

# Not target-wide: CTR collides with macOS SDK headers
set_source_files_properties(CryptoEngine.cpp thirdparty/tiny-AES-c/aes.c
    PROPERTIES COMPILE_DEFINITIONS "AES256=1;ECB=0;CTR=0")
target_link_libraries(ClipboardSync PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)

# Qt libs built with clang-cl reference clang_rt.builtins, which is not on MSVC's default
# lib path; find it in the Clang toolchain bundled with Visual Studio (inert for cl-built Qt).
if(MSVC)
    file(TO_CMAKE_PATH "$ENV{VCToolsInstallDir}" VC_TOOLS_DIR)
    file(GLOB CLANG_RT_LIB_DIR "${VC_TOOLS_DIR}/../../Llvm/x64/lib/clang/*/lib/windows")
    target_link_directories(ClipboardSync PRIVATE ${CLANG_RT_LIB_DIR})
endif()

set_target_properties(ClipboardSync PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)
