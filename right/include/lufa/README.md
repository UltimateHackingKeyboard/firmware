# LUFA library header files

This directory contains some header files from [LUFA, the Lightweight USB Framework for AVRs](http://www.fourwalledcubicle.com/LUFA.php).

LUFA is an extremely well designed library that exposes a beautiful and intuitive API that is a joy to work with. LUFA being an AVR based library can't be used as a whole for ARM, but some parts of it can be.

The selected header files contain macros for USB scancodes and USB HID report items. USB HID report item macros are especially helpful and it's a much better idea to use them than a bunch of magic numbers backed by loads of comments.
