# pibase self-test.
#
# Two targets on purpose: the header is claimed to be valid C *and* valid C++
# (that is what "usable from C++, Rust, C# through FFI" rests on), and a C-only
# test would not notice the day that stops being true.

add_executable(pibase_test_pi_base test_pi_base.c)
target_link_libraries(pibase_test_pi_base PRIVATE pibase)
set_target_properties(pibase_test_pi_base PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON)

add_executable(pibase_test_cpp_include test_pi_base_cpp.cpp)
target_link_libraries(pibase_test_cpp_include PRIVATE pibase)
set_target_properties(pibase_test_cpp_include PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

if(MSVC)
    target_compile_options(pibase_test_pi_base PRIVATE /W4)
    target_compile_options(pibase_test_cpp_include PRIVATE /W4)
else()
    target_compile_options(pibase_test_pi_base PRIVATE -Wall -Wextra)
    target_compile_options(pibase_test_cpp_include PRIVATE -Wall -Wextra)
endif()

add_test(NAME pibase_unit COMMAND pibase_test_pi_base)
add_test(NAME pibase_cpp_include COMMAND pibase_test_cpp_include)
