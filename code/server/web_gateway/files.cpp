/* Serving a folder over plain HTTP, so the browser game can be tried on
   one machine with the gateway alone (web_gateway --files web\build). For
   testing only: no caching, no ranges, one whole file per request, and
   paths with ".." or a backslash are refused. The live server's page
   comes from Caddy instead. */

internal const char *
GatewayContentType(const char *Path)
{
    const char *Dot = strrchr(Path, '.');
    if (!Dot) return "application/octet-stream";
    if (strcmp(Dot, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(Dot, ".js") == 0) return "text/javascript";
    if (strcmp(Dot, ".wasm") == 0) return "application/wasm";
    if (strcmp(Dot, ".css") == 0) return "text/css";
    if (strcmp(Dot, ".png") == 0) return "image/png";
    return "application/octet-stream";
}

internal void
GatewaySendFile(web_gateway *Gateway, gateway_link *Link, const char *UrlPath)
{
    char Path[512];
    char Clean[256];
    u32 Length = 0;
    for (const char *C = UrlPath; *C && *C != '?' && *C != '#' && Length + 1 < sizeof(Clean); C++)
    {
        Clean[Length++] = *C;
    }
    Clean[Length] = 0;
    if (strcmp(Clean, "/") == 0) snprintf(Clean, sizeof(Clean), "/index.html");

    FILE *File = 0;
    if (Clean[0] == '/' && !strstr(Clean, "..") && !strchr(Clean, '\\'))
    {
        snprintf(Path, sizeof(Path), "%s%s", Gateway->Files, Clean);
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // fopen is portable; MSVC's "unsafe" warning does not apply
#endif
        File = fopen(Path, "rb");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    }
    if (!File)
    {
        GatewayQueueText(Link, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        Link->State = GatewayLink_Closing;
        return;
    }
    fseek(File, 0, SEEK_END);
    long Size = ftell(File);
    fseek(File, 0, SEEK_SET);
    char Header[256];
    snprintf(Header, sizeof(Header),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n"
             "Cache-Control: no-cache\r\nConnection: close\r\n\r\n",
             GatewayContentType(Clean), Size);
    GatewayQueueText(Link, Header);
    u8 Chunk[65536];
    size_t Got;
    while ((Got = fread(Chunk, 1, sizeof(Chunk), File)) > 0)
    {
        GatewayQueue(Link, Chunk, (u32)Got, true);
    }
    fclose(File);
    Link->State = GatewayLink_Closing;
}
