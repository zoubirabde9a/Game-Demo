/* Hot reload: loading app.dll (through a copy, so the original can be
   rebuilt while the game runs) and unloading it. */

inline FILETIME
Win32GetFileLastWriteTime(char *fileName)
{
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(fileName, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }
    
    return lastWriteTime;
    
}

internal win32_app_code
Win32LoadAppCode(char *sourceDLLName, char*tempDLLName)
{
    win32_app_code result = {};

    CopyFile(sourceDLLName, tempDLLName, FALSE);
    result.DLL = LoadLibrary(tempDLLName);
    
    if (result.DLL)
    {
        result.lastWriteTime = Win32GetFileLastWriteTime(sourceDLLName);
        result.updateAndRender = (app_update_and_render  *)GetProcAddress(result.DLL, "AppUpdateAndRender");
        result.getSoundSamples = (app_get_sound_samples  *)GetProcAddress(result.DLL, "AppGetSoundSamples");

        result.isValid = result.updateAndRender &&
            result.getSoundSamples;
    }
    
    if (!result.isValid)
    {
        result.updateAndRender = AppUpdateAndRenderStub;
        result.getSoundSamples = AppGetSoundSamplesStub;    
    }
    
    return result;
}

internal void
Win32UnloadAppCode(win32_app_code *appCode)
{
    if (appCode->DLL)
    {
        FreeLibrary(appCode->DLL);
        appCode->DLL = 0;
    }
    
    appCode->isValid = false;
    appCode->updateAndRender = AppUpdateAndRenderStub;
    appCode->getSoundSamples = AppGetSoundSamplesStub;
}
