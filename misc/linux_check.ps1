# Builds and tests the server on Linux before a deploy, in a throwaway
# Docker container (alpine, x86_64): the live server is Linux, and nothing
# else here compiles the Linux socket code or runs the tests under
# AddressSanitizer. Takes a few minutes; the first run downloads g++ into
# the container.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File misc\linux_check.ps1
#
# Runs build_server.sh as deploy.sh would, then net_tests, server_tests and
# a short soak built with -fsanitize=address. Prints the content id, which
# must match the Windows build's, so players and the server agree. Exit 0
# when everything passed. Needs Docker; the repo is mounted read-only.
$Root = Split-Path -Parent $PSScriptRoot
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "linux_check: docker not found"
    exit 1
}
$Script = @'

set -e -o pipefail
apk add -q --no-cache g++ >/dev/null 2>&1
cp -r /src /work && cd /work
sed -i 's/\r$//' build_server.sh
echo "linux_check: build_server.sh"
sh build_server.sh
Flags="-std=c++11 -w -g -fsanitize=address -fno-omit-frame-pointer -DAPP_DEV=1 -DAPP_SLOW=1"
echo "linux_check: tests under AddressSanitizer"
g++ $Flags code/tests/net_tests.cpp -o net_tests && ./net_tests | tail -1
g++ $Flags code/tests/server_tests.cpp -o server_tests && ./server_tests | tail -2
g++ $Flags code/tests/soak_tests.cpp -o soak_tests && ./soak_tests 1 2 | tail -1
echo "linux_check: all passed"
'@
$Script = $Script -replace "`r", ""
# The script goes in on stdin: Windows argument passing mangles a multi-line
# -c. PowerShell may put a byte-order mark first, so line 1 is left empty.
$Script | docker run --rm -i -v "${Root}:/src:ro" alpine:latest sh
exit $LASTEXITCODE
