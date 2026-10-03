/* Hit numbers: when a player's or monster's health drops, the amount
   floats up from above it and fades, with a small burst of sparks where
   it was hit. Read from health alone, so it works the same offline and
   on replicas, with nothing extra sent. Damage over time (burning,
   poison) is added up and shown at most every HIT_NUMBER_GAP seconds per
   target instead of every frame. Hits on the local player are red. */

#define MAX_HIT_NUMBERS 64
#define HIT_NUMBER_SECONDS 0.7f
#define HIT_NUMBER_RISE 26.f
#define HIT_NUMBER_GAP 0.2f
#define HIT_SPARKS 6
#define HIT_SPARK_SECONDS 0.18f
#define HIT_SPARK_REACH 14.f
// NOTE(zoubir): health tracking covers every world entity slot
#define HIT_TRACKED ArrayCount(((world *)0)->Entities)

struct hit_number
{
    v2 Position;
    float Age;
    u32 Amount;
    bool32 OnLocalPlayer;
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
    float Gap[HIT_TRACKED];
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
             bool32 OnLocalPlayer)
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
            Fx->Gap[Index] = 0.f;
            continue;
        }

        float Lost = Fx->LastHp[Index] - Entity->Hp;
        Fx->LastHp[Index] = Entity->Hp;
        if (Lost > 0.f)
        {
            Fx->Pending[Index] += Lost;
        }
        Fx->Gap[Index] = Maximum(0.f, Fx->Gap[Index] - DeltaTime);
        if (Fx->Pending[Index] >= 1.f && Fx->Gap[Index] <= 0.f)
        {
            AddHitNumber(Fx, Entity, (u32)(Fx->Pending[Index] + 0.5f),
                         Entity == Local);
            Fx->Pending[Index] = 0.f;
            Fx->Gap[Index] = HIT_NUMBER_GAP;
        }
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

        if (Number->Age < HIT_SPARK_SECONDS)
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
        snprintf(Text, sizeof(Text), "%u", Number->Amount);
        u32 Color = Number->OnLocalPlayer ?
            UI_RGBA(255, 90, 70, Alpha) : UI_RGBA(255, 232, 120, Alpha);
        if (Alpha > 30)
        {
            UIText(RenderContext, Font, Start.X,
                   Start.Y - HIT_NUMBER_RISE * Rise - UILineHeight(Font),
                   Text, Color, UIAlign_Center);
        }
    }
}
