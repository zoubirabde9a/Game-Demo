/* The browser game: the entry point web\build.bat compiles with
   Emscripten. It makes a WebGL 2 context on the page's canvas, hands the
   game its memory and the platform calls, and runs one game frame per
   1/60 s from the browser's animation frames, with input, sound and the
   canvas size, as the Windows layer does. The parts are in web/; the
   whole game comes in through app.cpp, network code included (its
   packets go over WebSockets, net/platform/web_socket.cpp). On a touch
   screen the page draws a joystick and ability buttons, and web/touch.cpp
   turns them into keys and the cursor.

   The page (web/shell.html) can pass settings in its address: any
   GAME_... parameter becomes that environment variable, as on the
   desktop, so ?GAME_SERVER=127.0.0.1:27015 joins a local server. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include "../app_platform.h"

#include "../app.cpp"

#include "web/files.cpp"
#include "web/input.cpp"
#include "web/touch.cpp"
#include "web/sound.cpp"
#include "web/opengl.cpp"

#define WEB_FRAME_SECONDS (1.f / 60.f)

struct web_frame_loop
{
    thread_context Thread;
    open_gl OpenGL;
    app_memory Memory;
    app_input Inputs[2];
    app_input *OldInput;
    app_input *NewInput;
    double LastNow;
    double Owed; // seconds of game time the browser's clock is ahead by
    int CanvasWidth;
    int CanvasHeight;
};

global_variable web_frame_loop WebLoop;

EM_JS(int, WebIsFullscreen, (), {
    return document.fullscreenElement ? 1 : 0;
});

// NOTE(zoubir): a page may go full screen only shortly after a click or a
// key; if this frame is too late, the next mouse or key release does it
EM_JS(void, WebAskFullscreen, (int On), {
    var Apply = function()
    {
        if (On && !document.fullscreenElement && document.documentElement.requestFullscreen)
            return document.documentElement.requestFullscreen();
        if (!On && document.fullscreenElement) return document.exitFullscreen();
        return Promise.resolve();
    };
    Apply().catch(function()
    {
        var Retry = function()
        {
            window.removeEventListener("mouseup", Retry, true);
            window.removeEventListener("keyup", Retry, true);
            Apply().catch(function() {});
        };
        window.addEventListener("mouseup", Retry, true);
        window.addEventListener("keyup", Retry, true);
    });
});

// NOTE(zoubir): the page's GAME_... address parameters, one NAME=value
// per line
EM_JS(void, WebPageSettings, (char *Out, int OutSize), {
    Out = Number(Out);
    var Lines = "";
    new URLSearchParams(location.search).forEach(function(Value, Name)
    {
        if (/^GAME_[A-Z_]+$/.test(Name)) Lines += Name + "=" + Value + "\n";
    });
    stringToUTF8(Lines, Out, OutSize);
});

internal void
WebApplyPageSettings()
{
    char Settings[2048];
    WebPageSettings(Settings, (int)sizeof(Settings));
    for (char *Line = Settings; *Line;)
    {
        char *End = strchr(Line, '\n');
        if (!End) break;
        *End = 0;
        char *Equals = strchr(Line, '=');
        if (Equals)
        {
            *Equals = 0;
            setenv(Line, Equals + 1, 1);
        }
        Line = End + 1;
    }
}

// NOTE(zoubir): the canvas fills the page; its drawing buffer follows the
// page's size in device pixels, and the game works in CSS pixels, the
// browser's 96-DPI ones (times WebInput.Scale on a phone)
internal app_window
WebSizeCanvas(web_frame_loop *Loop)
{
    double CssWidth = 0, CssHeight = 0;
    emscripten_get_element_css_size("#canvas", &CssWidth, &CssHeight);
    double Ratio = emscripten_get_device_pixel_ratio();
    if (CssWidth < 1) CssWidth = 1280;
    if (CssHeight < 1) CssHeight = 720;
    int PixelWidth = (int)(CssWidth * Ratio + 0.5);
    int PixelHeight = (int)(CssHeight * Ratio + 0.5);
    if (PixelWidth != Loop->CanvasWidth || PixelHeight != Loop->CanvasHeight)
    {
        emscripten_set_canvas_element_size("#canvas", PixelWidth, PixelHeight);
        Loop->CanvasWidth = PixelWidth;
        Loop->CanvasHeight = PixelHeight;
    }
    glViewport(0, 0, PixelWidth, PixelHeight);

    // NOTE(zoubir): on a phone the game lays out a larger window than the
    // page, so its screens, sized for a desktop, fit round the touch
    // controls (touch.cpp); the mouse is scaled the same way (input.cpp)
    WebInput.Scale = WebTouchUiScale(CssWidth, CssHeight);
    app_window Window = {};
    Window.Width = Maximum(1, (int)(CssWidth * WebInput.Scale + 0.5));
    Window.Height = Maximum(1, (int)(CssHeight * WebInput.Scale + 0.5));
    Window.Fullscreen = WebIsFullscreen();
    return Window;
}

internal void
WebRunGameFrame(web_frame_loop *Loop)
{
    app_input *NewInput = Loop->NewInput;
    NewInput->DeltaTime = WEB_FRAME_SECONDS;
    WebApplyTouch(&Loop->Memory, NewInput);
    WebReadInput(Loop->OldInput, NewInput);

    app_window Window = WebSizeCanvas(Loop);
    AppUpdateAndRender(&Loop->Thread, &Loop->Memory, NewInput, &Window);
    if (Window.FullscreenRequest != FullscreenRequest_None &&
        (Window.FullscreenRequest == FullscreenRequest_On) != Window.Fullscreen)
    {
        WebAskFullscreen(Window.FullscreenRequest == FullscreenRequest_On);
    }
    WebWriteFrameSound(&Loop->Thread, &Loop->Memory);
    WebShowTouch(&Loop->Memory);

    Loop->NewInput = Loop->OldInput;
    Loop->OldInput = NewInput;
}

// NOTE(zoubir): called on every animation frame, which may come 60, 120
// or 144 times a second; the game runs at 60, at most two frames to catch
// up, and a long stall (a hidden tab) is forgiven rather than replayed
internal void
WebAnimationFrame()
{
    web_frame_loop *Loop = &WebLoop;
    double Now = emscripten_get_now() / 1000.0;
    Loop->Owed += Now - Loop->LastNow;
    Loop->LastNow = Now;
    if (Loop->Owed > 0.25) Loop->Owed = WEB_FRAME_SECONDS;

    // NOTE(zoubir): a 60 Hz screen's frames come a little early or late;
    // the slack keeps one game frame per screen frame there
    double Slack = 0.004;
    for (int Run = 0; Run < 2 && Loop->Owed >= WEB_FRAME_SECONDS - Slack; Run++)
    {
        WebRunGameFrame(Loop);
        Loop->Owed -= WEB_FRAME_SECONDS;
    }
    if (Loop->Owed < -Slack) Loop->Owed = -Slack;
}

int main()
{
    WebApplyPageSettings();

    EmscriptenWebGLContextAttributes Attributes;
    emscripten_webgl_init_context_attributes(&Attributes);
    Attributes.majorVersion = 2;
    Attributes.minorVersion = 0;
    Attributes.alpha = false;
    Attributes.antialias = false;
    Attributes.depth = false;
    Attributes.stencil = false;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE Context = emscripten_webgl_create_context("#canvas", &Attributes);
    if (Context <= 0)
    {
        EM_ASM({ if (Module.gameFailed) Module.gameFailed("This browser cannot run WebGL 2."); });
        return 1;
    }
    emscripten_webgl_make_context_current(Context);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    web_frame_loop *Loop = &WebLoop;
    WebLoadOpenglFunctions(&Loop->OpenGL);
    Loop->Thread.RenderContext.OpenGL = &Loop->OpenGL;
    WebFillPlatformApi(&Loop->Memory.PlatformApi);
    Loop->Memory.PermanentStorageSize = Megabytes(64);
    Loop->Memory.TransientStorageSize = Megabytes(64);
    Loop->Memory.PermanentStorage = calloc(Loop->Memory.PermanentStorageSize, 1);
    Loop->Memory.TransientStorage = calloc(Loop->Memory.TransientStorageSize, 1);
    if (!Loop->Memory.PermanentStorage || !Loop->Memory.TransientStorage)
    {
        EM_ASM({ if (Module.gameFailed) Module.gameFailed("Not enough memory to start the game."); });
        return 1;
    }
    Loop->OldInput = &Loop->Inputs[0];
    Loop->NewInput = &Loop->Inputs[1];
    Loop->LastNow = emscripten_get_now() / 1000.0;
    Loop->Owed = WEB_FRAME_SECONDS;

    WebKeepSavedFiles();
    WebStartInput();
    WebInitSound();
    emscripten_set_main_loop(WebAnimationFrame, 0, true);
    return 0;
}
