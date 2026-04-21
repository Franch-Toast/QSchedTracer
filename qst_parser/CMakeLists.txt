cmake_minimum_required(VERSION 3.14)
project(qst_parser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Library target (core parsing logic) ---
add_library(qst_core STATIC
    src/streaming_parser.cpp
    src/handler_thread.cpp
    src/handler_events.cpp
    src/flow_tracker.cpp
    src/kercall_tables.cpp
    src/event_names.cpp
)
target_include_directories(qst_core PUBLIC include)
target_compile_options(qst_core PRIVATE -O2 -Wall -Wextra)

# --- Executable ---
add_executable(qst_parser src/main.cpp)
target_link_libraries(qst_parser PRIVATE qst_core)
target_compile_options(qst_parser PRIVATE -O2 -Wall -Wextra)
