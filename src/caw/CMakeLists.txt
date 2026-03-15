add_executable(caw)

set_target_properties(caw PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

FetchContent_Declare(
    cw
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/libcw
)

FetchContent_MakeAvailable(cw)


target_sources(caw
  
  PRIVATE
  main.cpp
  cawUi.cpp
  cawUiDecl.h
  cawUi.h  
)


target_compile_options(caw PRIVATE
    # Add specific flags only for the Debug configuration (e.g., Address Sanitizer)
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fsanitize=undefined -Wall -Wextra -Wno-unused>    
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>,$<BOOL:${CW_ADDRESS_SANITIZER_FL}>>:-fsanitize=address>    
    $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>,$<BOOL:${CW_THREAD_SANITIZER_FL}>>:-fsanitize=thread>    
    $<$<AND:$<CONFIG:Debug>,$<BOOL:${CW_COVERAGE_FL}>>:--coverage>    
)

target_link_libraries(
  caw
  PRIVATE
  cw
  $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>>:-fsanitize=undefined>    
  $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>,$<BOOL:${CW_ADDRESS_SANITIZER_FL}>>:-lasan>    
  $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU>,$<BOOL:${CW_THREAD_SANITIZER_FL}>>:-fsanitize=thread>    
  $<$<AND:$<CONFIG:Debug>,$<BOOL:${CW_COVERAGE_FL}>>:--coverage>    
)



install( TARGETS caw DESTINATION bin )

