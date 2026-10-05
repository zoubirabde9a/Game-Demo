/* Render targets: drawing into a texture instead of the window, so a
   full-screen shader can then read what was drawn and bend it (the time
   rewind's post-process, client/rewind_fx/time_warp.cpp).

   BeginRenderTarget sends what follows into the target's texture, sized
   to the window's pixels (the viewport, which on a scaled display is
   larger than the window's size in game units) and remade when that
   changes;
   EndRenderTarget sends drawing back to the window. A driver that cannot
   make one marks it Failed, and BeginRenderTarget returns false from then
   on, so callers just draw to the window as before. */

struct render_target
{
    u32 Framebuffer;
    u32 Texture;
    u32 Width;
    u32 Height;
    bool32 Failed;
};

internal bool32
BeginRenderTarget(render_context *RenderContext, render_target *Target)
{
    open_gl *OpenGL = RenderContext->OpenGL;
    if (Target->Failed || !OpenGL->glGenFramebuffers || !OpenGL->glBindFramebuffer ||
        !OpenGL->glFramebufferTexture2D || !OpenGL->glCheckFramebufferStatus ||
        !OpenGL->glGetIntegerv)
    {
        return false;
    }
    GLint Viewport[4] = {};
    OpenGL->glGetIntegerv(GL_VIEWPORT, Viewport);
    u32 Width = (u32)Viewport[2];
    u32 Height = (u32)Viewport[3];
    if (Width == 0 || Height == 0)
    {
        return false;
    }
    if (!Target->Framebuffer)
    {
        OpenGL->glGenFramebuffers(1, &Target->Framebuffer);
        OpenGL->glGenTextures(1, &Target->Texture);
    }
    OpenGL->glBindFramebuffer(GL_FRAMEBUFFER, Target->Framebuffer);
    if (Target->Width != Width || Target->Height != Height)
    {
        Target->Width = Width;
        Target->Height = Height;
        OpenGL->glBindTexture(GL_TEXTURE_2D, Target->Texture);
        OpenGL->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, 0);
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        OpenGL->glBindTexture(GL_TEXTURE_2D, 0);
        OpenGL->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D, Target->Texture, 0);
        if (OpenGL->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            OpenGL->glBindFramebuffer(GL_FRAMEBUFFER, 0);
            Target->Failed = true;
            return false;
        }
    }
    OpenGL->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return true;
}

internal void
EndRenderTarget(render_context *RenderContext)
{
    RenderContext->OpenGL->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
