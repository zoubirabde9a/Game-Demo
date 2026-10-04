found-by: agent/resize
files: code/client/camera.cpp, code/app.cpp
since: 2026-10-04
what: when the screen is wider or taller than a bounded map (Old Arena is 2560x1280, so any fullscreen at 1440 tall or more), CenterCamera pins the map to the top-left and the leftover strip shows the pink clear colour (1.0, 0.5, 0.5).
reproduce: build.bat, then set GAME_WINDOW=3840x2160 and run misc\screenshot.bat out.png; or press F1 on a 2560x1440 or larger screen.
fix: in CenterCamera, on an axis where the window is larger than the map, centre the map (Result.X = MinX - (WindowWidth - (MaxX - MinX)) / 2) instead of clamping; and clear to a dark colour in app.cpp so any uncovered area reads as a border.
