set(src
  main.cpp
)

add_executable(${PROJECT_NAME} ${src})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_link_libraries(${PROJECT_NAME}
  socket
)

target_include_directories(${PROJECT_NAME} PUBLIC
  lib/socket-cpp/Socket/
)
