/*

  binsearch.c - general binary search functions

*/

#define _CRT_SECURE_NO_DEPRECATE 1
#include <stdio.h>
#include <string.h>

/* static char *Id = "$Id: binsrch.c,v 1.3 2007/10/07 15:13:16 magodra Exp $"; */

/* Binary search - looks for the key passed at the start of a line
   in the file associated with open file descriptor fp, and returns
   a buffer containing the line in the file. */

#define KEY_LEN		(1024)
#define LINE_LEN	(1024*25)

static char line[LINE_LEN]; 
long last_bin_search_offset = 0;

/* General purpose binary search function to search for key as first
   item on line in open file.  Item is delimited by space. */

#undef getc

char *read_index(long offset, FILE *fp) {
    char *linep;

    linep = line;
    line[0] = '0';

    fseek( fp, offset, SEEK_SET );
    fgets(linep, LINE_LEN, fp);
    return(line);
}

char *bin_search(char *searchkey, FILE *fp)
{
    int c;
    long top, mid, bot, diff;
    char *linep, key[KEY_LEN];
    long length, offset1, offset2;
    int cmp;

    /* do binary search to find correct place in file to insert line */

    linep = line;
    line[0] = '\0';

    fseek(fp, 0L, 2);
    top = 0;
    bot = ftell(fp);
    if (bot == 0) {
	last_bin_search_offset = 0;
	return(0);		/* empty file */
    }
    mid = (bot - top) / 2;

    /* If only one line in file, don't work through loop */

    length = 0;
    rewind(fp);
    while((c = getc(fp)) != '\n' && c != EOF)
	line[length++] =  c;
    if (getc(fp) == EOF) {	/* only 1 line in file */
	length = (int)(strchr(linep, ' ') - linep);
	strncpy(key, linep, length);
	key[length] = '\0';
	cmp = strcmp(key, searchkey);
	if(cmp > 0) {
	    last_bin_search_offset = 0;
	    return(0);		/* line with key is not found */
	} else if (cmp < 0) {
	    last_bin_search_offset = ftell(fp);
	    return(0);		/* line with key is not found */
	} else {
	    last_bin_search_offset = 0;
	    return(linep);      /* line with key is found */
	}
    }

    do {
	fseek(fp, mid - 1, 0);
	if(mid != 1)
	    while((c = getc(fp)) != '\n' && c != EOF);
	offset1 = ftell(fp);	/* offset at start of line */
	if (fgets(linep, LINE_LEN, fp) != NULL) {
  	    offset2 = ftell(fp); /* offset at start of next line */
	    length = (int)(strchr(linep, ' ') - linep);
	    strncpy(key, linep, length);
	    key[length] = '\0';
	    cmp = strcmp(key, searchkey);
	    if(cmp < 0) {	/* further in file */
		top = offset2 - 1;
		last_bin_search_offset = offset2;
	    } else if(cmp > 0) {	/* earlier in file */
		bot = mid;
		last_bin_search_offset = offset1;
            } else {
                last_bin_search_offset = offset1;
                return(linep);          /* line with key is found */
            }
	} else {
	    bot = mid;
            last_bin_search_offset = offset1;
	}
        diff = (bot - top) / 2;
        mid = top + diff;
    } while(diff != 0);

    return(0);		/* line with key is not found */
}

/* Copy contents from one file to another. */

void copyfile(FILE *fromfp, FILE *tofp)
{
    int c;

    while ((c = getc(fromfp)) != EOF)
	putc(c, tofp);
}

/* Function to replace a line in a file.  Returns the original line,
   or NULL in case of error. */

char *replace_line(char *new_line, char *searchkey, FILE *fp)
{
    FILE *tfp;			/* temporary file pointer */

    if (!bin_search(searchkey, fp))
	return(NULL);		/* line with key not found */

    if ((tfp = tmpfile()) == NULL)
	return(NULL);		/* could not create temp file */
    fseek(fp, last_bin_search_offset, 0);
    fgets(line, LINE_LEN, fp);	/* read original */
    copyfile(fp, tfp);
    if (fseek(fp, last_bin_search_offset, 0) == -1) {
        fclose(tfp);
	return(NULL);		/* could not seek to offset */
    }
    fprintf(fp, "%s", new_line);	/* write line */
    rewind(tfp);
    copyfile(tfp, fp);

    fclose(tfp);
    fflush(fp);

    return(line);
}

/* Find location to insert line at in file.  If line with this
   key is already in file, return NULL. */

char *insert_line(char *new_line, char *searchkey, FILE *fp)
{
    FILE *tfp;

    if (bin_search(searchkey, fp))
	return(NULL);
    
    if (fseek(fp, last_bin_search_offset, 0) == -1)
	return(NULL);		/* could not seek to offset */
    if ((tfp = tmpfile()) == NULL)
	return(NULL);		/* could not create temp file */
    copyfile(fp, tfp);
    if (fseek(fp, last_bin_search_offset, 0) == -1) {
        fclose(tfp);
	return(NULL);		/* could not seek to offset */
    }
    fprintf(fp, "%s", new_line);	/* write line */
    rewind(tfp);
    copyfile(tfp, fp);

    fclose(tfp);
    fflush(fp);

    return(new_line);
}
