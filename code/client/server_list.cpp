/* Server list: the servers the game knows by name. The game joins the
   first one at launch, unless GAME_SERVER or server.txt names another
   (online_config.cpp), so nobody has to type an address to play. The
   connect screen (F4) lists them all with their ping and player count
   (server_browser.cpp).

   A server is a row: what players call it, where it is, and its address.
   The address may be a DNS name ("eu.example.com" or
   "eu.example.com:27015"), so the machine can move without a game update;
   a.b.c.d:port works too. The name the server gives itself (server
   --name) replaces Name once it answers. */

struct server_entry
{
    char *Name;
    char *Region;
    char *Address;
};

global_variable server_entry ServerList[] =
{
    // NOTE(zoubir): the live server, deploy/README.md
    {"Game Demo EU", "Europe", "152.53.147.77:27015"},
#if APP_DEV
    // NOTE(zoubir): build\server.exe on this machine
    {"This computer", "Local", "127.0.0.1:27015"},
#endif
};

#define SERVER_LIST_COUNT ArrayCount(ServerList)

// NOTE(zoubir): the list's row for Address, 0 for an address typed by hand
internal server_entry *
FindServerByAddress(char *Address)
{
    server_entry *Result = 0;
    for(u32 Index = 0; Index < SERVER_LIST_COUNT && !Result; Index++)
    {
        char *A = ServerList[Index].Address;
        char *B = Address;
        for(; *A && *B; A++, B++)
        {
            char CA = (*A >= 'A' && *A <= 'Z') ? (char)(*A + 32) : *A;
            char CB = (*B >= 'A' && *B <= 'Z') ? (char)(*B + 32) : *B;
            if (CA != CB)
            {
                break;
            }
        }
        if (*A == 0 && *B == 0)
        {
            Result = &ServerList[Index];
        }
    }
    return Result;
}
