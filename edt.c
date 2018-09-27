/********************************************************************************
 * Edt.c - Text editor.  This text-editor emulates the VAX VMS text editor      *
 * known as Edt and to some extent, later versions called TPU/Eve.              *
 *                                                                              *
 * Works in Xterm windows under Linux and Unix.                                 *
 *                                                                              *
 * Compile as:                                                                  *
 *     gcc -O edt_1.9.c -o edt							*
 *  										*
 * Edt.c -  - LGPL License:                                                     *
 *  Copyright (C) 2001, Carl Kindman						*
 *  This library is free software; you can redistribute it and/or		*
 *  modify it under the terms of the GNU Lesser General Public			*
 *  License as published by the Free Software Foundation; either		*
 *  version 2.1 of the License, or (at your option) any later version.		*
 *  This library is distributed in the hope that it will be useful,		*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of		*
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU		*
 *  Lesser General Public License for more details.				*
 *  You should have received a copy of the GNU Lesser General Public		*
 *  License along with this library; if not, write to the Free Software		*
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	*
 *										*
 *  For updates/info, see:  http://sourceforge.net/projects/edt-text-editor	*
 *										*
 *  Carl Kindman 8-31-2010     carlkindman@yahoo.com				*
 ********************************************************************************/


/***********************************************************
 * Revision History:
 *  Rev 1.0 - Original version
 *  Rev 1.01 - Copy/Paste key with select enabled, plus many
 *		other enhancements, such as search/replace
 *  Rev 1.1 - Activated column limiting
 *  Rev 1.2 - Changed finite line/word buffers to unlimited
 *		Dynamic type
 *  Rev 1.3 - Automatic window sizing, resize, and reformat.
 *  Rev 1.4 - Naming of .bak across directories, and affirm
 *		exit from non-main buffer.
 *  Rev 1.5 - Accept ending colon:number in file name as line
 *		go to after opening file, such as on a compiler
 *		warning or error message.
 *  Rev 1.6 - General clean-ups.
 *  Rev 1.7 - Added XML key-pad setup file format.
 *	      Added ability to edit gzip'd files.
 *  Rev 1.8 - Added 'wq', 'wq!', 'q!', and 'begin cprog', 'begin c++prog',
 *		and 'begin javaprog' line commands.  Accepts 'start ..." too.
 *	      Added ability to skip '!' for 'ls' and 'dir' at edt cmd-line.
 *  Rev 1.9 - Added check against accdentally attempting to edit directory file.
 *	      Added 'begin html' line command.  Added Makefile.
 *	      Added Sun keyboard map file.
 *  Rev 2.0 - Plan to activate text-selection highlighting.
 *
 *
 * Still To Be Done (someday, though quite usable meanwhile):
 *	- Inverse video to show marked text.
 *	- Enable editing of marked text. Presently any adds or deletes remove
 *        the mark for safety.
 *
 * Presently, all screen management functions are in direct ANSI escape sequenes.
 * Works well in Xterm windows, but not Gnome-terminal windows.
 * In would be nice to convert to Curses screen management functions to support
 * a wider range of terminals.
 *
 * Contributors - Many people, including:  The screen handling sections
 * were originally written by Eric Elisney, Dean Henlason, and others.  The Edt
 * features and personality were later enhanced by A. Krumheuer and Jay
 * Chamberlain.  Major sections were ported to Unix by Robert Pesch.
 *
 *
 ***********************************************************/

/*----------------------------------------------------------------------*/
/* Program's Organization:                                              */
/*   1. Accept key, key handler figures new curser position (row, col)  */
/*      and brings cursor pointer to it.                                */
/*   2. Then adjust screen adjusts the screen accordingly, calculates   */
/*      the new relative cursor positions and puts the blinking cursor  */
/*      at the right location.                                          */
/*----------------------------------------------------------------------*/

/*
*  MODIFICATION HISTORY:
*	27-SEP-2018	RRL	Reformating C 'puppie-style' to something readable.
*/



#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<unistd.h>

#define	EDT$K_VERSION	2.0

/* 1 = SUN,  2 = VT240,  3 = VT100,   4 = Sun_sparc10,  5 = PC-Linux */
#define	EDT$K_TERMSUN	1
#define	EDT$K_TERMVT240	2
#define	EDT$K_TERMVT100	3
#define	EDT$K_TERMSPARC	4
#define	EDT$K_TERMLNX	5
#define	EDT$K_TERMTYPE	EDT$K_TERMLNX

#define	EDT$K_ESC	27
#define	EDT$K_BELL	7

int functkeys_table[19][8]=	/* Key-pad return codes table. Sets default values here. */
{
	{ 1000, 0, EDT$K_ESC, 79, 80, -1,  -1, -1 },  /*Gold */
	{ 1001, 0, EDT$K_ESC, 79, 81, -1,  -1, -1 },  /*Find*/
	{ 1002, 0, EDT$K_ESC, 79, 82, -1,  -1, -1 },  /*Delete_Line*/
	{ 1003, 0, EDT$K_ESC, 79, 83, -1,  -1, -1 },  /*Jump-Page*/
	{ 1004, 0, EDT$K_ESC, 91, 49, 53, 126, -1 },  /*Replace*/
	{ 1005, 0, EDT$K_ESC, 91, 49, 55, 126, -1 },  /*Forward*/
	{ 1006, 0, EDT$K_ESC, 91, 49, 56, 126, -1 },  /*Backward*/
	{ 1007, 0, EDT$K_ESC, 91, 49, 57, 126, -1 },  /*Cut*/
	{ 1008, 0, EDT$K_ESC, 91, 50, 48, 126, -1 },  /*Delete_Word*/
	{ 1009, 0, EDT$K_ESC, 91, 50, 49, 126, -1 },  /*Jump_Word*/
	{ 1010, 0, EDT$K_ESC, 91, 50, 51, 126, -1 },  /*Jump_EOL*/
	{ 1012, 0, EDT$K_ESC, 91, 72, -1,  -1, -1 },  /*Enter_Ascii*/
	{ 1011, 0, EDT$K_ESC, 91, 50, 52, 126, -1 },  /*Jump_BOL*/
	{ 1013, 0, EDT$K_ESC, 91, 70, -1,  -1, -1 },  /*Mark*/
	{ 1014, 0, EDT$K_ESC, 91, 50, 126, -1, -1 },  /*Delete_Character*/
	{ 1015, 0, EDT$K_ESC, 91, 65,  -1, -1, -1 },  /*Up-Arrow*/
	{ 1016, 0, EDT$K_ESC, 91, 66,  -1, -1, -1 },  /*Down-Arrow*/
	{ 1017, 0, EDT$K_ESC, 91, 67,  -1, -1, -1 },  /*Right-Arrow*/
	{ 1018, 0, EDT$K_ESC, 91, 68,  -1, -1, -1 },  /*Left-Arrow*/
};

FILE	*infile, *outfile, *jou_outfile;
char	jou_buf[64];

typedef	struct __text__
	{
	char	ch;
	struct	__text__ *prv, *nxt;
} TEXT;

TEXT	*EOB, *txt_head, *txt_free, *txt_tmp, *curse_pt, *new_curse_pt,
	*free_nil, *paste_buffer, *word_buf, *line_buf, *format_buffer;

typedef struct __buf_lis__ {
	struct __text__ *txt_head, *curse_pt, *EOB;

	int curse_row, last_row;
	char buff_name[256];

	struct __buf_lis__ *nxt;
} BUF_LIS;

BUF_LIS	*buffer_list, *tmp_buff_pt;

#define MAX_SRCH_STRING 4192

char active_buffer_name[1024];

/* Cursor position in file coordinates */
int	curse_row,
	last_curse_col;			/* active column if possible */

/* Position of curser with respect to screen */
int	rel_curse_row, rel_curse_col;

/* Top and bottom of screen in file coordinates */
int	tframe_row;

/* Top and bottom of work area in file coordinates */

/* Last row of the file/buffer */
int	last_row;

/* Jump / Search Direction */
int	direction;		/* 1=forward, -1=backward */

/* Editor mode (0=command line, 1=full screen). */
// int mode;

/* Special Modes */
int	read_only = 0,	/* read-only mode */
	encode_mode = 0;

char	*psswd;

/* Size of display device screen */
int	nrows, ncols;

int	message_pending, changed = 0;

int	inpt1, ctrl,
	Gold, Mark, mark_row, mark_col, paste_buffer_length = 0, right_margin = 70;
struct __text__ *mark_pt1, *mark_pt2;
char	ch_buf, srch_caps = 1, srch_strng[MAX_SRCH_STRING];


#include "scz_routines.c"


void read_line( FILE *infile, char *line, int maxlen )	/* Like fgets, but more descriptive name, */
{							/* and filters new-line away from returned string. */
int i = 0;

	do	{
	line[i++] = getc(infile);
	} while ( (!feof(infile)) && (line[i-1] != '\n')  && (line[i-1] != '\r') && (i < maxlen) );

	line[i] = '\0';
}


void edt_setkey( int k )
{
int	i = 0, change = 0;
char	seq[100];

	do	{
		seq[i++] = getchar();
	} while (seq[i-1] != '\n' );

	for (i = 0; seq[i] == functkeys_table[k][i+2]; i++ );

	if ( (seq [i] != '\n') || (functkeys_table[k][i+2] != -1) )
		change = 1;

	if  ( !change )
		{
		printf("	(Unchanged.)\n");
		return;
		}

	printf("	(Previously assumed to be: ");

	for (i = 0; functkeys_table[k][i+2] !=-1; i++ )
		printf("<%d> ", functkeys_table[k][i+2]);

	if ( seq[0] == '\n' )
		{
		printf("\n	(Unchanged)\n");
		return;
		}

	printf(")\n	(Setting to: ");

	for ( i = 0; seq[i] != '\n'; i++)
		functkeys_table[k][i+2] = seq[i];

	functkeys_table[k][i+2] = -1;

	for (i = 2; functkeys_table[k][i] != -1; i++ )
		printf("<%d> ", functkeys_table[k][i]);

	printf(")\n");
}


char	delimiters[] = " \t,:";


/*.......................................................................
  .     NEXT_WORD - accepts a line of text, and returns with the        .
  . next word in that text in the second parameter, the original line   .
  . is shortened from the beginning so that the word is removed.        .
  . If the line encountered is empty, then the word returned will be    .
  . empty.                                                              .
  . NEXTWORD can parse on an arbitrary number of delimiters.            .
  .......................................................................*/
void next_word( char *line, char *word, char *delim )
{
int	i = 0, j = 0, m = 0, flag = 1;

	/* Eat away preceding garbage */
	while ( (line[i]!='\0') && (flag) )
		{
		for ( j = 0; (delim[j]!='\0') && (line[i]!=delim[j]); j++);

		if (line[i] == delim[j] )
			i++;
		else	flag = 0;
		}

	/* Copy the word until the next delimiter. */
	while ( (line[i]!='\0') && (!flag) )
		{
		word[m++] = line[i++];

		if (line[i] != '\0')
			{
			j = 0;
			for (j = 0; (delim[j]!='\0') && (line[i]!=delim[j]); j++);

			if ( line[i] == delim[j] )
				flag = 1;
			}
		}

	/* Shorten line. */
	for (j = 0; line[i] != '\0'; i++, j++)
		line[j] = line[i];

	/* Terminate the char-strings. */
	line [j] = word [m] = '\0';
}


/*.......................................................................
  . XML_NEXT_WORD - accepts a line of text, and returns with the        .
  . next word in that text in the third parameter, the original line    .
  . is shortened from the beginning so that the word is removed.        .
  .......................................................................*/
void Xml_Next_Word( char *line, char *word, int maxlen, char *delim )
{
int	i = 0, j = 0, m = 0, flag = 1;

	while ( (line[i]!='\0') && (flag) )   /* Eat away preceding garbage */
		{
		j = 0;
		for (j = 0; (delim[j] !='\0' ) && ( line[i] != delim[j]); j++);

		if ( line[i] == delim[j] )
			i++;
		else  flag = 0;
		}

	maxlen--;

	while ( (line[i]!='\0') && (m < maxlen) && (!flag) )  /* Copy the word until the next delimiter. */
		{
		word[m++] = line[i++];

		if (line[i]!='\0')
			{
			for (j = 0; (delim[j] != '\0') && (line[i] != delim[j]); j++);

			flag = ( line[i] == delim[j] );
			}
		}

				 /* Shorten line. */
	for (j = 0; line[i] != '\0'; )
		line[j++] = line[i++];

	line[j] = word[m] = '\0';	 /* Terminate the char-strings. */

}


/********************************************************************************/
/* xml_strncpy - Copy src string to dst string, up to maxlen characters.	*/
/* Safer and faster than strncpy, because it does not fill destination string, 	*/
/* but only copies up to the length needed.  					*/
/********************************************************************************/
void xml_strncpy( char *dst, const char *src, int maxlen )
{
int j = 0, oneless = maxlen - 1;

	for (j = 0; (j < oneless) && (src[j] != '\0'); j++)
		dst[j] = src[j];

	dst[j] = '\0';
}


void xml_remove_leading_trailing_spaces( char *word )
{
int	i = 0, j = 0;

	while ( (word[i]!='\0') && ((word[i]==' ') || (word[i]=='\t') || (word[i]=='\n') || (word[i]=='\r')) )
		i = i + 1;

	do { word[j++] = word[i++]; } while (word[i-1]!='\0');

	j = j - 2;

	while ((j>=0) && ((word[j]==' ') || (word[j]=='\t') || (word[j]=='\n') || (word[j]=='\r')))
		j = j - 1;

	word[ j + 1] = '\0';
}


void xml_restore_escapes( char *phrase )
{ /* Replace any xml-escapes for (&), quotes ("), or brackets (<,>), with original symbols. */
  int j=0, k, m, n;
  n = strlen(phrase);
  do
   {
    if (phrase[j]=='&')
     {
      switch (phrase[j+1])
       {
	case 'a':  phrase[j++] = '&';
	  m = j;  k = j + 4;
	  if (k>n) {printf("xml_Parse: String ends prematurely after ampersand '%s'.\n",phrase); return;}  n = n - 4;
	  do phrase[m++] = phrase[k++]; while (phrase[k-1] != '\0');
	 break;
	case 'q':  phrase[j++] = '"';
	  m = j;  k = j + 5;
	  if (k>n) {printf("xml_Parse: String ends prematurely after ampersand '%s'.\n",phrase); return;}  n = n - 5;
	  do phrase[m++] = phrase[k++]; while (phrase[k-1] != '\0');
	 break;
	case 'l':  phrase[j++] = '<';
	  m = j;  k = j + 3;
	  if (k>n) {printf("xml_Parse: String ends prematurely after ampersand '%s'.\n",phrase); return;}  n = n - 3;
	  do phrase[m++] = phrase[k++]; while (phrase[k-1] != '\0');
	 break;
	case 'g':  phrase[j++] = '>';
	  m = j;  k = j + 3;
	  if (k>n) {printf("xml_Parse: String ends prematurely after ampersand '%s'.\n",phrase); return;}  n = n - 3;
	  do phrase[m++] = phrase[k++]; while (phrase[k-1] != '\0');
	 break;
	default: printf("xml_Parse: Unexpected char (%c) follows ampersand (&) in xml.\n", phrase[j+1]);  j++;
       }
     } else j++;
   }
  while (phrase[j] != '\0');
}


/************************************************************************/
/* XML_GRAB_TAG_NAME - This routine gets the tag-name, and shortens the	*/
/*  xml-tag by removing it from the tag-string.			 	*/
/************************************************************************/
void xml_grab_tag_name( char *tag, char *name, int maxlen )
{
 int j;
 Xml_Next_Word( tag, name, maxlen, " \t\n\r");
 j = strlen(name);
 if ((j > 1) && (name[j-1] == '/'))	/* Check for case where slash was attached to end of tag-name. */
  {
   name[j-1] = '\0';	/* Move slash back to tag. */
   j = 0;  do { tag[j+1] = tag[j];  j++; } while (tag[j-1] != '\0');
   tag[0] = '/';
  }
}


/************************************************************************/
/* XML_GRAB_ATTRIBVALUE - This routine grabs the next name-value pair	*/
/*  within an xml-tag, if any.  					*/
/************************************************************************/
void xml_grab_attrib( char *tag, char *name, char *value, int maxlen )
{
 int j=0, k=0, m;
 Xml_Next_Word( tag, name, maxlen, " \t=\n\r");	 /* Get the next attribute's name. */
 /* Now get the attribute's value-string. */
 /* Sequence up to first quote.  Expect only white-space and equals-sign. */
 while ((tag[j]!='\0') && (tag[j]!='\"'))
  {
   if ((tag[j]!=' ') && (tag[j]!='\t') && (tag[j]!='\n') && (tag[j]!='\r') && (tag[j]!='='))
    printf("xml error: unexpected char before attribute value quote '%s'\n", tag);
   j++;
  }
 if (tag[j]=='\0')  { value[0] = '\0';  tag[0] = '\0';  return; }
 if (tag[j++]!='\"')
  { printf("xml error: missing attribute value quote '%s'\n", tag); tag[0] = '\0'; value[0] = '\0'; return;}
 while ((tag[j]!='\0') && (tag[j]!='\"')) { value[k++] = tag[j++]; }
 value[k] = '\0';
 if (tag[j]!='\"') printf("xml error: unclosed attribute value quote '%s'\n", tag);  else j++;
 xml_restore_escapes( value );
 /* Now remove the attribute (name="value") from the original tag-string. */
 k = 0;
 do tag[k++] = tag[j++]; while (tag[k-1] != '\0');
}


/****************************************************************/
/* XML_PARSE - This routine finds the next <xxx> tag, and grabs	*/
/*	it, and then grabs whatever follows, up to the next tag.*/
/****************************************************************/
void xml_parse( FILE *fileptr, char *tag, char *content, int maxlen, int *lnn )
{
 int i;  char ch;

 /* Get up to next tag. */
 do { ch = getc(fileptr);  if (ch=='\n') (*lnn)++; } while ((!feof(fileptr)) && (ch != '<'));

 i = 0; 	/* Grab this tag. */
 do
  { do { tag[i] = getc(fileptr);  if (tag[i]=='\n') tag[i] = ' '; }
    while ((tag[i]=='\r') && (!feof(fileptr)));  i=i+1;
    if ((i==3) && (tag[0]=='!') && (tag[1]=='-') && (tag[2]=='-'))
     { /*Filter_comment.*/
       i = 0;
       do { ch = getc(fileptr); if (ch=='-') i = i + 1; else if ((ch!='>') || (i==1)) i = 0; }
       while ((!feof(fileptr)) && ((i<2) || (ch!='>')));
       do { ch = getc(fileptr);  if (ch=='\n') (*lnn)++; } while ((!feof(fileptr)) && (ch != '<'));
       i = 0;
     } /*Filter_comment.*/
  } while ((!feof(fileptr)) && (i < maxlen) && (tag[i-1] != '>'));
 if (i==0) i = 1;
 tag[i-1] = '\0';

 i = 0; 	/* Now grab contents until next tag. */
 do
  { do  content[i] = getc(fileptr);  while ((content[i]=='\r') && (!feof(fileptr)));
    if (content[i]==10) (*lnn)++; i=i+1;
  }
 while ((!feof(fileptr)) && (i < maxlen) && (content[i-1] != '<'));
 ungetc( content[i-1], fileptr );
 if (i==0) i = 1;
 content[i-1] = '\0';

 /* Clean-up contents by removing trailing white-spaces, and restoring any escaped characters. */
 xml_remove_leading_trailing_spaces( tag );
 xml_remove_leading_trailing_spaces( content );
 xml_restore_escapes( content );
}

/* ============================================================== */
/* End of Re-Usable XML Parser Routines.         		  */
/* ============================================================== */



void setstr( int *functkeys, char *tmpstr )
{
 int j=2;
 char word[20];

 tmpstr[0] = '\0';
 while ((j < 8) && (functkeys[j] != -1))
  {
   if (j > 2) strcat( tmpstr, " " );
   sprintf(word,"%d", functkeys[j++] );
   strcat( tmpstr, word);
  }
}




char *restore[20];
char tname[] = "/tmp/edtmpfileXXXXXX";

void configure_keyboard()
{
 int i=0, k, m, n, codes[40];
 char ans[400], tmpwrd[200], *cptr;
 FILE *infile;

 printf("\nKeyPad Configuration:\n");
 printf("\n");
 printf("	---------------------------------------------\n");
 printf("	|          |          |          |          |\n");
 printf("	|  (Alt    |          |  Search  |   Cut    |\n");
 printf("	|   Gold)  |          |  /Find   |   Line   |\n");
 printf("	|          |          |          |   (Fwd)  |\n");
 printf("	---------------------------------------------\n");
 printf("	|          |          |          |          |\n");
 printf("	|  Gold    | Jump by  | Replace  |   Cut    |\n");
 printf("	|  (func)  | 16-lines |          |   Word   |\n");
 printf("	|   (7)    |   (8)    |   (9)    |   (Fwd)  |\n");
 printf("	|          |          |          |          |\n");
 printf("	---------------------------------|	    |\n");
 printf("	|          |          |          |          |\n");
 printf("	| Set Dir  |  Set Dir | Cut/Paste|          |\n");
 printf("	| Forward  |  Bckward | Buffer   |          |\n");
 printf("	|   (4)    |   (5)    |   (6)    |          |\n");
 printf("	|          |          |          |          |\n");
 printf("	---------------------------------------------\n");
 printf("	|          |          |          |          |\n");
 printf("	| Jump by  | Jump to  | Enter    |          |\n");
 printf("	|  Word    | End Line | Ascii Val|  Cut     |\n");
 printf("	|   (1)    |   (2)    |   (3)    |  Char    |\n");
 printf("	|          |          |          |          |\n");
 printf("	---------------------------------|  (Fwd)   |\n");
 printf("	|                     |          |          |\n");
 printf("	|     Jump to         | Set Mark |          |\n");
 printf("	|  Beginning of Line  |(Canc Gld)|          |\n");
 printf("	|         (0)         |    (.)   |          |\n");
 printf("	|                     |          |          |\n");
 printf("	---------------------------------------------\n");
 printf("\n(Continue?) "); getchar();
 printf("\n");
 printf("\nKeyPad Configuration:\n");
 printf(" This is a two-step process.  The first step finds the 'raw X-key-codes'\n");
 printf(" of each of your key-pad's keys.  It will be skipped if you are not in\n");
 printf(" an environment where X-window calls can execute, such as a telnet window.\n");
 printf(" Otherwise, a small window will appear.  BE CAREFULL!!!  What you type\n");
 printf(" will affect your keyboard.\n\nStep 1:\n");
 printf(" Type each of the following keys in exact order into that window:\n");
 // tmpnam(tname);
 mkstemp(tname);
 sprintf(ans,"xev > %s", tname);
 printf("	(%s)\n",ans);
 printf("   Gold (usually keypad 7): \n");
 printf("   Gold alternate (usually num-lock): \n");
 printf("   Search (usually keypad *): \n");
 printf("   Delete-Line (usually keypad  - ): \n");
 printf("   Jump by Page (usually keypad 8): \n");
 printf("   Replace (usually keypad 9): \n");
 printf("   Forward (usually keypad 4): \n");
 printf("   Backward (usually keypad 5): \n");
 printf("   Cut (usually keypad 6): \n");
 printf("   Delete Word (usually keypad  + ): \n");
 printf("   Jump by Word (usually keypad 1): \n");
 printf("   Jump to EOL (usually keypad 2): \n");
 printf("   Enter Ascii (usually keypad 3): \n");
 printf("   Jump to BOL (usually keypad 0): \n");
 printf("   Mark (usually keypad  . ): \n");
 printf("   Delete Char (usually keypad  <Enter> ): \n");
 printf("   Space-bar (press this several times!!!)\n");
 printf(" Now, after typing the above keys, exit the small window.\n");
 system(ans);  printf("\n");
 infile = fopen(tname,"r");
 if (infile==0) printf("Could not get key-codes.  Proceeding to next step with existing codes.\n");
 else
  {
   for (m=0; m!=20; m++) codes[m] = -1;
   i = 0;
   read_line( infile, ans, 400 );
   while ((!feof(infile)) && (i<20))
    {
     if (strstr(ans,"keycode")!=0)
      {
       do next_word( ans, tmpwrd, delimiters ); while (strcmp(tmpwrd,"keycode")!=0);
       next_word( ans, tmpwrd, delimiters );
       sscanf(tmpwrd,"%d",&k);  n = 1;
       for (m=0; m!=20; m++) if (codes[m] == k) n = 0;
       if ((n) && (strstr(ans,"space")==0) && (strstr(ans,"Return")==0))
	{
	 codes[i] = k;  i = i + 1;  printf("	Keycode[%d] = %d\n", i-1, k);
	 next_word( ans, tmpwrd, delimiters ); next_word( ans, tmpwrd, delimiters ); next_word( ans, tmpwrd, delimiters );
	 cptr = (char *)strstr(tmpwrd,")"); if (cptr!=0) cptr[0] = '\0';
	 restore[i-1] = (char *)strdup(tmpwrd);
	}
      }
     read_line( infile, ans, 400 );
    }
   fclose(infile);
   remove(tname);
   if (i < 16)
    {
     printf("Error:  Wrong number of keys pressed (%d, should have been 16).\n", i);
     printf("Continue to second step (y) or abort (n) ? ");
     scanf("%s",tmpwrd); if (tmpwrd[0]!='y') return;
    }
   else
    {
     sprintf(ans,"xmodmap -e \"keycode %d = F1\"", codes[0] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F1\"", codes[1] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F2\"", codes[2] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F3\"", codes[3] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F4\"", codes[4] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F5\"", codes[5] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F6\"", codes[6] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F7\"", codes[7] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F8\"", codes[8] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F9\"", codes[9] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F10\"", codes[10] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F11\"", codes[11] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = Home\"", codes[12] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = F12\"", codes[13] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = End\"", codes[14] );   system(ans);
     sprintf(ans,"xmodmap -e \"keycode %d = Insert\"", codes[15] );   system(ans);
    }
  }

 printf("\nStep 2:");
 printf("\n Now I will learn the actual values returned by the keys.\n");
 printf("	(Press each of the following keys, followed by <Enter> or <Return>.\n");
 printf("	(Pressing <Return> only, will not change the key's setting.)\n");
 printf("	(Each keypad and arrow key should produce multiple (2-4) strange characters.\n");
 printf("	 If not, the key may need to be re-assigned to a function-key with xmodmap.)\n\n");
 i = 0;
 printf(" Gold (usually keypad-7): ");  	edt_setkey( i++ );
 printf(" Search (usually keypad-*): ");  	edt_setkey( i++ );
 printf(" Delete-Line (usually keypad- - ): "); edt_setkey( i++ );
 printf(" Jump by Page (usually keypad-8): ");  	edt_setkey( i++ );
 printf(" Replace (usually keypad-9): ");  	edt_setkey( i++ );
 printf(" Forward (usually keypad-4): ");  	edt_setkey( i++ );
 printf(" Backward (usually keypad-5): ");  	edt_setkey( i++ );
 printf(" Cut (usually keypad-6): ");	  	edt_setkey( i++ );
 printf(" Delete Word (usually keypad- + ): "); edt_setkey( i++ );
 printf(" Jump by Word (usually keypad-1): ");  	edt_setkey( i++ );
 printf(" Jump to EOL (usually keypad-2): ");  	edt_setkey( i++ );
 printf(" Enter Ascii (usually keypad-3): ");  	edt_setkey( i++ );
 printf(" Jump to BOL (usually keypad-0): ");  	edt_setkey( i++ );
 printf(" Mark (usually keypad- . ): ");  	edt_setkey( i++ );
 printf(" Delete Char (usually keypad- <Enter> ): ");  	edt_setkey( i++ );
 printf(" Up-Arrow: ");			  	edt_setkey( i++ );
 printf(" Down-Arrow: ");  			edt_setkey( i++ );
 printf(" Right-Arrow: keypad-): ");  		edt_setkey( i++ );
 printf(" Left-Arrow: keypad-): ");  		edt_setkey( i++ );
 printf("\nSave keypad configuration to 'edt_keypad.xml' (y/n) ? ");
 scanf("%s",ans);
 if (ans[0]=='y')
  {
   FILE *outfile;
   int j;
   char tmpstr[100];
   outfile = fopen("edt_keypad.xml","w");

   setstr( functkeys_table[0], tmpstr );
   fprintf(outfile,"<key_set function=\"Gold\" \tkey=\"%d\" mapto=\"F1\" returns=\"%s\" />\n", codes[0], tmpstr );
   setstr( functkeys_table[0], tmpstr );
   fprintf(outfile,"<key_set function=\"Gold\" \tkey=\"%d\" mapto=\"F1\" returns=\"%s\" />\n", codes[1], tmpstr );
   setstr( functkeys_table[1], tmpstr );
   fprintf(outfile,"<key_set function=\"Find\" \tkey=\"%d\" mapto=\"F2\" returns=\"%s\" />\n", codes[2], tmpstr );
   setstr( functkeys_table[2], tmpstr );
   fprintf(outfile,"<key_set function=\"Delete_Line\" key=\"%d\" mapto=\"F3\" returns=\"%s\" />\n", codes[3], tmpstr );
   setstr( functkeys_table[3], tmpstr );
   fprintf(outfile,"<key_set function=\"Jump_Page\" \tkey=\"%d\" mapto=\"F4\" returns=\"%s\" />\n", codes[4], tmpstr );
   setstr( functkeys_table[4], tmpstr );
   fprintf(outfile,"<key_set function=\"Replace\" \tkey=\"%d\" mapto=\"F5\" returns=\"%s\" />\n", codes[5], tmpstr );
   setstr( functkeys_table[5], tmpstr );
   fprintf(outfile,"<key_set function=\"Forward\" \tkey=\"%d\" mapto=\"F6\" returns=\"%s\" />\n", codes[6], tmpstr );
   setstr( functkeys_table[6], tmpstr );
   fprintf(outfile,"<key_set function=\"Backward\" \tkey=\"%d\" mapto=\"F7\" returns=\"%s\" />\n", codes[7], tmpstr );
   setstr( functkeys_table[7], tmpstr );
   fprintf(outfile,"<key_set function=\"Cut\" \tkey=\"%d\" mapto=\"F8\" returns=\"%s\" />\n", codes[8], tmpstr );
   setstr( functkeys_table[8], tmpstr );
   fprintf(outfile,"<key_set function=\"Delete_Word\" key=\"%d\" mapto=\"F9\" returns=\"%s\" />\n", codes[9], tmpstr );
   setstr( functkeys_table[9], tmpstr );
   fprintf(outfile,"<key_set function=\"Jump_Word\" \tkey=\"%d\" mapto=\"F10\" returns=\"%s\" />\n", codes[10], tmpstr );
   setstr( functkeys_table[10], tmpstr );
   fprintf(outfile,"<key_set function=\"Jump_EOL\" \tkey=\"%d\" mapto=\"F11\" returns=\"%s\" />\n", codes[11], tmpstr );
   setstr( functkeys_table[11], tmpstr );
   fprintf(outfile,"<key_set function=\"Enter_Ascii\" key=\"%d\" mapto=\"Home\" returns=\"%s\" />\n", codes[12], tmpstr );
   setstr( functkeys_table[12], tmpstr );
   fprintf(outfile,"<key_set function=\"Jump_BOL\" \tkey=\"%d\" mapto=\"F12\" returns=\"%s\" />\n", codes[13], tmpstr );
   setstr( functkeys_table[13], tmpstr );
   fprintf(outfile,"<key_set function=\"Mark\" \tkey=\"%d\" mapto=\"End\" returns=\"%s\" />\n", codes[14], tmpstr );
   setstr( functkeys_table[14], tmpstr );
   fprintf(outfile,"<key_set function=\"Delete_Character\" key=\"%d\" mapto=\"Insert\" returns=\"%s\" />\n", codes[15], tmpstr );
   setstr( functkeys_table[15], tmpstr );
   fprintf(outfile,"<key_set function=\"Up_Arrow\" \tkey=\"0\" mapto=\"\" returns=\"%s\" />\n", tmpstr );
   setstr( functkeys_table[16], tmpstr );
   fprintf(outfile,"<key_set function=\"Down_Arrow\" \tkey=\"0\" mapto=\"\" returns=\"%s\" />\n", tmpstr );
   setstr( functkeys_table[17], tmpstr );
   fprintf(outfile,"<key_set function=\"Right_Arrow\" key=\"0\" mapto=\"\" returns=\"%s\" />\n", tmpstr );
   setstr( functkeys_table[18], tmpstr );
   fprintf(outfile,"<key_set function=\"Left_Arrow\" \tkey=\"0\" mapto=\"\" returns=\"%s\" />\n", tmpstr );
   fprintf(outfile,"\n");
   for (j=0; j <= 15; j++)
    fprintf(outfile,"<key_restore key=\"%d\" mapto=\"%s\" />\n", codes[j], restore[j] );
   fclose(outfile);
   printf("\n Saved edt_keypad.xml\n\n");
   printf(" Set environment variable EDT_KEYPAD_SETUP to point to that file.\n");
   printf(" (Make permanent by setting it in your .cshrc or .bash.)\n\n");
  }
}



#define MAXSTR 500


void set_functkeycode( char *name, char *value, int *keycode )
{
 int j=2;
 char word[MAXSTR];
 Xml_Next_Word( value, word, MAXSTR, " \t," );
 while (word[0] != '\0')
  {
   if (j == 7) { printf("Error: Keycode too long for %s = '%s'.\n", name, value );  return; }
   if (sscanf(word,"%d", &(keycode[j++])) != 1)
    printf("ERROR: Reading keypad setup for %s as '%s'\n", name, word );
   Xml_Next_Word( value, word, MAXSTR, " \t," );
  }
}


/*
  Read EDT_KEYPAD_SETUP as:
   <key_set function="JumpEOL" key="73" mapto="F1" returns="27 79 80" />
   . . .
   <key_restore key="73" mapto="F7" />
   . . .
   <!--  Comments  -->
*/


void get_keypad_setup()
{
 int i, j, line_num=0, keyval, setnum=0;
 char *stupfile, line[MAXSTR], tag[MAXSTR], content[MAXSTR], name[MAXSTR],
      functname[MAXSTR], mapto[MAXSTR], value[MAXSTR], cmd[MAXSTR];
 FILE *infile;
 char keyname[20][20]={"Gold", "Find", "Delete_Line", "Jump_Page", "Replace",
			"Forward", "Backward", "Cut", "Delete_Word", "Jump_Word",
			"Jump_EOL", "Enter_Ascii", "Jump_BOL", "Mark", "Delete_Character",
			"Up_Arrow", "Down_Arrow", "Right_Arrow", "Left_Arrow", "none" };

 stupfile = (char *)getenv("EDT_KEYPAD_SETUP");
 if (stupfile==0)
  {
   printf("\nWarning:  EDT_KEYPAD_SETUP  not set.  (Type help_config for more info.)\n\n");
   return;
  }
 infile = fopen(stupfile,"r");
 if (infile==0) {printf("ERROR: Could not open KeyPad Setup File EDT_KEYPAD_SETUP='%s'\n",stupfile); return;}

 for (i=0; i!=19; i++) 	/* Pre-Initialize key-pad table. */
  for (j=2; j!=8; j++)
   functkeys_table[i][j] = -1;

 if (strstr(stupfile,".xml") != 0)
  { /*xml_setupfile*/
    xml_parse( infile, tag, content, MAXSTR, &line_num );
    while (!feof(infile))
     { /*file*/
       xml_grab_tag_name( tag, name, MAXSTR );
       if (strcmp(name,"key_set") == 0)
	{ /*key_set*/
	  xml_grab_attrib( tag, name, functname, MAXSTR );
	  if (strcmp(name,"function") != 0)
	   printf("Error: Reading keypad setup, expected 'function' in key_set tag, but found '%s',\n",name);

	  xml_grab_attrib( tag, name, value, MAXSTR );
	  if (strcmp(name,"key") != 0)
	   printf("Error: Reading keypad setup, expected 'key' in key_set tag, but found '%s',\n",name);
	  if (sscanf(value,"%d", &keyval) != 1) printf("Error: Reading keypad setup, key value %s not integer.\n", value);

	  xml_grab_attrib( tag, name, mapto, MAXSTR );
	  if (strcmp(name,"mapto") != 0)
	   printf("Error: Reading keypad setup, expected 'mapto' in key_set tag, but found '%s',\n",name);

	  if (mapto[0] != '\0')
	   {
	    sprintf(cmd,"xmodmap -e \"keycode %d = %s\"", keyval, mapto );
	    system( cmd );
	   }

	  xml_grab_attrib( tag, name, value, MAXSTR );
	  if (strcmp(name,"returns") != 0)
	   printf("Error: Reading keypad setup, expected 'returns' in key_set tag, but found '%s',\n",name);
	  j = 0;
	  while ((j < 19) && (strcasecmp(functname, keyname[j]) != 0)) j++;
	  if (j == 19) printf("Error: Reading keypad setup, '%s' unknown.\n", functname);
	  else set_functkeycode( functname, value, functkeys_table[j] );
	  setnum++;
	} /*key_set*/

      xml_parse( infile, tag, content, MAXSTR, &line_num );
    } /*file*/
    if (setnum < 4) printf("Error: Did not read keypad setups.\n");
  } /*xml_setupfile*/
 else
  { /*oldstyle_setupfile*/
    do
     {
      read_line( infile, line, 200 );
      if (strstr(line,"xmodmap")!=0) system(line);
      else
      if (strstr(line,"<key_") != 0) {printf("Error: XML keypad setup file must have .xml suffix. Must rename it."); return;}
     }
    while ((!feof(infile)) && (strstr(line,"Key_Returns:")==0));
    for (i=0; i!=19; i++)
     for (j=0; j!=8; j++)
      if (fscanf(infile,"%d ", &functkeys_table[i][j])!=1)
	printf("ERROR: Reading keypad setup %d %d\n", i, j);
  } /*oldstyle_setupfile*/

 fclose(infile);
}



void restore_keypad_setup()
{
 int i, j, line_num=0, keyval, setnum=0;
 char *stupfile, line[MAXSTR], tag[MAXSTR], content[MAXSTR], name[MAXSTR],
      value[MAXSTR], mapto[MAXSTR], cmd[MAXSTR];
 FILE *infile;

 stupfile = (char *)getenv("EDT_KEYPAD_SETUP");
 if (stupfile==0)
  {
   printf("\nWarning:  EDT_KEYPAD_SETUP  not set.  (Type help_config for more info.)\n\n");
   return;
  }
 infile = fopen(stupfile,"r");
 if (infile==0) {printf("ERROR: Could not open KeyPad Setup File EDT_KEYPAD_SETUP='%s'\n",stupfile); return;}

 if (strstr(stupfile,".xml") != 0)
  { /*xml_setupfile*/
    xml_parse( infile, tag, content, MAXSTR, &line_num );
    while (!feof(infile))
     { /*file*/
       xml_grab_tag_name( tag, name, MAXSTR );
       if (strcmp(name,"key_restore") == 0)
	{ /*key_set*/
	  xml_grab_attrib( tag, name, value, MAXSTR );
	  if (strcmp(name,"key") != 0)
	   printf("Error: Reading keypad setup, expected 'key' in key_set tag, but found '%s',\n",name);
	  if (sscanf(value,"%d", &keyval) != 1) printf("Error: Reading keypad setup, key value %s not integer.\n", value);

	  xml_grab_attrib( tag, name, mapto, MAXSTR );
	  if (strcmp(name,"mapto") != 0)
	   printf("Error: Reading keypad setup, expected 'mapto' in key_set tag, but found '%s',\n",name);
	  sprintf(cmd,"xmodmap xmodmap -e \"keycode %d = %s\"", keyval, mapto );
	  system( cmd );
	  setnum++;
	} /*key_set*/

      xml_parse( infile, tag, content, MAXSTR, &line_num );
    } /*file*/
    if (setnum == 0) printf("Error: Did not read any keypad restores.\n");
  } /*xml_setupfile*/
 else
  { /*oldstyle_setupfile*/
    while ((!feof(infile)) && (strstr(line,"KeyPad_Restore:")==0))   read_line( infile, line, 200 );
    read_line( infile, line, 200 );
    while (!feof(infile))
     {
      if (strstr(line,"xmodmap")!=0) system(line);
      read_line( infile, line, 200 );
     }
  } /*oldstyle_setupfile*/
 fclose(infile);
}



/* Get a new character pointer */
struct __text__ *new_ch()
{
 struct __text__ *tmp_pt;
 int i, frblksz = 256;

 if (txt_free==free_nil)	/* Allocate a new free block. */
  {
   txt_free = (struct __text__ *)calloc( frblksz, sizeof(struct __text__) );
   tmp_pt = txt_free;
   for (i=0; i!=frblksz; i++)
    {
     tmp_pt->prv = tmp_pt - 1;
     tmp_pt->nxt = tmp_pt + 1;
     if (i!=frblksz-1) tmp_pt = tmp_pt->nxt;
    }
   tmp_pt->nxt = free_nil;  txt_free->prv = 0;
  }

 tmp_pt = txt_free;
 txt_free = txt_free->nxt;
 return tmp_pt;
}





void dispose_ch( struct __text__ *tmp_pt )
{
 /* Add char space to free list. */
 tmp_pt->nxt = txt_free;
 txt_free = tmp_pt;
 txt_free->prv = 0;
}




/* Inserts character infront of current pointer */
void insert_char( char ch, struct __text__ *(*tmp_txt) )
{
 struct __text__ *tmp_pt;

  changed++;
  tmp_pt = new_ch();
  tmp_pt->ch = ch;
  if (*tmp_txt==txt_head)
   {
    printf("ERROR: pt was on head!\n"); exit(1);
   }
  else
   {
    (*tmp_txt)->prv->nxt = tmp_pt;
   }
  tmp_pt->prv = (*tmp_txt)->prv;
  tmp_pt->nxt = *tmp_txt;
  (*tmp_txt)->prv = tmp_pt;
}




/* Pushes copy of character onto singly linked list (buffer). */
void push_buffer( char ch, struct __text__ *(*buffer_pt) )
{
 struct __text__ *tmp_pt;
  tmp_pt = new_ch();
  tmp_pt->ch = ch;
  tmp_pt->nxt = *buffer_pt;
  *buffer_pt = tmp_pt;
}



void delete_char( struct __text__ *tmp_txt )
{
 changed++;
 if ((tmp_txt != EOB) && (tmp_txt != txt_head))
 {
  tmp_txt->nxt->prv = tmp_txt->prv;
  tmp_txt->prv->nxt = tmp_txt->nxt;
  dispose_ch( tmp_txt );
 }

}



void load_file()
{
 char ch;
 struct __text__ *tmp_txt;
 int i=0, j=0;
 int pwi, pwl, chp;

 tmp_txt = curse_pt;	/* keep inserting at eob */
 if (encode_mode)
  {
   pwi = 0;  pwl = strlen(psswd);
   while (!feof(infile))
   {
    chp = getc(infile);
    chp = chp - psswd[pwi]; if (chp<0) chp = chp + 255;
    pwi = pwi + 1;  if (pwi==pwl) pwi = 0;
    if (!feof(infile))
     {
      if (chp==10) { last_row=last_row + 1;   j = j + 1; }
      insert_char( chp, &tmp_txt );         i = i + 1;
     }
   }
  }
 else
  {
   while (!feof(infile))
   {
    ch = getc(infile);
    if (!feof(infile))
     {
      if (ch==10) { last_row=last_row + 1;   j = j + 1; }
      insert_char( ch, &tmp_txt );	   i = i + 1;
     }
   }
  }
 if ((EOB->prv!=txt_head) && (EOB->prv->ch!=10)) {printf("MISSING <CR> INSERTED at [EOF]\n"); insert_char( 10, &EOB ); last_row=last_row + 1;}
 printf("	(%d-lines	%d-characters read-in to buffer '%s').\n", j, i, active_buffer_name);
}




/***************************************************************************/
/* Scz_Decompress_File2Buffer - Decompresses input file to output buffer.  */
/*  This routine is handy for applications wishing to read compressed data */
/*  directly from disk and to uncompress the input directly while reading  */
/*  from the disk.             						   */
/*  First argument is input file name.  Second argument is output buffer   */
/*  with return variable for array length. 				   */
/*  This routine allocates the output array and passes back the size.      */
/*                                                                         */
/**************************************************************************/
int load_compressed_file( char *infilename )
{
 int j, k, success, sz1=0, sz2=0, buflen, continuation, totalin=0, totalout=0;
 unsigned char ch, chksum, chksum0;
 struct scz_item *buffer0_hd, *buffer0_tl, *bufpt, *bufprv, *sumlst_hd=0, *sumlst_tl=0;
 FILE *infile=0;
 struct __text__ *tmp_txt;

 tmp_txt = curse_pt;	/* keep inserting at eob */

 infile = fopen(infilename,"rb");
 if (infile==0) {printf("ERROR: Cannot open input file '%s'.  Exiting\n", infilename);  return 0;}

 do
  { /*Segment*/

   /* Read file segment into linked list for expansion. */
   /* Get the 6-byte header to find the number of chars to read in this segment. */
   /*  (magic number (101), magic number (98), iter-count, seg-size (MBS), seg-size, seg-size (LSB). */
   buffer0_hd = 0;  buffer0_tl = 0;

   ch = getc(infile);   sz1++;		/* Byte 1, expect magic numeral 101. */
   if ((feof(infile)) || (ch!=101)) {printf("Error1: This does not look like a compressed file.\n"); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 2, expect magic numeral 98. */
   if ((feof(infile)) || (ch!=98)) {printf("Error2: This does not look like a compressed file. (%d)\n", ch); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);  sz1++;		/* Byte 3, iteration-count. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );

   ch = getc(infile);			/* Byte 4, MSB of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = ch << 16;
   ch = getc(infile);			/* Byte 5, middle byte of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = (ch << 8) | buflen;
   ch = getc(infile);			/* Byte 6, LSB of segment buffer length. */
   if (feof(infile)) {printf("Error3: This does not look like a compressed file.\n"); return 0;}
   buflen = ch | buflen;

   k = 0;
   ch = getc(infile);
   while ((!feof(infile)) && (k<buflen))
    {
     scz_add_item( &buffer0_hd, &buffer0_tl, ch );
     sz1++;  k++;
     ch = getc(infile);
    }

   chksum0 = ch;
   ch = getc(infile);
   // printf("End ch = '%c'\n", ch);

   if (k<buflen) {printf("Error: Unexpectedly short file.\n");}
   totalin = totalin + sz1 + 4;		/* (+4, because chksum+3buflen chars not counted above.) */
   /* Decode the 'end-marker'. */
   if (ch==']') continuation = 0;
   else
   if (ch=='[') continuation = 1;
   else {printf("Error4: Reading compressed file. (%d)\n", ch);  return 0; }

   success = Scz_Decompress_Seg( &buffer0_hd );	/* Decompress the buffer !!! */
   if (!success) return 0;

   /* Check checksum and sum length. */
   sz2 = 0;       /* Get buffer size. */
   bufpt = buffer0_hd;
   chksum = 0;
   bufprv = 0;
   while (bufpt!=0)
    {
     chksum += bufpt->ch;
     sz2++;  bufprv = bufpt;
     bufpt = bufpt->nxt;
    }
   if (chksum != chksum0) {printf("Error: Checksum mismatch (%dvs%d)\n", chksum, chksum0);}

   /* Attach to tail of main list. */
   totalout = totalout + sz2;
   if (sumlst_hd==0) sumlst_hd = buffer0_hd;
   else sumlst_tl->nxt = buffer0_hd;
   sumlst_tl = bufprv;

  } /*Segment*/
 while (continuation);

 /* Convert list into buffer. */
 sz2 = 0;  j = 0;
 if (encode_mode)
  {
   int pwi, pwl, chp;
   pwi = 0;  pwl = strlen(psswd);
   while (sumlst_hd!=0)
    {
     bufpt = sumlst_hd;
     sz2;
     chp = bufpt->ch;
     chp = chp - psswd[pwi]; if (chp<0) chp = chp + 255;
     pwi = pwi + 1;  if (pwi==pwl) pwi = 0;
     if (chp==10) { last_row=last_row + 1;  j++; }
     insert_char( chp, &tmp_txt );
     sumlst_hd = bufpt->nxt;
     sczfree(bufpt);
    }
  }
 else
  {
   while (sumlst_hd!=0)
    {
     bufpt = sumlst_hd;
     sz2;
     if (bufpt->ch==10) { last_row=last_row + 1;  j++; }
     insert_char( bufpt->ch, &tmp_txt );
     sumlst_hd = bufpt->nxt;
     sczfree(bufpt);
    }
  }

 fclose(infile);
 printf("Decompression ratio = %g\n", (float)totalout / (float)totalin );

 if ((EOB->prv!=txt_head) && (EOB->prv->ch!=10)) {printf("MISSING <CR> INSERTED at [EOF]\n"); insert_char( 10, &EOB ); last_row=last_row + 1;}
 printf("	(%d-lines	%d-characters read-in to buffer '%s').\n", j, totalout, active_buffer_name);

 return 1;
}




void print_char( char ch )
{
 if (ch == 127) printf("<DEL>");
 else
 if (ch == 9) printf("%c", ch);
 else
 if (ch < 32)
  {
   if (ch == 12) printf("<FF>");
   else
   if (ch < EDT$K_ESC) printf("^%c", ch + 64);
   else
   if (ch == EDT$K_ESC) printf("<ESC>");
   else
    printf("#");
  }
 else
  printf("%c", ch);
}




/* This routine returns the space used on the screen by a character. */
int spaces( char ch, int rel_col )
{
  if (ch == 9) return (rel_col / 8 ) * 8 + 8 - rel_col;
  else
  if (ch == 127) return 5;
  else
  if (ch == 12) return 4;
  else
  if (ch == EDT$K_ESC) return 5;
  else
  if (ch < EDT$K_ESC) return 2;
  else return 1;
}




void resize(int verb)
{
 char cstrng[256];
 FILE *infile1;
 int i;
 // tmpnam(fn);
 // sprintf(cstrng,"stty size > %s", fn);
 // system(cstrng);
 // iz1 = fopen(fn,"r");
 infile1 = popen("stty size","r");
 if (infile1==0) printf("Error: Getting size.\n");
 else
  {
   if (fscanf(infile1,"%d",&i)!=1) printf("Error: Getting row size.\n"); else nrows = i;
   if (fscanf(infile1,"%d",&i)!=1) printf("Error: Getting col size.\n"); else ncols = i-1;
   if (verb) printf("Window_Size set to %d-rows, %d-cols\n", nrows, ncols );
   // fclose(infile1);
   // remove(fn);
   pclose(infile1);
  }
}




void display_screen( int position_cursor )
/* Input: new row position */
{
 int row, col, rel_col, bottom;
 struct __text__ *tmp_pt;

 printf("%c[2J%c[H", EDT$K_ESC, EDT$K_ESC );
 row = 0;  tmp_pt = txt_head->nxt;
 while (row<tframe_row)
  {
   if (tmp_pt->ch == 10) row = row + 1;
   tmp_pt = tmp_pt->nxt;
  }

 bottom = tframe_row + nrows - 3;
 col = 0;  rel_col = 0;
 while ((row<=bottom) && (tmp_pt!=EOB))
  {
   col = col + 1;
   rel_col = rel_col + spaces(tmp_pt->ch, rel_col);
   if (tmp_pt->ch != 10)
    {
     if (rel_col < ncols) print_char( tmp_pt->ch );
    }
   else
    { row = row + 1; col = 0;  rel_col = 0;
      if (row<=bottom) printf("%c%c", 10,13);
    }
   tmp_pt = tmp_pt->nxt;
  }

 if (tmp_pt==EOB)
  {
   if ((row<=bottom) && (col!=0)) printf("%c%c", 10, 13 );
   printf("[EOB %s]", active_buffer_name);
  }

 if (rel_curse_col+1>ncols) rel_curse_col = ncols;

 if (position_cursor)
  printf("%c[%d;%dH", EDT$K_ESC, rel_curse_row+1, rel_curse_col+1 );

}



/* Gets curser pointing to proper position on screen */
void reposition_cursor( )
{
 int rel_col;
 rel_curse_row = curse_row - tframe_row;
 if (rel_curse_col+1>ncols) rel_col = ncols; else rel_col = rel_curse_col+1;
 printf("%c[%d;%dH", EDT$K_ESC, rel_curse_row+1, rel_col );
}



/* Gets curser pointing to given position in file */
void position_curser( int nrow, int ncol )
{
 int row, col;
 struct __text__ *tmp_pt;

 row = 0;  tmp_pt = txt_head->nxt;
 while ((row<nrow) && (tmp_pt->nxt!=EOB))
  {
   if (tmp_pt->ch == 10) row = row + 1;
   tmp_pt = tmp_pt->nxt;
  }
 col = 0;
 while ((col<ncol) && (tmp_pt->nxt!=EOB) && (tmp_pt->ch!=10))
  {
   col = col + 1;
   tmp_pt = tmp_pt->nxt;
  }
 curse_pt = tmp_pt;
 curse_row = row;
}





/* Leaves cursor pointing to end of previous line. */
void move_pt_up_line( struct __text__ *(*tmp_pt) )
{
 do { if ((*tmp_pt)!=txt_head->nxt) *tmp_pt = (*tmp_pt)->prv; }
 while (((*tmp_pt)->ch != 10) && ((*tmp_pt)!=txt_head->nxt));
}

/* Leaves cursor pointing to first char in current line. */
void move_pt_begin_of_line( struct __text__ *(*tmp_pt) )
{
  if ((*tmp_pt)==txt_head) {printf("ERROR1: ON TXT_HEAD\n"); exit(1);}
  *tmp_pt = (*tmp_pt)->prv;
  while (((*tmp_pt)->ch != 10) && ((*tmp_pt)!=txt_head))
   { *tmp_pt = (*tmp_pt)->prv; }
  *tmp_pt = (*tmp_pt)->nxt;
}



/* Leaves cursor pointing to beginning of next line. */
void move_pt_down_line( struct __text__ *(*tmp_pt) )
{
 while (((*tmp_pt)->ch != 10) && ((*tmp_pt)!=EOB))
  { *tmp_pt = (*tmp_pt)->nxt; }
 if ((*tmp_pt)!=EOB) *tmp_pt = (*tmp_pt)->nxt;
}

/* Leaves cursor pointing to end of current line. */
void move_pt_end_of_line( tmp_pt )
struct __text__ *(*tmp_pt);
{
 while (((*tmp_pt)->ch != 10) && (*tmp_pt!=EOB))
  { *tmp_pt = (*tmp_pt)->nxt; }
}


/* Moves curser right x-columns */
void move_right( struct __text__ *(*tmp_pt), int x )
{
 int i;
 i = 0;
 while ((i != x) && ((*tmp_pt)->nxt!=EOB))
  { i = i + 1; *tmp_pt = (*tmp_pt)->nxt; }
}





void spew_line( struct __text__ *tmp_pt )
{
 struct __text__ *tmp_pt2;
 int rel_col;

 tmp_pt2 = tmp_pt;
 move_pt_begin_of_line( &tmp_pt2 );
 rel_col = 0;
 while (tmp_pt2!=tmp_pt) { rel_col = spaces( tmp_pt2->ch, rel_col ) + rel_col; tmp_pt2 = tmp_pt2->nxt; }

 if (tmp_pt==EOB) printf("[EOB %s]", active_buffer_name);
 else
 {
  if ((tmp_pt->ch!=10) && (rel_col<ncols)) print_char(tmp_pt->ch);
  while ((tmp_pt->nxt!=EOB) && (tmp_pt->ch!=10) && (rel_col<ncols))
  {
   rel_col = spaces( tmp_pt->ch, rel_col ) + rel_col;
   tmp_pt = tmp_pt->nxt;
   if ((tmp_pt->ch!=10) && (rel_col<ncols)) print_char(tmp_pt->ch);
  }
 }
}



void compute_curse_col( struct __text__ *cursor_ptr )
{
 struct __text__ *tmp_pt;
 int i, rel_col;

 tmp_pt = cursor_ptr;
 move_pt_begin_of_line( &tmp_pt );
 i = 0; rel_col = 0;
 while (tmp_pt!=cursor_ptr)
 {
  if (tmp_pt->ch == 9) rel_col = (rel_col / 8 ) * 8 + 8;
  else
  if (tmp_pt->ch == 127) rel_col = rel_col + 5;
  else
  if (tmp_pt->ch == 12) rel_col = rel_col + 4;
  else
  if (tmp_pt->ch == EDT$K_ESC) rel_col = rel_col + 5;
  else
  if (tmp_pt->ch < EDT$K_ESC) rel_col = rel_col + 2;
  else rel_col = rel_col + 1;
  tmp_pt = tmp_pt->nxt;
  i = i + 1;
 }
 rel_curse_col = rel_col;
}


void  adjust_screen_parameters()
{
   compute_curse_col(curse_pt); last_curse_col =  rel_curse_col;
   if (curse_row < tframe_row + 6)    /* If cursor is above work area */
    { /*above*/
     tframe_row = curse_row - 6;
     if (tframe_row<0) tframe_row = 0;    /* If new top row is above TOB, set */
    } /*above*/
   else
    if (curse_row > tframe_row + nrows - 10)
     { /*below*/                          /* If cursor is below work area */
      tframe_row = curse_row - nrows + 10;
				  /* If new bottom row is below EOB, set to EOB */
      if (tframe_row + nrows - 3 > last_row) tframe_row = last_row - nrows + 3;
      if (tframe_row<0) tframe_row = 0;
     } /*below*/
   rel_curse_row = curse_row - tframe_row;
}

/**/

/* Use this when a line deletion has caused screen to scroll up */
/* (Call this after deleting the line on the screen, but before */
/*  actually deleting any characters from the file, or changing */
/*  any of the row or column, or last_row pointers.)		*/
void REPLACE_BOTTOM_LINE( int fswitch )
{
 int i, bottom_row;
 struct __text__ *tmp_pt1;

 /*get pointer to 1st char in bottom line of current screen*/
 bottom_row = tframe_row + nrows - 3;
 if (bottom_row<last_row)
 {
  tmp_pt1 = curse_pt;
  if (curse_row < bottom_row)
   for (i=curse_row; i!=bottom_row; i++) move_pt_down_line( &tmp_pt1 );
  else
   for (i=curse_row; i!=bottom_row; i--) move_pt_up_line( &tmp_pt1 );
  if (bottom_row>last_row) bottom_row = last_row;
  printf("%c[%d;1H", EDT$K_ESC, nrows-2 );
  if (fswitch)  move_pt_down_line( &tmp_pt1 );
  spew_line( tmp_pt1 );
 }
}



void scroll_window_up( int old_tframe_row )
{
 int i;
 struct __text__ *tmp_pt1;

  /*get pointer to 1st char in top line of current screen*/
  tmp_pt1 = curse_pt;
  if (curse_row<old_tframe_row)
   for (i=curse_row+1; i!=old_tframe_row; i++) move_pt_down_line( &tmp_pt1 );
  else
   for (i=curse_row+1; i!=old_tframe_row; i--) move_pt_up_line( &tmp_pt1 );
  for (i=old_tframe_row; i>tframe_row; i--)    /* scroll up by        */
   {						/*  inserting lines at top */
    printf("%c[H%cM", EDT$K_ESC, EDT$K_ESC);
    move_pt_begin_of_line( &tmp_pt1 );
    spew_line( tmp_pt1 );
    move_pt_up_line( &tmp_pt1 );
   }
}


void scroll_window_down( int old_tframe_row )
{
 int i, bottom_row, new_bottom_row;
 struct __text__ *tmp_pt1;

 /*get pointer to 1st char in bottom line of current screen*/
 bottom_row = old_tframe_row + nrows - 3;
 if (bottom_row>last_row) bottom_row = last_row;
 tmp_pt1 = curse_pt;
 if (curse_row < bottom_row)
  for (i=curse_row; i!=bottom_row; i++) move_pt_down_line( &tmp_pt1 );
 else
  for (i=curse_row; i!=bottom_row; i--) move_pt_up_line( &tmp_pt1 );
 new_bottom_row = tframe_row + nrows - 3;
 if (new_bottom_row>last_row) new_bottom_row = last_row;
 for (i=bottom_row; i!=new_bottom_row; i++)     /*scroll up by     */
  {				 	     /* inserting lines at bottom */
   printf("%c[%d;1H%cE", EDT$K_ESC, nrows-2, EDT$K_ESC);
   move_pt_down_line( &tmp_pt1 );
   spew_line( tmp_pt1 );
  }
}



void ADJUST_DISPLAY()
/* Input:  curse_row, (the prevaling tframe_row), and last_row */
{
 int old_tframe_row;

 old_tframe_row = tframe_row;

 if (curse_row < old_tframe_row + 6)	/* If cursor is above work area */
  { /*above*/
   tframe_row = curse_row - 6;
   if (tframe_row<0) tframe_row = 0;	/* If new top row is above TOB, */
					/*			set to TOB */
   if (tframe_row != old_tframe_row)
   { /*ok*/
    if (old_tframe_row - tframe_row < nrows - 6)
     { /*1*/	/* Scroll-up until new tframe */
      scroll_window_up( old_tframe_row );
     } /*1*/
    else
     /*redraw_screen_at_new_location*/
     display_screen(0);
   } /*ok*/

  } /*above*/
 else
  if (curse_row > old_tframe_row + nrows - 10)
   { /*below*/				/* If cursor is below work area */
    tframe_row = curse_row - nrows + 10;
				/* If new bottom row is below EOB, set to EOB */
    if (tframe_row + nrows - 3 > last_row) tframe_row = last_row - nrows + 3;
    if (tframe_row<0) tframe_row = 0;

    if (tframe_row != old_tframe_row)
    {
     if (abs(tframe_row - old_tframe_row) < nrows - 5)
      { /*2*/	/* Scroll-down until new bottom row */
       if (tframe_row > old_tframe_row)
	{
	 /* if (curse_pt!=EOB) if (curse_pt->nxt!=EOB) */
	  scroll_window_down( old_tframe_row );
	}
       else
	scroll_window_up( old_tframe_row );
      } /*2*/
     else
      /*redraw_screen_at_new_location*/
      display_screen(0);
    }

   } /*below*/

 reposition_cursor();
} /*ADJUST_DISPLAY*/


/**/

/*Device Dependent Screen Operations*/
/*for vt100*/
void INSERT_CHAR_INLINE( char ch )
{
 if (ch==10)			/* If inserting a new line: */
  {
    curse_row = curse_row + 1;
    last_row = last_row + 1;
    if (curse_row-tframe_row<nrows-2)  /* If not at very bottom of screen */
     printf("%c[K%c%c",EDT$K_ESC,10,13); /* Erase to EOLN and move cursor */
					/*  to 1st position on next line */
    if (curse_row-tframe_row<nrows-3)  /* If not at bottom of screen */
    {
     printf("%c[L",EDT$K_ESC);			 /* Insert New Line */
    }
    else
     printf("%c[K", EDT$K_ESC);

    if (curse_row-tframe_row<nrows-2)  /* If not at very bottom of screen */
     spew_line( curse_pt );	/* Spew line from the new char */
  }
 else
  {
   printf("%c[K", EDT$K_ESC);	/* Clr to EOLN */
   if (curse_pt==EOB)
   {
    insert_char( 10, &curse_pt );
    curse_pt = curse_pt->prv;
    if (curse_row-tframe_row<nrows-3)  /* If not at bottom of screen */
     {
      printf("%c[K\n\r[EOB %s]%c[A%c[%dD",EDT$K_ESC, active_buffer_name, EDT$K_ESC, EDT$K_ESC, 6 + strlen(active_buffer_name) );
     }
    else printf("%c[K", EDT$K_ESC);
    last_row = last_row + 1;
    ch = 10;
   }
   spew_line( curse_pt->prv );	/* Spew line from the new char */
  }
 compute_curse_col(curse_pt);
 reposition_cursor();
 last_curse_col = rel_curse_col;
 if (ch==10) ADJUST_DISPLAY();
}

/*END Device Dependent Screen Operations*/
/*for vt100*/


/**/


void DELETE_CHAR_BACKWARD()
{
 struct __text__ *tmp_pt;
 int do_delete;
 char tmp_ch;

 if (curse_pt != txt_head->nxt)
  {
   if ((curse_pt==EOB) && (curse_pt->prv!=txt_head->nxt))
    if ((curse_pt->prv->prv->ch!=10) && (curse_pt->prv!=txt_head->nxt))
    {
     curse_pt = curse_pt->prv;
     curse_row = curse_row - 1;
     compute_curse_col(curse_pt);
     reposition_cursor();
     last_curse_col = rel_curse_col;
     do_delete = 0;
    }
    else
     do_delete = 1;
   else
    do_delete = 1;

   if (do_delete)
   {
    tmp_pt = curse_pt->prv;
    tmp_ch = tmp_pt->ch;
    ch_buf = tmp_ch;
    if (tmp_ch == 10)	/* If deleting a <CR>: */
    {
     printf("%c[M", EDT$K_ESC);
     REPLACE_BOTTOM_LINE(1);
     curse_row = curse_row - 1;  rel_curse_row = rel_curse_row - 1;
     last_row = last_row - 1;
    }
    delete_char( tmp_pt );
    compute_curse_col(curse_pt);
    reposition_cursor();
    printf("%c[K", EDT$K_ESC);		/* clear to EOLN */
    spew_line( curse_pt );
    reposition_cursor();
    last_curse_col = rel_curse_col;
    if (tmp_ch == 10)	/* If deleting a <CR>: */
     ADJUST_DISPLAY();
   }
  }
 else
  {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mBackup past top of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    message_pending = 1;
    reposition_cursor();
  }
}




void DELETE_CHAR_FORWARD()
{
 struct __text__ *tmp_pt;
 char tmp_ch;

 if (curse_pt != EOB)
  {
    tmp_pt = curse_pt;
    curse_pt = curse_pt->nxt;
    tmp_ch = tmp_pt->ch;
    ch_buf = tmp_ch;
    if ((tmp_ch==10) && (curse_pt==EOB) && (tmp_pt->prv!=txt_head)
	&& (tmp_pt->prv->ch!=10))
    {
     curse_row = curse_row + 1;
    }
    else
    {
     if (tmp_ch == 10)	/* If deleting a <CR>: */
     {
      printf("\n\r%c[M", EDT$K_ESC);
      REPLACE_BOTTOM_LINE(0);
      last_row = last_row - 1;
     }
     delete_char( tmp_pt );
    }
    compute_curse_col(curse_pt);
    reposition_cursor();
    printf("%c[K", EDT$K_ESC);		/* clear to EOLN */
    spew_line( curse_pt );
    reposition_cursor();
    last_curse_col = rel_curse_col;
    if (tmp_ch == 10)	/* If deleting a <CR>: */
     ADJUST_DISPLAY();
  }
 else
  {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mAdvance past top of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    message_pending = 1;
    reposition_cursor();
  }
}




void DELETE_WORD_FORWARD()
{
 struct __text__ *tmp_pt;
 char tmp_ch;

if (!Gold)
{ /*!Gold*/
 if (curse_pt != EOB)
  {

   /* Free anything on word_buf. */
   while (word_buf!=0) { txt_tmp = word_buf; word_buf = word_buf->nxt; dispose_ch( txt_tmp ); }

   if ((curse_pt->ch==10) || (curse_pt->ch=='	'))
    {
     push_buffer( curse_pt->ch, &word_buf );
     DELETE_CHAR_FORWARD();
    }
   else
    { /*Not_starting_at_CR_or_TAB*/
     /* Find EOW */
     tmp_pt = curse_pt;
     /* while not white_space, advance */
     while ((tmp_pt!=EOB) && (tmp_pt->ch!=10) && (tmp_pt->ch!=' ')
			&& (tmp_pt->ch!='	'))   tmp_pt = tmp_pt->nxt;
     while ((tmp_pt!=EOB) && (tmp_pt->ch==' '))  tmp_pt = tmp_pt->nxt;
     tmp_pt = tmp_pt->prv;

     /* push and delete from EOW upto and including curse_pt. */
     while (tmp_pt!=curse_pt)
      {
       push_buffer( tmp_pt->ch, &word_buf );
       tmp_pt = tmp_pt->prv;
       delete_char( tmp_pt->nxt );
      }
     push_buffer( curse_pt->ch, &word_buf );
     curse_pt = curse_pt->prv;
     delete_char( curse_pt->nxt );
     curse_pt = curse_pt->nxt;

     compute_curse_col(curse_pt);
     reposition_cursor();
     printf("%c[K", EDT$K_ESC);         /* clear to EOLN */
     spew_line( curse_pt );
     reposition_cursor();
     last_curse_col = rel_curse_col;
    } /*Not_starting_at_CR_or_TAB*/

  }
 else
  {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mAdvance past top of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    message_pending = 1;
    reposition_cursor();
  }
 } /*!Gold*/
else
 { /*Gold*/	/* Undelete word */

  tmp_pt = curse_pt->prv;

  if (word_buf!=0)
  { /*word_buf_not_empty*/

   txt_tmp = word_buf;
   while (txt_tmp!=0)
    {
     insert_char( txt_tmp->ch, &curse_pt );
     txt_tmp = txt_tmp->nxt;
    }

  if (curse_pt->prv->ch == 10)
  {
    last_row = last_row + 1;
    if (curse_row-tframe_row<nrows-2)	/* If not at very bottom of screen */
     printf("%c[K\n\r",EDT$K_ESC); 	/* Erase to EOLN and move cursor */
					/*  to 1st position on next line */
    if (curse_row-tframe_row < nrows-3)	/* If not at bottom of screen */
    {
     printf("%c[L",EDT$K_ESC);                  /* Insert New Line */
    }
    else
     printf("%c[K", EDT$K_ESC);

    if (curse_row-tframe_row<nrows-2)  /* If not at very bottom of screen */
     spew_line( curse_pt );     /* Spew line from the new char */
    if (curse_row-tframe_row < nrows-2) printf("%c[A",EDT$K_ESC);
   tmp_ch = '\n';
  }
  else
  if (curse_pt==EOB)
   {
    insert_char( 10, &curse_pt );
    curse_pt = curse_pt->prv;
    if (curse_row-tframe_row<nrows-3)  /* If not at bottom of screen */
    {
     printf("%c[K\n\r[EOB %s]%c[A%c[%dD",EDT$K_ESC, active_buffer_name, EDT$K_ESC, EDT$K_ESC, 6 + strlen(active_buffer_name) );
    }
    else printf("%c[K", EDT$K_ESC);
    last_row = last_row + 1;
    tmp_ch = 10;
   }

   curse_pt = tmp_pt->nxt;
   compute_curse_col(curse_pt);
   reposition_cursor();
   last_curse_col = rel_curse_col;

   printf("%c[K", EDT$K_ESC);  /* Clr to EOLN */
   spew_line( curse_pt );  /* Spew line from the new char */
   reposition_cursor();
   ADJUST_DISPLAY();

  } /*word_buf_not_empty*/

  Gold = 0;
 } /*Gold*/
}



void DELETE_LINE_FORWARD()
{
 struct __text__ *tmp_pt;

if (!Gold)
{ /*!Gold*/
 if (curse_pt != EOB)
  {

   /* Free anything on line_buf. */
   while (line_buf!=0) { txt_tmp = line_buf; line_buf = line_buf->nxt; dispose_ch( txt_tmp ); }

   /* Find EOL */
   tmp_pt = curse_pt;
   while ((tmp_pt!=EOB) && (tmp_pt->ch!=10)) tmp_pt = tmp_pt->nxt;

   /* push and delete from EOL upto and including curse_pt. */
   while (tmp_pt!=curse_pt)
    {
     push_buffer( tmp_pt->ch, &line_buf );
     tmp_pt = tmp_pt->prv;
     delete_char( tmp_pt->nxt );
    }
   push_buffer( curse_pt->ch, &line_buf );
   curse_pt = curse_pt->prv;
   delete_char( curse_pt->nxt );
   curse_pt = curse_pt->nxt;

   printf("\n\r%c[M", EDT$K_ESC);
   REPLACE_BOTTOM_LINE(0);
   last_row = last_row - 1;

   compute_curse_col(curse_pt);
   reposition_cursor();
   printf("%c[K", EDT$K_ESC);         /* clear to EOLN */
   spew_line( curse_pt );
   reposition_cursor();
   last_curse_col = rel_curse_col;
  }
 else
  {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mAdvance past top of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    message_pending = 1;
    reposition_cursor();
  }
 } /*!Gold*/
else
 { /*Gold*/     /* Undelete line */
  tmp_pt = curse_pt->prv;

  if (line_buf!=0)
  { /*line_buf_not_empty*/

   txt_tmp = line_buf;
   while (txt_tmp!=0)
    {
     insert_char( txt_tmp->ch, &curse_pt );
     txt_tmp = txt_tmp->nxt;
    }

   last_row = last_row + 1;

   if (curse_row-tframe_row < nrows-2)	/* If not at very bottom of screen */
    {
     printf("%c[K\n\r",EDT$K_ESC);	/* Erase to EOLN and move cursor */
					/*  to 1st position on next line */
     if (curse_row-tframe_row < nrows-3)  /* If not at bottom of screen */
       printf("%c[L",EDT$K_ESC);        /* Insert New Line */
     else
       printf("%c[K", EDT$K_ESC);

     spew_line( curse_pt );     /* Spew line from the new char */
    }

   if (curse_pt==EOB)
    {
     curse_pt = curse_pt->prv;
    }

   curse_pt = tmp_pt->nxt;
   compute_curse_col(curse_pt);
   reposition_cursor();
   last_curse_col = rel_curse_col;

   printf("%c[K", EDT$K_ESC);  /* Clr to EOLN */
   spew_line( curse_pt );  /* Spew line from the new char */
   reposition_cursor();
   ADJUST_DISPLAY();

  } /*line_buf_not_empty*/
  Gold = 0;
 } /*Gold*/
}





void left_arrow()
{
  if (curse_pt!=txt_head->nxt)
   {
    curse_pt = curse_pt->prv;
    if (curse_pt->ch==10) curse_row = curse_row - 1;
    compute_curse_col(curse_pt);
    last_curse_col = rel_curse_col;
    reposition_cursor();
    if (curse_pt->ch==10) ADJUST_DISPLAY();
   }
  else
   {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mBackup past top of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    message_pending = 1;
    reposition_cursor();
   }
}


void up_arrow()
{
  curse_row = curse_row - 1;
  if (curse_row < 0)
   {
    curse_row = 0;
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mBackup past top of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    reposition_cursor();
    message_pending = 1;
   }
  else
   {
    /*Get curser to end of previous line, Then to begining of that line, */
    /* then try to move to last_curse_col */
    move_pt_up_line( &curse_pt );
    move_pt_begin_of_line( &curse_pt );
    rel_curse_col = 0;
    while ((curse_pt->nxt!=EOB) && (curse_pt->ch!=10) &&
					(rel_curse_col<last_curse_col))
     {
      if (curse_pt->ch != 9)
       rel_curse_col = rel_curse_col + 1;
      else
       rel_curse_col = (rel_curse_col/8)*8 + 8;
      curse_pt = curse_pt->nxt;
     }
    if (rel_curse_col>last_curse_col)
     if (curse_pt!=txt_head->nxt)
      {
	curse_pt = curse_pt->prv;
	compute_curse_col(curse_pt);
      }
    reposition_cursor();
    ADJUST_DISPLAY();
   }
}



void right_arrow()
{

  if (curse_pt!=EOB)
   {
    if (curse_pt->ch==10) curse_row = curse_row + 1;
    curse_pt = curse_pt->nxt;
    compute_curse_col(curse_pt);
    last_curse_col = rel_curse_col;
    reposition_cursor();
    if (curse_pt->prv->ch==10) ADJUST_DISPLAY();
   }
  else
   {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mAdvance past bottom of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    reposition_cursor();
    message_pending = 1;
   }
}


void down_arrow()
{
  /*Get curser to begining of next line, Then try to move to last_curse_col */
  if (curse_pt==EOB)
   {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mAdvance past bottom of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    reposition_cursor();
    message_pending = 1;
   }
  else
  {
   curse_row = curse_row + 1;
   move_pt_end_of_line( &curse_pt );
   curse_pt = curse_pt->nxt;
    rel_curse_col = 0;
    while ((curse_pt!=EOB) && (curse_pt->ch!=10) &&
					(rel_curse_col<last_curse_col))
     {
      if (curse_pt->ch != 9)
       rel_curse_col = rel_curse_col + 1;
      else
       rel_curse_col = (rel_curse_col/8)*8 + 8;
      curse_pt = curse_pt->nxt;
     }
    if (rel_curse_col>last_curse_col)
     if (curse_pt!=txt_head->nxt)
      {
	curse_pt = curse_pt->prv;
	compute_curse_col(curse_pt);
      }
    reposition_cursor();
    ADJUST_DISPLAY();
  }
}





char cap_ch( char ch )	/* Like toupper. */
{
 if ((ch>96) && (ch<123)) ch = ch - 32;
 return ch;
}





/* This routine determines if curse_pt is aligned at the start of a match to the search-string in the text buffer. */
/* Returns '1' if so, else returns '0'. */
int srch_match()
{
 int i;
 char ch, s_strng[512];
 struct __text__ *tmp_pt1;

  if (srch_caps)        /* Capitolize the search string */
  {
   i=0;
   while (srch_strng[i] != EDT$K_ESC)
    {s_strng[i] = cap_ch( srch_strng[i] ); i = i + 1;}
   s_strng[i] = EDT$K_ESC;
  }

  tmp_pt1 = curse_pt;

  i = 0;
  if (srch_caps)
  {
   while ((tmp_pt1 != EOB) && (s_strng[i] == cap_ch(tmp_pt1->ch)) && (srch_strng[i] != EDT$K_ESC))
    { i = i + 1;  tmp_pt1 = tmp_pt1->nxt; }
  }
  else
  {
   while ((tmp_pt1 != EOB) && (srch_strng[i] == tmp_pt1->ch) && (srch_strng[i] != EDT$K_ESC))
    { i = i + 1;  tmp_pt1 = tmp_pt1->nxt; }
  }
  if ((srch_strng[i] == EDT$K_ESC) && (tmp_pt1 != curse_pt)) return 1; else return 0;
}




void CAPITOLIZE_CHAR()
{
 char ch;
 int ch_index, still_online;
 struct __text__ *tmp_pt1;

 if ((Mark) && (mark_pt1!=curse_pt))
 {
   /* First decide direction to change-caps in. */
   if ((mark_row<curse_row) || ((curse_row==mark_row) && (mark_col<last_curse_col)))
     { /*change_backward*/  /* Change back-up to mark, from left of cursor. */
       tmp_pt1 = curse_pt->prv;  /* Cursor stays where it is. */
       still_online = 1;
       while (tmp_pt1!=mark_pt1->prv)
	{
	 if ((tmp_pt1->ch>64) && (tmp_pt1->ch<91)) tmp_pt1->ch = tmp_pt1->ch + 32;
	 else
	 if ((tmp_pt1->ch>96) && (tmp_pt1->ch<123)) tmp_pt1->ch = tmp_pt1->ch - 32;
	 if (tmp_pt1->ch==10) still_online = 0;
	 tmp_pt1 = tmp_pt1->prv;
	 if (tmp_pt1==txt_head) {printf("SEVERE_ERROR: BOB\n"); /* tmp_pt1=mark_pt1->prv; */}
	}
       display_screen(1);
     } /*change_backward*/
    else
     { /*change_forward*/   /* Change back-up to cursor, from mark. */
       tmp_pt1 = mark_pt1->prv;
       while (tmp_pt1!=curse_pt->prv)
	{
	 if ((tmp_pt1->ch>64) && (tmp_pt1->ch<91)) tmp_pt1->ch = tmp_pt1->ch + 32;
	 else
	 if ((tmp_pt1->ch>96) && (tmp_pt1->ch<123)) tmp_pt1->ch = tmp_pt1->ch - 32;
	 if (tmp_pt1->ch==10) still_online = 0;
	 tmp_pt1 = tmp_pt1->prv;
	 if (tmp_pt1==txt_head) {printf("SEVERE_ERROR: BOB\n"); /* tmp_pt1=mark_pt1; */}
	}
       display_screen(1);
     } /*change_forward*/
    // Mark = 0;  	/* Safe to keep marked. */
 }
 else
 /* Check if you are positioned at front of match to search_string. */
 if ((srch_match()) && (!((srch_strng[0]==' ') && (srch_strng[1]==EDT$K_ESC))))
 {
   ch_index = 0;  still_online = 1;
   tmp_pt1 = curse_pt;
   while (srch_strng[ch_index]!=EDT$K_ESC)
    {
     if ((tmp_pt1->ch>64) && (tmp_pt1->ch<91)) tmp_pt1->ch = tmp_pt1->ch + 32;
     else
     if ((tmp_pt1->ch>96) && (tmp_pt1->ch<123)) tmp_pt1->ch = tmp_pt1->ch - 32;
     if (tmp_pt1->ch==10) still_online = 0;
     tmp_pt1 = tmp_pt1->nxt;
     ch_index = ch_index + 1;
    }
   if (still_online)
    {
     printf("%c[K", EDT$K_ESC);  /* Clr to EOLN */
     spew_line( curse_pt );  /* Spew line from the new char */
     reposition_cursor();
    }
   else
    display_screen(1);
 }
 else
 if (direction==1)
 { /*dir=1*/
  if (curse_pt==EOB) ch = 0;
  else ch = curse_pt->ch;
  if ((ch>64) && (ch<91)) ch = ch + 32;
  else
  if ((ch>96) && (ch<123)) ch = ch - 32;
  else ch = 0;
  if (ch!=0)
  {
   curse_pt->ch = ch;
   printf("%c",ch);
   curse_pt = curse_pt->nxt;
   compute_curse_col(curse_pt);
   last_curse_col = rel_curse_col;
   reposition_cursor();
   changed++;
  }
  else right_arrow();
 } /*dir=1*/
 else
 { /*dir=0*/
  if (curse_pt==txt_head->nxt) ch = 0;
  else ch = curse_pt->prv->ch;
  if ((ch>64) && (ch<91)) ch = ch + 32;
  else
  if ((ch>96) && (ch<123)) ch = ch - 32;
  else ch = 0;
  if (ch!=0)
  {
   curse_pt->prv->ch = ch;
   curse_pt = curse_pt->prv;
   compute_curse_col(curse_pt);
   reposition_cursor();
   printf("%c",ch);
   last_curse_col = rel_curse_col;
   reposition_cursor();
   changed++;
  }
  else left_arrow();
 } /*dir=0*/
 Gold = 0;
}




void jump_by_word()
{
 int ln_flag = 0;

 if (direction==1)
 { /*forward*/
  if (curse_pt==EOB)
   {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mAdvance past bottom of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    reposition_cursor();
    message_pending = 1;
   }
  else
  { /*ok*/
   if (curse_pt->ch=='\n')  /* If at end of line */
   {
    curse_pt = curse_pt->nxt;
    ln_flag = 1; curse_row = curse_row + 1;
   }
   else
   if (curse_pt->ch=='	') 	/* if at TAB */
     {curse_pt = curse_pt->nxt;}
   else
   { /*not_at_end_of_line*/
    /* while not white_space, advance */
    while ((curse_pt!=EOB) && (curse_pt->ch!=10) && (curse_pt->ch!=' ')
			&& (curse_pt->ch!='	'))
     { curse_pt = curse_pt->nxt; }
    /* while white space, advance */
    while ((curse_pt!=EOB) && ((curse_pt->ch==' ')
			/* || (curse_pt->ch=='	') */ ) )
     { curse_pt = curse_pt->nxt; }
   } /*not_at_end_of_line*/
  } /*ok*/
 } /*forward*/
 else
 { /*backward*/
  if (curse_pt==txt_head->nxt)
   {
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c%c[%d;1H%c[7mBackup past top of buffer%c[m%c[1;1H",
	    EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
    message_pending = 1;
    reposition_cursor();
   }
  else
  { /*ok*/
   if (curse_pt->prv->ch=='\n')  /* If at front of line */
    {curse_pt = curse_pt->prv; ln_flag = 1; curse_row = curse_row - 1;}
   else
   if (curse_pt->prv->ch=='	')      /* if at TAB */
     {curse_pt = curse_pt->prv;}
   else
   { /*not_at_front_of_line*/
    curse_pt = curse_pt->prv;
    /* while white space, advance */
    while ((curse_pt!=txt_head) &&
	((curse_pt->ch==' ') /* || (curse_pt->ch=='	') */ ) )
     { curse_pt = curse_pt->prv; }
    /* while not white_space, advance */
    while ((curse_pt!=txt_head)  && (curse_pt->ch!=10)
	 && (curse_pt->ch!=' ') && (curse_pt->ch!='	'))
    { curse_pt = curse_pt->prv; }
    curse_pt = curse_pt->nxt;

   } /*not_at_front_of_line*/
  } /*ok*/
 } /*backward*/

  compute_curse_col(curse_pt);
  last_curse_col = rel_curse_col;
  reposition_cursor();
  if (ln_flag) ADJUST_DISPLAY();
}




void screen_jump()
{
 int i, old_curse_row;
 old_curse_row = curse_row;
 rel_curse_col = 0; last_curse_col = 0;
 curse_row = curse_row + 16 * direction;

 if (curse_row<0)
  {
   curse_row = 0;
   printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
   printf("%c%c[%d;1H%c[7mBackup past top of buffer%c[m%c[1;1H",
	   EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
   printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   message_pending = 1;
  }
 if (curse_row>last_row)
  {
   curse_row = last_row;
   printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
   printf("%c%c[%d;1H%c[7mAdvance past bottom of buffer%c[m%c[1;1H",
	   EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
   printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   message_pending = 1;
  }

 if (direction>0)
 {
  for (i=old_curse_row; i<curse_row; i++)
   move_pt_down_line( &curse_pt );
 }
 else
 {
  for (i=old_curse_row; i>curse_row; i--)
   move_pt_up_line( &curse_pt );
  move_pt_begin_of_line( &curse_pt );
 }

 ADJUST_DISPLAY();
}



void jump_eol()
{
int old_curse_row;
 old_curse_row = curse_row;
 if (direction==1)
 {
  if (curse_pt!=EOB)
  {
   if (curse_pt->ch==10)
   {
    curse_row = curse_row + direction;
    move_pt_down_line( &curse_pt );
   }
  } else curse_row = last_row + 2;
 }
 else
 {
  curse_row = curse_row +  direction;
  move_pt_up_line( &curse_pt );
 }
 move_pt_end_of_line( &curse_pt );

 if (curse_row<0)
  {
   curse_row = 0;
   printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
   printf("%c%c[%d;1H%c[7mBackup past top of buffer%c[m%c[1;1H",
	   EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
   printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   message_pending = 1;
  }
 if (curse_row>last_row)
  {
   curse_row = last_row;
   printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
   printf("%c%c[%d;1H%c[7mAdvance past bottom of buffer%c[m%c[1;1H",
	   EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
   printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   message_pending = 1;
  }

 compute_curse_col(curse_pt);
 last_curse_col = rel_curse_col;

 ADJUST_DISPLAY();
}



void jump_bol()
{
 int old_curse_row;

 old_curse_row = curse_row;
 rel_curse_col = 0; last_curse_col = 0;
 if (direction==1)
 {
  curse_row = curse_row + direction;
  move_pt_down_line( &curse_pt );
 }
 else
 {
  if (curse_pt->prv!=txt_head)
  {
   if (curse_pt->prv->ch==10)
   {
    curse_row = curse_row +  direction;
    move_pt_up_line( &curse_pt );
   }
  } else curse_row = -1;
 }
 move_pt_begin_of_line( &curse_pt );

 if (curse_row<0)
  {
   curse_row = 0;
   printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
   printf("%c%c[%d;1H%c[7mBackup past top of buffer%c[m%c[1;1H",
	   EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
   printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   message_pending = 1;
  }
 if (curse_row>last_row)
  {
   curse_row = last_row;
   printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
   printf("%c%c[%d;1H%c[7mAdvance past bottom of buffer%c[m%c[1;1H",
	   EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC, EDT$K_ESC );
   printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   message_pending = 1;
  }

 ADJUST_DISPLAY();
}




int er_message()
{
  printf("ERROR: Non-decimal digit after percent-sign\n");
  printf("	Percent-sign must be followed by three decimal digits.\n");
  printf("	Percent-sign is special character for expressing non-printable\n");
  printf("	characters as ASCII number.\n");
  printf("	(To enter a 'real'-precent-sign, use %c037).\n", 37);
  return -1;
}


/* Replace any %'s with ascii equivalents. */
/* Returns 1 if substitution made, else returns zero, else returns -1 on error. */
int replace_percents_with_ascii( char *word )
{
 int i, j, k, replacement=0;

 i = 0;
 while ((word[i]!='\0') && (replacement != -1))
  {
   if (word[i]=='%')
    {
     if ((word[i+1]<48) || (word[i+1]>57)) replacement = er_message();
     if (replacement != -1)
      { k = word[i+1]-48;
	if ((word[i+2]<48) || (word[i+2]>57)) replacement = er_message();
	if (replacement != -1)
	 { k = 10 * k + word[i+2]-48;
	   if ((word[i+3]<48) || (word[i+3]>57)) replacement = er_message();
	   if (replacement != -1)
	    { k = 10 * k + word[i+3]-48;
	      word[i] = k;
	      /* Now move the rest of the characters down. */
	      j = i+1;
	      do {word[j] = word[j+3]; j=j+1;} while (word[j-1]!='\0');
	      replacement = 1;
	    }
	 }
      }
    }
   i = i + 1;
  }
 return replacement;
}




int getctrl()
{
  int spkey=0, letter, col, ch=EDT$K_ESC, getanother;

  for (letter=0; letter<19; letter++) functkeys_table[letter][1] = 1;
  col = 2;
  do
   {
    getanother = 0;
    for (letter=0; letter<19; letter++)
     if (functkeys_table[letter][1]==1)
      {
       if (ch==functkeys_table[letter][col])
	{
	 if (functkeys_table[letter][col+1] == -1)  spkey = functkeys_table[letter][0];
	 else getanother = 1;
	}
       else functkeys_table[letter][1] = 0;
      }
     col = col + 1;
     if ((spkey<1000) && (getanother) && (col<8))  ch = getchar();
    }
   while ((spkey<1000) && (getanother) && (col<8));
 return spkey;
}




void search()
{
 int match, i, j1, j2, cntl=0, eos=0, new_row;
 char ch, s_strng[MAX_SRCH_STRING];
 struct __text__ *tmp_pt, *tmp_pt1;

 if (Gold)
 { /*Accept_strng*/
  printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
  printf("%c[%d;1H%c[7mSearch for: ", EDT$K_ESC, nrows - 1, EDT$K_ESC);
  i = 0;
  do
  {
   ch  = getchar();  fprintf(jou_outfile,"%c",ch);
   if (ch==13) ch = 10;
   if (ch==EDT$K_ESC) cntl = 1;
   srch_strng[i] = ch;   i = i + 1;  if (i==MAX_SRCH_STRING) eos = 1;
   if ((ch==127) || (ch==8))    /* handle delete key */
   { /*4*/
    if (i>1)
    { /*3*/
     i = i - 2;
     if (srch_strng[i]==9)	/* deleting a TAB */
     { /*2*/
      j1 = 5;
     } /*2*/
     else
     if (srch_strng[i]==12)
      j1 = 4;
     else
     if (srch_strng[i]==EDT$K_ESC)
      j1 = 5;
     else
     if (srch_strng[i] < EDT$K_ESC)
      j1 = 2;
     else
      j1 = 1;
     for (j2=0; j2!=j1; j2++) printf("%c",8);
     printf("%c[m",EDT$K_ESC);
     for (j2=0; j2!=j1; j2++) printf(" ");
     for (j2=0; j2!=j1; j2++) printf("%c",8);
     printf("%c[7m",EDT$K_ESC);
    } /*3*/
    else
    {
     printf("%c",EDT$K_BELL);
     i = 0;
    }
   } /*4*/
   else
   if (!cntl)
   {
    if (ch==9) printf("<TAB>"); else    print_char( ch );
   }
   else
   {
    int spkey;

    if (ch==EDT$K_ESC) spkey = getctrl();

    if (spkey>=1000)
     {
      eos = 1;
      if (spkey==1005) direction = 1;
      if (spkey==1006) direction = -1;
     }

   }
  } while (!eos);
  printf("%c[m", EDT$K_ESC);
 } /*Accept_strng*/

 { /*Do_Search*/
  if (srch_caps)	/* Capitolize the search string */
  {
   i=0;
   while (srch_strng[i]!=EDT$K_ESC)
    {s_strng[i] = cap_ch( srch_strng[i] ); i = i + 1;}
   s_strng[i] = EDT$K_ESC;
  }
  match = 0;  tmp_pt = curse_pt;  new_row = curse_row;
  while ((!match) && (((direction==-1) && (tmp_pt!=txt_head))  ||
		((direction==1) && (tmp_pt!=EOB))))
  {
   i = 0;   tmp_pt1 = tmp_pt;
   if (srch_caps)
   {
    while ((tmp_pt1!=txt_head) && (tmp_pt1!=EOB) &&
	   (s_strng[i]==cap_ch(tmp_pt1->ch)) && (srch_strng[i]!=EDT$K_ESC))
     { i = i + 1;  tmp_pt1 = tmp_pt1->nxt; }
   }
   else
   {
    while ((tmp_pt1!=txt_head) && (tmp_pt1!=EOB) &&
	   (srch_strng[i]==tmp_pt1->ch) && (srch_strng[i]!=EDT$K_ESC))
     { i = i + 1;  tmp_pt1 = tmp_pt1->nxt; }
   }
   if ((srch_strng[i]==EDT$K_ESC) && (tmp_pt!=curse_pt)) match = 1;

   if (!match)   /* If still no match, then advance pointer */
   {
    if (direction==1)
     {
      if (tmp_pt->ch==10) new_row = new_row + direction;
      if (tmp_pt!=EOB) tmp_pt = tmp_pt->nxt;
     }
    else
     {
      if (tmp_pt!=txt_head) tmp_pt = tmp_pt->prv;
      if (tmp_pt->ch==10) new_row = new_row + direction;
     }
   }
  }

  if (!match)
  {
   printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
   printf("%c%c[%d;1H%c[K%c[7mString /", EDT$K_BELL, EDT$K_ESC, nrows - 1, EDT$K_ESC, EDT$K_ESC);
   i=0; while (srch_strng[i]!=EDT$K_ESC) {print_char(srch_strng[i]); i=i+1; }
   printf("/ was not found%c[m",  EDT$K_ESC);
   printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   reposition_cursor();
   message_pending = 1;
  }
  else
  {
   if (Gold)
   { /* Clear the message box */
    printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
    printf("%c[%d;1H%c[K", EDT$K_ESC, nrows - 1, EDT$K_ESC );
    printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   }
   curse_pt = tmp_pt;
   curse_row = new_row;
   compute_curse_col(curse_pt);
   last_curse_col = rel_curse_col;
   ADJUST_DISPLAY();
  }
 } /*Do_Search*/

 Gold = 0;
}





void enter_ascii()
{
 int i, p = 0, cntl = 0, ascii_number, eos = 0;
 char ch;
  printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
  printf("%c[%d;1H%c[7mEnter ASCII value in decimal: ", EDT$K_ESC, nrows - 1, EDT$K_ESC);
  i = 1;  ascii_number = 0;
  do
  {
   ch  = getchar();  fprintf(jou_outfile,"%c",ch);
   if (ch==EDT$K_ESC) cntl = 1;
   if (!cntl)
    {
     if ((ch>47) && (ch<57))
      { ascii_number = ascii_number * 10 + ch - 48; printf("%c",ch);}
     else printf("%c",EDT$K_BELL);
    }
   else
    {
     if (p==2) {if ((ch>64) && (ch<69)) {eos = 1;} }
#if ((EDT$K_TERMTYPE == 2) || (EDT$K_TERMTYPE == 3))
     if (p==2) eos = 1;
#endif
     if (p==4) eos = 1;
     if ((p==1) && ((ch!='[') && (ch!='O'))) {printf("%c",EDT$K_BELL); cntl=0; p = 0;}
     else    p = p + 1;
    }
   } while (!eos);
  printf("%c[m", EDT$K_ESC);
  /* Clear the message box */
  printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
  printf("%c[%d;1H%c[K", EDT$K_ESC, nrows - 1, EDT$K_ESC );
  printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
  reposition_cursor();


  if (ascii_number>255) printf("%c",EDT$K_BELL);
  else
   {
    if (ascii_number==13) { ascii_number = 10; }
    insert_char( ascii_number, &curse_pt );
    INSERT_CHAR_INLINE(ascii_number);
   }
}
/**/



void handle_key(ch)
char ch;
{
 int ch_index, old_ch, letter, col, getanother, spkey;
 struct __text__ *txt_tmp2;

 if (message_pending ==1)
  { /* Clear the message box */
   printf("%c[1;%dr", EDT$K_ESC, nrows); /*Temporarily expand scrolling region*/
   printf("%c[%d;1H%c[K", EDT$K_ESC, nrows - 1, EDT$K_ESC );
   printf("%c[1;%dr", EDT$K_ESC, nrows-2); /*Re-establish scrolling region*/
   reposition_cursor();
  }


 spkey = 0;
 for (letter=0; letter<19; letter++) functkeys_table[letter][1] = 1;
 col = 2;
 do
  {
   getanother = 0;
   for (letter=0; letter<19; letter++)
    {
     if (functkeys_table[letter][1]==1)
      {
       if (ch==functkeys_table[letter][col])
	{
	 if (functkeys_table[letter][col+1] == -1)  spkey = functkeys_table[letter][0];
	 else getanother = 1;
	}
       else functkeys_table[letter][1] = 0;
      }
    }

   /* printf("COL=%d spkey=%d  getanother=%d\n", col, spkey, getanother); */

   col = col + 1;
   if ((spkey<1000) && (getanother) && (col<8))  ch = getchar();
  }
 while ((spkey<1000) && (getanother) && (col<8));


 if (spkey==0)  ctrl = 0;  else  ctrl = 1;

 if (!ctrl)
  { /*notcntrl*/
    /*If valid ascii, enque it into file.*/
    Gold = 0;
    if (ch==23) display_screen(1);
    else
    if ((ch==127) || (ch==8))
     { /*deletechar*/
	DELETE_CHAR_BACKWARD();
	Mark = 0;		/* <<<==== For safety, for now.  Need to guard against deleting the marked character. */
     } /*deletechar*/
    else
     {
      if (ch==13) { ch = 10; }
      insert_char( ch, &curse_pt );
      INSERT_CHAR_INLINE(ch);
      Mark = 0;			/* <<<==== For safety, for now.  Need to guard against deleting the marked character. */
     }
  } /*notcntrl*/
 else
  { /*cntrl*/

    switch (spkey)
    {
     case 1016:	/*down*/
		Gold = 0;
		down_arrow();
	break;
     case 1017:	/*right*/
		Gold = 0;
		right_arrow();
	break;
     case 1018:	/*left*/
		Gold = 0;
		left_arrow();
	break;
     case 1015:	/*up*/
		Gold = 0;
		up_arrow();
	break;

     case 1002:  /* Delete by Line key */
		DELETE_LINE_FORWARD();
		Mark = 0;
	break;
     case 1008:  /* Delete by Word key */
		DELETE_WORD_FORWARD();
		Mark = 0;
	break;
     case 1000:  /* GOLD */
		Gold = 1;
	break;
     case 1003:	/* Jump by Page / Reformat Paragraph */
		if (Gold)
		{
		 if (Mark)
		 { /*Re-format*/

		   /* First decide direction work in. */

		   if (mark_pt1!=curse_pt)  /* Don't do anything, if curse_pt is on mark. */
		   { /*ok2reformat*/
		   if ((mark_row<curse_row) || ((curse_row==mark_row) && (mark_col<last_curse_col)))
		    { /*forward*/

		      /* First get txt_tmp to first white-space. */
		      txt_tmp = mark_pt1;
		      while ((txt_tmp!=curse_pt) && (txt_tmp->ch!=10) &&
				(txt_tmp->ch!=' ') && (txt_tmp->ch!='	'))
			 txt_tmp = txt_tmp->nxt;


		      while (txt_tmp!=curse_pt)
		      { /*loop*/

		       /* Next get txt_tmp2 to end of next word, changing <cr>s to spaces. */
		       txt_tmp2 = txt_tmp;  old_ch = ' ';
		       while ((txt_tmp2!=curse_pt) && ((txt_tmp2->ch==10) ||
				(txt_tmp2->ch==' ') || (txt_tmp2->ch=='	')))
			{
			 if ((txt_tmp2->ch==10) && (txt_tmp2->nxt->ch!=10) && (old_ch!=10))
			  if (EOB->prv!=txt_tmp2)
			  { txt_tmp2->ch=' '; last_row = last_row - 1; curse_row = curse_row - 1; }
			 old_ch = txt_tmp2->ch;
			 txt_tmp2 = txt_tmp2->nxt;
			}
		       /* Now txt_tmp2 points to first char of next word. */
		       /* Now, get it to the end of that word. */
		       while ((txt_tmp2!=curse_pt) && (txt_tmp2->ch!=10) &&
				(txt_tmp2->ch!=' ') && (txt_tmp2->ch!='	'))
			{
			 txt_tmp2 = txt_tmp2->nxt;
			}

		       /* Now txt_tmp2 points to (after) end of next word, or curse_pt. */
		       if (txt_tmp2!=curse_pt)
		       {
			txt_tmp2 = txt_tmp2->prv;
			compute_curse_col( txt_tmp2 );
			if (rel_curse_col>right_margin)
			 {
			  txt_tmp->ch = 10;  last_row = last_row + 1; curse_row = curse_row + 1;
			  txt_tmp = txt_tmp->nxt;
			  /* Delete intervening white-space. */
			  while ((txt_tmp->ch==' ') || (txt_tmp->ch=='	'))
			   {
			     txt_tmp = txt_tmp->nxt;
			     delete_char( txt_tmp->prv );
			   }
			 }
			txt_tmp = txt_tmp2->nxt;
		       } else txt_tmp = txt_tmp2;
		      } /*loop*/

		    } /*forward*/
		   else
		    { /*backward*/

		      /* First get txt_tmp to first white-space. */
		      txt_tmp = curse_pt;
		      while ((txt_tmp!=mark_pt1) && (txt_tmp->ch!=10) &&
				(txt_tmp->ch!=' ') && (txt_tmp->ch!='	'))
			 txt_tmp = txt_tmp->nxt;

		      while (txt_tmp!=mark_pt1)
		      { /*loop*/

		       /* Next get txt_tmp2 to end of next word, changing <cr>s to spaces. */
		       txt_tmp2 = txt_tmp;  old_ch = ' ';
		       while ((txt_tmp2!=mark_pt1) && ((txt_tmp2->ch==10) ||
				(txt_tmp2->ch==' ') || (txt_tmp2->ch=='	')))
			{
			 if ((txt_tmp2->ch==10) && (txt_tmp2->nxt->ch!=10) && (old_ch!=10))
			  if (EOB->prv!=txt_tmp2)
			  { txt_tmp2->ch=' '; last_row = last_row - 1; }
			 old_ch = txt_tmp2->ch;
			 txt_tmp2 = txt_tmp2->nxt;
			}
		       /* Now txt_tmp2 points to first char of next word. */
		       /* Now, get it to the end of that word. */
		       while ((txt_tmp2!=mark_pt1) && (txt_tmp2->ch!=10) &&
				(txt_tmp2->ch!=' ') && (txt_tmp2->ch!='	'))
			{ txt_tmp2 = txt_tmp2->nxt; }

		       /* Now txt_tmp2 points to (after) end of next word, or curse_pt. */
		       if (txt_tmp2!=mark_pt1)
		       {
			txt_tmp2 = txt_tmp2->prv;
			compute_curse_col( txt_tmp2 );
			if (rel_curse_col>right_margin)
			 {
			  txt_tmp->ch = 10;  last_row = last_row + 1;
			  txt_tmp = txt_tmp->nxt;
			  /* Delete intervening white-space. */
			  while ((txt_tmp->ch==' ') || (txt_tmp->ch=='	'))
			   {
			     txt_tmp = txt_tmp->nxt;
			     delete_char( txt_tmp->prv );
			   }
			 }
			txt_tmp = txt_tmp2->nxt;
		       } else txt_tmp = txt_tmp2;
		      } /*loop*/

		    } /*backward*/
		   } /*ok2reformat*/

		  adjust_screen_parameters();
		  display_screen(1);
		  Mark = 0;
		  Gold = 0;
		 } /*Re-format*/
		}
		else
		 screen_jump();
	break;
     case 1001:	/* Find */
		search();
	break;
     case 1014:	/* Delete by Char */
		if (! Gold)
		 {
		  DELETE_CHAR_FORWARD();
		 }
		else
		 {  /* Un-delete char */
		  if (ch_buf!='\0')
		  {
		   insert_char( ch_buf, &curse_pt );
		   INSERT_CHAR_INLINE(ch_buf);
		  }
		  Gold = 0;
		 }
	       Mark = 0;

	break;
     case 1005:	/* Forward Switch */
		if (Gold)
		{  /* Jump to Bottom of Buffer */
		 curse_pt = EOB;
		 curse_row = last_row;  last_curse_col = 0;
		 rel_curse_col = 0;
		 ADJUST_DISPLAY();
		 Gold = 0;  direction = -1;
		}
		else
		 direction = 1;
	break;
      case 1006:	/* Backward Switch */
		if (Gold)
		{  /* Jump to Top of Buffer */
		 curse_pt = txt_head->nxt;
		 curse_row = 0;  last_curse_col = 0;
		 rel_curse_col = 0;
		 ADJUST_DISPLAY();
		 Gold = 0;  direction = 1;
		}
		else
		direction = -1;
	break;

     case 1004:    /* Replace_and_Search_for_next */
	  /* First, check that you are positioned at front of match to search_string. */
	  if (srch_match())
	   {
	    /* Then, cut matched string away, and paste buffer in. */
	      /* Cursor moves to after end of matched string. File shortens. */
	      ch_index = 0;
	      while (srch_strng[ch_index] != EDT$K_ESC)
	       {
		curse_pt = curse_pt->nxt;
		if (curse_pt==EOB) {printf("SEVERE_ERROR: BOB\n"); /* txt_tmp=mark_pt1; */}
		if (curse_pt->prv->ch==10) { last_row = last_row - 1; }
		delete_char( curse_pt->prv );
		ch_index = ch_index + 1;
	       }
	      adjust_screen_parameters();
	      display_screen(1);

	    /* Now Paste the buffer in. */
	     txt_tmp = paste_buffer;
	     if (paste_buffer_length<512)
	      { /*foreground*/
		while (txt_tmp!=0)
		{
		 insert_char( txt_tmp->ch, &curse_pt );
		 INSERT_CHAR_INLINE(txt_tmp->ch);
		 txt_tmp = txt_tmp->nxt;
		}
	      } /*foreground*/
	     else
	      { /*batch*/
		while (txt_tmp!=0)
		{
		 insert_char( txt_tmp->ch, &curse_pt );
		 if (txt_tmp->ch==10)                    /* If inserting a new line: */
		  { curse_row = curse_row + 1;  last_row = last_row + 1; }
		 txt_tmp = txt_tmp->nxt;
		}
		adjust_screen_parameters();
		display_screen(1);
	      } /*batch*/

	    if (Gold)
	    /* Then, search for next occurrence. */
	     { Gold = 0;  search(); }
	   } else printf("%c", EDT$K_BELL);
	break;

      case 1007:  /* Cut / Paste */
	if (Gold) /*paste buffer*/
	 {
	   txt_tmp = paste_buffer;
	   if (paste_buffer_length<512)
	    { /*foreground*/
	      while (txt_tmp!=0)
	      {
	       insert_char( txt_tmp->ch, &curse_pt );
	       INSERT_CHAR_INLINE(txt_tmp->ch);
	       txt_tmp = txt_tmp->nxt;
	      }
	    } /*foreground*/
	   else
	    { /*batch*/
	      while (txt_tmp!=0)
	      {
	       insert_char( txt_tmp->ch, &curse_pt );
	       if (txt_tmp->ch==10)                    /* If inserting a new line: */
		{ curse_row = curse_row + 1;  last_row = last_row + 1; }
	       txt_tmp = txt_tmp->nxt;
	      }
	      adjust_screen_parameters();
	      display_screen(1);
	    } /*batch*/
	  Gold = 0;
	 }
	else /*cut to marker*/
	 if (Mark)
	 {
	   /* If paste_buffer is not empty, free it. */
	   while (paste_buffer!=0) { txt_tmp = paste_buffer; paste_buffer = paste_buffer->nxt; dispose_ch( txt_tmp ); }
	   paste_buffer_length = 0;

	   /* First decide direction to cut in. */
	   /* Then push characters on in reverse order. */

	   if (mark_pt1!=curse_pt)  /* If curse_pt is on mark, then don't cut, and leave buffer empty. */
	   { /*cut*/		    /* Otherwise cut. */
	   if ((mark_row<curse_row) || ((curse_row==mark_row) && (mark_col<last_curse_col)))
	    { /*cut_backward*/  /* Cut back-up to mark, from left of cursor. */
	      txt_tmp = curse_pt->prv;	/* Cursor stays where it is. */
	      mark_pt1 = mark_pt1->prv;
	      while (txt_tmp!=mark_pt1) /* File shortens. */
	       {
		push_buffer( txt_tmp->ch, &paste_buffer );
		txt_tmp = txt_tmp->prv;   paste_buffer_length = paste_buffer_length + 1;
		if (txt_tmp==txt_head) {printf("SEVERE_ERROR: BOB\n"); /* txt_tmp=mark_pt1->prv; */}
		if (txt_tmp->nxt->ch==10) { last_row = last_row - 1; curse_row = curse_row - 1; }
		delete_char( txt_tmp->nxt );
	       }
	      adjust_screen_parameters();
	      display_screen(1);

	    } /*cut_backward*/
	   else
	    { /*cut_forward*/	/* Cut back-up to cursor, from mark. */

	      txt_tmp = mark_pt1->prv;	/* Cursor moves to mark_pt. File shortens. */
	      curse_pt = curse_pt->prv;
	      while (txt_tmp!=curse_pt)
	       {
		push_buffer( txt_tmp->ch, &paste_buffer );
		txt_tmp = txt_tmp->prv;   paste_buffer_length = paste_buffer_length + 1;
		if (txt_tmp==txt_head) {printf("SEVERE_ERROR: BOB\n"); /* txt_tmp=mark_pt1; */}
		if (txt_tmp->nxt->ch==10) { last_row = last_row - 1; mark_row = mark_row - 1; }
		delete_char( txt_tmp->nxt );
	       }
	      curse_pt = mark_pt1;  curse_row = mark_row;
	      adjust_screen_parameters();
	      display_screen(1);

	    } /*cut_forward*/
	   } /*cut*/

	 }
	else
	 printf("%c%c",EDT$K_BELL,EDT$K_BELL);

	 Mark = 0;
	break;
     case 1009:	/* Jump by Word */
		if (Gold) CAPITOLIZE_CHAR();
		else
		jump_by_word();
	break;
     case 1010: /* Move to End-Of-Line */
		jump_eol();

	break;
     case 1012:	/* Enter Non-Ascii */
		enter_ascii();   Mark = 0;

	break;
     case 1011:	/* Move to Begining-Of-Line */
		jump_bol();

	break;
     case 1013:	/* Dot (Set Mark or Cancel) */
		if (Gold) Gold = 0;	/* Cancel Gold. */
		else
		 { /* Set mark. */
		  Mark = 1;
		  mark_pt1 = curse_pt;
		  mark_row = curse_row;
		  mark_col = last_curse_col;
		 }
	break;
     }

  } /*cntrl*/

 inpt1 = 0;
}



void open_journal_file( char *fname_in )
{
 int i;
 char fname[5120];

  strcpy(fname,fname_in);
  i = strlen(fname)-1;   /* Search for the last '.' in the file-name-string. */
  while ((i>=0) && (fname[i]!='.')) i = i - 1;
  if ((i<0) || ((i>0) && (fname[i-1]=='.'))) i = strlen(fname);  /* If no 'dot', then append to end. */
  fname[i] = '.'; fname[i+1] = 'j'; fname[i+2] = 'o';
  fname[i+3] = 'u'; fname[i+4] = '\0';
  if (encode_mode) jou_outfile = fopen("/dev/null","w");
  else
   jou_outfile = fopen(fname,"w");
  if (jou_outfile==0)
   {
    printf("%cWARNING:  Could not open Journal file.  There will be no journaling.\n",EDT$K_BELL);
    jou_outfile = fopen("/dev/null","w");
   }
  setbuffer( jou_outfile, jou_buf, 4 );
}



void remove_journal_file( char *fname_in )
{
 int i;
 char fname[5120];

  strcpy(fname,fname_in);
  i = strlen(fname)-1;   /* Search for the last '.' in the file-name-string. */
  while ((i>=0) && (fname[i]!='.')) i = i - 1;
  if ((i<0) || ((i>0) && (fname[i-1]=='.'))) i = strlen(fname);  /* If no 'dot', then append to end. */
  fname[i] = '.'; fname[i+1] = 'j'; fname[i+2] = 'o';
  fname[i+3] = 'u'; fname[i+4] = '\0';
  if (!encode_mode)
   remove(fname);
}



void copy_backup(char *fname_in)
{
 FILE *infile, *outfile;
 int i;
 char ch, fname[5120];

 i = 0;
 do {fname[i] = fname_in[i];  i = i + 1;}
 while (fname_in[i-1]!='\0');;

 infile = fopen(fname,"r");
 if (infile==0) printf("ERROR: file %s does not exist.\n", fname);
 else
 {
  i = strlen(fname)-1;   /* Search for the last '.' in the file-name-string. */
  while ((i>=0) && (fname[i]!='.')) i = i - 1;
  if ((i<0) || ((i>0) && (fname[i-1]=='.'))) i = strlen(fname);  /* If no 'dot', then append to end. */
  fname[i] = '.'; fname[i+1] = 'b'; fname[i+2] = 'a';
  fname[i+3] = 'k'; fname[i+4] = '\0';
  outfile = fopen(fname,"w");
  if (outfile==0)
   {
    printf("%cWARNING: Cannot write '%s' file.\n", EDT$K_BELL, fname );
   }
  else
   {
    ch = getc(infile);
    while (!feof(infile))
     { fprintf(outfile,"%c",ch); ch = getc(infile); }
    fclose(outfile);
   }
  fclose(infile);
 }
}





int write_compressed_file( char *outfilename )
{
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt;
 int sz1=0, sz2=0, szi, success=1, flen, buflen, N=0;
 unsigned char ch, chksum;
 FILE *outfile=0;
 struct __text__ *tmp_pt;
 int nln=0, nch=0, err=0;
 int pwi, pwl;

 outfile = fopen(outfilename,"wb");
 if (outfile==0)
  {
   printf("%cCANNOT OPEN FILE /%s/ FOR WRITING.\n",EDT$K_BELL,outfilename); err=1;
   printf("FILE WAS NOT WRITTEN.\n");
   return err;
  }

 /* Determine buffer length. */
 tmp_pt = txt_head->nxt;
 while (tmp_pt!=EOB) { N++;  tmp_pt = tmp_pt->nxt; }

 buflen = N / sczbuflen + 1;
 buflen = N / buflen + 1;
 if (buflen>=SCZ_MAX_BUF) {printf("Error: Buffer length too large.\n"); exit(0);}

 if (encode_mode) { pwi = 0;  pwl = strlen(psswd); }

 tmp_pt = txt_head->nxt;
 while (sz1 < N)
  { /*outerloop*/

    chksum = 0;  szi = 0;
    if (N-sz1 < buflen) buflen = N-sz1;

    /* Convert buffer into linked list. */
    buffer0_hd = 0;   buffer0_tl = 0;
    if (encode_mode)
     {
      while ((tmp_pt!=EOB) && (szi < buflen))
       {
	ch = (unsigned char)(tmp_pt->ch);
	ch = ch + psswd[pwi]; // if (ch>255) ch = ch - 255;
	pwi = pwi + 1;  if (pwi==pwl) pwi = 0;
	scz_add_item( &buffer0_hd, &buffer0_tl, ch );
	chksum += ch;
	sz1++;  szi++;
	if (ch==10) nln = nln + 1;
	nch = nch + 1;
	tmp_pt = tmp_pt->nxt;
       }
     }
    else
     {
      while ((tmp_pt!=EOB) && (szi < buflen))
       {
	scz_add_item( &buffer0_hd, &buffer0_tl, (unsigned char)(tmp_pt->ch) );
	chksum += (unsigned char)(tmp_pt->ch);
	sz1++;  szi++;
	if (tmp_pt->ch==10) nln = nln + 1;
	nch = nch + 1;
	tmp_pt = tmp_pt->nxt;
       }
     }

   success = success & Scz_Compress_Seg( &buffer0_hd, szi );

   /* Write the file out. */
   while (buffer0_hd!=0)
    {
     fputc( buffer0_hd->ch, outfile );
     sz2++;
     bufpt = buffer0_hd;
     buffer0_hd = buffer0_hd->nxt;
     sczfree(bufpt);
    }
   fprintf(outfile,"%c", chksum);
   sz2++;
   if (sz1 >= N) fprintf(outfile,"]");	/* Place no-continuation marker. */
   else fprintf(outfile,"[");		/* Place continuation marker. */
   sz2++;

  } /*outerloop*/
 if (fclose(outfile) == EOF) err = 1;
 if (err) printf("%cERROR writing file %s.\n",EDT$K_BELL, outfilename);
 else     printf("File '%s' has been written (%d-lines, %d-characters)\n", outfilename, nln, nch);

 printf("Initial size = %d,  Final size = %d\n", sz1, sz2);
 printf("Compression ratio = %g : 1\n", (float)sz1 / (float)sz2 );
 free(scz_freq2);
 scz_freq2 = 0;
 return !success;
}




int write_file( char *fname )	/* Returns 0 on success, 1 on error. */
{
 FILE *outfile;
 struct __text__ *tmp_pt;
 int nln=0, nch=0, err=0;
 int pwi, ch, pwl;
 int gzipd_file=0;
 char *suffix;

 if ((strstr(fname,".scz")!=0) && (strlen(strstr(fname,".scz"))==4))
  { return write_compressed_file(fname); }

 suffix = strstr(fname,".gz");
 if ((suffix != 0) && (strlen(suffix) == 3))
  {
   gzipd_file = 1;
   suffix[0] = '\0';	/* Trucate the gzip suffix. */
  }

 outfile = fopen(fname,"w");
 if (outfile==0)
  {
   printf("%cCANNOT OPEN FILE /%s/ FOR WRITING.\n",EDT$K_BELL,fname);
   err = 1;
   printf("FILE WAS NOT WRITTEN.\n");
  }
 else
 {
  tmp_pt = txt_head->nxt;
  if (encode_mode)
   {
    pwi = 0;  pwl = strlen(psswd);
    while (tmp_pt!=EOB)
     { ch = tmp_pt->ch;
       ch = ch + psswd[pwi]; if (ch>255) ch = ch - 255;
       pwi = pwi + 1;  if (pwi==pwl) pwi = 0;
       if (fprintf(outfile,"%c", ch) == EOF) err = 1;;
       if (tmp_pt->ch==10) nln = nln + 1;
       nch = nch + 1;
       tmp_pt = tmp_pt->nxt;
     }
   }
  else
   {
    while (tmp_pt!=EOB)
     { if (fprintf(outfile,"%c", tmp_pt->ch) == EOF) err = 1;;
       if (tmp_pt->ch==10) nln = nln + 1;
       nch = nch + 1;
       tmp_pt = tmp_pt->nxt;
     }
   }
  if (fclose(outfile) == EOF) err = 1;
  if (err) printf("%cERROR writing file %s.\n",EDT$K_BELL, fname);
  else
   printf("File '%s' has been written (%d-lines, %d-characters)\n", fname, nln, nch);
  if (gzipd_file)
   { char cmd[4096]="gzip -f ";
    printf(" Gzip'ing %s\n", fname );
    strcat( cmd, fname );
    system(cmd);
   }
 }
 return err;
}



int write_buffer(struct __text__ *bufpt, char *fname)
{
 FILE *outfile;
 struct __text__ *tmp_pt;
 int nln=0, nch=0, err=0;
 int pwi, ch, pwl;
 char lastch=0;

 outfile = fopen(fname,"w");
 if (outfile==0)
  {
   printf("%cCANNOT OPEN FILE /%s/ FOR WRITING.\n",EDT$K_BELL,fname); err=1;
   printf("FILE WAS NOT WRITTEN.\n");
  }
 else
  {
   tmp_pt = bufpt;
   if (encode_mode)
    {
     pwi = 0;  pwl = strlen(psswd);
     while (tmp_pt != 0)
      { ch = tmp_pt->ch;
	ch = ch + psswd[pwi]; if (ch>255) ch = ch - 255;
	lastch = ch;
	pwi = pwi + 1;  if (pwi==pwl) pwi = 0;
	if (fprintf(outfile,"%c", ch) == EOF) err = 1;;
	if (tmp_pt->ch==10) nln = nln + 1;
	nch = nch + 1;
	tmp_pt = tmp_pt->nxt;
      }
    }
   else
    {
     while (tmp_pt != 0)
      { if (fprintf(outfile,"%c", tmp_pt->ch) == EOF) err = 1;;
	lastch = tmp_pt->ch;
	if (tmp_pt->ch==10) nln = nln + 1;
	nch = nch + 1;
	tmp_pt = tmp_pt->nxt;
      }
    }
   if (lastch != 10) fprintf(outfile,"\n");
   if (fclose(outfile) == EOF) err = 1;
   if (err) printf("%cERROR writing file %s.\n",EDT$K_BELL, fname);
   else
    printf("File '%s' has been written (%d-lines, %d-characters)\n", fname, nln, nch);
  }
 return err;
}






void global_substitute( char *sub_srch_strng, char *sub_rplcmnt_strng )
{
 int i, s_len, r_len, match, match_found=0, match_online=0;
 struct __text__ *tmp_pt, *tmp_pt1;

 if (srch_caps)        /* Capitalize the search string */
  {
   i=0;
   while (sub_srch_strng[i]!='\0') {sub_srch_strng[i] = cap_ch( sub_srch_strng[i] ); i = i + 1;}
  }

 if ((replace_percents_with_ascii(sub_srch_strng) != -1) &&
	(replace_percents_with_ascii(sub_rplcmnt_strng) != -1))
 { /*ok*/
 s_len = strlen(sub_srch_strng);
 r_len = strlen(sub_rplcmnt_strng);
 tmp_pt = txt_head->nxt;
 match = 0;

  while (tmp_pt!=EOB)
  { /*scan_file*/
   i = 0;   tmp_pt1 = tmp_pt;
   if (srch_caps)
   {
    while ((tmp_pt1!=txt_head) && (tmp_pt1!=EOB) &&
	   (sub_srch_strng[i]==cap_ch(tmp_pt1->ch)) && (sub_srch_strng[i]!=EDT$K_ESC))
     { i = i + 1;  tmp_pt1 = tmp_pt1->nxt; }
   }
   else
   {
    while ((tmp_pt1!=txt_head) && (tmp_pt1!=EOB) &&
	   (sub_srch_strng[i]==tmp_pt1->ch) && (sub_srch_strng[i]!='\0'))
     { i = i + 1;  tmp_pt1 = tmp_pt1->nxt; }
   }
   if (sub_srch_strng[i]=='\0') match = 1;

   if (match)   /* If match, then replace */
   {
    match = 0;
    match_found = match_found + 1;  match_online = 1;
    /* First remove the old string. */
    for (i=0; i!=s_len; i++)
     {
      tmp_pt1 = tmp_pt;
      tmp_pt = tmp_pt->nxt;
      if (tmp_pt1->ch==10) last_row = last_row - 1;
      delete_char( tmp_pt1 );
     }

    /* Now insert the replacement string. */
    for (i=0; i!=r_len; i++)
     { insert_char( sub_rplcmnt_strng[i], &tmp_pt ); if (sub_rplcmnt_strng[i]==10) last_row = last_row + 1; }
   }
   else  if (tmp_pt!=EOB) tmp_pt = tmp_pt->nxt;

   if ((tmp_pt->ch==10) && (match_online))
    { /*Display_modified_line*/
      match_online = 0;
      tmp_pt1 = tmp_pt;
      move_pt_begin_of_line( &tmp_pt1 );
      spew_line( tmp_pt1 ); printf("\n");
    } /*Display_modified_line*/
  } /*scan_file*/

 } /*ok*/

 if (match_found!=0)
  {
   if ((EOB->prv!=txt_head) && (EOB->prv->ch!=10)) {printf("MISSING <CR> INSERTED at [EOF]\n"); insert_char( 10, &EOB ); last_row=last_row + 1;}
   printf("\n%d substitutions made.\n", match_found );
   curse_pt = txt_head->nxt;
   curse_row = 0;  last_curse_col = 0;
   rel_curse_col = 0;
   adjust_screen_parameters();
   Gold = 0;  direction = 1;
  }
 else printf("No substitutions made.\n");
}



void substitute_command( char *com_line )
{
 int i,j;
 char sub_srch_strng[256], sub_rplcmnt_strng[256];
   /* Expect s/srch_strng/rplcmnt_strng/w  or line range in brackets. */
   j = 0; i = 2;  /* Use com_line[1] as the search delimiter. */
   if (com_line[1]=='\0') printf("Badly formed Substitute command.\n");
   else
   {
    while ((com_line[i]!=com_line[1]) && (com_line[i]!='\0'))
     { sub_srch_strng[j] = com_line[i]; i=i+1; j=j+1;}
    sub_srch_strng[j] = '\0';
    if (com_line[i]==com_line[1])
     {
      i = i + 1;  j = 0;
      while ((com_line[i]!=com_line[1]) && (com_line[i]!='\0'))
       { sub_rplcmnt_strng[j] = com_line[i]; i=i+1; j=j+1;}
      sub_rplcmnt_strng[j] = '\0';
      if (com_line[i]==com_line[1])
       global_substitute( sub_srch_strng, sub_rplcmnt_strng );
      else printf("Badly formed Substitute command.\n");
     } else printf("Badly formed Substitute command.\n");
   }
}





void help_quick()
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



void help_long()
{
 FILE *fz;
 char tcline[260];

 // tmpnam(fnm);
 mkstemp(tname);
 fz = fopen(tname,"w");
 if (fz==0) {printf("Cannot open %s\n",tname); return;}
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
 sprintf(tcline,"lrss %s",tname);
 system(tcline);
 remove(tname);
}



void help_keypad_setup()
{
 FILE *fz;
 char tcline[260];

 // tmpnam(fnm);
 mkstemp(tname);
 fz = fopen(tname,"w");
 if (fz==0) {printf("Cannot open %s\n",tname); return;}
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
 sprintf(tcline,"less %s",tname);
 system(tcline);
 remove(tname);
}




int edt_isnum( char ch )	/* Return true if character is a numeral, else return false. */
{
 if (((unsigned char)ch >= 48) && ((unsigned char)ch <= 57)) return 1;  else  return 0;
}



void insert_string( char *line )
{
 int k=0;
 struct __text__ *tmp_txt;

 tmp_txt = curse_pt;	/* keep inserting at eob */
 while (line[k] != '\0')
  {
   if (line[k] == '\n') { last_row++; }
   insert_char( line[k++], &tmp_txt );
  }
}



int main( int argc, char *argv[] )
{
char	ch, *suffix, fname[2560], name1[2560], com_line[4196];
int	i, j, k, jj, file_exists, openatlinenum = 1, gzipd_file = 0, leave = 0;
struct __text__ *tmp_pt;
struct stat file_info;

#if ( EDT$K_TERMTYPE == 1 )
	nrows = 58;
	ncols = 120;
#endif

#if ( EDT$K_TERMTYPE == 4 )
	// nrows = 66;  /* Super VGA */
	nrows = 58;	  /* Regular VGA ? */
	ncols = 120;
#endif

#if ( (EDT$K_TERMTYPE == 2) || (EDT$K_TERMTYPE == 3) )
	nrows = 24;
	ncols = 80;
#endif

	/* Find window size, and set parameters appropriately. */
	resize(1);

	strcpy(active_buffer_name, "main");
	buffer_list = (struct __buf_lis__ *) malloc(sizeof(struct __buf_lis__));

	strcpy(buffer_list->buff_name, active_buffer_name);
	buffer_list->nxt = 0;



	curse_row = tframe_row = rel_curse_row = rel_curse_col = last_curse_col = 0;

	direction = 1;	/*Forward Direction*/
	srch_strng[0] = ' ';
	srch_strng[1] = EDT$K_ESC;

	ch_buf = '\0';
	word_buf = 0, line_buf = 0, paste_buffer = mark_pt1 = 0;  Mark = 0;

	get_keypad_setup();

	txt_free = (struct __text__ *)calloc( 1, sizeof(struct __text__) );
	free_nil = txt_free;

	EOB = new_ch();
	EOB->nxt = EOB->ch = NULL;

	txt_head = new_ch();
	txt_head->prv = NULL; txt_head->nxt = EOB;
	txt_head->ch = 'B';
	EOB->prv = txt_head;

	infile = outfile = NULL;
	fname[0] = '\0';

	for ( j = 1, k = 0; argc > j; j++)
		{
		if ( argv[j][0] == '-' )
			{ /*accept_option*/
			if ( !strncmp(argv[j], "-read", 5) )
				{
				read_only = 1;
				printf("FILE OPENED AS 'READ-ONLY'.\n");
				}
			else if ( !strncmp(argv[j], "-encode", 7) )
				{
				encode_mode = 1;
				printf("ENCODING-MODE:\nPassword: ");
				psswd = (char *) malloc(256);
				scanf("%s", psswd);
				}
			else	printf("%cNO SUCH OPTION AS /%s/\n", EDT$K_BELL, argv[j]);
			} /*accept_option*/
		else	{
			strcpy(fname,argv[j]);
			k++;

			if ( k > 1 )
				{
				printf("Too many files on command-line. Exiting.\n");
				exit(1);
				}
			}
		}

	if ( strstr(fname, ":" ))	/* Check for colon:number on file name. */
		{
		int m;

		j = strlen(fname) - 1;

		while ( fname[j] != ':')
			j--;

		openatlinenum = 0;

		m = j + 1;

		while ( (fname[m] != '\0') && (edt_isnum(fname[m])) )
			openatlinenum = openatlinenum * 10 + fname[m++] - 48;


		if ( fname[m] == '\0')
			fname[j] = '\0';		/* Truncate colon:number off file name. */
		else	openatlinenum = 1;
		}

	if ( fname[0] == '\0')
		strcpy(fname, "noname");

	suffix = strstr(fname,".gz");

	if ( (suffix) && (strlen(suffix) == 3))
		{
		char cmd[4096] = "gunzip ";
		gzipd_file = 1;

		printf(" Temporarily gunzip'ing %s\n", fname );

		strcat( cmd, fname );
		system( cmd );

		suffix[0] = '\0';	/* Trucate the gzip suffix. */
		}

	stat( fname, &file_info );

	if ( S_ISDIR( file_info.st_mode ) )
		{
		printf ("Error: File '%s' is a directory; not a text file.  Cannot edit it.\n", fname );
		exit(1);
		}

	if ( !(infile = fopen(fname, "r")) )
		{
		file_exists = 0;

		printf("Input file '%s' does not exist\n[EOB %s]\n", fname, active_buffer_name);

		tframe_row = last_row = curse_row = last_curse_col = rel_curse_row = rel_curse_col = 0;
		curse_pt = EOB;
		}
	else	{
		file_exists = 1;
		last_row = 0;
		curse_pt = EOB; /* keep inserting at eob */

		if ( (strstr(fname,".scz")) && (strlen(strstr(fname,".scz"))==4) )
			{
			fclose(infile);
			load_compressed_file(fname);
			}
		else	{
			load_file();
			fclose(infile);
			}

		tframe_row = curse_row = last_curse_col = rel_curse_row = rel_curse_col = 0;
		curse_pt = txt_head->nxt;
		changed = 0;
		}

	if ( gzipd_file )
		{
		char cmd[4096] = "gzip ";
		strcat( cmd, fname );
		system( cmd );
		strcat( fname, ".gz" );
		}

	open_journal_file(fname);

	if ( openatlinenum > 1 )
		{
		tmp_pt = curse_pt;
		move_pt_begin_of_line( &tmp_pt );
		printf("Opening at line %d.\n", openatlinenum);
		i = 1;
		curse_pt = txt_head->nxt;

		while ( (i < openatlinenum) && (curse_pt != EOB))
			{
			if (curse_pt->ch == '\n')
				i++;

			curse_pt = curse_pt->nxt;
			}

		if (i != openatlinenum )
			printf("ONLY %d LINES in FILE.\n", i);

		curse_row = i - 1;
		//adjust_screen_parameters();
		}


	do	{ /*line_mode_loop*/
		printf("%d: ", curse_row + 1);
		tmp_pt = curse_pt;
		move_pt_begin_of_line( &tmp_pt );
		spew_line( tmp_pt );
		printf("\n");

		if ( strcmp(active_buffer_name, "main") )
			printf("%s", active_buffer_name);

		printf("*");

		/* scanf("%s",com_line);  ch = getchar(); */
		i = 0;
		do	{
			com_line[i++] = getchar();
		} while ( (com_line[i-1] != '\n') && (com_line[i-1] != '\r') );

		com_line[i-1] = '\0';
		xml_remove_leading_trailing_spaces( com_line );
		printf("%s\n", com_line);
		fprintf(jou_outfile,"%s\n", com_line );

		if (com_line[0]=='\0')
			{
			curse_row = curse_row + 1;
			move_pt_down_line( &curse_pt );

			if (curse_row > last_row)
				curse_row = last_row;
			}
		else	if ( !strcmp(com_line, "c") )
			{ /*Screen_mode*/
			resize(0);

			printf("%c[m%c)B", EDT$K_ESC, EDT$K_ESC);
			printf("%c[1;%dr", EDT$K_ESC, nrows-2 );	/*set scrolling region*/

			system("stty raw");
			system("stty -echo");

			adjust_screen_parameters();
			display_screen(1);

			inpt1 = ctrl = 0;
			ch = getchar();
			fprintf(jou_outfile, "%c", ch);

			/* While ^Z is not pressed. */
			/* This is the main screen-mode editing loop. */
			while (ch != 26)
				{
				handle_key(ch);
				ch = getchar();
				fprintf(jou_outfile,"%c", ch);
				}

			/* Nice Exit (Return terminal screen to nice way) */
			printf("%c[m%c[1;%dr", EDT$K_ESC, EDT$K_ESC, nrows); /* Re-expand scrolling region*/
			printf("%c[%d;1H%c[K", EDT$K_ESC, nrows - 1, EDT$K_ESC );
			printf("%c[%d;1H%cE", EDT$K_ESC, nrows, EDT$K_ESC);
			printf("%c[%d;1H", EDT$K_ESC, nrows-1 );

			system("stty -raw");
			system("stty echo");
			} /*Screen_mode*/
		else if (!strcmp(com_line,"configure_keypad") )
			configure_keyboard();
		else if (com_line[0] == 'q' )
			{
			leave = 1;

			if (changed != 0)
				{
				if ( com_line[1] != '!' )
					{
					printf("Really Quit (y/n) ? ");
					scanf("%s", com_line );

					if (com_line[0] != 'y')
						leave = 0;
					}
				}
			else	printf("No Changes.\n");
			}
		else	if ( !strncmp(com_line, "ex", 2) )
			{
			if ( read_only )
				{
				if ( !changed )
					{
					printf("  FILE WAS READ_ONLY, AND THERE WHERE NO CHANGES.\n");
					printf("  THEREFORE, FILE WAS NOT RE-WRITTEN.\n");
					leave = 1;
					}
				else	printf("%c  FILE WAS OPENED AS 'READ_ONLY'.\n  NO WRITE PERFORMED.\n",EDT$K_BELL);
				}
			else	{
				if ( !changed )
					printf("There were NO changes.\n");

				if ( strcmp(active_buffer_name, "main") )
					{
					printf("\nWARNING:\n You are not exiting from the 'main' buffer.\n");
					printf(" Only the current buffer '%s' will be saved.\n", active_buffer_name );
					printf(" The contents of the 'main' buffer will be lost.\n");
					printf(" Do you really want to exit from this buffer (y/n) ? ");
					scanf("%s",com_line);
					}
				else	com_line[0] = 'y';

				if (com_line[0] == 'y')
					{
					if ( file_exists == 1 )
					 /* Before over-writing file, copy existing file to back-up */
					copy_backup(fname);

					if ( !write_file(fname) )
						leave = 1;
					}
				else	printf("\nExit was averted.  File was NOT WRITTEN.\n\n");
				}
			}
		else if ( !strncmp(com_line, "wpb", 3) )
			{ /* Write paste buffer to file. */
			next_word(com_line,name1,delimiters); next_word(com_line,name1,delimiters);

			if ( name1[0] == '\0')
				printf("Missing file name:  wpb <file_name>%c\n", EDT$K_BELL);
			else	{
				if ( paste_buffer )
					{
					printf("Writing Paste Buffer to File %s\n", name1);
					i = write_buffer(paste_buffer, name1);
					}
				else	printf("Paste Buffer Empty.  No file written.%c\n", EDT$K_BELL );
				}
			}
		else if ( !strncmp(com_line, "wb", 2) )
			{
			char bufname[256];

			next_word(com_line, name1, delimiters);
			next_word(com_line, bufname, delimiters);
			next_word(com_line, name1, delimiters);

			if ( (bufname[0] == '\0') || (name1[0] == '\0') )
				printf("Missing argument:  wb <buffer_name> <file_name>%c\n", EDT$K_BELL);
			else	{
				printf("Writing Buffer: %s to file %s\n", bufname, name1);

				/* Search for existing buffer. */
				tmp_buff_pt = buffer_list;

				while ( (tmp_buff_pt) && (strcmp(tmp_buff_pt->buff_name,bufname)) )
					tmp_buff_pt = tmp_buff_pt->nxt;

				if ( tmp_buff_pt )
					i = write_buffer(tmp_buff_pt->txt_head,name1);
				else	printf("No Buffer called '%s'.  No file written.%c\n", bufname, EDT$K_BELL );
				}
			}
		else	if (com_line[0] == 'w')
			{
			printf("Writing File: ");
			next_word(com_line,name1,delimiters); next_word(com_line,name1,delimiters);

			if ( name1[0] == '\0')
				{
				if (strcmp(active_buffer_name, "main") != 0)
					{
					printf("\nWARNING:\n You are not saving from the 'main' buffer.\n");
					printf(" Only the current buffer '%s' is being written to file '%s'.\n\n", active_buffer_name, fname );
					}

				i=0;
				do	{
					name1[i] = fname[i];
					i++;
					} while ( fname[i-1] != '\0' );

				}

			/* scanf("%s", name1); */
			printf("'%s'\n", name1);
			i = write_file(name1);

			if (com_line[1] == 'q')
				{ /*write-quit*/
				leave = 1;

				if ((changed != 0) && (com_line[2] != '!'))
					{
					printf("Really Quit (y/n) ? "); scanf("%s", com_line );
					if (com_line[0] != 'y') leave = 0;
					}
				else	printf("No Changes.\n");
				} /*write-quit*/
			}
		else if ( (!strncmp(com_line, "rea", 3)) || (!strncmp(com_line, "incl", 4)) )
			{
			char	*suffix;
			int	tmpgzunzip = 0;
			next_word(com_line, name1, delimiters);
			next_word(com_line, name1, delimiters);
			/* scanf("%s", name1); */

			suffix = strstr(name1,".gz");

			if ( (suffix ) && (strlen(suffix) == 3))
				{
				char cmd[4096]="gunzip ";
				printf(" Temporarily gunzip'ing %s\n", fname );
				strcat( cmd, name1 );
				system( cmd );
				suffix[0] = '\0';	/* Trucate the gzip suffix. */
				tmpgzunzip = 1;
				}

			infile = fopen(name1, "r");

			if ( !infile)
				printf("File '%s' NOT FOUND.\n", name1);
			else	{
				printf("File '%s': ", name1);
				i = last_row;  			    /* save number of rows */
				load_file();
				fclose(infile);
				curse_row = curse_row + last_row - i;  /* compute new curse_row */
				adjust_screen_parameters();
				if (tmpgzunzip)
					{
					char cmd[4096] = "gzip ";		/* Restore gzip'd file. */

					strcat( cmd, name1 );
					system( cmd );
					}
				}
			}
		else	if ( !strcmp(com_line, "rk") )		/* Restore Keyboard Map */
			{
			printf("\n Restoring numeric keypad to original (pre-editor) configuration.\n\n");
			restore_keypad_setup();
			}
		else	if ( !strcmp(com_line, "sk") )		/* Restore Keyboard Map */
			{
			printf("\n Restoring numeric keypad to Edt configuration.\n\n");
			get_keypad_setup();
			}
		else	if (edt_isnum(com_line[0]))	/* line number */
			{
			for( i = j = 0; edt_isnum(com_line[i]); i++)
				j = 10 * j + com_line[i] - 48;

			if ( com_line[i] != '\0' )
				printf("NON-NUMERIC CHAR '%c' in line number.\n", com_line[i]);
			else	{
				curse_pt = txt_head->nxt;
				i = 1;

				while ( (i < j) && (curse_pt != EOB) )
					{
					if ( curse_pt->ch == '\n' )
						i++;

					curse_pt = curse_pt->nxt;
					}

				if (i != j)
					printf("ONLY %d LINES in FILE.\n", i);

				curse_row = i - 1;
				adjust_screen_parameters();
				}
			}
		else	if ( com_line[0] == '!' )		/* System escape.  Perform external OS command. */
			{
			com_line[0] = ' ';  /* Remove the leading bang ('!'). */
			system(com_line);
			}
		else	if ( !strncmp(com_line, "ls", 2) ) /* List directory. */
			{
			system(com_line);
			}
		else	if ( !strncmp(com_line, "dir", 3) ) /* List directory, full style. */
			{
			strcpy(name1, com_line);
			strcpy(com_line, "ls -l ");
			strcat(com_line, &(name1[3]) );
			system(com_line);
			}
		else	if ( (!strncmp(com_line, "case", 4)) || (!strncmp(com_line, "cap", 3)) )
			{
			srch_caps = !srch_caps;

			printf("Search will be CAPS %sSENSITIVE\n", srch_caps ? "IN" : "");
			}
		else	if ( !strcmp(com_line, "file") )
			printf("Editing file '%s'.\n", fname );	/* Show the name of file being edited. */
		else	if ( !strcmp(com_line, "help_config") )
			help_keypad_setup();
		else	if ( !strncmp(com_line, "help", 4) )
			help_long();
		else	if (( !strcmp(com_line, "h")) || (!strcmp(com_line, "?")))
			help_quick();
		else	if (!strncmp(com_line, "set", 3) )
			{
			next_word(com_line, name1, delimiters);
			next_word(com_line, name1, delimiters);

			if ( !strncmp(name1, "ro", 2) )
				{
				next_word(com_line, name1, delimiters);

				if (sscanf(name1, "%d", &i) != 1 )
					printf("Expected Integer Number of Rows.\n");
				else	nrows = i;

				printf("Nrows = %d\n", nrows);
				}
			else	if ( !strncmp(name1, "co", 2) )
				{
				next_word(com_line, name1, delimiters);

				if (sscanf(name1, "%d", &i) !=1 )
					printf("Expected Integer Number of Columns (Right Margin).\n");
				else	ncols = i;

				printf("Ncols = %d\n", ncols);
				}
			else	if ( (!strncmp(name1, "ma", 2)) || (!strncmp(name1, "wr", 2)) )
				{
				next_word(com_line, name1, delimiters);

				if (sscanf(name1, "%d", &i) !=1 )
					printf("Expected Integer Margin Number (Right Margin).\n");
				else	right_margin = i;

				printf("Right Margin Formatting Wrap Setting = %d\n", right_margin);
				}
			else	printf("Unknown set /%s/\n", name1);
			}
		else	if ( (com_line[0] == 's') && (strncmp(com_line, "start", 5)))   /*Substitute command*/
			{
			/* Expect s/srch_strng/rplcmnt_strng/w  or line range in brackets. */
			substitute_command( com_line );
			}
		else	if ( com_line[0] == '=' )  /*switch buffer*/
		{
		/* First, store away current buffer's parameters on buffer list. */
		tmp_buff_pt = buffer_list;

		while ( (tmp_buff_pt->nxt) && (strcmp(tmp_buff_pt->buff_name, active_buffer_name)) )
			tmp_buff_pt = tmp_buff_pt->nxt;

		tmp_buff_pt->txt_head = txt_head;
		tmp_buff_pt->curse_pt = curse_pt;
		tmp_buff_pt->EOB = EOB;
		tmp_buff_pt->curse_row = curse_row;
		tmp_buff_pt->last_row = last_row;

		/* Now get the new active buffer name. */
		for (i = 0; com_line[i+1] != '\0'; i++ )
			active_buffer_name[i] = com_line[i + 1];

		active_buffer_name[i] = '\0';

		/* Chop away any white-space surrounding it. */
		while ((active_buffer_name[0] == ' ') || (active_buffer_name[0] == '\t') )
			{
			for (i = 0; active_buffer_name[i] != '\0'; i++)
				active_buffer_name[i] = active_buffer_name[i+1];
			}

		for(i = 0; (active_buffer_name[i] !=' ') && (active_buffer_name[i]!='\t') && (active_buffer_name[i] != '\0'); i++)

		active_buffer_name[i] = '\0';

		/* Search for existing buffer. */
		tmp_buff_pt = buffer_list;
		while ( (tmp_buff_pt->nxt !=0 ) && (strcmp(tmp_buff_pt->buff_name,active_buffer_name) != 0) )
			tmp_buff_pt = tmp_buff_pt->nxt;

		if ( strcmp(tmp_buff_pt->buff_name,active_buffer_name) )  /*Then new buffer.*/
			{
			printf("Establishing new buffer '%s'.\n", active_buffer_name );

			tmp_buff_pt = (struct __buf_lis__ *)malloc(sizeof(struct __buf_lis__));
			strcpy(tmp_buff_pt->buff_name,active_buffer_name);

			tmp_buff_pt->nxt = buffer_list;
			buffer_list = tmp_buff_pt;

			tmp_buff_pt->curse_row = 0;
			tmp_buff_pt->last_row = 0;

			tmp_buff_pt->EOB = new_ch();
			tmp_buff_pt->EOB->nxt = 0;
			tmp_buff_pt->EOB->ch = 0;

			tmp_buff_pt->curse_pt = tmp_buff_pt->EOB;
			tmp_buff_pt->txt_head = new_ch();
			tmp_buff_pt->txt_head->prv = 0;
			tmp_buff_pt->txt_head->nxt = tmp_buff_pt->EOB;
			tmp_buff_pt->txt_head->ch = 'B';
			tmp_buff_pt->EOB->prv = tmp_buff_pt->txt_head;
			}

		txt_head = tmp_buff_pt->txt_head;
		curse_pt = tmp_buff_pt->curse_pt;
		EOB = tmp_buff_pt->EOB;
		curse_row = tmp_buff_pt->curse_row;
		last_row = tmp_buff_pt->last_row;
		mark_pt1 = 0;  Mark = 0;
		}
	else	if ( !strncmp(com_line, "list", 4) )
		{
		printf("\n Buffer Name List:\n");

		for (tmp_buff_pt = buffer_list; tmp_buff_pt; tmp_buff_pt = tmp_buff_pt->nxt)
			printf("			%s\n", tmp_buff_pt->buff_name);

		printf("\n");
		}
	else	if ( !strncmp(com_line, "resi", 4) )
		resize(1);
	else	if ( !strncmp(com_line, "encr", 4) )
		{
		if (encode_mode)
			{
			free(psswd);
			encode_mode = 0;
			printf("Unencoded-mode:\n");
			}
		else	{
			encode_mode = 1;  printf("ENCODING-MODE:\nEnter Encode Password: ");
			psswd = (char *)malloc(256);
			scanf("%s", psswd);
			}
		}
	else	if ( (!strncmp(com_line, "begin", 3)) || (!strncmp(com_line, "start", 5)) )
		{ /*insert-a-program-shell*/
		next_word(com_line, name1, " \t,:="); /* Skip past 'begin'. */
		next_word(com_line, name1, " \t,:="); /* Pull out programming language name. */
		jj = last_row;  /* save number of rows */

		if (strncmp(name1,"c++prog", 2) == 0)
			{
			printf(" Inserting beginning lines of new C++ program.\n");
			insert_string( "#include <iostream.h>\nusing namespace std;\n\n\n" );
			insert_string( "int main( int argc, char *argv[] )\n{\n\n\n return 0;\n}\n" );
			}
		else	if (!strncmp(name1, "cprog", 1) )
			{
			printf(" Inserting beginning lines of new C program.\n");
			insert_string( "#include\t<stdio.h>\n" );
			insert_string( "#include\t<string.h>\n" );
			insert_string( "#include\t<stdlib.h>\n\n\n" );
			insert_string( "int main( int argc, char *argv[] )\n{\n\n\n\t return 0;\n}\n" );
			}
		else	if ( !strncmp(name1, "javaprog", 3) )
			{
			printf(" Inserting beginning lines of new Java program.\n");
			strcpy(name1,fname);	/* Pick class name as file-name, minus suffix. */
			for (j = 0; (name1[j] != '\0') && (name1[j] != '.'); j++);

			if (name1[j] == '.')
				name1[j] = '\0';

			insert_string( "class ");
			insert_string(name1);
			insert_string("\n{\n");
			insert_string( "  public static void main(String args[])\n  {\n\n\n  }\n}\n" );
			}
		else	if ( !strncmp(name1, "html", 3) )
			{
			printf(" Inserting beginning lines of new HTML page.\n");
			insert_string( "<html>\n <head>\n  <title></title>\n </head>\n" );
			insert_string( " <body bgcolor = \"#f0f0e4\">\n\n\n\n\n\n\n </body>\n</html>\n" );
			}
		else	printf("Unknown language '%s'\n", name1);

		curse_row = curse_row + last_row - jj;  /* compute new curse_row */
		adjust_screen_parameters();
		} /*insert-a-program-shell*/
	else	if ( com_line[0] != '\0' )
		printf("Unknown command /%s/\n", com_line);

		} while (!leave);	/* Continue interpretting commands while not 'leave'. */



	remove_journal_file(fname);


	return 0;
}
