
For building this application, Qt 5 is required  

You can get it here: https://www.qt.io/download/  

First build ShareYourClipboard.pro with Qt Creator - it is a typical Qt project  

After building do the following:  

#Windows

Place ShareYourClipboard.exe in build folder, copy data folder,   
copy platforms plugin with qwindows, copy necessary qt libs  
Pack in compressed folder ShareYourClipboard_Windows.zip  

#Mac OS X

Copy data into ShareYourClipboard.app/Contents/MacOS  
copy platforms/libqcocoa.dylib into ShareYourClipboard.app/Contents/PlugIns/platforms
add key LSUIElement with value 1 into ShareYourClipboard.app/Contents/Info.plist
execute macdeployqt with -dmg flag(<path_to_qt/macdeployqt ShareYourClipboard.app -dmg>)
Rename package to ShareYourClipboard_MacOSX.dmg

#Linux
Place ShareYourClipboard in build folder, 
copy data folder, copy checksystem
Pack into ShareYourClipboard_Linux.tar.gz
