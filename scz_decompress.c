/****************************************************************************/ 
/* SCZ_Decompress.c - Decompresses what SCZ_Compress.c produces.
   A simple compression/decompression algorithm/utility.

  Compile:
    cc -O scz_decompress.c -o scz_decompress

  Usage:
    scz_decompress  file.dat.scz		-- Produces "file.dat"
  or:
    scz_decompress  file.dat.scz  file2		-- Produces "file2"
*****************************************************************************/

#include <stdio.h>

struct scz_item		/* Data structure for holding buffer items. */
 {
   char ch;
   struct scz_item *nxt;
 };

/*-----------------------*/
/* Add item to a buffer. */
/*-----------------------*/
void scz_add_item( struct scz_item **hd, struct scz_item **tl, char ch )
{
 struct scz_item *tmppt;
 tmppt = (struct scz_item *)malloc(sizeof(struct scz_item));
 tmppt->ch = ch;
 tmppt->nxt = 0;
 if (*hd==0) *hd = tmppt; else (*tl)->nxt = tmppt;
 *tl = tmppt;
}


/****************************************************************/
/* Decompress - Decompress a buffer.  Returns 1 on success, 0 	*/
/*  if failure.							*/
/****************************************************************/
int scz_decompress( struct scz_item **buffer0_hd )
{
 char forcingchar, marker[257], phrase[256][256];
 int i, j, k, nreplace=40, nreplaced, replaced, iterations, iter;
 struct scz_item *bufpt, *tmpbuf;
 char ch;

 /* Expect magic number(s). */
 bufpt = *buffer0_hd;
 if ((bufpt==0) || (bufpt->ch!=101))
  {printf("Error1: This does not look like a compressed file.\n"); return(0);}
 bufpt = bufpt->nxt;
 if ((bufpt==0) || (bufpt->ch!=99))
  {printf("Error2: This does not look like a compressed file.\n");  return(0);}
 bufpt = bufpt->nxt;

 /* Get the compression iterations count. */
 iterations = 255 & bufpt->ch;
 bufpt = bufpt->nxt;
 *buffer0_hd = bufpt;

 for (iter=0; iter<iterations; iter++)
  { /*iter*/
    bufpt = *buffer0_hd;
    forcingchar = bufpt->ch;	bufpt = bufpt->nxt;
    nreplaced = 255 & (int)(bufpt->ch);	  bufpt = bufpt->nxt;
    for (j=0; j<nreplaced; j++)   /* Accept the markers and phrases. */
     {
      marker[j] =    bufpt->ch;	bufpt = bufpt->nxt;
      phrase[j][0] = bufpt->ch;	bufpt = bufpt->nxt;
      phrase[j][1] = bufpt->ch;	bufpt = bufpt->nxt;
     }
    ch = bufpt->ch; 		bufpt = bufpt->nxt;
    if (ch!=91) /* Boundary marker. */
      {
	printf("Error3: Corrupted compressed file. (%d)\n", 255 & (int)ch); 
	return(0);
      }

    /* Remove the header. */
    *buffer0_hd = bufpt;

   /* Replace chars. */
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
       j = 0;
       while ((j<nreplaced) && (bufpt->ch != marker[j])) j++;
       if (j<nreplaced)
        {	/* If match, insert the phrase. */
  	  bufpt->ch = phrase[j][0];
          tmpbuf = (struct scz_item *)malloc(sizeof(struct scz_item));
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


/************************************************/
/*  Main  - Simple DeCompression Utility. (SCZ) */
/************************************************/
main( int argc, char *argv[] )
{
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt, *bufprv;
 FILE *infile=0, *outfile;
 char fname[3][1024], ch;
 int i, j, k, verbose=0, success, sz1=0, sz2=0;

 /* Get the command-line arguments. */
 j = 1;  k = 0;  fname[1][0] = '\0';
 while (argc>j)
  { /*argument*/
   if (argv[j][0]=='-')
    { /*optionflag*/
     if (strcasecmp(argv[j],"-v")==0) verbose = 1;
     else {printf("\nERROR:  Unknown command line option /%s/.\n", argv[j]); exit(0); }
    } /*optionflag*/
   else
    { /*file*/
     strcpy(fname[k],argv[j]);
     if (k==0)
      { 
	infile = fopen(fname[k],"rb");
	if (infile==0) {printf("ERROR: Cannot open input file '%s'.  Exiting\n", fname[k]); exit(1);}
      }
     else
     if (k>1) { printf("\nERROR: Too many file names on command line.\n"); exit(0); }
     k = k + 1;
    } /*file*/
   j = j + 1;
  } /*argument*/
 if (infile==0) { printf("Error: Missing file name. Exiting."); exit(0); }

 /* Allow user to specify an optional destination file name. */
 if (fname[1][0] == '\0')
  {
    /* Find final '.' or position to end of name */
    strcpy(fname[1],fname[0]);
    j = strlen(fname[1])-1;
    while ((fname[1][j]!='.') && (j>0)) j = j - 1;
    if (fname[1][j]!='.') j = strlen(fname[1]);
    if (strcmp(&(fname[1][j]),".scz")==0)
      fname[1][j] = '\0'; 
    else
     { fname[1][j] = '\0';  strcat(fname[1], ".uscz"); }
  }
 if (strcmp(fname[0],fname[1])==0) {printf("ERROR: Attempt to write over source file. Exiting\n"); exit(1);}
 
 /* Read file into linked list. */
 ch = getc(infile);  bufprv = 0;
 while (!feof(infile))
  {
   bufprv = buffer0_tl;
   scz_add_item( &buffer0_hd, &buffer0_tl, ch );
   sz1++;
   ch = getc(infile);
  }
 fclose(infile);

 if (buffer0_tl==0) {printf("Empty file.\n"); exit(0);}
 if (buffer0_tl->ch!=']') {printf("Error4: Reading compressed file. (%x)\n", ch);  exit(0); }
 bufprv->nxt = 0;	/* Remove the end marker. */
 buffer0_tl = bufprv;

 success = scz_decompress( &buffer0_hd );	/* Decompress the buffer !!! */
 if (!success) exit(0);

/* Write the file the decompressed out. */
 printf("\n Writing output to file %s\n", fname[1]);
 outfile = fopen(fname[1],"wb");
 if (outfile==0) {printf("ERROR: Cannot open output file '%s' for writing.  Exiting\n", fname[1]); exit(1);}
 sz2 = 0;
 bufpt = buffer0_hd;
 while (bufpt!=0)
  {
   fputc( bufpt->ch, outfile );
   sz2++;
   bufpt = bufpt->nxt;
  }
 fclose(outfile);
 printf("Decompression ratio = %g\n", (float)sz2 / (float)sz1 );
}
