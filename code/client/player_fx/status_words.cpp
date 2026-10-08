/* Status words: when a status effect starts on a player or monster
   (sim/status_effects.cpp), its name pops up over the unit in the
   effect's colour and drifts up ("Stunned", "Rooted", "Burning"), so
   the player hit by it knows why they cannot move and the player who
   landed it knows it took. Read from the status timers alone, which
   replicas carry as bits, so it shows the same online with nothing
   extra sent. An effect refreshed while it runs shows nothing; several
   starting together stack upward. */

#define MAX_STATUS_WORDS 48
#define STATUS_WORD_SECONDS 1.1f
#define STATUS_WORD_RISE 18.f
#define STATUS_WORD_POP_SECONDS 0.12f
#define STATUS_WORD_TRACKED ArrayCount(((world *)0)->Entities)

struct status_word
{
    u32 EntityID;
    u32 EntityIndex;
    u8 Effect;
    u8 Stack;
    float Age;
};

struct status_words
{
    status_word Words[MAX_STATUS_WORDS];
    u32 Count;

    // NOTE(zoubir): per entity slot, which entity it held last frame and
    // which effects ran on it; a slot reused by another entity restarts
    u32 SeenID[STATUS_WORD_TRACKED];
    u16 Running[STATUS_WORD_TRACKED];
};

// NOTE(zoubir): the effects with no pip colour of their own get one here
inline u32
StatusWordColor(u32 Effect)
{
    u32 Result = StatusTable[Effect].Color;
    if (Effect == StatusEffect_Stunned)
    {
        Result = UI_RGBA(255, 236, 110, 255);
    }
    else if (!Result)
    {
        Result = UI_RGBA(200, 150, 255, 255);
    }
    return Result;
}

internal void
UpdateStatusWords(status_words *Fx, app_state *AppState, float DeltaTime)
{
    world *World = &AppState->World;
    u32 Count = Minimum(World->EntityCount, (u32)STATUS_WORD_TRACKED);
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (!Entity->IsPresent || Entity->Hp <= 0.f ||
            (Entity->Type != EntityType_Player && Entity->Type != EntityType_Monster))
        {
            Fx->SeenID[Index] = 0;
            continue;
        }
        u16 Running = 0;
        for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
        {
            if (Entity->StatusTimers[Effect] > 0.f)
            {
                Running |= (u16)(1 << Effect);
            }
        }
        // NOTE(zoubir): an entity seen for the first time (a join, a
        // spawn, the first snapshot) does not announce what it came with
        if (Fx->SeenID[Index] != Entity->ID + 1)
        {
            Fx->SeenID[Index] = Entity->ID + 1;
            Fx->Running[Index] = Running;
            continue;
        }
        u16 Started = Running & ~Fx->Running[Index];
        Fx->Running[Index] = Running;
        u8 Stack = 0;
        for(u32 Effect = 1; Started && Effect < StatusEffect_Count; Effect++)
        {
            if ((Started & (1 << Effect)) && Fx->Count < MAX_STATUS_WORDS)
            {
                status_word *Word = &Fx->Words[Fx->Count++];
                Word->EntityID = Entity->ID;
                Word->EntityIndex = Index;
                Word->Effect = (u8)Effect;
                Word->Stack = Stack++;
                Word->Age = 0.f;
            }
        }
    }

    for(u32 Index = 0; Index < Fx->Count;)
    {
        status_word *Word = &Fx->Words[Index];
        Word->Age += DeltaTime;
        if (Word->Age >= STATUS_WORD_SECONDS)
        {
            *Word = Fx->Words[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }
}

// NOTE(zoubir): the word follows its unit, starting above the hit
// numbers' lane; it pops in a little large, then rises and fades over
// its last 40%
internal void
DrawStatusWords(render_context *RenderContext, app_state *AppState,
                status_words *Fx, v3 CameraOffset)
{
    world *World = &AppState->World;
    font *Font = AppState->Fonts.Small;
    float Line = UILineHeight(Font);
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        status_word *Word = &Fx->Words[Index];
        if (Word->EntityIndex >= World->EntityCount)
        {
            continue;
        }
        world_entity *Entity = &World->Entities[Word->EntityIndex];
        if (!Entity->IsPresent || Entity->ID != Word->EntityID)
        {
            continue;
        }
        float Progress = Word->Age / STATUS_WORD_SECONDS;
        float Fade = Progress < 0.6f ? 1.f : (1.f - Progress) / 0.4f;
        float Rise = STATUS_WORD_RISE * Progress;
        // NOTE(zoubir): the pop: a short drop into place from above
        float Pop = Word->Age < STATUS_WORD_POP_SECONDS ?
            1.f - Word->Age / STATUS_WORD_POP_SECONDS : 0.f;
        v2 Head = Entity->Position.XY - CameraOffset.XY -
            V2(0.f, Entity->Position.Z + Entity->Dimensions.Y + 12.f);
        float Y = Head.Y - Rise - (float)Word->Stack * Line - 6.f * Pop - Line;

        u32 Alpha = (u32)(255.f * Fade);
        if (Alpha < 20)
        {
            continue;
        }
        u32 Color = (StatusWordColor(Word->Effect) & 0x00FFFFFF) | (Alpha << 24);
        u32 Shadow = UI_RGBA(10, 8, 14, (u32)(0.8f * (float)Alpha));
        char *Name = StatusTable[Word->Effect].Name;
        UIText(RenderContext, Font, Head.X + 1.f, Y + 1.f, Name, Shadow, UIAlign_Center);
        UIText(RenderContext, Font, Head.X, Y, Name, Color, UIAlign_Center);
    }
}
