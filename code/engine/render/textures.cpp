/* Textures made at run time: RGBA pixels drawn by code (icons, effects)
   uploaded straight to OpenGL, outside the asset pack. */

// NOTE(zoubir): Pixels are RGBA rows from the top; Smooth filters when
// scaled (icons), otherwise pixels stay sharp (pixel art). Returns the
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
    GLint Filter = Smooth ? GL_LINEAR : GL_NEAREST;
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter);
    return ID;
}
