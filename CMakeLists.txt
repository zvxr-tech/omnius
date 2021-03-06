cmake_minimum_required(VERSION 3.2)
project(secmem)

include_directories(/usr/include/libudis86)

set_source_files_properties(omnius/ PROPERTIES COMPILE_FLAGS -std=c90)
set_source_files_properties(omnius-cli/ PROPERTIES COMPILE_FLAGS -std=c90)
set_source_files_properties(clr_msg/ PROPERTIES COMPILE_FLAGS -std=c90)

add_executable(omnius omnius/ragasm.h omnius/ragasm.c omnius/fsm_descriptor.h omnius/fsm_descriptor.c omnius/memory.h omnius/memory.c omnius/process.h omnius/process.c omnius/comm.h omnius/comm.c omnius/global.h omnius/omnius.h omnius/omnius.c omnius/regex_parse/regex_parse.cpp omnius/regex_parse/common.h omnius/regex_parse/dfa.h omnius/regex_parse/nfa.cpp omnius/regex_parse/nfa.h omnius/regex_parse/subset_construct.cpp omnius/regex_parse/subset_construct.h omnius/regex_parse/regex_parse.h )
add_executable(omnius-cli omnius-cli/omnius-cli.c omnius/comm.c)
add_executable(clr_msg utility/clr_msg.c)
add_executable(shim omnius-shim/shim.c)
add_executable(standalone_shim omnius-shim/standalone_shim.c )
add_executable(erasmus omnius-shim/erasmus.c omnius-shim/policy.h)
add_executable(udis86_test omnius-shim/udis86_test.c /usr/include/udis86.h)
add_executable(xed_test omnius-shim/xed_test.c)
add_executable(target omnius-shim/target.c)

link_directories(/pin-2.14-71313-gcc.4.4.7-linux/extras/xed-intel64/include/)
target_link_libraries(udis86_test udis86)