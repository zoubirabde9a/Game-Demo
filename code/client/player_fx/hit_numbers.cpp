/* Hit numbers: when a player's or monster's health drops, the amount
   floats up from above it and fades, with a small burst of sparks where
   it was hit. Read from health alone, so it works the same offline and
   on replicas, with nothing extra sent. Damage over time (burning,
   poison) is added up and shown at most every HIT_NUMBER_GAP seconds per
   target instead of every frame. Hits on the local player are red.
   Healing on a player (a dungeon healer's spells) rises in green with a
   plus, added up the same way; coming back to life is not counted.

   The hit counter: monsters near the local player that lose at least
   HIT_COMBO_MIN at once count as hits; hits closer together than
   HIT_COMBO_WINDOW add up, and the count shows over the player's head
   from the second one, flashing white on each new hit. */

#define MAX_HIT_NUMBERS 64
#define HIT_NUMBER_SECONDS 0.7f
#define HIT_NUMBER_RISE 26.f
#define HIT_NUMBER_GAP 0.2f
#define HIT_SPARKS 6
#define HIT_SPARK_SECONDS 0.18f
#define HIT_SPARK_REACH 14.f
#define HIT_COMBO_MIN 5.f
#define HIT_COMBO_WINDOW 1.5f
#define HIT_COMBO_RANGE 350.f
#define HIT_COMBO_FLASH_SECONDS 0.15f
// NOTE(zoubir): health tracking covers every world entity slot
#define HIT_TRACKED ArrayCount(((world *)0)->Entities)

struct hit_number
{
    v2 Position;
    float Age;
    u32 Amount;
    bool32 OnLocalPlayer;
    bool32 Heal;
};

struct hit_numbers
{
    hit_number Numbers[MAX_HIT_NUMBERS];
    u32 Count;

    // NOTE(zoubir): per entity slot, what it looked like last frame; a
    // slot reused by another entity (other type or max health) restarts
    u8 Seen[HIT_TRACKED];
    u8 Type[HIT_TRACKED];
    float MaxHp[HIT_TRACKED];
    float LastHp[HIT_TRACKED];
    float Pending[HIT_TRACKED];
    float PendingHeal[HIT_TRACKED];
    float Gap[HIT_TRACKED];

    u32 Combo;
    float ComboLeft;
    float ComboFlash;
};

inline bool32
ShowsHitNumbers(world_entity *Entity)
{
    bool32 Result = Entity->IsPresent && Entity->MaxHp > 0.f &&
        (Entity->Type == EntityType_Monster ||
         Entity->Type == EntityType_Player);
    return Result;
}

internal void
AddHitNumber(hit_numbers *Fx, world_entity *Entity, u32 Amount,
             bool32 OnLocalPlayer, bool32 Heal = false)
{
    if (Fx->Count < MAX_HIT_NUMBERS)
    {
        hit_number *Number = &Fx->Numbers[Fx->Count++];
        // NOTE(zoubir): above the sprite, which stands up from the feet
        Number->Position = Entity->Position.XY -
            V2(0.f, Entity->Position.Z + 0.9f * Entity->Dimensions.Y);
        Number->Age = 0.f;
        Number->Amount = Amount;
        Number->OnLocalPlayer = OnLocalPlayer;
        Number->Heal = Heal;
    }
}

internal void
UpdateHitNumbers(hit_numbers *Fx, app_state *AppState, float DeltaTime)
{
    world *World = &AppState->World;
    world_entity *Local = GetLocalPlayer(AppState);
    u32 Count = Minimum(World->EntityCount, (u32)HIT_TRACKED);
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (!ShowsHitNumbers(Entity))
        {
            Fx->Seen[Index] = false;
            continue;
        }
        if (!Fx->Seen[Index] || Fx->Type[Index] != (u8)Entity->Type ||
            Fx->MaxHp[Index] != Entity->MaxHp)
        {
            Fx->Seen[Index] = true;
            Fx->Type[Index] = (u8)Entity->Type;
            Fx->MaxHp[Index] = Entity->MaxHp;
            Fx->LastHp[Index] = Entity->Hp;
            Fx->Pending[Index] = 0.f;
            Fx->PendingHeal[Index] = 0.f;
            Fx->Gap[Index] = 0.f;
            continue;
        }

        float Before = Fx->LastHp[Index];
        float Lost = Before - Entity->Hp;
        Fx->LastHp[Index] = Entity->Hp;
        if (Lost > 0.f)
        {
            Fx->Pending[Index] += Lost;
        }
        else if (Lost < 0.f && Before > 0.f && Entity->Type == EntityType_Player)
        {
            Fx->PendingHeal[Index] -= Lost;
        }
        // NOTE(zoubir): damage over time comes in small ticks and is not a hit
        if (Lost >= HIT_COMBO_MIN && Local && Entity->Type == EntityType_Monster &&
            LengthSq(Entity->Position.XY - Local->Position.XY) <
            Square(HIT_COMBO_RANGE))
        {
            Fx->Combo = Fx->ComboLeft > 0.f ? Fx->Combo + 1 : 1;
            Fx->ComboLeft = HIT_COMBO_WINDOW;
            Fx->ComboFlash = HIT_COMBO_FLASH_SECONDS;
        }
        Fx->Gap[Index] = Maximum(0.f, Fx->Gap[Index] - DeltaTime);
        if (Fx->Pending[Index] >= 1.f && Fx->Gap[Index] <= 0.f)
        {
            AddHitNumber(Fx, Entity, (u32)(Fx->Pending[Index] + 0.5f),
                         Entity == Local);
            Fx->Pending[Index] = 0.f;
            Fx->Gap[Index] = HIT_NUMBER_GAP;
        }
        else if (Fx->PendingHeal[Index] >= 1.f && Fx->Gap[Index] <= 0.f)
        {
            AddHitNumber(Fx, Entity, (u32)(Fx->PendingHeal[Index] + 0.5f), false, true);
            Fx->PendingHeal[Index] = 0.f;
            Fx->Gap[Index] = HIT_NUMBER_GAP;
        }
    }

    Fx->ComboLeft = Maximum(0.f, Fx->ComboLeft - DeltaTime);
    Fx->ComboFlash = Maximum(0.f, Fx->ComboFlash - DeltaTime);
    if (Fx->ComboLeft <= 0.f)
    {
        Fx->Combo = 0;
    }

    for(u32 Index = 0; Index < Fx->Count;)
    {
        hit_number *Number = &Fx->Numbers[Index];
        Number->Age += DeltaTime;
        if (Number->Age >= HIT_NUMBER_SECONDS)
        {
            *Number = Fx->Numbers[--Fx->Count];
        }
        else
        {
            Index++;
        }
    }
}

// NOTE(zoubir): the number rises fast then eases, fading over its last
// 30%; the sparks fly out from where it started in its first moments
internal void
DrawHitNumbers(render_context *RenderContext, app_state *AppState,
               hit_numbers *Fx, v3 CameraOffset)
{
    font *Font = AppState->Fonts.Body;
    for(u32 Index = 0; Index < Fx->Count; Index++)
    {
        hit_number *Number = &Fx->Numbers[Index];
        float Progress = Number->Age / HIT_NUMBER_SECONDS;
        float Rise = 1.f - (1.f - Progress) * (1.f - Progress);
        float Fade = Progress < 0.7f ? 1.f : (1.f - Progress) / 0.3f;
        u32 Alpha = (u32)(255.f * Fade);
        v2 Start = Number->Position - CameraOffset.XY;

        if (Number->Age < HIT_SPARK_SECONDS && !Number->Heal)
        {
            float SparkT = Number->Age / HIT_SPARK_SECONDS;
            u32 SparkColor = ((u32)(255.f * (1.f - SparkT)) << 24) | 0x0060E0FF;
            for(u32 Spark = 0; Spark < HIT_SPARKS; Spark++)
            {
                float Angle = 2.f * Pi32 * ((float)Spark + 0.5f) / (float)HIT_SPARKS;
                v2 P = Start + V2(0.f, 10.f) +
                    (HIT_SPARK_REACH * SparkT) * V2(Cos(Angle), Sin(Angle));
                DrawFxDot(RenderContext, P, 3.f, SparkColor);
            }
        }

        char Text[16];
        snprintf(Text, sizeof(Text), Number->Heal ? "+%u" : "%u", Number->Amount);
        u32 Color = Number->OnLocalPlayer ?
            UI_RGBA(255, 90, 70, Alpha) : UI_RGBA(255, 232, 120, Alpha);
        if (Number->Heal)
        {
            Color = UI_RGBA(120, 245, 140, Alpha);
        }
        if (Alpha > 30)
        {
            UIText(RenderContext, Font, Start.X,
                   Start.Y - HIT_NUMBER_RISE * Rise - UILineHeight(Font),
                   Text, Color, UIAlign_Center);
        }
    }
}

// NOTE(zoubir): the hit count over the local player's head, fading in
// the last half second of its window
internal void
DrawHitCombo(render_context *RenderContext, app_state *AppState,
             hit_numbers *Fx, v3 CameraOffset)
{
    world_entity *Local = GetLocalPlayer(AppState);
    if (Fx->Combo < 2 || !Local || !Local->IsPresent)
    {
        return;
    }
    font *Font = AppState->Fonts.Body;
    float Fade = Minimum(1.f, Fx->ComboLeft / 0.5f);
    u32 Alpha = (u32)(255.f * Fade);
    u32 Color = Fx->ComboFlash > 0.f ? UI_RGBA(255, 255, 255, Alpha) :
        UI_RGBA(255, 200, 80, Alpha);
    char Text[24];
    snprintf(Text, sizeof(Text), "%u HITS", Fx->Combo);
    v2 Head = Local->Position.XY - CameraOffset.XY -
        V2(0.f, Local->Position.Z + Local->Dimensions.Y + 28.f);
    UIText(RenderContext, Font, Head.X, Head.Y, Text, Color, UIAlign_Center);
}
