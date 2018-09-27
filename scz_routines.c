/************************************************************************
 * SCZ.h - Common header definitions for SCZ compression routines.	*
 ************************************************************************/

#ifndef	SCZ_DEFS
#define	SCZ_DEFS	1

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#define	SCZ_MAX_BUF	16777215
#define	SCZFREELSTSZ	2048
#define	nil		0
int	sczbuflen = 4 * 1048576;

struct scz_item		/* Data structure for holding buffer items. */
{
	unsigned char ch;
	struct scz_item *nxt;
} *sczfreelist1 = NULL;

struct scz_amalgam	/* Data structure for holding markers and phrases. */
{
	unsigned char phrase[2];
	int value;
};

struct scz_item2		/* Data structure for holding buffer items with index. */
{
	unsigned char ch;
	int j;
	struct scz_item2 *nxt;
} *sczphrasefreq[256], *scztmpphrasefreq, *sczfreelist2 = NULL;





struct scz_item *new_scz_item()
{
int j;
struct scz_item *tmppt;

	if ( !sczfreelist1 )
		{
		sczfreelist1 = (struct scz_item *) malloc(SCZFREELSTSZ * sizeof(struct scz_item));

		tmppt = sczfreelist1;

		for (j = SCZFREELSTSZ - 1; j; j--)
			{
			tmppt->nxt = tmppt + 1;	/* Pointer arithmetic. */
			tmppt = tmppt->nxt;
			}

		tmppt->nxt = NULL;
		}


	tmppt = sczfreelist1;
	sczfreelist1 = sczfreelist1->nxt;

	return tmppt;
}

void sczfree( struct scz_item *tmppt )
{
	tmppt->nxt = sczfreelist1;
	sczfreelist1 = tmppt;
}

struct scz_item2 *new_scz_item2()
{
int	j;
struct scz_item2 *tmppt;

	if ( !sczfreelist2 )
		{
		sczfreelist2 = (struct scz_item2 *) malloc(SCZFREELSTSZ * sizeof(struct scz_item2));
		tmppt = sczfreelist2;

		for (j = SCZFREELSTSZ-1; j; j--)
			{
			tmppt->nxt = tmppt + 1;	/* Pointer arithmetic. */
			tmppt = tmppt->nxt;
			}

		tmppt->nxt = nil;
		}

	tmppt = sczfreelist2;
	sczfreelist2 = sczfreelist2->nxt;

	return tmppt;
}

void sczfree2( struct scz_item2 *tmppt )
{
	tmppt->nxt = sczfreelist2;
	sczfreelist2 = tmppt;
}


/*-----------------------*/
/* Add item to a buffer. */
/*-----------------------*/
void scz_add_item( struct scz_item **hd, struct scz_item **tl, unsigned char ch )
{
struct scz_item *tmppt;

	tmppt = new_scz_item();
	tmppt->ch = ch;
	tmppt->nxt = 0;

	if (*hd == 0)
		*hd = tmppt;
	else	(*tl)->nxt = tmppt;

	*tl = tmppt;
}




/****************************************************************************/
/* SCZ_Decompress_lib.c - Decompress what SCZ_Compress.c compresses.
   Simple decompression algorithms for SCZ compressed data.

   This file contains the following user-callable routines:
     Scz_Decompress_File( *infilename, *outfilename );
     Scz_Decompress_File2Buffer( *infilename, **outbuffer, *M );
     Scz_Decompress_Buffer2Buffer( *inbuffer, *N, **outbuffer, *M );
  See below for formal definitions and comments.

 SCZ_Compress - LGPL License:
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

  For updates/info, see:  http://sourceforge.net/projects/scz-compress/

  Carl Kindman 11-21-2004     carlkindman@yahoo.com
		7-5-2005        Added checksums and blocking.
*****************************************************************************/



/****************************************************************/
/* Decompress - Decompress a buffer.  Returns 1 on success, 0 	*/
/*  if failure.							*/
/****************************************************************/
int Scz_Decompress_Seg( struct scz_item **buffer0_hd )
{
 unsigned char forcingchar, marker[257], phrase[256][256];
 int i, j, k, nreplace=40, nreplaced, replaced, iterations, iter, markerlist[256];
 struct scz_item *bufpt, *tmpbuf;
 unsigned char ch;

 /* Expect magic number(s). */
 bufpt = *buffer0_hd;
 if ((bufpt==0) || (bufpt->ch!=101))
  {printf("Error1a: This does not look like a compressed file.\n"); return 0;}
 bufpt = bufpt->nxt;
 if ((bufpt==0) || (bufpt->ch!=98))
  {printf("Error2a: This does not look like a compressed file.\n");  return 0;}
 bufpt = bufpt->nxt;

 /* Get the compression iterations count. */
 iterations = bufpt->ch;
 bufpt = bufpt->nxt;
 *buffer0_hd = bufpt;

 for (iter=0; iter<iterations; iter++)
  { /*iter*/
    bufpt = *buffer0_hd;
    forcingchar = bufpt->ch;	bufpt = bufpt->nxt;
    nreplaced = bufpt->ch;	bufpt = bufpt->nxt;
    for (j=0; j<nreplaced; j++)   /* Accept the markers and phrases. */
     {
      marker[j] =    bufpt->ch;	bufpt = bufpt->nxt;
      phrase[j][0] = bufpt->ch;	bufpt = bufpt->nxt;
      phrase[j][1] = bufpt->ch;	bufpt = bufpt->nxt;
     }
    ch = bufpt->ch; 		bufpt = bufpt->nxt;
    if (ch!=91) /* Boundary marker. */
     {
      printf("Error3: Corrupted compressed file. (%d)\n", ch);
      return 0;
     }

    for (j=0; j!=256; j++) markerlist[j] = nreplaced;
    for (j=0; j!=nreplaced; j++) markerlist[ marker[j] ] = j;


    /* Remove the header. */
    *buffer0_hd = bufpt;

   /* Replace chars. */
   if (nreplaced>0)
    while (bufpt!=0)
    {
     if (bufpt->ch == forcingchar)
      {
	bufpt->ch = bufpt->nxt->ch;	/* Remove the forcing char. */
	bufpt->nxt = bufpt->nxt->nxt;
	bufpt = bufpt->nxt;
      }
     else
      { /* Check for match to marker characters. */
       j = markerlist[ bufpt->ch ];
       if (j<nreplaced)
	{	/* If match, insert the phrase. */
	  bufpt->ch = phrase[j][0];
	  tmpbuf = new_scz_item();
	  tmpbuf->ch = phrase[j][1];
	  tmpbuf->nxt = bufpt->nxt;
	  bufpt->nxt = tmpbuf;
	  bufpt = tmpbuf->nxt;
	}
       else bufpt = bufpt->nxt;
      }
    }
  } /*iter*/
 return 1;
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
int Scz_Decompress_File2Buffer( char *infilename, char **outbuffer, int *M )
{
 int j, k, success, sz1=0, sz2=0, buflen, continuation, totalin=0, totalout=0;
 unsigned char ch, chksum, chksum0;
 struct scz_item *buffer0_hd, *buffer0_tl, *bufpt, *bufprv, *sumlst_hd=0, *sumlst_tl=0;
 FILE *infile=0;

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
 *outbuffer = (char *)malloc((totalout)*sizeof(char));
 sz2 = 0;
 while (sumlst_hd!=0)
  {
   bufpt = sumlst_hd;
   (*outbuffer)[sz2++] = bufpt->ch;
   sumlst_hd = bufpt->nxt;
   sczfree(bufpt);
  }
 if (sz2 > totalout) printf("Unexpected overrun error\n");

 *M = totalout;

 fclose(infile);
 printf("Decompression ratio = %g\n", (float)totalout / (float)totalin );
 return 1;
}








/*************************************************************************/
/* SCZ_Compress_Lib.c - Compress files or buffers.  Simple compression.	*/
/*
/  This file contains the following user-callable routines:
/    Scz_Compress_File( *infilename, *outfilename );
/    Scz_Compress_Buffer2File( *buffer, N, *outfilename );
/    Scz_Compress_Buffer2Buffer( *inbuffer, N, **outbuffer, *M, lastbuf_flag );
/
/  See below for formal definitions.

  SCZ Method:
   Finds the most frequent pairs, and least frequent chars.
   Uses the least frequent char as 'forcing-char', and other infrequent
   char(s) as replacement for most frequent pair(s).

   Advantage of working with pairs:  A random access incidence array
   can be maintained and rapidly searched.
   Repeating the process builds effective triplets, quadruplets, etc..

   At each stage, we are interested only in knowing the least used
   character(s), and the most common pair(s).

   Process for pairs consumes: 256*256 = 65,536 words or 0.26-MB.
   (I.E. Really nothing on modern computers.)

   Process could be expanded to triplets, with array of 16.7 words or
   67 MB.  Recommend going to trees after that.  But have not found
   advantages to going above pairs. On the contrary, pairs are faster
   to search and allow lower granularity replacement (compression).
 ---

 SCZ_Compress - LGPL License:
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

  For updates/info, see:  http://sourceforge.net/projects/scz-compress/

  Carl Kindman 11-21-2004     carlkindman@yahoo.com
		7-5-2005	Added checksums and blocking.
*************************************************************************/

int *scz_freq2=0;


/*------------------------------------------------------------*/
/* Add an item to a value-sorted list of maximum-length N.    */
/* Sort from largest to smaller values.  (A descending list.) */
/*------------------------------------------------------------*/
void scz_add_sorted_nmax( struct scz_amalgam *list, unsigned char *phrase, int lngth, int value, int N )
{
 int j, k=0, m;

 while ((k<N) && (list[k].value >= value)) k++;
 if (k==N) return;
 j = N-2;
 while (j>=k)
  {
   list[j+1].value = list[j].value;
   for (m=0; m<lngth; m++) list[j+1].phrase[m] = list[j].phrase[m];
   j--;
  }
 list[k].value = value;
 for (j=0; j<lngth; j++) list[k].phrase[j] = phrase[j];
}


/*------------------------------------------------------------*/
/* Add an item to a value-sorted list of maximum-length N.    */
/* Sort from smaller to larger values.  (An ascending list.)  */
/*------------------------------------------------------------*/
void scz_add_sorted_nmin( struct scz_amalgam *list, unsigned char *phrase, int lngth, int value, int N )
{
 int j, k=0, m;

 while ((k<N) && (list[k].value <= value)) k++;
 if (k==N) return;
 j = N-2;
 while (j>=k)
  {
   list[j+1].value = list[j].value;
   for (m=0; m<lngth; m++) list[j+1].phrase[m] = list[j].phrase[m];
   j--;
  }
 list[k].value = value;
 for (j=0; j<lngth; j++) list[k].phrase[j] = phrase[j];
}


/*----------------------------------------------------------------------*/
/* Analyze a buffer to determine the frequency of characters and pairs. */
/*----------------------------------------------------------------------*/
void scz_analyze( struct scz_item *buffer0_hd, int *freq1, int *freq2 )
{
 int j, k;
 struct scz_item *ptr;

 memset( freq1, 0, sizeof(int)*256 );
 memset( freq2, 0, sizeof(int)*256*256 );
 ptr = buffer0_hd;
 if (ptr==0) return;
 k = ptr->ch;
 freq1[k]++;
 ptr = ptr->nxt;
 while (ptr!=0)
  {
   j = k;
   k = ptr->ch;
   freq1[k]++;
   freq2[j*256+k]++;
   ptr = ptr->nxt;
  }
}




/*------------------------------------------------------*/
/* Compress a buffer, step.  Called iteratively.	*/
/*------------------------------------------------------*/
int scz_compress_iter( struct scz_item **buffer0_hd )
{
 int nreplace=250;
 int freq1[256], markerlist[256];
 int i, j, k, replaced, nreplaced, sz3=0, saved=0, saved_pairfreq[256], saved_charfreq[257];
 struct scz_item *head1, *head2, *bufpt, *tmpbuf, *cntpt;
 unsigned char word[10], forcingchar;
 struct scz_amalgam char_freq[257], phrase_freq_max[256];
 FILE *outfile;

 /* Examine the buffer. */
 /* Determine histogram of character usage, and pair-frequencies. */
 if (scz_freq2==0) scz_freq2 = (int *)malloc(256*256*sizeof(int));
 scz_analyze( *buffer0_hd, freq1, scz_freq2 );

 /* Initialize rank vectors. */
 memset( saved_pairfreq, 0, 256 * sizeof(int) );
 memset( saved_charfreq, 0, 256 * sizeof(int) );
 for (k=0; k<256; k++)
  {
   char_freq[k].value = 1073741824;
   phrase_freq_max[k].value = 0;
  }

 /* Sort and rank chars. */
 for (j=0; j!=256; j++)
  {
   word[0] = j;
   scz_add_sorted_nmin( char_freq, word, 1, freq1[j], nreplace+1 );
  }

 /* Sort and rank pairs. */
 i = 0;
 for (k=0; k!=256; k++)
  for (j=0; j!=256; j++)
   if (scz_freq2[j*256+k]!=0)
    {
     word[0] = j;  word[1] = k;
     scz_add_sorted_nmax( phrase_freq_max, word, 2, scz_freq2[j*256+k], nreplace );
    }

 /* Use the least-used character(s) for special expansion symbol(s). I.E. "markers". */
 /* Now go through, and replace any instance of the max_pairs_phrase's with the respective markers. */
 /* And insert before any natural occurrences of the markers, the forcingchar. */
 /*  These two sets should be mutually exclusive, so it should not matter which order this is done. */

 head1 = 0;	head2 = 0;
 forcingchar = char_freq[0].phrase[0];
 k = -1;  j = 0;
 while ((j<nreplace) && (char_freq[j+1].value < phrase_freq_max[j].value - 3))
  { j++;   k = k + phrase_freq_max[j].value - char_freq[j+1].value; }
 nreplaced = j;

 if (nreplaced == 0) return 0; /* Not able to compress this data further with this method. */

 /* Now go through the buffer, and make the following replacements:
    - If equals forcingchar or any of the other maker-chars, then insert forcing char in front of them.
    - If the next pair match any of the frequent_phrases, then replace the phrase by the corresponding marker.
 */


/* Define a new array, phrase_freq_max2[256], which is a lookup array for the first character of
   a two-character phrase.  Each element points to a list of second characters with "j" indices.
   First chars which have none are null, stopping the search immediately.
*/
memset( sczphrasefreq, 0, 256 * sizeof(struct scz_item2 *) );
for (j=0; j!=256; j++) markerlist[j] = nreplaced+1;
for (j=0; j!=nreplaced+1; j++) markerlist[ char_freq[j].phrase[0] ] = j;
for (j=0; j!=nreplaced; j++)
 if (phrase_freq_max[j].value>0)
  {
   scztmpphrasefreq = new_scz_item2();
   scztmpphrasefreq->ch = phrase_freq_max[j].phrase[1];
   scztmpphrasefreq->j = j;
   k = phrase_freq_max[j].phrase[0];
   scztmpphrasefreq->nxt = sczphrasefreq[k];
   sczphrasefreq[k] = scztmpphrasefreq;
  }


 /* First do a tentative check. */
 bufpt = *buffer0_hd;
 if (nreplaced>0)
 while (bufpt!=0)
  {
   replaced = 0;
   if (bufpt->nxt!=0)
    {  /* Check for match to frequent phrases. */
     k = bufpt->ch;
     if (sczphrasefreq[k]==0) j = nreplaced;
     else
      { unsigned char tmpch;
	tmpch = bufpt->nxt->ch;
	scztmpphrasefreq = sczphrasefreq[k];
	while ((scztmpphrasefreq!=0) && (scztmpphrasefreq->ch != tmpch)) scztmpphrasefreq = scztmpphrasefreq->nxt;
	if (scztmpphrasefreq==0) j = nreplaced; else j = scztmpphrasefreq->j;
      }

     if (j<nreplaced)
      { /* If match, the phrase would be replaced with corresponding marker. */
	saved++;
	saved_pairfreq[j]++;		/* Keep track of how many times this phrase occured. */
	bufpt = bufpt->nxt->nxt;	/* Skip over. */
	replaced = 1;
      }
    }
   if (!replaced)
    {  /* Check for match of marker characters. */
     j = markerlist[ bufpt->ch ];
     if (j<=nreplaced)
      {	/* If match, insert forcing character. */
	saved--;
	saved_charfreq[j]--;		/* Keep track of how many 'collisions' this marker-char caused. */
      }
     bufpt = bufpt->nxt;
    }
  }

 // printf("%d saved\n", saved);
 if (saved<=1) return 0; /* Not able to compress this data further with this method. Buffer unchanged. */

 /* Now we know which marker/phrase combinations do not actually pay. */
 /* Reformulate the marker list with reduced set. */
 /* The least frequent chars become the forcing char and the marker characters. */
 /* Store out the forcing-char, markers and replacement phrases after a magic number. */
 scz_add_item( &head1, &head2, char_freq[0].phrase[0] );	/* First add forcing-marker (escape-like) value. */
 scz_add_item( &head1, &head2, 0 );	/* Next, leave place-holder for header-symbol-count. */
 cntpt = head2;
 k = 0;  saved = 0;
 for (j=0; j<nreplaced; j++)
  if (saved_pairfreq[j] + saved_charfreq[j+1] > 3)
   { unsigned char ch;
    saved = saved + saved_pairfreq[j] + saved_charfreq[j+1] - 3;
    ch = char_freq[j+1].phrase[0];
    scz_add_item( &head1, &head2, ch );  /* Add phrase-marker. */
    char_freq[k+1].phrase[0] = ch;

    ch = phrase_freq_max[j].phrase[0];
    scz_add_item( &head1, &head2, ch );	 /* Add phrase 1st char. */
    phrase_freq_max[k].phrase[0] = ch;

    ch = phrase_freq_max[j].phrase[1];
    scz_add_item( &head1, &head2, ch );	 /* Add phrase 2nd char. */
    phrase_freq_max[k].phrase[1] = ch;
    k++;
   }
 saved = saved + saved_charfreq[0];
 if ((k == 0) || (saved < 6))
  { /* Not able to compress this data further with this method. Leave buffer basically unchanged. */
   /* Free the old lists. */
   for (j=0; j!=256; j++)
    while (sczphrasefreq[j] != 0)
     { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }
   return 0;
  }

 sz3 = 3*(k+1);
 cntpt->ch = k;		/* Place the header-symbol-count. */
 nreplaced = k;
 scz_add_item( &head1, &head2, 91 );	/* Magic barrier. */

 /* Update the phrase_freq tree. */
 /* First free the old list. */
 for (j=0; j!=256; j++)
  while (sczphrasefreq[j] != 0)
   { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }
 /* Now make the new list. */
 for (j=0; j!=nreplaced; j++)
  if (phrase_freq_max[j].value>0)
   {
    scztmpphrasefreq = new_scz_item2();
    scztmpphrasefreq->ch = phrase_freq_max[j].phrase[1];
    scztmpphrasefreq->j = j;
    k = phrase_freq_max[j].phrase[0];
    scztmpphrasefreq->nxt = sczphrasefreq[k];
    sczphrasefreq[k] = scztmpphrasefreq;
   }

for (j=0; j!=256; j++) markerlist[j] = nreplaced+1;
for (j=0; j!=nreplaced+1; j++) markerlist[ char_freq[j].phrase[0] ] = j;
// printf("Replacing %d\n", nreplaced);

 bufpt = *buffer0_hd;
 while (bufpt!=0)
  {
   sz3++;
   replaced = 0;
   if (bufpt->nxt!=0)
    {  /* Check for match to frequent phrases. */
     k = bufpt->ch;
     if (sczphrasefreq[k]==0) j = nreplaced;
     else
      { unsigned char tmpch;
	tmpch = bufpt->nxt->ch;
	scztmpphrasefreq = sczphrasefreq[k];
	while ((scztmpphrasefreq!=0) && (scztmpphrasefreq->ch != tmpch)) scztmpphrasefreq = scztmpphrasefreq->nxt;
	if (scztmpphrasefreq==0) j = nreplaced; else j = scztmpphrasefreq->j;
      }

     if (j<nreplaced)
      { /* If match, replace phrase with corresponding marker. */
	bufpt->ch = char_freq[j+1].phrase[0];
	tmpbuf = bufpt->nxt;
	bufpt->nxt = tmpbuf->nxt;
	sczfree( tmpbuf );
	replaced = 1;
      }
    }
   if (!replaced)
    {  /* Check for match of marker characters. */
     j = markerlist[ bufpt->ch ];
     if (j<=nreplaced)
      {	/* If match, insert forcing character. */
       tmpbuf = new_scz_item();
       tmpbuf->ch = bufpt->ch;
       tmpbuf->nxt = bufpt->nxt;
       bufpt->nxt = tmpbuf;
       bufpt->ch = forcingchar;
       bufpt = tmpbuf;
       sz3 = sz3 + 1;
      }
    }

   bufpt = bufpt->nxt;
  }

 /* Now attach header to main buffer. */
 head2->nxt = *buffer0_hd;
 *buffer0_hd = head1;

 /* Free the old list. */
 for (j=0; j!=256; j++)
  while (sczphrasefreq[j] != 0)
   { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }

 return sz3;
}





/*******************************************************************/
/* Scz_Compress - Compress a buffer.  Entry-point to Scz_Compress. */
/*  Compresses the buffer passed in.				   */
/* Pass-in the buffer and its initial size, as sz1. 		   */
/* Returns 1 on success, 0 on failure.				   */
/*******************************************************************/
int Scz_Compress_Seg( struct scz_item **buffer0_hd, int sz1 )
{
 struct scz_item *tmpbuf_hd=0, *tmpbuf_tl=0;
 int sz2, iter=0;

 /* Compress. */
 sz2 = scz_compress_iter( buffer0_hd );
 while ((sz2!=0) && (iter<255))
  {
   sz1 = sz2;
   sz2 = scz_compress_iter( buffer0_hd );
   iter++;
  }

 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, 101 );   /* Place magic start-number(s). */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, 98 );
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, iter );  /* Place compression count. */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, sz1>>16 );         /* Place size count (MSB). */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, (sz1>>8) & 255 );  /* Place size count. */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, sz1 & 255 );       /* Place size count (LSB). */

 tmpbuf_tl->nxt = *buffer0_hd;
 *buffer0_hd = tmpbuf_hd;
 return 1;
}




/************************************************************************/
/* Scz_Compress_Buffer2File - Compresses character array input buffer	*/
/*  to an output file.  This routine is handy for applications wishing	*/
/*  to compress their output directly while saving to disk.		*/
/*  First argument is input array, second is the array's length, and	*/
/*  third is the output file name to store to.				*/
/*  (It also shows how to compress and write data out in segments for	*/
/*   very large data sets.)						*/
/*									*/
/************************************************************************/
int Scz_Compress_Buffer2File( unsigned char *buffer, int N, char *outfilename )
{
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt;
 int sz1=0, sz2=0, szi, success=1, flen, buflen;
 unsigned char ch, chksum;
 FILE *outfile=0;

 outfile = fopen(outfilename,"wb");
 if (outfile==0) {printf("ERROR: Cannot open output file '%s' for writing.  Exiting\n", outfilename); exit(1);}

 buflen = N / sczbuflen + 1;
 buflen = N / buflen + 1;
 if (buflen>=SCZ_MAX_BUF) {printf("Error: Buffer length too large.\n"); exit(0);}

 while (sz1 < N)
  { /*outerloop*/

    chksum = 0;  szi = 0;
    if (N-sz1 < buflen) buflen = N-sz1;

    /* Convert buffer into linked list. */
    buffer0_hd = 0;   buffer0_tl = 0;
    while (szi < buflen)
     {
      scz_add_item( &buffer0_hd, &buffer0_tl, buffer[sz1] );
      chksum += buffer[sz1];
      sz1++;  szi++;
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
 fclose(outfile);

 printf("Initial size = %d,  Final size = %d\n", sz1, sz2);
 printf("Compression ratio = %g : 1\n", (float)sz1 / (float)sz2 );
 free(scz_freq2);
 scz_freq2 = 0;
 return success;
}

#endif
