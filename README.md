# ShareYourClipboard

#Downloads
[Latest Stable Version](https://github.com/Allexin/ShareYourClipboard/releases)

Allow automaticly sharing clipboard betwenn devices.  
For example - between host and virtual machine

SYC - is simple application wich monitor clipboard and allow share clipboard content between PCs.
This is first alpha release. All functions works fine, but not tested hard.
You can use this software, but remember about possibility to lose some clipboard data while transfering.
Main danger is transferming multiply files. I hope all works fine, but who knows...
Three tips:
1) Secret key must be equal on all computers to share clipboard
2) Do not forget to allow ports 26855 and 26856 in firewall settings
3) Use hot key Ctrl+Alt+V to paste remote files. Files not transfered automaticly, only by manual request(to avoid perfomance issues and net overuse)

#Build
Just use QtCreator  
Works fine under modern Windows, Linux and Mac OS X.
Have issue under Windows XP(some times app may block CopyPaste on XP)
