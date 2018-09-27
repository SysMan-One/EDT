Edt.c - Edt text editor emulator.  This text-editor emulates the VAX VMS
text editor known as Edt, and to some extent, later versions called TPU/Eve.

Compile as:
	gcc -O edt_1.9.c -o edt


Configure:
 Place an alias in your .bashrc or .tcshrc to the absolute location of edt.
    Example:   alias edt=/home/bart/bin/edt
 Set environment variable, EDT_KEYPAD_SETUP, to point at the edt_keypad.xml file.
    Example:   export EDT_KEYPAD_SETUP=/home/bart/edt_keypad.xml
 If default keypad map does not match your keyboard, type "help_config" at
 editor prompt for instructions on generating new edt_keypad.xml file for your
 keyboard.  See web-page also.
 Several default/example keypad map files are also provided, "edt_keypad_*.xml".


Run as:
	edt {your_text_file.txt}



For updates/info, see:  http://sourceforge.net/projects/edt-text-editor


Revision History:

1.9 - Added check against accdentally attempting to edit directory file.
	Added 'begin html' line command.  Added Makefile.
	Added Sun keyboard map file.

1.8 - Added ability to use 'wq', 'wq!', 'q!' at the editor's line-prompt.
  The 'wq' command is like 'exit' or 'ex', but does not save .bak file.
  The trailing '!' causes Edt to not ask if you really want to quit, when 
   there were changes.
  Added ability for editor's line-prompt to accept: 
   'begin cprog', 'begin c++prog', and 'begin javaprog'.  
  These commands cause the editor to insert an outer program shell, of the
   selected language, into the edit buffer.
   Handy for starting new programs.  Accepts 'start ..." too.
  Added ability to skip '!' (Escape to OS command) for 'ls' and 'dir' at
   edt line-prompt.
  Several code style cleanups were done for consistency.

1.7 - Added XML key-pad setup file format.
      Added ability to edit gzip'd files.


Edt.c -  - LGPL License:
  Copyright (C) 2001, Carl Kindman
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


Carl Kindman 12-23-2011     carlkindman@yahoo.com
