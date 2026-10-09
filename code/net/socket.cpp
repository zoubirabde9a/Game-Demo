/* Sockets: the UDP calls the network layer makes (socket.h), with one
   implementation per system: platform/win32_socket.cpp on Windows,
   platform/web_socket.cpp in the browser (WebSockets to the web gateway),
   platform/posix_socket.cpp on Linux (the live server) and elsewhere. */

#include "socket.h"

#if defined(_WIN32)
#include "platform/win32_socket.cpp"
#elif defined(__EMSCRIPTEN__)
#include "platform/web_socket.cpp"
#else
#include "platform/posix_socket.cpp"
#endif
