/* Role targeting (sim/dungeon/role_abilities.cpp): in a dungeon run, who
   the local player's role spells go at, and how standard cast aims them.

   A healer's Ward and Mending Bolt and a tank's Intercept land on an
   ally, so for those roles the cursor picks allies, not foes
   (targeting.cpp asks IsCursorPickable). The ally the spell goes to, in
   order:
   1. the ally whose party frame the mouse is on (ui/dungeon/
      party_frames.cpp), the local player's own frame included, so a
      healer can heal from the frames without looking away;
   2. the ally whose body the cursor is on;
   3. the ally picked by clicking their party frame (Focus), until they
      fall or another frame is clicked; a click on the picked frame lets
      go of it;
   4. nobody: the server heals the most hurt ally in reach instead.
   The pick goes to the server as player_input.Target, like the kunai's.

   Standard cast: an ally spell casts at once (the pick is the aim); a
   ground spell (Sanctuary, Inferno) aims first like Launch did, its
   circle drawn at the cursor (previews.cpp); a foe spell (Shield Throw,
   Holy Fire, Quick Shot) aims first like the kunai, its reach and the
   foes in it drawn; a line spell (Giant Fireball, Piercing Shot) aims
   first like the fireball; a spell that needs none of these (Taunt, a
   swing in front, the right click) casts at once. Clicks on the party
   frames never fire. */

struct party_pick
{
    // NOTE(zoubir): each party frame as the HUD drew it last frame, in
    // window pixels, by player slot; Width 0 for none
    float FrameX[MAX_PLAYERS];
    float FrameY[MAX_PLAYERS];
    float FrameWidth[MAX_PLAYERS];
    float FrameHeight[MAX_PLAYERS];
    // NOTE(zoubir): the ally picked on the frames, slot + 1, 0 for none
    u32 Focus;
    // NOTE(zoubir): who the local ally spells would land on this frame,
    // slot + 1 (0 for the server's choice), and whether it was picked
    // rather than guessed
    u32 Target;
    bool32 Chosen;
};

internal party_pick *
GetPartyPick(app_state *AppState)
{
    if (!AppState->PartyPick)
    {
        AppState->PartyPick = AllocateStruct(&AppState->MemoryArena, party_pick);
        *AppState->PartyPick = {};
    }
    return AppState->PartyPick;
}

// NOTE(zoubir): the slot + 1 whose party frame the mouse is on, 0 for none
internal u32
PartyFrameUnderMouse(app_state *AppState, app_input *Input)
{
    if (!IsDungeon(AppState))
    {
        return 0;
    }
    party_pick *Pick = GetPartyPick(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (Pick->FrameWidth[SlotIndex] > 0.f &&
            IsMouseOnRectangle(Input->MouseX, Input->MouseY, Pick->FrameX[SlotIndex],
                               Pick->FrameY[SlotIndex], Pick->FrameWidth[SlotIndex],
                               Pick->FrameHeight[SlotIndex]))
        {
            return SlotIndex + 1;
        }
    }
    return 0;
}

// NOTE(zoubir): a click there picks an ally instead of casting
internal bool32
PartyFramesHaveMouse(app_state *AppState, app_input *Input)
{
    bool32 Result = PartyFrameUnderMouse(AppState, Input) != 0;
    return Result;
}

// NOTE(zoubir): each role's spell colour on the ground, 0x00BBGGRR, by
// player_role: fire for the fire mage, steel blue for the tank, a pale
// gold-green for the healer, the class colour for the later classes
internal u32
RoleSpellRGB(u32 Role)
{
    u32 Colors[3] = {0x002878FF, 0x00F0A060, 0x0080F0C0};
    u32 Result = Role < 3 ? Colors[Role] : RoleRGB(Role);
    return Result;
}

// NOTE(zoubir): the local role spell on Button, 0 when the game's own
// ability is on it
inline role_spell *
LocalRoleSpell(app_state *AppState, u32 Button)
{
    role_spell *Result = RoleSpellOnButton(AppState, GetLocalPlayer(AppState), Button);
    return Result;
}

// NOTE(zoubir): whether the local player's cursor picks allies: its class
// has an ally spell on one of its keys (the healer, and the tank's
// Intercept)
internal bool32
LocalPicksAllies(app_state *AppState)
{
    bool32 Result = false;
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        role_spell *Spell = LocalRoleSpell(AppState, RoleKeys[Key]);
        Result |= Spell && Spell->Aim == RoleAim_Ally;
    }
    return Result;
}

// NOTE(zoubir): whether the cursor may pick Unit for Local
internal bool32
IsCursorPickable(app_state *AppState, world_entity *Unit, world_entity *Local)
{
    bool32 Result = false;
    if (LocalPicksAllies(AppState))
    {
        Result = Unit != Local && Unit->IsPresent && Unit->Type == EntityType_Player &&
            !IsDeadPlayer(Unit);
    }
    else
    {
        Result = IsKunaiTarget(Unit, Local);
    }
    return Result;
}

// NOTE(zoubir): whether a role spell aims first in standard cast
inline bool32
IsAimedRoleSpell(role_spell *Spell)
{
    bool32 Result = Spell && (Spell->Aim == RoleAim_Ground || Spell->Aim == RoleAim_Foe ||
                              Spell->Aim == RoleAim_Line);
    return Result;
}

// NOTE(zoubir): the local role's foe spell being aimed, 0 for none
inline role_spell *
AimedFoeSpell(app_state *AppState, u32 Aiming)
{
    role_spell *Spell = Aiming ? LocalRoleSpell(AppState, Aiming) : 0;
    role_spell *Result = (Spell && Spell->Aim == RoleAim_Foe) ? Spell : 0;
    return Result;
}

// NOTE(zoubir): Targeted (the buttons standard cast aims) as the local
// role changes them: a role spell aims when it lands on the ground, on a
// foe or along a line
internal u32
RoleTargetedButtons(app_state *AppState, u32 Targeted)
{
    u32 Result = Targeted;
    for(u32 Key = 0; Key < ROLE_KEYS; Key++)
    {
        role_spell *Spell = LocalRoleSpell(AppState, RoleKeys[Key]);
        if (Spell)
        {
            Result &= ~RoleKeys[Key];
            if (IsAimedRoleSpell(Spell))
            {
                Result |= RoleKeys[Key];
            }
        }
    }
    return Result;
}

// NOTE(zoubir): the living player in slot + 1 Slot, 0 for none
inline world_entity *
PartyMember(app_state *AppState, u32 Slot)
{
    world_entity *Result = 0;
    if (Slot > 0 && Slot <= MAX_PLAYERS)
    {
        player_slot *Member = &AppState->Players[Slot - 1];
        if (Member->Active && Member->Entity && Member->Entity->IsPresent &&
            !IsDeadPlayer(Member->Entity))
        {
            Result = Member->Entity;
        }
    }
    return Result;
}

// NOTE(zoubir): once a frame from UpdateCursorTarget while the local role
// picks allies: Picked is the ally body under the cursor (local entity
// index + 1); returns player_input.Target (the rules at the top)
internal u32
PickAllyTarget(app_state *AppState, app_input *Input, u32 Picked)
{
    party_pick *Pick = GetPartyPick(AppState);
    world *World = &AppState->World;
    if (!PartyMember(AppState, Pick->Focus))
    {
        Pick->Focus = 0;
    }
    world_entity *Ally = PartyMember(AppState, PartyFrameUnderMouse(AppState, Input));
    if (!Ally && Picked > 0 && Picked <= World->EntityCount)
    {
        Ally = &World->Entities[Picked - 1];
    }
    if (!Ally)
    {
        Ally = PartyMember(AppState, Pick->Focus);
    }
    Pick->Chosen = Ally != 0;
    Pick->Target = Ally ? Ally->PlayerIndex + 1 : 0;
    u32 Result = Ally ? (u32)(Ally - World->Entities) + 1 : 0;
    return Result;
}

// NOTE(zoubir): from the party frames: a click on Slot's frame picks it,
// a click on the picked one lets go
internal void
ClickPartyFrame(app_state *AppState, u32 Slot)
{
    party_pick *Pick = GetPartyPick(AppState);
    Pick->Focus = (Pick->Focus == Slot + 1) ? 0 : Slot + 1;
}
