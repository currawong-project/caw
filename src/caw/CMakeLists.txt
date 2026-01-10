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

target_link_libraries(caw PRIVATE cw)


install( TARGETS caw DESTINATION bin )

