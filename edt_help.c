#define	__MODULE__	"EDT_HELP"

/*
**++
**
**  FACILITY:  EDT
**
**  ABSTRACT: Simple text editor emulates VAX VMS EDT
**
**  DESCRIPTION: Thsi module is a part of the EDT project, contains HELP-related routines
*
**  AUTHORS: Carl Kindman 12-23-2011 carlkindman@yahoo.com
**
**  CREATION DATE:  28-SEP-2018
**
**  MODIFICATION HISTORY:
**
**	28-SEP-2018	RRL	Moved help-routines from main EDT.c
**
**
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>

#define	EDT$K_VERSION	2.0


extern	char	*tname;

void	help_quick	(void)
{
	printf("\n	EDT Help Quick Summary  v%g\n", EDT$K_VERSION);
	printf("	----------------------\n");
	printf(" c	    - Enter screen mode editing.\n");
	printf(" ^z	    - To exit screen mode, once in screen mode.)\n");
	printf(" q 	    - [quit]  Quit from editor without saving file.\n");
	printf(" ex 	    - [exit]  Exit from the editor and save the edited file.\n");
	printf(" w <file>   - [write] Write edit buffer to named file without exiting, i.e save-as.\n");
	printf(" incl <file> - [include] Inserts/imports contents of named file.\n");
	printf(" r <file>   - [read]   Same as 'include'.\n");
	printf(" s </s1/s2/> - [substitute] Substitute character string (s/string1/string2/\n");
	printf(" case	    - Toggles case sensitivity for searches and search/replace.\n");
	printf(" <line number> - Typing a line number moves cursor to that line.\n");
	printf(" ! <unix command> - Temporary escape to Unix command, without leaving.\n");
	printf(" file	    - Tell what file is being edited.\n");
	printf(" = <buffer_name>  - Switch text buffers. Default buffer is 'main'.\n");
	printf(" list	    - Lists the names of the currently defined text buffers.\n");
	printf(" set margin - Sets the right margin parameter, used by re-format paragraph function.\n");
	printf(" rk	    - Restores numeric keypad configuration to pre-editor.\n");
	printf(" encode     - Toggles encode mode.\n");
	printf(" h or ?     - Show this short help-list.\n");
	printf(" help	    - Show full help manual.\n");
	printf("\n");
}

void	help_long	(void)
{
FILE	*fz;
char	tcline[260];

	// tmpnam(fnm);
	mkstemp(tname);

	if ( !(fz = fopen(tname,"w")) )
		{
		printf("Cannot open %s\n", tname);
		return;
		}

	fprintf(fz,"         EDT Help / Documentation File    v%g\n", EDT$K_VERSION);
	fprintf(fz,"         -----------------------------\n");
	fprintf(fz,"\n");
	fprintf(fz,"This editor is designed for rapid efficient text manipulation.  \n");
	fprintf(fz,"The most often repeated operations are accessed by single \n");
	fprintf(fz,"key-strokes on the numeric key-pad.  Automatic screen centering \n");
	fprintf(fz,"enables a few lines above and below the cursor to always be in \n");
	fprintf(fz,"view, without you needing th re-center the screen explicitly.\n");
	fprintf(fz,"\n");
	fprintf(fz,"The editor contains four built-in separate buffers:\n");
	fprintf(fz,"	1. character buffer,\n");
	fprintf(fz,"	2. word buffer (delineated by white-space),\n");
	fprintf(fz,"	3. line buffer (delineated by <carriage-returns>),\n");
	fprintf(fz,"	4. paste buffer (arbitrary length).\n");
	fprintf(fz,"Additionally, you may create and switch between an arbitrary \n");
	fprintf(fz,"number of separate text buffers.\n");
	fprintf(fz,"\n");
	fprintf(fz,"The editor maintains one level of back-up for any file\n");
	fprintf(fz,"edited so you can always revert to the previous version.\n");
	fprintf(fz,"The editor also maintains continuous journaling for error\n");
	fprintf(fz,"recovery down to within a few keystrokes, in the event\n");
	fprintf(fz,"of a system failure during an editing session.\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz,"Invocation:\n");
	fprintf(fz,"-----------\n");
	fprintf(fz,"\n");
	fprintf(fz,"To invoke this editor, you should be using an ANSI complaint \n");
	fprintf(fz,"terminal or window, such as a VT-100 terminal, emulator, or \n");
	fprintf(fz,"X-term window.\n");
	fprintf(fz,"\n");
	fprintf(fz,"Type:\n");
	fprintf(fz,"	edt file_name\n");
	fprintf(fz,"\n");
	fprintf(fz,"The editor contains two modes:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	Line mode\n");
	fprintf(fz,"	  and\n");
	fprintf(fz,"	Screen Mode\n");
	fprintf(fz,"\n");
	fprintf(fz,"Upon entering the editor will be in line mode.  To go to \n");
	fprintf(fz,"full-screen mode, type 'c' and return.  To return to line \n");
	fprintf(fz,"mode from the screen mode, type control-Z.\n");
	fprintf(fz,"\n");
	fprintf(fz,"There are several command-line options that can be used \n");
	fprintf(fz,"when invoking the editor, such as:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	-readonly\n");
	fprintf(fz,"	-encode\n");
	fprintf(fz,"\n");
	fprintf(fz,"When the '-read_only' or '-read' command-line option is placed\n");
	fprintf(fz,"anywhere on the command-line when the editor is invoked, then \n");
	fprintf(fz,"the editor will be placed in a 'read-only' mode.\n");
	fprintf(fz,"In read-only mode, the editor will not allow modification\n");
	fprintf(fz,"of the original file by way of an exit and save to that\n");
	fprintf(fz,"file.  This is helpful when you need a powerful viewer,\n");
	fprintf(fz,"but want to avoid accidental changes to an important file.\n");
	fprintf(fz,"If you attempt to save from readonly mode, you will be\n");
	fprintf(fz,"reminded that you are in readonly mode and cannot save.\n");
	fprintf(fz,"If you however want to preserve your edits anyway, you\n");
	fprintf(fz,"may write to any arbitrary file name using the 'w' (write\n");
	fprintf(fz,"to file-name) command.\n");
	fprintf(fz,"\n");
	fprintf(fz,"When invoked in the '-encode' mode, the editor will ask\n");
	fprintf(fz,"for a unique encode key, or password, used for encoding\n");
	fprintf(fz,"and decoding the document when stored as a file.  This\n");
	fprintf(fz,"is useful when creating sensitive material, since at no\n");
	fprintf(fz,"time will the sensitive text appear in ascii format on the\n");
	fprintf(fz,"disk system.  Be cautious in selecting a password key that\n");
	fprintf(fz,"you can remember, because there is no way to edit an\n");
	fprintf(fz,"encoded document without the exact key.\n");
	fprintf(fz,"\n");
	fprintf(fz,"Remember to use an 'xterm' window.\n");
	fprintf(fz,"\n");
	fprintf(fz,"The editor checks the size of your viewing window whenever\n");
	fprintf(fz,"you enter the 'screen mode'.  If you change the window size \n");
	fprintf(fz,"while editing, simply go out-of and back into the screen mode.\n");
	fprintf(fz,"It will establish new size parameters.\n");
	fprintf(fz,"\n");
	fprintf(fz,"You can also set the size parameters to arbitrary values\n");
	fprintf(fz,"by using the 'set rows/cols' commands described below.  \n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz,"Line Mode Commands:\n");
	fprintf(fz,"-------------------\n");
	fprintf(fz,"\n");
	fprintf(fz," c - Go into full screen editor mode.\n");
	fprintf(fz,"\n");
	fprintf(fz," q [quit] - Quit from editor without saving file.\n");
	fprintf(fz," 	    You are asked to confirm this command if you \n");
	fprintf(fz,"	    modified the file.\n");
	fprintf(fz,"\n");
	fprintf(fz," ex [exit] - Exit from the editor and save the edited file.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	     The previous version of the file (if it exists)\n");
	fprintf(fz,"	     is moved to a file of the same root name but with\n");
	fprintf(fz,"	     a '.bak' suffix just prior to the save operation.\n");
	fprintf(fz,"	     This provides a backup file.\n");
	fprintf(fz,"\n");
	fprintf(fz," w [write] - Writes a copy of the edit buffer to a named file.\n");
	fprintf(fz,"	     Example:  write doc.txt\n");
	fprintf(fz,"\n");
	fprintf(fz," r [read]  - Same as 'include'.\n");
	fprintf(fz,"\n");
	fprintf(fz," incl [include] - Brings contents of named file into the active\n");
	fprintf(fz,"		  edit buffer.  The new contents are inserted\n");
	fprintf(fz,"		  at the current cursor position as if they\n");
	fprintf(fz,"		  were typed in.\n");
	fprintf(fz,"		  Example:  incl data.txt\n");
	fprintf(fz,"\n");
	fprintf(fz," s [substitute] - Perform character string substitution.\n");
	fprintf(fz,"		  Example:   s/string1/string2/\n");
	fprintf(fz,"		  Searches for all instances of string1 and\n");
	fprintf(fz,"		  replaces them with string2.\n");
	fprintf(fz,"\n");
	fprintf(fz,"		  Note that any non-alphanumeric character\n");
	fprintf(fz,"		  can be used as the delimiter, but the same\n");
	fprintf(fz,"		  one must be used in all three positions.\n");
	fprintf(fz,"		  This allows you to search for and replace\n");
	fprintf(fz,"		  strings containing any delimiters.\n");
	fprintf(fz,"		  Example:  s!string1!string2!\n");
	fprintf(fz,"\n");
	fprintf(fz," case - Toggles case sensitivity for searches and search/replace.\n");
	fprintf(fz,"	The default is case-insensitive.\n");
	fprintf(fz,"\n");
	fprintf(fz," <line number> - Typing a number at the line prompt moves the cursor\n");
	fprintf(fz,"		 to that line number.  The line and number are\n");
	fprintf(fz,"		 displayed.\n");
	fprintf(fz,"\n");
	fprintf(fz," ! <unix command> - The exclamation mark can be used to escape\n");
	fprintf(fz,"	 	 to a unix command without leaving or exiting\n");
	fprintf(fz,"		 from the editor.  This is useful for listing\n");
	fprintf(fz,"		 directories, etc..\n");
	fprintf(fz,"		 Example:  ! ls -la\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz," file - Tells what file is being edited.\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz," = <buffer_name>  - Switch text buffers.  The default buffer is\n");
	fprintf(fz,"	called 'main'.  You can give any arbitrary buffer name\n");
	fprintf(fz,"	to create a new buffer.  And you can switch between them\n");
	fprintf(fz,"	at any time by simply naming them after the equals sign.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	Be careful at 'exit' / 'saving' / 'file-writing' time\n");
	fprintf(fz,"	however.  Only the contents of the currently active buffer \n");
	fprintf(fz,"	will be saved.\n");
	fprintf(fz,"\n");
	fprintf(fz," list - Lists the names of the currently defined text buffers.\n");
	fprintf(fz,"\n");
	fprintf(fz," set rows - Sets the editor screen mode to display the specified \n");
	fprintf(fz,"	    number of rows.  \n");
	fprintf(fz,"	    Example:  set rows 24\n");
	fprintf(fz,"	    This sets the screen to display a maximum of 24 rows \n");
	fprintf(fz,"	    at a time.\n");
	fprintf(fz,"\n");
	fprintf(fz," set cols - Sets the editor screen mode to display the specified \n");
	fprintf(fz,"            number of character columns.  \n");
	fprintf(fz,"            Example:  set cols 80\n");
	fprintf(fz,"            This sets the screen to display a maximum of 80 columns.\n");
	fprintf(fz,"\n");
	fprintf(fz," set margin - Sets the right margin parameter, used by the re-format\n");
	fprintf(fz,"	operation, to a specified number of columns.\n");
	fprintf(fz,"	Example:   set margin 65\n");
	fprintf(fz,"	See re-format key-pad operation below.\n");
	fprintf(fz,"\n");
	fprintf(fz," encode - Toggles edt-encode mode.\n");
	fprintf(fz,"\n");
	fprintf(fz," rk     - Restore numeric keypad configuration to pre-editor.\n");
	fprintf(fz,"\n");
	fprintf(fz," sk     - Restore Edt keypad configuration for editing (inverse of 'rk' above).\n");
	fprintf(fz,"\n");
	fprintf(fz," h or ? - Displays quick help list of commands.\n");
	fprintf(fz,"\n");
	fprintf(fz," Help - Displays this help manual.\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz,"Screen Mode Commands:\n");
	fprintf(fz,"---------------------\n");
	fprintf(fz,"\n");
	fprintf(fz," Typing characters anywhere will be inserted where the cursor is.\n");
	fprintf(fz," To move around use the arrow keys, or keypad 'jump'/'search' keys.\n");
	fprintf(fz,"\n");
	fprintf(fz,"   The editor considers the text buffer to be a single linear string \n");
	fprintf(fz,"   of characters which are wrapped across the screen.  You can move\n");
	fprintf(fz,"   to the end of the previous line by hitting <left-arrow> until\n");
	fprintf(fz,"   the beginning of the current line is reached.\n");
	fprintf(fz,"\n");
	fprintf(fz,"   You can likewise exploit this same property for joining lines\n");
	fprintf(fz,"   by deleting the intervening <carriage-return>, breaking lines\n");
	fprintf(fz,"   by inserting a <carriage-return>, etc..\n");
	fprintf(fz,"\n");
	fprintf(fz," To delete characters backward use the delete key.\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz," Editor Keypad:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	---------------------------------------------\n");
	fprintf(fz,"	|          |          |          |          |\n");
	fprintf(fz,"	|  (Old    |  Help    |  Search  |   Cut    |\n");
	fprintf(fz,"	|   Gold)  |          |  /Find   |   Line   |\n");
	fprintf(fz,"	|          |          |          |   (Fwd)  |\n");
	fprintf(fz,"	---------------------------------------------\n");
	fprintf(fz,"	|          |          |          |          |\n");
	fprintf(fz,"	|  Gold    | Jump by  | Replace  |   Cut    |\n");
	fprintf(fz,"	|  (func)  | 16-lines |          |   Word   |\n");
	fprintf(fz,"	|   (7)    |   (8)    |   (9)    |   (Fwd)  |\n");
	fprintf(fz,"	|          |          |          |          |\n");
	fprintf(fz,"	---------------------------------|	    |\n");
	fprintf(fz,"	|          |          |          |          |\n");
	fprintf(fz,"	| Set Dir  |  Set Dir | Cut/Paste|          |\n");
	fprintf(fz,"	| Forward  |  Bckward | Buffer   |          |\n");
	fprintf(fz,"	|   (4)    |   (5)    |   (6)    |          |\n");
	fprintf(fz,"	|          |          |          |          |\n");
	fprintf(fz,"	---------------------------------------------\n");
	fprintf(fz,"	|          |          |          |          |\n");
	fprintf(fz,"	| Jump by  | Jump to  | Enter    |          |\n");
	fprintf(fz,"	|  Word    | End Line | Ascii Val|  Cut     |\n");
	fprintf(fz,"	|   (1)    |   (2)    |   (3)    |  Char    |\n");
	fprintf(fz,"	|          |          |          |          |\n");
	fprintf(fz,"	---------------------------------|  (Fwd)   |\n");
	fprintf(fz,"	|                     |          |          |\n");
	fprintf(fz,"	|     Jump to         | Set Mark |          |\n");
	fprintf(fz,"	|  Beginning of Line  |(Canc Gld)|          |\n");
	fprintf(fz,"	|         (0)         |    (.)   |          |\n");
	fprintf(fz,"	|                     |          |          |\n");
	fprintf(fz,"	---------------------------------------------\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz,"Keypad Key Description:\n");
	fprintf(fz,"\n");
	fprintf(fz," Gold - This key selects the second function of many of\n");
	fprintf(fz,"        the keypad keys.  (Traditionally, gold was the\n");
	fprintf(fz,"	upper-left-most key.  Unfortunately many keypads\n");
	fprintf(fz,"	now implement that as a hardware implemented\n");
	fprintf(fz,"	num-lock which does not return a code.\n");
	fprintf(fz,"	Therefore, gold has been reassigned to the 7-key.)\n");
	fprintf(fz,"\n");
	fprintf(fz," Search/Find - Use the key as follows:\n");
	fprintf(fz,"	To enter a search string, hit the <Gold><Search>\n");
	fprintf(fz,"	keys.  This will bring up an entry area at the\n");
	fprintf(fz,"	bottom of your screen for typing in your search\n");
	fprintf(fz,"	string.  The search string can contain any type\n");
	fprintf(fz,"	of characters, including <carriage-return> and\n");
	fprintf(fz,"	non-alpha-numeric characters such as punctuation\n");
	fprintf(fz,"	etc..  \n");
	fprintf(fz,"\n");
	fprintf(fz,"	Upon completing the search string, initiate the\n");
	fprintf(fz,"	search by hitting the <Dir-Frwrd> or <Dir-Bckwrd>\n");
	fprintf(fz,"	key-pad keys according to which direction relative\n");
	fprintf(fz,"	to the cursor you want to search.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	To search (ie. again or repeatedly) for the \n");
	fprintf(fz,"	current search string, simply hit the <Search>\n");
	fprintf(fz,"	keypad key.\n");
	fprintf(fz,"\n");
	fprintf(fz," Delete-Line-Forward - The <delete-line> key deletes the\n");
	fprintf(fz,"	remaining characters on the line from the cursor\n");
	fprintf(fz,"	position to the end of the line, including the\n");
	fprintf(fz,"	<carriage-return>.  The deleted characters are\n");
	fprintf(fz,"	held in the 'delete-line-buffer'.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	<Gold><delete-line> pastes the characters held\n");
	fprintf(fz,"	in the 'delete-line-buffer' to wherever the\n");
	fprintf(fz,"	cursor is.    This can be used to undo an\n");
	fprintf(fz,"	accidental deletion, or to replicate.\n");
	fprintf(fz,"\n");
	fprintf(fz," Delete-Word-Forward - The <delete-word> key deletes the\n");
	fprintf(fz,"	remaining characters in the word to the right of\n");
	fprintf(fz,"	the cursor.  Words are delineated by the white \n");
	fprintf(fz,"	space characters: <space>, <tab>, and \n");
	fprintf(fz,"	<carriage-return>.  The deleted characters are\n");
	fprintf(fz,"	held in the 'delete-Word-buffer'.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	<Gold><delete-Word> pastes the characters held\n");
	fprintf(fz,"	in the 'delete-Word-buffer' to wherever the\n");
	fprintf(fz,"	cursor is.   This can be used to undo an\n");
	fprintf(fz,"	accidental deletion, or to replicate.\n");
	fprintf(fz,"\n");
	fprintf(fz," Delete-Char-Forward - The <delete-char> key deletes the\n");
	fprintf(fz,"	character to the right of the cursor.\n");
	fprintf(fz,"	The deleted character is held in the \n");
	fprintf(fz,"	'delete-char-buffer'.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	<Gold><delete-char> paste the character held\n");
	fprintf(fz,"	in the 'delete-line-buffer' to wherever the\n");
	fprintf(fz,"	cursor is.   This can be used to undo an\n");
	fprintf(fz,"	accidental deletion, or to replicate.\n");
	fprintf(fz,"\n");
	fprintf(fz," Jump by 16-lines (keypad 8) - The <jump-page> key moves \n");
	fprintf(fz,"	the cursor position by 16 lines, either up or down \n");
	fprintf(fz,"	based on the current direction setting.  You can use\n");
	fprintf(fz,"	this key to scroll through a document quickly.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	<Gold><keypad 8> re-formats a selected text region\n");
	fprintf(fz,"	to the current margin width.  The margin setting\n");
	fprintf(fz,"	defaults to 65, and can be set using the set margin\n");
	fprintf(fz,"	command-line command described above.\n");
	fprintf(fz,"\n");
	fprintf(fz," Jump by Word (keypad 1) - The <jump-word> key moves the \n");
	fprintf(fz,"	cursor either right or left based on the current \n");
	fprintf(fz,"	direction setting.  This helps to quickly move\n");
	fprintf(fz,"	to a position within a line.\n");
	fprintf(fz,"	\n");
	fprintf(fz,"	<Gold><jump-word> = <change-case>, reverses the \n");
	fprintf(fz,"	capitalization of the character to the right or \n");
	fprintf(fz,"	left of the cursor according to the direction \n");
	fprintf(fz,"	setting.  If the cursor is positioned at the start \n");
	fprintf(fz,"	of an instance of the current search-string, then\n");
	fprintf(fz,"	the capitalization of that instance of the \n");
	fprintf(fz,"	search-string will be reversed.  Or, if there\n");
	fprintf(fz,"	is a selection region active (see Set-Mark),\n");
	fprintf(fz,"	then the capitalization of the selected region\n");
	fprintf(fz,"	will be reversed.\n");
	fprintf(fz,"\n");
	fprintf(fz," Jump to Begin of Line (keypad 0) - The <jump-BOL> key \n");
	fprintf(fz,"	moves the cursor to the first character of the \n");
	fprintf(fz,"	next line if the direction is set to forward, \n");
	fprintf(fz,"	or to the first character of the current line \n");
	fprintf(fz,"	if the direction is set to backward, unless it is \n");
	fprintf(fz,"	already on the first character in which case it\n");
	fprintf(fz,"	moves to the first character of the previous\n");
	fprintf(fz,"	line.  The key can be pressed repeatedly to\n");
	fprintf(fz,"	move through a file.\n");
	fprintf(fz,"\n");
	fprintf(fz," Jump to End of Line (keypad 2) - The <jump-EOL> key \n");
	fprintf(fz,"	moves the cursor to the last character of the \n");
	fprintf(fz,"	previous line if the direction is set to backward, \n");
	fprintf(fz,"	or to the last character of the current line if \n");
	fprintf(fz,"	the direction is set to forward, unless it is \n");
	fprintf(fz,"	already on the last character of the current \n");
	fprintf(fz,"	line in which case it moves to the last\n");
	fprintf(fz,"	character of the next line.  The key can be \n");
	fprintf(fz,"	pressed repeatedly to move through a file.\n");
	fprintf(fz,"\n");
	fprintf(fz," Enter character as ASCII Decimal Value (keypad 3) -\n");
	fprintf(fz,"	This key is useful when you need to enter an\n");
	fprintf(fz,"	ASCII value which you do not have a key for,\n");
	fprintf(fz,"	such as non-alpha-numerics.  For instance, to\n");
	fprintf(fz,"	insert a <control> value into a file, such as\n");
	fprintf(fz,"	<cntrl-G>, you would hit <keypad-3>, an entry\n");
	fprintf(fz,"	window at the bottom of your screen prompts \n");
	fprintf(fz,"	you for the ASCII value, which for <control-G>\n");
	fprintf(fz,"	is '7', then you hit <keypad-3> again, and the\n");
	fprintf(fz,"	ASCII value is inserted into the file.\n");
	fprintf(fz,"\n");
	fprintf(fz," Set Direction Forward (keypad-4) - This key sets the\n");
	fprintf(fz,"	direction of many of the other keypad functions\n");
	fprintf(fz,"	to forward, such as jumps and searches.\n");
	fprintf(fz,"	Once set, the direction remains, until changed\n");
	fprintf(fz,"	by the <set-direction-backward> key.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	<Gold><set-direction-forward> moves the cursor\n");
	fprintf(fz,"	to the very end (or bottom) of the file.\n");
	fprintf(fz,"\n");
	fprintf(fz," Set Direction Backward (keypad-5) - This key sets the\n");
	fprintf(fz,"	direction of many of the other keypad functions\n");
	fprintf(fz,"	to backward, such as jumps and searches.\n");
	fprintf(fz,"	Once set, the direction remains, until changed\n");
	fprintf(fz,"	by the <set-direction-forward> key.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	<Gold><set-direction-backward> moves the cursor\n");
	fprintf(fz,"	to the very beginning (or top) of the file.\n");
	fprintf(fz,"\n");
	fprintf(fz," Cut/Paste (keypad-6> - This key cuts a selected section\n");
	fprintf(fz,"	of text from the file, and places it into the\n");
	fprintf(fz,"	paste-buffer.  Note that the cutting action\n");
	fprintf(fz,"	only occurs when a marker has been set and is\n");
	fprintf(fz,"	valid.  (See the <Set-Mark> key.)\n");
	fprintf(fz,"	This can be used to put text into the paste-buffer\n");
	fprintf(fz,"	for <search/replace> operations.  \n");
	fprintf(fz,"	(See <search/replace> key.)\n");
	fprintf(fz,"	This can also be used to cut very large regions \n");
	fprintf(fz,"	from a file.  For instance, you could set a mark on\n");
	fprintf(fz,"	line 30,000, move to line 60,000 and perform the\n");
	fprintf(fz,"	cut to remove 30,000 lines.  This is much faster\n");
	fprintf(fz,"	than using <delete-line>.\n");
	fprintf(fz,"\n");
	fprintf(fz,"	<Gold><paste> pastes the contents of the paste\n");
	fprintf(fz,"	buffer into the file at the location of the\n");
	fprintf(fz,"	cursor.  This can be used to undo an\n");
	fprintf(fz,"        accidental cut, or to replicate sections of\n");
	fprintf(fz,"	text many times.\n");
	fprintf(fz,"\n");
	fprintf(fz," Set Mark (keypad '.'> - This key sets a marker in the\n");
	fprintf(fz,"	file corresponding to the current cursor position.\n");
	fprintf(fz,"	The cursor can then be moved by any of the movement\n");
	fprintf(fz,"	commands, such as arrow-keys, jump-keys, \n");
	fprintf(fz,"	search-keys, or direct line number command.\n");
	fprintf(fz,"	When moved, the text between the cursor and the\n");
	fprintf(fz,"	marker defines a selection region that can be\n");
	fprintf(fz,"	operated on by various other keys, such as the\n");
	fprintf(fz,"	<cut/paste> key, the <reformat> key, or the \n");
	fprintf(fz,"	<change-case> key.\n");
	fprintf(fz,"\n");
	fprintf(fz," Replace (keypad 9> - If the cursor is positioned at the \n");
	fprintf(fz,"	start of an instance of the current search-string, \n");
	fprintf(fz,"	then the instance of the search-string will be\n");
	fprintf(fz,"	replaced by the contents of the paste-buffer when\n");
	fprintf(fz,"	you hit <replace>.  If <Gold><replace> is used,\n");
	fprintf(fz,"	Then he search function will be automatically \n");
	fprintf(fz,"	invoked after the replacement to position the\n");
	fprintf(fz,"	cursor at the next instance of the search-string\n");
	fprintf(fz,"	if there is one.  \n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz,"\n");
	fprintf(fz,"File Recovery\n");
	fprintf(fz,"\n");
	fprintf(fz,"During file editing, the editor maintains a journal file\n");
	fprintf(fz,"of the editing session.  The journal file is useful for\n");
	fprintf(fz,"recovering work that would otherwise be lost in the event\n");
	fprintf(fz,"of a system crash or similar calamity.  Normally, the journal\n");
	fprintf(fz,"file is removed on a successful exit.  In the event of a\n");
	fprintf(fz,"crash or system-shutdown during editing, the journal file\n");
	fprintf(fz,"will remain.\n");
	fprintf(fz,"\n");
	fprintf(fz,"To recover from a journal file, restart the editor on the \n");
	fprintf(fz,"edited file as before, but with the input directed from a\n");
	fprintf(fz,"copy of the journal file.  You will see the editing session \n");
	fprintf(fz,"replayed.  (You must append a ctrl-Z and 'exit' to the journal\n");
	fprintf(fz,"file prior to replay.)\n");
	fprintf(fz,"\n");
	fprintf(fz,"Example:\n");
	fprintf(fz,"\n");
	fprintf(fz,"  Suppose in your directory there is a file called 'text.doc':\n");
	fprintf(fz,"\n");
	fprintf(fz,"	text.doc  Jun 6 1978   8:02\n");
	fprintf(fz,"\n");
	fprintf(fz,"  You begin editing the file 'text.doc' by typing:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	ed text.doc\n");
	fprintf(fz,"\n");
	fprintf(fz,"  An hour into your editing session, the system crashes\n");
	fprintf(fz,"  at 10:37 AM.\n");
	fprintf(fz,"\n");
	fprintf(fz,"  Upon resumption, you will notice there are two files:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	text.doc  Jun 6 1978   8:02\n");
	fprintf(fz,"	text.jou  Jun 7 1978  10:36\n");
	fprintf(fz,"\n");
	fprintf(fz,"  The first file contains the original contents of the\n");
	fprintf(fz,"  'text.doc' file prior to the crashed editing session.\n");
	fprintf(fz,"  The 'text.jou' file contains a record of the edits\n");
	fprintf(fz,"  made during the recent session.\n");
	fprintf(fz,"\n");
	fprintf(fz,"  To be safe, make a copy of the journal file:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	cp  text.jou  recover.rec\n");
	fprintf(fz,"\n");
	fprintf(fz,"  To be extra safe, you should make a copy of your original \n");
	fprintf(fz,"  text file as well:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	cp text.doc  text_recovered.doc\n");
	fprintf(fz,"\n");
	fprintf(fz,"  You can now recover the edits from the interrupted\n");
	fprintf(fz,"  editing session as follows:\n");
	fprintf(fz,"\n");
	fprintf(fz,"  You begin the recovery process by editing the 'recover.rec'\n");
	fprintf(fz,"  file so as to put a control-Z and 'exit' at its end so that\n");
	fprintf(fz,"  the file will be saved.  \n");
	fprintf(fz," \n");
	fprintf(fz,"    Specifically, edit 'recover.rec', enter screen mode (c), \n");
	fprintf(fz,"    go to the end of the file (gold 4), hit space, then enter \n");
	fprintf(fz,"    the control character ^Z by hitting in sequence:\n");
	fprintf(fz,"\n");
	fprintf(fz,"		gold\n");
	fprintf(fz,"		3\n");
	fprintf(fz,"		26\n");
	fprintf(fz,"		gold\n");
	fprintf(fz,"\n");
	fprintf(fz,"    (26 is the ascii value for ^Z.)\n");
	fprintf(fz,"    Then enter 'ex' after the ^Z.  This will cause an exit \n");
	fprintf(fz,"    and save of the document when you execute the recovery file.\n");
	fprintf(fz,"\n");
	fprintf(fz,"    Then save and close the recovery file.\n");
	fprintf(fz,"\n");
	fprintf(fz,"  Now execute the recovery file by typing:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	ed text_recovered.doc < recovery.rec\n");
	fprintf(fz,"\n");
	fprintf(fz,"  You will see the replay of your editing session and the \n");
	fprintf(fz,"  editor should save the final version and close.\n");
	fprintf(fz,"\n");
	fprintf(fz,"  If you are satisfied with the recovered file, then re-copy\n");
	fprintf(fz,"  it over-top of the original:\n");
	fprintf(fz,"\n");
	fprintf(fz,"	cp text_recovered.doc text.doc\n");
	fprintf(fz,"\n");

	fclose(fz);

	sprintf(tcline,"lrss %s", tname);
	system(tcline);

	unlink(tname);
}



void help_keypad_setup()
{
FILE *fz;
char tcline[260];

	// tmpnam(fnm);
	mkstemp(tname);

	if ( !(fz = fopen(tname, "w")) )
		{
		printf("Cannot open %s\n", tname);
		return;
		}

	fprintf(fz,"         Configuring the Key-Pad:\n");
	fprintf(fz,"         ------------------------\n");
	fprintf(fz,"\n");
	fprintf(fz,"When first installing EDT, the key-pad configuration procedure\n");
	fprintf(fz,"should be run to acquaint EDT with your particular keyboard.\n");
	fprintf(fz,"This process makes the 'edt_keypad.xml' file that EDT references\n");
	fprintf(fz,"when starting all future sessions.\n\n");
	fprintf(fz,"To run the configuration proceedure, at the editor's prompt, type:\n\n");
	fprintf(fz,"	configure_keypad\n\n");
	fprintf(fz,"It will lead you through a set of instructions involving pressing the\n");
	fprintf(fz,"keypad keys.  When complete, it will store the configuration file as\n");
	fprintf(fz,"'edt_keypad.xml'.  The editor will rely on the environment variable:\n");
	fprintf(fz,"	EDT_KEYPAD_SETUP\n to point to that file on future invocations.  We recommend\n");
	fprintf(fz,"moving the edt_keypad.xml file to your main directory, and placing a line in \n");
	fprintf(fz,"your .cshrc or .bashrc file to set the variable automatically on future sessions.\n\n");
	fprintf(fz,"If you have previously created the configuration, but get a warning\n");
	fprintf(fz,"when starting EDT that it cannot find it, and the keypad bindings\n");
	fprintf(fz,"do not take their functions, then the environment variable needs\n");
	fprintf(fz,"to be set to properly point to the edt_keypad.xml file.\n\n");
	fclose(fz);

	sprintf(tcline,"less %s", tname);
	system(tcline);

	unlink(tname);
}

