cmake_minimum_required(VERSION 2.8)

project(helloasm)

set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS "asm")
set(CMAKE_ASM_COMPILE_OBJECT "nasm -f elf64 -g -F dwarf -o <OBJECT> <SOURCE>")
SET(CMAKE_ASM_LINK_EXECUTABLE "ld <OBJECTS> -o <TARGET>")
enable_language(ASM)

add_executable(hello hello.asm)
add_executable(add longio.asm add.asm)
add_executable(sub longio.asm sub.asm)
add_executable(mul longio.asm mul.asm)
