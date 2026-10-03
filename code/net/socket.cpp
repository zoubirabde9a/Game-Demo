#include "socket.h"

#if defined(_WIN32)
#include "platform/win32_socket.cpp"
#else
#include "platform/posix_socket.cpp"
#endif
