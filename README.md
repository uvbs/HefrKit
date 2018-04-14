# HefrKit
Capcom exploit with reflective driver loader all packaged into one deliverable executable

This is another exploit example for capcom.sys. The code packages the capcom.sys and a driver to deliver into a user executable, HefrSploit.exe. HefrSploit, will write capcom.sys to disk, exploit it and load the packaged driver. Loader code is modified from [Professor Plum's Driver Loader](https://github.com/Professor-plum/Reflective-Driver-Loader).

To use it is recommended to use the provided solution. Add your driver code and you're good to go.

Might in future make the loader a little more advanced by not using resource files and eliminating strings.

Build tested on Visual Studio 2017 with SDK 10.0.10586.0.
Tested on Windows 10 RS3
