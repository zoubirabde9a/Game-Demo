/* Textures made at run time: RGBA pixels drawn by code (icons, effects)
   uploaded straight to OpenGL, outside the asset pack. */

// NOTE(zoubir): Pixels are RGBA rows from the top. Every texture is
// filtered; pixel art still keeps square texels because the sprite shader
// only softens their edges, so Smooth changes nothing today. Returns the
// OpenGL texture id
internal u32
RenderUploadTexture(open_gl *OpenGL, u32 Width, u32 Height, u32 *Pixels,
                    bool32 Smooth)
{
    u32 ID = 0;
    OpenGL->glGenTextures(1, &ID);
    OpenGL->glBindTexture(GL_TEXTURE_2D, ID);
    OpenGL->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    (void)Smooth;
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return ID;
}
