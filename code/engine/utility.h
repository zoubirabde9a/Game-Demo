#if !defined(UTILITY_H)
/* Utility types: the texture cache and its slots, used by utility.cpp.
   A slot is empty or holds a texture; the tomb state is for a removal
   that does not exist yet, and inserts reuse a tomb slot. */

#define UTILITY_H

enum texture_cache_element_type
{
    TEXTURE_CACHE_ELEMENT_EMPTY,
    TEXTURE_CACHE_ELEMENT_TOMB,
    TEXTURE_CACHE_ELEMENT_VALUE
};

struct texture_cache_element
{
    loaded_texture Texture;
    char *Key;
    u32 KeyLength;
    int Type;
};

struct texture_cache
{
    u32 AllocatedTextureCount;
    u32 TextureCount;
    char *CurrentCharacterBlockPointer;
    texture_cache_element *Textures;
};

#endif
