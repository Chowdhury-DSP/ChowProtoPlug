#!/bin/bash

script_file=ChowProtoPlug.pkgproj

app_version=$(cut -f 2 -d '=' <<< "$(grep 'CMAKE_PROJECT_VERSION:STATIC' ../../build/CMakeCache.txt)")
echo "Setting app version: $app_version..."
sed -i '' "s/##APPVERSION##/${app_version}/g" $script_file
sed -i '' "s/##APPVERSION##/${app_version}/g" Intro.txt

echo "Copying License..."
cp ../../LICENSE LICENSE.txt

# build installer
echo Building...
/usr/local/bin/packagesbuild $script_file

# reset version number
sed -i '' "s/${app_version}/##APPVERSION##/g" $script_file
sed -i '' "s/${app_version}/##APPVERSION##/g" Intro.txt

# clean up license file
rm LICENSE.txt

# sign the installer package
echo "Signing installer package..."
TEAM_ID=$(more ~/Developer/mac_id)
pkg_dir=ChowProtoPlug_Installer_Packaged
rm -Rf $pkg_dir
mkdir $pkg_dir
productsign -s "$TEAM_ID" ../../build/ChowProtoPlug.pkg $pkg_dir/ChowProtoPlug-signed.pkg

echo "Notarizing installer package..."
INSTALLER_PASS=$(more ~/Developer/mac_installer_pass)
npx notarize-cli --file $pkg_dir/ChowProtoPlug-signed.pkg --bundle-id com.chowdsp.ChowProtoPlug --asc-provider "$TEAM_ID" --username chowdsp@gmail.com --password "$INSTALLER_PASS"

echo "Building disk image..."
vol_name=Install_ChowProtoPlug-$app_version
rm -f "$vol_name"
hdiutil create "$vol_name.dmg" -fs HFS+ -srcfolder $pkg_dir -format UDZO -volname "$vol_name"
