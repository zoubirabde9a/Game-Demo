/* Replay checker: plays a replay (server --record <file>) back through a
   fresh game and checks the world hash after every tick against the one
   the server recorded (server/replay.cpp).
   Usage: replay <file>
   Prints the ticks played and, when the match did not come out the same,
   the first tick where it went another way. Exit code 0 when every tick
   matched, 1 when one did not or the file is unreadable. A replay from
   another build (another content id) is played anyway, with a warning:
   its rules may differ, and the first mismatch says from when. Built by
   build_server.bat / .sh. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER)
#pragma warning(disable: 4996) // fopen: a tool reading one file it was handed
#endif
#include "../server/game_api.h"
#include "../net/connections.h"
#include "../server/sim_game.cpp"

int
main(int ArgCount, char **Args)
{
    if (ArgCount < 2)
    {
        fprintf(stderr, "usage: replay <file>\n");
        return 1;
    }
    FILE *File = fopen(Args[1], "rb");
    if (!File)
    {
        fprintf(stderr, "cannot open %s\n", Args[1]);
        return 1;
    }
    fseek(File, 0, SEEK_END);
    long Size = ftell(File);
    fseek(File, 0, SEEK_SET);
    u8 *Data = (u8 *)malloc(Size > 0 ? (size_t)Size : 1);
    size_t Read = (Size > 0 && Data) ? fread(Data, 1, (size_t)Size, File) : 0;
    fclose(File);
    if (Size <= 0 || !Data || Read != (size_t)Size)
    {
        fprintf(stderr, "cannot read %s\n", Args[1]);
        return 1;
    }

    u32 Header[2] = {};
    memcpy(Header, Data, Size >= 8 ? 8 : 0);
    if (Header[0] == REPLAY_MAGIC && Header[1] != REPLAY_VERSION)
    {
        printf("replay: written in format %u; this build reads format %u\n",
               Header[1], REPLAY_VERSION);
        free(Data);
        return 1;
    }
    static server_game Game;
    replay_result Result = PlayReplay(&Game, Data, (u32)Size);
    free(Data);
    if (Result.ContentId && Result.ContentId != SimContentId())
    {
        printf("replay: recorded by another build (content id %08x, this one %08x)\n",
               Result.ContentId, SimContentId());
    }
    printf("replay: %u ticks on map %s, final world hash %08x\n", Result.Ticks,
           Result.MapId < MapId_Count ? GetMapDef((map_id)Result.MapId)->Name : "?",
           Result.FinalHash);
    if (!Result.Readable)
    {
        printf("replay: the file is cut short or damaged after tick %u\n", Result.Ticks);
        return 1;
    }
    if (Result.Mismatches)
    {
        printf("replay: %u ticks came out differently, the first at tick %u\n",
               Result.Mismatches, Result.FirstMismatch);
        return 1;
    }
    printf("replay: every tick matched\n");
    return 0;
}
