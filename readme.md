# vDosXY3
## vDos modifications for use with the XY3 text editor

This is a fork of the vDos project by Jos Schaars. 

This fork is ONLY suitable for use with the XY3 text editor. For all other uses, please use the original vDos found at
[http://sourceforge.net/projects/vdos/](http://sourceforge.net/projects/vdos/). vDos itself is derived from the DOSBox project found at [http://sourceforge.net/projects/dosbox/](http://sourceforge.net/projects/dosbox/). The license for this fork is GPLv2 to match the original DOSBOX project as well as the vDos project.

This fork tracks the master source code of the vDos project and adds the following features:

* Implement full IRQ1/INT9 keyboard handling, necessary for all XY3 functionality to work. **NOTE:** you can disable this by adding KBXY3 = OFF to your config.txt.

* Implement very rough sound support for XY3. **NOTE:** you can disable this by adding BEEPXY3 = OFF to your config.txt.

* Implement support for short file name versions of long filenames. **NOTE:** you can disable this by adding SFN83 = OFF to your config.txt.


---

#### Notes on the modified keyboard handling:
You will notice that on the vDos command line, the keyboard layout is fixed to US. Within XY3 the keyboard will be mapped correctly by XY3's keyboard handling routines.

