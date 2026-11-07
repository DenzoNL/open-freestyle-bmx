#!/usr/bin/env bash
# Packs the macOS build into OpenFreestyleBMX.app (docs/PORTING.md "X5 part 1"): the executable, SDL3's dylib in
# Contents/Frameworks (found through @rpath, so the bundle does not depend on the nix store), a minimal
# Info.plist, and an ad-hoc signature (arm64 runs nothing unsigned; install_name_tool breaks the linker's).
# Run by the `bmxnative_app` target: cmake --build --preset macos --target bmxnative_app
#
#   tools/macos_bundle.sh <OpenFreestyleBMX> <libSDL3.x.dylib> <output .app>
set -euo pipefail
exe=$1
sdl=$2
app=$3
sdlName=$(basename "$sdl")
sdlId=$(otool -D "$sdl" | sed -n 2p)

rm -rf "$app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Frameworks" "$app/Contents/Resources"
cp "$exe" "$app/Contents/MacOS/OpenFreestyleBMX"
cp "$sdl" "$app/Contents/Frameworks/$sdlName"
chmod u+w "$app/Contents/MacOS/OpenFreestyleBMX" "$app/Contents/Frameworks/$sdlName"
install_name_tool -id "@rpath/$sdlName" "$app/Contents/Frameworks/$sdlName"
install_name_tool -change "$sdlId" "@rpath/$sdlName" "$app/Contents/MacOS/OpenFreestyleBMX"
# The build's rpath (the nix store) goes; the bundle's Frameworks folder comes in.
for r in $(otool -l "$app/Contents/MacOS/OpenFreestyleBMX" | awk '/LC_RPATH/ {getline; getline; print $2}'); do
    install_name_tool -delete_rpath "$r" "$app/Contents/MacOS/OpenFreestyleBMX"
done
install_name_tool -add_rpath "@executable_path/../Frameworks" "$app/Contents/MacOS/OpenFreestyleBMX"
cat >"$app/Contents/Info.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>OpenFreestyleBMX</string>
    <key>CFBundleIdentifier</key>
    <string>org.openfreestylebmx.app</string>
    <key>CFBundleName</key>
    <string>Open Freestyle BMX</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>0.1</string>
    <key>LSMinimumSystemVersion</key>
    <string>13.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF
codesign --force --sign - "$app/Contents/Frameworks/$sdlName"
codesign --force --sign - "$app"
echo "wrote $app"
