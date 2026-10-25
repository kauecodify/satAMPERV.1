cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(iot-energy-monitor)

# Adicionar arquivos Assembly
set(ASM_SOURCES
    src/assembly_opt.asm
)

# Compilar Assembly
set_source_files_properties(${ASM_SOURCES} PROPERTIES LANGUAGE ASM)
