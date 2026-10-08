/* Reaction words: a short word over a unit when something happens to it
   that health alone does not show.

   - A status effect starting on a player or monster
     (sim/status_effects.cpp): its name in the effect's colour
     ("Stunned", "Rooted", "Burning"), so the player hit by it knows why
     they cannot move and the player who landed it knows it took. Read
     from the status timers, which replicas carry as bits, so it shows
     the same online with nothing extra sent. An effect refreshed while
     it runs shows nothing.
   - A hit that did nothing because the player was shielded or mid-dash
     (sim/hit.cpp emits SimBurst_Blocked or SimBurst_Dodged), or that a
     monster's shell mostly turned away (sim/entity.cpp): "Blocked" or
     "Dodged" where it landed, so neither side thinks the hit was lost.
     These come as bursts (fx_bursts.cpp calls AddBurstWord), which the
     server already forwards.
   - A monster that starts coming for the local player: a big "!" over
     it, the moment the player steps inside its AggroRange while the
     nearest living player to it (UpdateMonster, sim/update.cpp). Worked
     out here from positions, so nothing extra is sent; it clears once
     the player is REACTION_ALERT_SLACK past the range again, so walking
     along the edge does not flicker. Not in dungeons, where the server's
     threat table picks the target (sim/dungeon/threat.cpp).

   Words starting together over one unit stack upward. Entry points:
   UpdateReactionWords and DrawReactionWords from player_fx.cpp. */

#define MAX_REACTION_WORDS 48
#define REACTION_WORD_SECONDS 1.1f
#define REACTION_WORD_RISE 18.f
#define REACTION_WORD_POP_SECONDS 0.12f
// NOTE(zoubir): a shower of hits on one shielded player says so once
// per this many seconds, within this many pixels
#define REACTION_WORD_REPEAT_SECONDS 0.4f
#define REACTION_WORD_REPEAT_PIXELS 40.f
#define REACTION_WORD_TRACKED ArrayCount(((world *)0)->Entities)
#define REACTION_WORD_NO_ENTITY 0xFFFFFFFF
#define REACTION_ALERT_SLACK 60.f
#define REACTION_ALERT_COLOR UI_RGBA(255, 120, 60, 255)

struct reaction_word
{
    char *Text;
    u32 Color;
    // NOTE(zoubir): the word follows this entity while it is there, else
    // stays where it started
    u32 EntityID;
    u32 EntityIndex;
    v3 Position;
    u8 Stack;
    // NOTE(zoubir): the "!" of a monster coming for the player: a bigger
    // font and a bounce
    bool32 Alert;
    float Age;
};

struct reaction_words
{
    reaction_word Words[MAX_REACTION_WORDS];
    u32 Count;

    // NOTE(zoubir): per entity slot, which entity it held last frame
    // (its ID + 1, 0 for none) and which effects ran on it; a slot reused
    // by another entity restarts
    u32 SeenID[REACTION_WORD_TRACKED];
    u16 Running[REACTION_WORD_TRACKED];
    // NOTE(zoubir): per monster slot, whether it is coming for the local
    // player
    u8 Alerted[REACTION_WORD_TRACKED];
};

internal reaction_words *
GetReactionWords(app_state *AppState)
{
    if (!AppState->ReactionWords)
    {
        AppState->ReactionWords = AllocateStruct(&AppState->MemoryArena, reaction_words);
        *AppState->ReactionWords = {};
    }
    return AppState->ReactionWords;
}

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

internal reaction_word *
AddReactionWord(reaction_words *Fx, char *Text, u32 Color, v3 Position,
                u32 EntityID, u32 EntityIndex, u8 Stack)
{
    reaction_word *Word = 0;
    if (Fx->Count < MAX_REACTION_WORDS)
    {
        Word = &Fx->Words[Fx->Count++];
        Word->Text = Text;
        Word->Color = Color;
        Word->Position = Position;
        Word->EntityID = EntityID;
        Word->EntityIndex = EntityIndex;
        Word->Stack = Stack;
        Word->Alert = false;
        Word->Age = 0.f;
    }
    return Word;
}

// NOTE(zoubir): from AddBurst (fx_bursts.cpp) for the bursts that are a
// word; Position is the struck unit's chest
internal void
AddBurstWord(app_state *AppState, sim_burst Kind, v3 Position)
{
    reaction_words *Fx = GetReactionWords(AppState);
    char *Text = Kind == SimBurst_Blocked ? (char *)"Blocked" : (char *)"Dodged";
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        reaction_word *Word = &Fx->Words[Index];
        if (Word->Text == Text && Word->Age < REACTION_WORD_REPEAT_SECONDS &&
            LengthSq(Word->Position.XY - Position.XY) < Square(REACTION_WORD_REPEAT_PIXELS))
        {
            return;
        }
    }
    u32 Color = Kind == SimBurst_Blocked ? UI_RGBA(150, 215, 255, 255) :
        UI_RGBA(235, 240, 255, 255);
    // NOTE(zoubir): from the chest to the head's height, where status
    // words start
    Position.Z += 14.f;
    AddReactionWord(Fx, Text, Color, Position, 0, REACTION_WORD_NO_ENTITY, 0);
}

// NOTE(zoubir): whether Monster is after Local, as UpdateMonster picks
// outside dungeons: the nearest living player, within AggroRange (plus
// Slack, to keep an alert going)
internal bool32
IsMonsterAfter(world *World, world_entity *Monster, world_entity *Local, float Slack)
{
    monster_stats *Stats = GetMonsterStats(Monster->MonsterKind);
    float Mine = LengthSq(Local->Position.XY - Monster->Position.XY);
    if (Stats->AggroRange <= 0.f || Mine > Square(Stats->AggroRange + Slack))
    {
        return false;
    }
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Other = &World->Entities[Index];
        if (Other != Local && Other->IsPresent && Other->Type == EntityType_Player &&
            Other->Hp > 0.f &&
            LengthSq(Other->Position.XY - Monster->Position.XY) < Mine)
        {
            return false;
        }
    }
    return true;
}

internal void
UpdateReactionWords(app_state *AppState, float DeltaTime)
{
    reaction_words *Fx = GetReactionWords(AppState);
    world *World = &AppState->World;
    world_entity *Local = GetLocalPlayer(AppState);
    u32 Count = Minimum(World->EntityCount, (u32)REACTION_WORD_TRACKED);
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
        bool32 Alerted = false;
        if (Entity->Type == EntityType_Monster && Local && Local->IsPresent &&
            Local->Hp > 0.f && !IsDungeon(AppState))
        {
            Alerted = IsMonsterAfter(World, Entity, Local,
                                     Fx->Alerted[Index] ? REACTION_ALERT_SLACK : 0.f);
        }
        // NOTE(zoubir): an entity seen for the first time (a join, a
        // spawn, the first snapshot) does not announce what it came with
        if (Fx->SeenID[Index] != Entity->ID + 1)
        {
            Fx->SeenID[Index] = Entity->ID + 1;
            Fx->Running[Index] = Running;
            Fx->Alerted[Index] = (u8)Alerted;
            continue;
        }
        if (Alerted && !Fx->Alerted[Index])
        {
            reaction_word *Word = AddReactionWord(Fx, "!", REACTION_ALERT_COLOR,
                                                  Entity->Position, Entity->ID, Index, 0);
            if (Word)
            {
                Word->Alert = true;
            }
        }
        Fx->Alerted[Index] = (u8)Alerted;
        u16 Started = Running & ~Fx->Running[Index];
        Fx->Running[Index] = Running;
        u8 Stack = 0;
        for(u32 Effect = 1; Started && Effect < StatusEffect_Count; Effect++)
        {
            if (Started & (1 << Effect))
            {
                AddReactionWord(Fx, StatusTable[Effect].Name, StatusWordColor(Effect),
                                Entity->Position, Entity->ID, Index, Stack++);
            }
        }
    }

    for(u32 Index = 0; Index < Fx->Count;)
    {
        reaction_word *Word = &Fx->Words[Index];
        Word->Age += DeltaTime;
        if (Word->Age >= REACTION_WORD_SECONDS)
        {
            *Word = Fx->Words[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }
}

// NOTE(zoubir): a word over a unit starts above its health bar; it pops
// in with a short drop into place, then rises and fades over its last 40%
internal void
DrawReactionWords(render_context *RenderContext, app_state *AppState, v3 CameraOffset)
{
    reaction_words *Fx = GetReactionWords(AppState);
    world *World = &AppState->World;
    font *Font = AppState->Fonts.Small;
    float Line = UILineHeight(Font);
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        reaction_word *Word = &Fx->Words[Index];
        v2 Head;
        if (Word->EntityIndex != REACTION_WORD_NO_ENTITY)
        {
            world_entity *Entity = Word->EntityIndex < World->EntityCount ?
                &World->Entities[Word->EntityIndex] : 0;
            if (!Entity || !Entity->IsPresent || Entity->ID != Word->EntityID)
            {
                continue;
            }
            Head = Entity->Position.XY - CameraOffset.XY -
                V2(0.f, Entity->Position.Z + Entity->Dimensions.Y + 12.f);
        }
        else
        {
            Head = Word->Position.XY - CameraOffset.XY - V2(0.f, Word->Position.Z + 12.f);
        }
        float Progress = Word->Age / REACTION_WORD_SECONDS;
        float Fade = Progress < 0.6f ? 1.f : (1.f - Progress) / 0.4f;
        float Pop = Word->Age < REACTION_WORD_POP_SECONDS ?
            1.f - Word->Age / REACTION_WORD_POP_SECONDS : 0.f;
        float Y = Head.Y - REACTION_WORD_RISE * Progress - (float)Word->Stack * Line -
            6.f * Pop - Line;

        u32 Alpha = (u32)(255.f * Fade);
        if (Alpha < 20)
        {
            continue;
        }
        u32 Color = (Word->Color & 0x00FFFFFF) | (Alpha << 24);
        u32 Shadow = UI_RGBA(10, 8, 14, (u32)(0.8f * (float)Alpha));
        font *WordFont = Font;
        if (Word->Alert)
        {
            // NOTE(zoubir): the "!" stays put and hops twice instead of
            // rising: it marks the monster, it is not a hit
            WordFont = AppState->Fonts.Title;
            float Hop = Absolute(Sin(2.f * Pi32 * Minimum(Progress, 0.5f) * 2.f));
            Y = Head.Y - 10.f * Hop - UILineHeight(WordFont);
        }
        UIText(RenderContext, WordFont, Head.X + 2.f, Y + 2.f, Word->Text, Shadow, UIAlign_Center);
        UIText(RenderContext, WordFont, Head.X, Y, Word->Text, Color, UIAlign_Center);
    }
}
