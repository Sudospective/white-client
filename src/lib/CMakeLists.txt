set(SOCKET_CPP_BUILD_WITHOUT_SECURE_CLASSES 1)
add_subdirectory(socket-cpp)

set_property(TARGET socket PROPERTY FOLDER Libraries)
