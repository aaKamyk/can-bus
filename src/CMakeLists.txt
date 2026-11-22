add_executable(can_bus
        main.cpp
        canNetwork.cpp
        canNode.cpp
        canFrame.cpp
        canBus.cpp
)

set_target_properties(can_bus PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(can_bus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(can_bus PRIVATE cxx_std_23)
target_compile_options(can_bus PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion>
)
