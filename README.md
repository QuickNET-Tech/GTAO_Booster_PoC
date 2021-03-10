## PoC that fixes two GTA Online bugs and drastically improves load times for all systems

Original work by [tostercx](https://github.com/tostercx), massive thanks to them

All addresses are found at run time so that it works on all platforms

## Disclaimer

While chances are low, modifying your game might get your account suspended, __**use at your own risk! I am not liable!**__

## How to use

Inject the dll found in [releases](https://github.com/QuickNET-Tech/GTAO_Booster_PoC/releases) with a DLL injector of your choice ([Xenos](https://github.com/DarthTon/Xenos) is my go to, [Extreme Injector](https://github.com/master131/ExtremeInjector) is another popular choices) and then just head online  
NOTE : You can inject whenever you want as long as GTA5.exe is running and before you've started to head online

## Anti-virus

If you're having issues with your AV flagging the file, I'm sorry that there's not much I can do. The only AV that seems to really be having an issue is Defender. [I've ran the file through VirusTotal](https://www.virustotal.com/gui/file/d0a11ac9908548d7fa210832538eaf6dda08c40d9fd606a636f95fed3e3c2534/detection) and there's only a single detection. I've submitted the file to Microsoft in the hopes that they no longer flag the files but there's a good chance that won't help much. If you're scared away by this I understand but unfortunately it's beyond my control.

## How to build

* `git clone --recurse-submodules https://github.com/QuickNET-Tech/GTAO_Booster_PoC`
* build the project with MSVC
* inject the DLL with your favorite injector while the game is starting up

or

* in visual studio start a new project and provide a link to this repository

## More details

If you want more information, I recommend you read tostercx's original [writeup](https://nee.lv/2021/02/28/How-I-cut-GTA-Online-loading-times-by-70/)
