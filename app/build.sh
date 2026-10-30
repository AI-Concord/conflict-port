#!/bin/sh
# Builds Conflict.app into ../build using the command line tools only.
set -e
cd "$(dirname "$0")"
OUT=../build; APP=$OUT/Conflict.app
rm -rf "$APP"; mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/GameData" $OUT/obj
# Native game core (ported from the DOS original); no emulator is involved.
GAME_SRC=$(grep '^GAME_SRC' ../port/Makefile | cut -d= -f2)
rm -f $OUT/obj/*.o
for f in $GAME_SRC platform.c engine_native.c; do cc -O2 -w -c ../port/$f -o $OUT/obj/${f%.c}.o; done
swiftc -O -swift-version 5 -import-objc-header Engine/engine.h App/*.swift $OUT/obj/*.o \
  -framework AppKit -framework Metal -framework MetalKit -o "$APP/Contents/MacOS/Conflict"
cp ../original/*.DAT ../original/*.FNT ../original/*.MP ../port/conflict.dat "$APP/Contents/Resources/GameData/"
cp Resources/Conflict.icns "$APP/Contents/Resources/"
cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleName</key><string>Conflict</string>
<key>CFBundleDisplayName</key><string>Conflict</string>
<key>CFBundleIdentifier</key><string>io.github.ai-concord.conflict</string>
<key>CFBundleExecutable</key><string>Conflict</string>
<key>CFBundleIconFile</key><string>Conflict</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>1.0</string>
<key>CFBundleVersion</key><string>1</string>
<key>LSMinimumSystemVersion</key><string>13.0</string>
<key>NSHighResolutionCapable</key><true/>
<key>NSPrincipalClass</key><string>NSApplication</string>
<key>LSApplicationCategoryType</key><string>public.app-category.strategy-games</string>
</dict></plist>
PLIST
codesign --force --sign - "$APP" >/dev/null 2>&1 || true
echo "built $APP"
