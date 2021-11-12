# Custom UAC

## What is it
It is an open source replacement of [UAC](https://msdn.microsoft.com/en-us/library/bb756945.aspx).

It was a successor of my previous project *UAC Renderer*. As the functionalities and usages have all changed, and the source code has been widely re-written, I decided to open this new project.

The previous project will still be kept, as my original idea was to build some simple samples and write blog posts to record what I found interesting. And this project is not 'small' enough.

## System requirements
​	Though the predecessor of this program supports Windows 7 and 10, and runs on both x86 and x64, it soon becomes so hard work that I decided to drop support for Windows 7 or x86, and focus on 64-bit Windows 10 and Windows 11.

​	As replacing UAC is not officially supported, this program is not promised to run on future versions of Windows. If it fails to run, raise an issue or email me.

## Installation & Uninstallation
Installation is a little complex. It is suggested that you follow the following steps.

1. Download installer from the latest release, which is automatically built.
2. As I cannot afford a signing certificate, and Windows system requires [IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY](https://docs.microsoft.com/en-us/cpp/build/reference/integritycheck-require-signature-check?view=msvc-170)  be set in UAC executable, it is required to have [test signing](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/test-signing) turned on to use this program. To do this, open command prompt with administrator privileges, run `bcdedit /set testsigning on` , and reboot.
3. (recommended) To recover from faults, start command prompt with *TrustedInstaller* privileges. I personally suggest using [NSudo by M2Team](https://github.com/M2Team/NSudo). Copy C:\Windows\System32\consent.exe to anywhere for backup. Do not close the command prompt.
4. Run installer.
5. Just do some tests before finishing. If errors occur, use the command prompt and backup file in step 3 to fix.
6. Reboot is recommended but not necessary. Now, quit the installer and have fun.

An uninstaller is ready to use. Just uninstall the program from control panel, or directly run the uninstaller.

## Allowlist

​	You can add a program to allowlist through a right-click on the main window of the UAC prompt, and click on the item *Add to allowlist*. 

​	The hash and path of the program will be saved, so you will not be asked again.

​	You can remove it from the allowlist with the *CustomUAC Control Panel*.

​	It is not suggested to add programs that accept arguments and run commands to allowlist, as dangerous commands may run by accident.

​	Banlist is not supported (as you can simply delete whatever you do not like).

## Themes
* A theme is a compiled rcc (Qt resource file) which contains necessary information to define an UI for UAC. An example is included in the source tree, which is also used as the default theme.
* You can import and manage themes with the *CustomUAC Control Panel*. And you can restrict the themes from accessing the local storage database or the Internet.

## Note
* Enabling test signing may cause security issues.
* And this program may not be as secure or robust as the original UAC shipped with the system, as in fact I haven't figured out much details in the structure passed in.
* After all, this is just a toy project. Use it if you want to try something new, but not if you need high security or robustness.

## License
* Open source under MIT license.
* On the other hand, if MIT license is still too strict for you, the very first version which demonstrates basic parsing of UAC structure can be found in [this github repository](https://github.com/6ziv/Custom-Samples/tree/master/UAC), which is open-sourced under WTFPL. Do whatever you want based on that repository.



## Contact
* Email：[root@6ziv.com](mailto://root@6ziv.com)

* Or just use Github to raise issues or PRs

  

## 3rd-party License
* [Qt project](http://qt.io/), which is open-sourced under [LGPLv3](https://opensource.org/licenses/LGPL-3.0).
* Some code to acquire *TrustedInstaller* privilege comes from [Privexec](https://github.com/M2Team/Privexec) by [M2Team](https://github.com/M2Team/), which is open-sourced under [MIT](https://opensource.org/licenses/MIT).
