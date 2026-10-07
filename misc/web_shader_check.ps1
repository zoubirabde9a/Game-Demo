# Compiles every effect shader in build\shaders\fx the way the web build
# does (GLSL ES 1.00 with the prelude in engine\shader_library.cpp), in a
# real browser's WebGL, so a shader that only breaks in the browser shows
# up without building the web version. Uses headless Edge with its software
# renderer; needs no GPU and no Emscripten.
#   powershell -File misc\web_shader_check.ps1          WebGL 1, as the web build runs
#   powershell -File misc\web_shader_check.ps1 -WebGL2  also WebGL 2
# Prints each shader that fails with the compiler's log; exits 1 if any do.
param([switch]$WebGL2)

$Root = Split-Path -Parent $PSScriptRoot
$Edge = @("${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
          "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe") |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Edge) { Write-Host "web_shader_check: Edge not found, skipped"; exit 0 }

# NOTE(zoubir): keep in step with ShaderVertexPrelude and
# ShaderFragmentPrelude under COMPILER_EMSCRIPTEN
$VertexPrelude = "#define ATTRIBUTE attribute`n#define VARYING varying`n"
$FragmentPrelude = "#extension GL_OES_standard_derivatives : enable`nprecision mediump float;`n" +
    "#define VARYING varying`n#define TEXTURE texture2D`n#define FragColor gl_FragColor`n"

# NOTE(zoubir): [string], since Windows PowerShell's Get-Content hangs
# properties on what it returns and ConvertTo-Json would write them out
$Shaders = [ordered]@{}
foreach ($File in Get-ChildItem "$Root\build\shaders\fx" -File) {
    $Shaders[$File.Name] = [string](Get-Content $File.FullName -Raw)
}
$Contexts = if ($WebGL2) { '["webgl","webgl2"]' } else { '["webgl"]' }
$Json = $Shaders | ConvertTo-Json -Compress
$Script = @"
const fx=$Json;
const vpre=$(ConvertTo-Json $VertexPrelude);
const fpre=$(ConvertTo-Json $FragmentPrelude);
const res={};
try {
 for (const name of $Contexts) {
  const gl=document.createElement("canvas").getContext(name);
  if(!gl){res[name+":context"]="no context";continue;}
  gl.getExtension("OES_standard_derivatives");
  for(const [n,src] of Object.entries(fx)){
   const vert=n.endsWith(".vert");
   const s=gl.createShader(vert?gl.VERTEX_SHADER:gl.FRAGMENT_SHADER);
   gl.shaderSource(s,(vert?vpre:fpre)+src); gl.compileShader(s);
   if(!gl.getShaderParameter(s,gl.COMPILE_STATUS)) res[name+":"+n]=gl.getShaderInfoLog(s);
  }
 }
} catch(e) { res.error=String(e); }
document.getElementById("o").textContent="RESULT"+JSON.stringify(res)+"END";
"@
$Temp = Join-Path ([IO.Path]::GetTempPath()) "game_web_shader_check"
New-Item -ItemType Directory -Force $Temp | Out-Null
$Page = Join-Path $Temp "check.html"
Set-Content $Page "<!doctype html><meta charset=utf-8><pre id=o></pre><script>$Script</script>" -Encoding UTF8
$Url = "file:///" + ($Page -replace '\\', '/')
$Dom = Join-Path $Temp "dom.txt"
cmd /c "`"$Edge`" --headless=new --enable-unsafe-swiftshader --use-angle=swiftshader --user-data-dir=`"$Temp\profile`" --dump-dom `"$Url`" > `"$Dom`" 2>NUL"
$Text = Get-Content $Dom -Raw
$Match = [regex]::Match($Text, 'RESULT(.*?)END', 'Singleline')
if (-not $Match.Success) { Write-Host "web_shader_check: the page did not report"; exit 1 }
$Failures = [System.Net.WebUtility]::HtmlDecode($Match.Groups[1].Value) | ConvertFrom-Json
$Names = @($Failures.PSObject.Properties)
if ($Names.Count -eq 0) {
    Write-Host "web_shader_check: $($Shaders.Count) shaders compile"
    exit 0
}
foreach ($Failure in $Names) { Write-Host "$($Failure.Name)`n$($Failure.Value)" }
exit 1
