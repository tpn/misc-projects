/*
 *  File:             tal.c
 *  Date created:     April 25, 1997 (Friday, 20:12h)
 *  Author:           Thomas Jensen
 *                    tsjensen@cip.informatik.uni-erlangen.de
 *  Version:          $Id: tal.c,v 1.9 1999/03/11 14:24:57 tsjensen Exp $
 *  Language:         ANSI C
 *  Platforms:        This version was tested on SUNOS 5, HPUX 10, LINUX,
 *                    DEC alpha (OSF/1 V4), and IRIX 6. Previous versions
 *                    were also tested on SUNOS 4, AIX 4.1, IRIX 5, DEC
 *                    alpha (OSF/1 V2) and HPUX 9.
 *  World Wide Web:   http://home.pages.de/~jensen/tal/
 *  Purpose:          Filter to align common characters at the ends of lines.
 *                    Intended for use with vim(1).
 *  Remarks:          This is the final version of the software.
 *                    Development is being discontinued (unless there are
 *                    reports of discrepancies between documentation and
 *                    actual behavior (bugs) or ports to new platforms).
 *                    Particularly, there will be no further additions in
 *                    terms of functionality.
 *
 *  Revision History:
 *
 *    $Log: tal.c,v $
 *    Revision 1.9  1999/03/11 14:24:57  tsjensen
 *    Maxalign feature can now be disabled by setting its value to 0
 *    Changed default for maxalign to 0 (since most users will otherwise be
 *       confused)
 *    Updates and corrections on the manual page
 *    Clearly marked editable default values in source file
 *    Added -e option (explicit trailer specification)
 *    Added -i option (ignore case)
 *    No longer creates zero-byte outfile when an illegal third file is given
 *    Clarified some comments
 *
 *    Revision 1.8  1998/05/17 17:42:49  tsjensen
 *    Some further speed-ups. Now it is quick enough for my tastes.
 *    Trailer is determined on the expanded line of text (no tabs).
 *    Updates on the manual page.
 *    -v only outputs rcsid. Moved the rest to -h.
 *    Changed default for maxalign to 79
 *    Eliminated global data
 *    Allowed input and output files to be given (including "-")
 *    Trailer determination shortens on space (not fillchar)
 *    Grouped all exit()s in main(). Error codes in functions.
 *    Introduced TAL_FREE macro to replace check-free-set_null sequence
 *    Eliminated some minor memory leaks
 *    Renamed "DEC alpha" platform to OSF/1.
 *    Consolidated orgtext and text usage in lines array.
 *
 *    Revision 1.7  1997/12/05 15:27:45  tsjensen
 *    Added Web page URL to version info
 *    Some speed-up and clarification in input routine
 *    Moved tadd() into calling code (was called only once)
 *    Bugfix: dangling reference in find_trailer()  (reported: Lee)
 *
 *    Revision 1.6  1997/11/28 13:15:06  tsjensen
 *    Port to SunOS 4
 *    Change of development platform from AIX 4.1 to Solaris
 *    Some optimization in trailer determination (generate shorter list)
 *    Bugfix: Did not handle zero input.
 *
 *    Revision 1.5  1997/06/30 02:42:56  l95coop4
 *    1. Changed padding default value to 1
 *    2. Taken out trailer frequency/tolerance options
 *    3. Bugfix: Padding was added even if trailer frequency was 1
 *    4. Added bounds check to trailing whitespace removal
 *    5. Increased maximum allowed tab stop distance to 40
 *    6. If it doesn't conflict with -l, we now choose a shorter trailer
 *       if the current one starts with a fillchar (improves \ handling)
 *
 *    Revision 1.4  1997/05/09  01:06:39  l95coop4
 *    Port to sunos5, irix5, irix6, hpux9, hpux10, and alpha (osf1)
 *
 *    Revision 1.3  1997/05/03  02:27:24  l95coop4
 *    Changed some of the default values
 *    Added padding option, added fixed column option
 *    Added print version option, added force option
 *
 *    Revision 1.2  1997/05/02  03:13:15  l95coop4
 *    Added tab handling. Now, only tabs that are after the end of the text
 *    (but maybe before the trailer) are expanded. All others are preserved.
 *    Bugfix: concerned specification of invalid options
 *
 *    Revision 1.1  1997/04/27  20:05:16  l95coop4
 *    Initial revision
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

#include <limits.h>              /* for LINE_MAX only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*
 *  default settings for command line options (MAY BE EDITED)
 */
#define DEF_FILLCHAR    ' '      /* default fill character (-c) */
#define DEF_PREFTLEN    0        /* default preferred trailer length (-l) */
#define DEF_MAXALIGN    0        /* default maximum desired line length (-m) */
#define DEF_TABSTOP     8        /* default tab stop distance (-t) */
#define DEF_COLUMN      0        /* default end column (-k) */
#define DEF_PADDING     1        /* default number of chars bef. trailer (-p) */


/* - - - - - - - - -  END OF USER-CONFIGURABLE SETTINGS  - - - - - - - - - */


static const char rcsid[] =
   "$Id: tal.c,v 1.9 1999/03/11 14:24:57 tsjensen Exp $";

/*  name of the program
 */
#define PROJECT         "tal"

/*  max. supported line length
 *  This is how many characters of a line will be read. Anything beyond
 *  that will be discarded. Output may be longer than that, though.
 *  The line feed character at the end does not count.
 *  (This should have been done via sysconf(), but I didn't do it in order
 *  to ease porting to non-unix platforms.)
 */
#if defined(LINE_MAX) && (LINE_MAX < 1024)
#undef LINE_MAX
#endif
#ifndef LINE_MAX
#define LINE_MAX        2048
#endif

/*  max. allowed tab stop distance
 */
#define MAX_TABSTOP     40
                                         
/*  some systems (especially sunos4) do not define EXIT_* constants
 */
#ifndef EXIT_FAILURE
#define EXIT_FAILURE    1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS    0
#endif

/*  free memory and set pointer to NULL
 */
#define TAL_FREE(p) {            \
   if (p) {                      \
      free (p);                  \
      (p) = NULL;                \
   }                             \
}

/*  handle errors in main()
 */
#define HANDLE_ERRORS(rc) {      \
   if (rc) {                     \
      destruct_wd (&wd);         \
      perror (PROJECT);          \
      if (opt.infile != stdin)   \
         fclose (opt.infile);    \
      if (opt.outfile != stdout) \
         fclose (opt.outfile);   \
      exit (EXIT_FAILURE);       \
   }                             \
   else {                        \
      if (wd.exit_soon)          \
         exit (EXIT_SUCCESS);    \
   }                             \
}


/*
 *  one line and all its data
 */
typedef struct {
   char *orgtext;                 /* original contents of line as read */
   int   orgtext_len;             /* length of string 'orgtext' */

   char *text;                    /* contents of line with tabs expanded */
                                  /* may be NULL if line is not active   */
   int   text_len;                /* length of string 'text' */

   int   active;                  /* set if line is considered */

   char *tend;                    /* pointer to last char of (orgtext, if the */
                                  /* line is not active, text if it is active)*/
} line_t;

/*
 *  a trailer
 */
typedef struct {
   char *text;                    /* pointer to trailer text */
   int   len;                     /* length of that trailer */
   unsigned long freq;            /* absolute frequency */
} trailer_t;

/*
 *  all data being worked on (lines of text plus a few flags)
 */
typedef struct {
   line_t      **lines;           /* the actual input */
   unsigned long lines_size;      /* number of line pointers reserved */
   unsigned long anz_lines;       /* number of lines read */
   int           output_input;    /* true if input should not be modified */
   int           exit_soon;       /* true if we should exit asap */
   trailer_t     used_trail;      /* the trailer we'll be using in the end */
} work_data_t;

/*
 *  command line options
 */
typedef struct {
   int       column;              /* Trailer must end on that column */
   char      fillchar;            /* used for filling up the new space */
   int       force_tlen;          /* choose trailer of length pref_tlen */
   int       igncase;             /* true if case is to be ignored */
   int       maxalign;            /* maximum desired line length */
   int       padding;             /* number of fillchars inserted before trailer */
   int       pref_tlen;           /* preferred trailer length (0 == disable) */
   int       tabstop;             /* distance between tab stops */
   trailer_t trailer;             /* user-supplied trailer (no autodetect) */
   FILE     *infile;              /* where we get our data */
   FILE     *outfile;             /* where we deliver our data */
} opt_t;

extern char *optarg;              /* Should have been declared with getopt() */
extern int   optind;



/****************************************************************************/

static void usage (FILE *st)
/*
 *  Display usage information on stream 'st'.
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   fprintf (st, "Usage: %s [options] [infile [outfile]]\n", PROJECT);
   fprintf (st, "       -c char  fill character [def.: '%c']\n", DEF_FILLCHAR);
   fprintf (st, "       -e str   use str as trailer (no auto-detection)\n");
   fprintf (st, "       -f       force preferred trailer length [def.: off]\n");
   fprintf (st, "       -h       usage info\n");
   fprintf (st, "       -i       ignore case when matching trailers [def.: match case]\n");
   fprintf (st, "       -k uint  align trailers to end at this column (0 == disable) [def.: %d]\n", DEF_COLUMN);
   fprintf (st, "       -l uint  preferred trailer length (0 == disable) [def.: %d]\n", DEF_PREFTLEN);
   fprintf (st, "       -m uint  maximum allowed line length (0 == disable) [def.: %d]\n", DEF_MAXALIGN);
   fprintf (st, "       -p uint  number of chars inserted before trailer [def.: %d]\n", DEF_PADDING);
   fprintf (st, "       -t uint  distance between tab stops [def.: %d]\n", DEF_TABSTOP);
   fprintf (st, "       -v       print out current version number\n");
}    



/****************************************************************************/

static int process_commandline (int argc, char *argv[], opt_t *opt, work_data_t *wd)
/*
 *  Process command line options and set global options in *opt.
 *  If necessary, usage() is displayed.
 *
 *  argc, argv     Command line as received by main()
 *  *opt           Buffer to store the option sttings in
 *  *wd            used only for the exit flags (esp. exit_soon)
 *
 *  RETURNS: == 0   on success (*opt has been set correctly)
 *           != 0   on error
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   int oc;                               /* option character */
   int idummy;
   size_t tlength;                       /* length of user-specified trailer */
   int outfile_existed;                  /* true if we overwrite a file */

   wd->exit_soon = 0;                    /* assume all goes well */

   opt->column     = DEF_COLUMN;         /* set the defaults */
   opt->fillchar   = DEF_FILLCHAR;
   opt->force_tlen = 0;
   opt->igncase    = 0;
   opt->maxalign   = DEF_MAXALIGN;
   opt->padding    = DEF_PADDING;
   opt->pref_tlen  = DEF_PREFTLEN;
   opt->tabstop    = DEF_TABSTOP;
   memset (&(opt->trailer), 0, sizeof(trailer_t));

   do {
      oc = getopt (argc, argv, "c:e:fhik:l:m:p:t:v");

      switch (oc) {

         case 'c':
            /*
             *  Fill character
             */
            if (strlen (optarg) != 1) {
               fprintf (stderr,
                  "%s: Fill char must be exactly one character.\n", PROJECT);
               return 1;
            }
            opt->fillchar = optarg[0];
            break;

         case 'e':
            /*
             *  User-supplied trailer
             */
            tlength = strlen (optarg);
            if (tlength < 1 || tlength > LINE_MAX) {
               fprintf (stderr,
                     "%s: Length of user-supplied trailer must be between 1 and %d.\n",
                     PROJECT, LINE_MAX);
               return 11;
            }
            opt->trailer.len = (int) tlength;
            opt->trailer.text = (char *) strdup (optarg);
            break;

         case 'i':
            /*
             *  Case sensitivity
             */
            opt->igncase = 1;
            break;

         case 'l':
            /*
             *  Preferred trailer length
             */
            idummy = (int) strtol (optarg, NULL, 10);
            if (idummy < 0 || idummy > LINE_MAX) {
               fprintf (stderr,
                  "%s: Preferred trailer length must be between 0 and %d.\n",
                  PROJECT, LINE_MAX);
               return 2;
            }
            opt->pref_tlen = idummy;
            break;

         case 'm':
            /*
             *  Maximum desired line length
             */
            idummy = (int) strtol (optarg, NULL, 10);
            if (idummy < 0) {
               fprintf (stderr,
                  "%s: Negative line lengths not allowed.\n", PROJECT);
               return 3;
            }
            opt->maxalign = idummy;
            break;

         case 't':
            /*
             *  Distance between tab stops
             */
            idummy = (int) strtol (optarg, NULL, 10);
            if (idummy < 1 || idummy > MAX_TABSTOP) {
               fprintf (stderr,
                  "%s: '%d' is an invalid tab stop distance.\n",
                  PROJECT, idummy);
               return 4;
            }
            opt->tabstop = idummy;
            break;

         case 'k':
            /*
             *  Make all trailers end at a particular column
             */
            idummy = (int) strtol (optarg, NULL, 10);
            if (idummy < 0 || idummy > LINE_MAX) {
               fprintf (stderr,
                  "%s: '%d' is an invalid trailer ending column.\n",
                  PROJECT, idummy);
               return 5;
            }
            opt->column = idummy;
            break;

         case 'p':
            /*
             *  Padding
             */
            idummy = (int) strtol (optarg, NULL, 10);
            if (idummy < 0 || idummy > (LINE_MAX>>1)) {
               fprintf (stderr,
                  "%s: '%d' is an invalid padding value.\n",
                  PROJECT, idummy);
               return 6;
            }
            opt->padding = idummy;
            break;

         case 'f':
            /*
             *  Force preferred trailer length
             */
            opt->force_tlen = 1;
            break;

         case 'v':
            /*
             *  Display current version and terminate
             */
            printf ("%s\n", rcsid);
            wd->exit_soon = 1;
            return 0;

         case 'h':
            /*
             *  Display usage information and terminate
             */
            printf ("%s -  trailer alignment ", PROJECT);
            printf ("(aligns common characters at the end of lines)\n");
            printf ("       (c) 1997 Thomas Jensen ");
            printf ("<tsjensen@stud.informatik.uni-erlangen.de>\n");
            printf ("       Web page: http://home.pages.de/~jensen/tal/\n");
            usage (stdout);
            wd->exit_soon = 1;
            return 0;

         case ':': case '?':
            /*
             *  Missing argument or illegal option - do nothing else
             */
            return 7;

         case EOF:
            /*
             *  End of list, do nothing more
             */
            break;

         default:                        /* This case must never be */
             fprintf (stderr,
                "%s: Uh-oh! This should have been unreachable code. %%-)\n",
                PROJECT);
             return 8;
      }

   } while (oc != EOF);

   /*
    *  Input and Output Files
    *
    *  After any command line options, an input file and an output file may
    *  be specified (in that order). "-" may be substituted for standard
    *  input or output. A third file name would be invalid.
    *  The alogrithm is as follows:
    *
    *  If no files are given, use stdin and stdout.
    *  Else If infile is "-", use stdin for input
    *       Else open specified file (die if it doesn't work)
    *       If no output file is given, use stdout for output
    *       Else If outfile is "-", use stdout for output
    *            Else open specified file for writing (die if it doesn't work)
    *            If a third file is given, die.
    */
   if (argv[optind] == NULL) {           /* neither infile nor outfile given */
      opt->infile = stdin;
      opt->outfile = stdout;
   }

   else {
      if (strcmp (argv[optind], "-") == 0) {
         opt->infile = stdin;            /* use stdin for input */
      }
      else {
         opt->infile = fopen (argv[optind], "r");
         if (opt->infile == NULL) {
            perror (PROJECT);
            return 9;                    /* can't read infile */
         }
      }

      if (argv[optind+1] == NULL) {
         opt->outfile = stdout;          /* no outfile given */
      }
      else {
         if (strcmp (argv[optind+1], "-") == 0) {
            opt->outfile = stdout;       /* use stdout for output */
         }
         else {
            outfile_existed = !access (argv[optind+1], F_OK);
            opt->outfile = fopen (argv[optind+1], "w");
            if (opt->outfile == NULL) {
               perror (PROJECT);
               if (opt->infile != stdin)
                  fclose (opt->infile);
               return 10;
            }
         }
         if (argv[optind+2]) {           /* illegal third file */
            fprintf (stderr, "%s: illegal parameter -- '%s'\n",
                  PROJECT, argv[optind+2]);
            usage (stderr);
            if (opt->infile != stdin)
               fclose (opt->infile);
            if (opt->outfile != stdout) {
               fclose (opt->outfile);
               if (!outfile_existed) unlink (argv[optind+1]);
            }
            wd->exit_soon = 1;
         }
      }
   }

   return 0;
}


/****************************************************************************/

static void destruct_wd (work_data_t *wd)
/*
 *  Destruct work data in *wd. Leave completely zeroed *wd.
 *  Free all memory except the area directly pointed to by wd.
 *
 *  RETURNS: ---
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   unsigned long i;

   if (wd == NULL) return;

   if (wd->lines) {

      for (i=0; i<wd->lines_size; ++i) {

         if (wd->lines[i] == NULL)
            continue;
         TAL_FREE (wd->lines[i]->text);     /* text with tabs expanded */
         TAL_FREE (wd->lines[i]->orgtext);  /* text as read */
         wd->lines[i]->tend = NULL;         /* points to orgtext */
         TAL_FREE (wd->lines[i]);
      }
   }

   TAL_FREE (wd->used_trail.text);          /* trailer in use */

   memset (wd, 0, sizeof (work_data_t));
}


/****************************************************************************/

static int expand_tabs_into (const char *input_buffer, const int in_len,
      const int tabstop, char **text)
/*
 *  Expand tab chars in input_buffer and store result in text.
 *
 *  input_buffer   Line of text with tab chars
 *  in_len         length of the string in input_buffer
 *  tabstop        tab stop distance
 *  text           address of the pointer that will take the result
 *
 *  Memory will be allocated for the result. This should only be called for
 *  active lines that consist of at least one non-whitespace character.
 *
 *  RETURNS:  Success: Length of the result line in characters (> 0)
 *            Error:   0
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   static char temp [LINE_MAX*MAX_TABSTOP+1];  /* work string */
   int ii;                               /* position in input string */
   int io;                               /* position in work string */
   int jp;                               /* tab expansion jump point */

   *text = NULL;

   for (ii=0, io=0; ii<in_len && io<(LINE_MAX*tabstop-1); ++ii) {
      if (input_buffer[ii] == '\t') {
         for (jp=io+tabstop-(io%tabstop); io<jp; ++io)
            temp[io] = ' ';
      }
      else {
         temp[io] = input_buffer[ii];
         ++io;
      }
   }
   temp[io] = '\0';

   *text = (char *) strdup (temp);
   if (*text == NULL) return 0;

   return strlen (*text);
}


/****************************************************************************/

static int read_all_input (const opt_t opt, work_data_t *wd)
/*
 *  Read all input lines from opt.infile and build lines data structures.
 *  *wd is assumed to be preallocated. All other space is allocated.
 *  No memory is allocated in error conditions.
 *
 *  RETURNS:  == 0  on success (lines data structures successfully built (*wd))
 *            != 0  on error   (*wd is zeroed out)
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   char input_buffer[LINE_MAX+2];    /* where we store each line for processing */
   char *p;                              /* used for line traversal */
   int in_len;                           /* length of string in input_buffer */
   line_t **ptemp;                       /* temp for realloc()s */
   int rc;                               /* return codes */


   memset (wd, 0, sizeof(work_data_t));  /* Initialize result with zero */

   wd->lines = (line_t **) calloc (100, sizeof (line_t *));
   if (wd->lines == NULL) return 1;
   wd->lines_size = 100;                 /* allocated 100 line pointers */


   while (fgets (input_buffer, LINE_MAX+1, opt.infile)) {

      /*
       *  If needed, expand line pointer array
       */
      if (wd->anz_lines == wd->lines_size) {
         wd->lines_size += 100;
         ptemp = (line_t **) realloc
            (wd->lines, wd->lines_size * sizeof (line_t *));
         if (ptemp == NULL)
            return 2;
         else
            wd->lines = ptemp;
      }

      /*
       *  Allocate line structure and check if input present
       */
      wd->lines[wd->anz_lines] = (line_t *) calloc (1, sizeof (line_t));
      if (wd->lines[wd->anz_lines] == NULL) return 3;

      in_len = strlen (input_buffer);
      if (in_len == 0) {
         /* There must be some read error, because we should */
         /* at least have received a newline.                */
         TAL_FREE (wd->lines[wd->anz_lines]);
         break;
      }

      /*
       *  Remove trailing whitespace from input buffer, then store length
       */
      p = input_buffer + in_len - 1;
      while (in_len && (*p == ' ' || *p == '\t' || *p == '\n')) {
         *p-- = '\0';
         --in_len;
      }
      wd->lines[wd->anz_lines]->orgtext_len = in_len;

      /*
       *  Store line in lines list
       */
      wd->lines[wd->anz_lines]->orgtext = (char *) strdup (input_buffer);
      if (wd->lines[wd->anz_lines]->orgtext == NULL)
         return 4;
      wd->lines[wd->anz_lines]->text = NULL;
      wd->lines[wd->anz_lines]->tend =
         wd->lines[wd->anz_lines]->orgtext + in_len - 1;

      /*
       *  Deactivate line if it is empty, else activate
       */
      if (wd->lines[wd->anz_lines]->orgtext[0] == '\0') {
         wd->lines[wd->anz_lines]->tend = wd->lines[wd->anz_lines]->orgtext;
         wd->lines[wd->anz_lines]->orgtext_len = 0;
         wd->lines[wd->anz_lines]->active = 0;
      }
      else {
         wd->lines[wd->anz_lines]->active = 1;
      }

      /*
       *  If it is an active line, expand tabs into 'text' string
       */
      if (wd->lines[wd->anz_lines]->active) {
         rc = expand_tabs_into (wd->lines[wd->anz_lines]->orgtext, in_len,
               opt.tabstop, &(wd->lines[wd->anz_lines]->text));
         if (rc == 0) {
            /* result line length == 0 means we're in trouble */
            return 5;
         }
         wd->lines[wd->anz_lines]->text_len = rc;
         wd->lines[wd->anz_lines]->tend =
            (wd->lines[wd->anz_lines]->text)
            + (wd->lines[wd->anz_lines]->text_len) - 1;
      }

      /*
       *  next please
       */
      ++(wd->anz_lines);
   }

   /*
    *  Don't do anything if there was no input at all.
    */
   if (wd->lines[0] == NULL)
      wd->exit_soon = 1;

   /*
    *  Now let's take a look at what we have here:
    *
    *  In wd of type work_data_t there is
    *
    *      exit_soon    set to 1 if there was no error, but we should exit
    *                   as soon as possible (no input, for instance)
    *      output_input set to 1 if it has been decided that we should not
    *                   modify any of the lines, and just output everything
    *                   just like we got it. Lines may be chopped off at
    *                   the LINE_MAXth character, though.
    *
    *      anz_lines    is the number of lines of input we have received
    *                   so it tells you how many elements there are in the
    *                   lines array
    *      lines_size   is the size of the lines array, the upper limit of
    *                   how many lines we can store before we must realloc()
    *
    *  Then there is the lines array itself, holding all the input lines,
    *  and their counterparts with tabs expanded:
    *
    *      orgtext      is the original input as read (at least the first
    *                   LINE_MAX charcters of each line)
    *      orgtext_len  the length of the string orgtext
    *
    *      text         is the orgtext line with tabs expanded
    *      text_len     the length of the string text
    *
    *  text is only set for sure when the line is active. text may be
    *  longer than LINE_MAX due to tab expansion. Neither orgtext nor
    *  text contain any trailing whitespace.
    *
    *      tend         the pointer to the last character in orgtext or
    *                   text. It points to text only if the line is active,
    *                   because otherwise text might not be set.
    *
    *      active       tells us if the line is active (1) or not (0)
    *                   inactive lines are always output unmodified and
    *                   they play no part in the selection of the trailer.
    *                   This function only deactivates lines if they are
    *                   empty.
    *
    *  Here is some debugging code you can use to verify that the working
    *  data actually follows this specification (note that lines might not
    *  start on a tab stop position):
    */
   #if 0
   {
      unsigned long i;

      fprintf (stderr, "exit_soon:    %d\n", wd->exit_soon);
      fprintf (stderr, "output_input: %d\n", wd->output_input);
      fprintf (stderr, "lines_size:   %u\n", wd->lines_size);
      fprintf (stderr, "anz_lines:    %u\n", wd->anz_lines);
      fprintf (stderr, "lines:        %p\n", wd->lines);

      for (i=0; i<wd->anz_lines; ++i) {
         fprintf (stderr, "%4d: %s org:%4d:    \"%s\"\n", i,
               wd->lines[i]->active? "[x]": "[ ]",
               wd->lines[i]->orgtext_len, wd->lines[i]->orgtext);
         if (wd->lines[i]->text) {
            fprintf (stderr, "          exp:%4d:    \"%s\"\n",
                  wd->lines[i]->text_len, wd->lines[i]->text);
         }
         fprintf (stderr, "         tend:    :    \"%s\"\n", wd->lines[i]->tend);
      }
      exit (EXIT_SUCCESS);
   }
   #endif

   return 0;
}


/****************************************************************************/

static int longest_line_len (const work_data_t *wd)
/*
 *  Returns length of longest active line in characters, considering only
 *  the lines with their tabs expanded, not the original input lines.
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   unsigned long i;
   int max = 0;

   for (i=0; i<wd->anz_lines; ++i) {
      if (wd->lines[i]->active && wd->lines[i]->text_len > max)
         max = wd->lines[i]->text_len;
   }
   return max;
}


/****************************************************************************/

static trailer_t *texist (trailer_t **temple,
                   const char *wanted,
                   const unsigned long anz_temple,
                   const int igncase)
/*
 *  Look for the wanted trailer in the temporary list of trailers.
 *
 *  temple        temporary list of trailers (all same length)
 *  wanted        trailer we want to locate
 *  anz_temple    number of entries in temporary list
 *  igncase       true if we are not supposed to be case sensitive (from opt)
 *
 *  Returns:  == NULL   trailer not found
 *            != NULL   pointer to existing trailer
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   unsigned long i;
   int found;

   for (i=0; i<anz_temple; ++i) {
      if (igncase)
         found = !strcasecmp (temple[i]->text, wanted);
      else
         found = !strcmp (temple[i]->text, wanted);
      if (found)
         return (trailer_t *) (temple[i]);
   }
   return NULL;
}


/****************************************************************************/

static unsigned long count_active_lines (const work_data_t *wd)
/*
 *  Count lines in lines list that are tagged active.
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   unsigned long count = 0;
   unsigned long i;

   for (i=0; i<wd->anz_lines; ++i)
      if (wd->lines[i]->active) ++count;
   return count;
}


/****************************************************************************/

static int find_a_good_trailer (const opt_t opt, work_data_t *wd)
/*
 *  This will examine the input lines and try to find a good trailer.
 *
 *  RETURNS: == 0 on success (trailer is stored in wd->used_trail)
 *           != 0 on error
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   trailer_t   **trailers;               /* list of trailer candidates */
   unsigned long trailers_size;          /* num of trailer pointers reserved */
   unsigned long anz_trailers;           /* how much of the list is in use */
   int           cur_tlen;               /* currently considered trailer len */
   trailer_t   **temple;                 /* start of temporary list entries */
   unsigned long temple_space;           /* space left for temp list */
   unsigned long anz_temple;             /* number of entries in temple */
   line_t       *p;                      /* roving line pointer */
   unsigned long i;                      /* loop counter */
   char         *candidate;              /* currently considered trailer */
   unsigned long tmostfreq;              /* temple index to most frequent temp trailer */
   trailer_t    *tp;
   int longest = longest_line_len(wd);   /* length of longest line */
   unsigned long anz_valines =
      count_active_lines(wd);            /* number of considered lines */


   if (anz_valines == 0) {
      wd->output_input = 1;
      return 0;                          /* nothing to do */
   }

   /*
    *  Start by allocating enough memory for the completed trailer list
    */
   trailers = (trailer_t **) calloc (longest, sizeof (trailer_t *));
   if (trailers == NULL)
      return 1;
   trailers_size = longest;
   anz_trailers = 0;

   /*
    *  This loop builds 'trailers', the list of the most frequent trailers
    *  for each possible trailer length. We stop building as soon as we see
    *  the frequency decrease, unless the user forced a higher trailer length.
    */
   for (cur_tlen=1; cur_tlen<=longest; ++cur_tlen) {
      
      temple = trailers + (cur_tlen-1);
      temple_space = trailers_size - (cur_tlen-1);
      anz_temple = 0;

      /*
       *  Create list of all possible trailers of the current length
       */
      for (i=0, p=wd->lines[0]; i<wd->anz_lines; p=wd->lines[++i]) {

         /*
          *  If the line is tagged inactive (for whatever reason), or
          *  it is too short to contain a trailer of the current length,
          *  skip to next line.
          */
         if (p->active == 0) continue;
         if (p->text_len < cur_tlen) continue;

         /*
          *  candidate is the currently considered trailer
          */
         candidate = (p->tend) - cur_tlen + 1;

         /*
          *  If the candidate is already in the temporary list, just
          *  increment its frequency, then skip to next line.
          */
         tp = texist (temple, candidate, anz_temple, opt.igncase);
         if (tp != NULL) {
            ++(tp->freq);
            continue;
         }

         /*
          *  If the trailer list is full, increase its size
          */
         if (temple_space == 0) {
            trailers_size += 100;
            trailers = (trailer_t **) realloc (trailers,
                  trailers_size * sizeof (trailer_t *));
            if (trailers == NULL)
               return 2;
            temple = trailers + (cur_tlen-1);         /* update reference */
            temple_space += 100;
         }

         /*
          *  Add potential trailer to temporary list
          */
         temple[anz_temple] = (trailer_t *) calloc (1, sizeof(trailer_t));
         if (temple[anz_temple] == NULL) 
            return 3;
         temple[anz_temple]->text = candidate;
         temple[anz_temple]->len  = strlen (candidate);
         temple[anz_temple]->freq = 1;

         --temple_space;
         ++anz_temple;
      }

      /*
       *  From the temporary list, choose the most frequent trailer
       *  and put it up front. Then delete all other candidates.
       */
      tmostfreq = 0;
      for (i=1; i<anz_temple; ++i) {
         if (temple[i]->freq > temple[tmostfreq]->freq)
            tmostfreq = i;
      }
      tp = temple[0];
      temple[0] = temple[tmostfreq];
      temple[tmostfreq] = tp;
      for (i=1; i<anz_temple; ++i)
         TAL_FREE (temple[i]);
      ++anz_trailers;

      if (cur_tlen > 1) {
         if (opt.pref_tlen > 0) {
            /*
             *  There is a preferred trailer length. Break only if reached.
             */
            if (cur_tlen >= opt.pref_tlen)
               break;
         }
         else {
            /*
             *  There is no preferred trailer length.
             *  Break if frequency is decreasing or one.
             */
            if ((trailers[cur_tlen-1]->freq == 1)
             || (trailers[cur_tlen-1]->freq < trailers[cur_tlen-2]->freq))
               break;
         }
      }
   }

   /*
    *  At this point, the trailer list contains the most frequent trailer
    *  for each possible length and its text, plus information about its
    *  actual frequency. Verification code:
    */
   #if 0
      for (i=0; i<anz_trailers; ++i) {
         fprintf (stderr, "%2d: len:%d, freq:%ld, \"%s\"\n",
               i, trailers[i]->len, trailers[i]->freq, trailers[i]->text);
      }
      exit (EXIT_SUCCESS);
   #endif

   /*
    *  Choose the most frequent trailer
    */
   tmostfreq = 0;
   for (cur_tlen=1; cur_tlen<anz_trailers; ++cur_tlen) {
      if (trailers[cur_tlen]->freq >= trailers[tmostfreq]->freq)
         tmostfreq = cur_tlen;
   }
   /*
    *  If the chosen trailer is longer than preferred, take the preferred size
    */
   if (opt.pref_tlen != 0) {               /* 0 means disable preference */
      if (opt.force_tlen) {                /* we *must* take the preferred one */
         if (opt.pref_tlen > anz_trailers)
            tmostfreq = anz_trailers-1;    /* this is the longest one we've got */
         else
            tmostfreq = opt.pref_tlen-1;   /* this is the preferred trailer */
      }
      else {
         if (opt.pref_tlen < tmostfreq+1)
            tmostfreq = opt.pref_tlen-1;   /* only if longer one isn't more frequent */
      }
   }
   else {
      /*
       *  Since we have no particular preference as to the length of the
       *  trailer, check if the chosen trailer starts with a fillchar.
       *  If so, choose a shorter one. Do not eliminate the trailer. :)
       */
      for (cur_tlen = tmostfreq; cur_tlen > 0; --cur_tlen) {
         if (*(trailers[cur_tlen]->text) == ' ')
            --tmostfreq;
         else
            break;
      }
   }

   /*
    *  Save chosen trailer, then destruct trailer list
    */
   wd->used_trail.text = (char *) strdup (trailers[tmostfreq]->text);
   wd->used_trail.len  = trailers[tmostfreq]->len;
   wd->used_trail.freq = trailers[tmostfreq]->freq;
   #if 0
      fprintf (stderr, "Chosen Trailer: \"%s\" (freq %d, len %d)\n",
            wd->used_trail.text, wd->used_trail.freq, wd->used_trail.len);
   #endif

   for (cur_tlen=0; cur_tlen<anz_trailers; ++cur_tlen) {
      TAL_FREE (trailers[cur_tlen]);
   }
   TAL_FREE (trailers);

   if (wd->used_trail.freq < 2)
      wd->output_input = 1;

   return 0;
}


/****************************************************************************/

static int do_stuff_to_lines (const opt_t opt, work_data_t *wd)
/*
 *  (1) Deactivate lines that do not contain a trailer.
 *  (2) Enforce 'maxalign' rule.
 *  (3) Cut off the trailer from the 'text' and 'orgtext' of active lines.
 *  (4) Remove the now trailing whitespace from 'text' and 'orgtext'.
 *
 *  Note that inactive lines remain untouched. Since active ones are
 *  permanently modified, this function is the last one that can deactivate
 *  lines. At this point, the trailer to be used is already chosen.
 *
 *  RETURNS:  == 0  on success
 *            != 0  on error
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   unsigned long i;                      /* line loop counter */
   char         *p;                      /* rover in 'temp' */
   char         *temp;                   /* temporary copy of 'text' string */
   int           pot_newlen;             /* length of 'temp' string */
   char         *pts;                    /* potential trailer start */
   int           tmatch;                 /* str[case]cmp result */

   for (i=0; i<wd->anz_lines; ++i) {

      /*
       *  If the line is inactive, we will never change it.
       */
      if (!wd->lines[i]->active)
         continue;

      /*
       *  If the line does not contain a trailer, we will never change it.
       */
      pts = (wd->lines[i]->tend) + 1 - (wd->used_trail.len);
      if (pts < wd->lines[i]->text) {
         wd->lines[i]->active = 0;
         continue;
      }
      if (opt.igncase)
         tmatch = strcasecmp (pts, wd->used_trail.text);
      else
         tmatch = strcmp (pts, wd->used_trail.text);
      if (tmatch) {
         wd->lines[i]->active = 0;
         continue;
      }

      /*
       *  Cut off trailer and then trailing whitespace from 'text'.
       *  Since we might not really want that (maxalign), use a temp copy.
       */
      temp = (char *) strdup (wd->lines[i]->text);
      if (temp == NULL)
         return 1;
      pot_newlen = (wd->lines[i]->text_len) - wd->used_trail.len;
      p = temp + pot_newlen;
      *p-- = '\0';                       /* -snip-, now last char of text */
      while (p >= temp && (*p == ' ' || *p == '\t' || *p == '\n')) {
         *p-- = '\0';
         --pot_newlen;
      }
      if (opt.maxalign > 0 &&
       (pot_newlen + (wd->used_trail.len) + opt.padding > opt.maxalign)) {
         /* this line does not satisfy maxalign rule */
         TAL_FREE (temp);
         pot_newlen = 0;
         wd->lines[i]->active = 0;
         continue;
      }
      else {
         /* line is okay, so make changes permanent */
         TAL_FREE (wd->lines[i]->text);
         wd->lines[i]->text = temp;
         temp = NULL;
         wd->lines[i]->text_len = pot_newlen;
         wd->lines[i]->tend = wd->lines[i]->text + (wd->lines[i]->text_len) - 1;
      }

      /*
       *  We now know that this line satisfies the maxalign rule.
       *  Cut off trailer and whitespace from 'orgtext', too.
       *  If 'orgtext' is shorter than the trailer, this means we'll have
       *  to expand all tabs anyway, so 'orgtext' simply becomes 'text'.
       *  (Yes, this *can* happen if someone uses the -l option.)
       */
      if (wd->lines[i]->orgtext_len <= wd->used_trail.len) {
         TAL_FREE (wd->lines[i]->orgtext);
         wd->lines[i]->orgtext = (char *) strdup (wd->lines[i]->text);
         if (wd->lines[i]->orgtext == NULL)
            return 2;
         wd->lines[i]->orgtext_len = wd->lines[i]->text_len;
      }
      else {
         p = (wd->lines[i]->orgtext)
            + (wd->lines[i]->orgtext_len) - (wd->used_trail.len);
         *p-- = '\0';                    /* -snip-, now last char of orgtext */
         wd->lines[i]->orgtext_len -= wd->used_trail.len;
         while (p >= (wd->lines[i]->orgtext)
               && (*p == ' ' || *p == '\t' || *p == '\n'))
         {
            *p-- = '\0';
            --(wd->lines[i]->orgtext_len);
         }
      }

   } /*for*/

   return 0;                             /* success */
}


/****************************************************************************/

static int output_aligned_lines (const opt_t opt, work_data_t *wd)
/*
 *  This takes the list of lines that have been stripped of their trailer
 *  and the then trailing whitespace, and produces the complete aligned
 *  output by filling in fillchars between text and trailer.
 *
 *  RETURNS:  == 0  success
 *            != 0  error
 *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
{
   int           newlen;           /* length of lines to be aligned (w/o trailer) */
   unsigned long i;                /* loop counter */
   char         *sfill;            /* string with fill characters */
   char         *fp;               /* pointer to zero byte in sfill */
   int           fillsize;         /* number of fillchars in current line */

   /*
    *  If we are not supposed to do anything, just print the lines and go.
    */
   if (wd->output_input) {
      for (i=0; i<wd->anz_lines; ++i)
         fprintf (opt.outfile, "%s\n", wd->lines[i]->orgtext);
      return 0;
   }

   /*
    *  Determine final line length
    */
   if (opt.column > 0                       /* user wants special column */
    && (opt.maxalign == 0 || opt.column <= opt.maxalign)) /* -m beats -k */
      newlen = opt.column;
   else
      newlen = longest_line_len(wd) + (wd->used_trail.len) + opt.padding;

   /*
    *  Set up fill string
    */
   sfill = (char *) malloc (newlen+1);
   if (sfill == NULL)
      return 1;
   memset (sfill, opt.fillchar, newlen);
   fp = sfill;

   /*
    * Output inactive lines, adjust and output active ones
    */
   for (i=0; i<wd->anz_lines; ++i) {

      if (wd->lines[i]->active == 0) {
         fprintf (opt.outfile, "%s\n", wd->lines[i]->orgtext);
         continue;
      }

      *fp = opt.fillchar;
      fillsize = newlen - wd->lines[i]->text_len - wd->used_trail.len;
      if (fillsize < 0) fillsize = 0;
      fp = sfill + fillsize;
      *fp = '\0';

      fprintf (opt.outfile, "%s%s%s\n",
            wd->lines[i]->orgtext, sfill, wd->used_trail.text);
   }

   return 0;
}



/*==========================================================================*/
/*  t a l    m a i n ()                                                     */
/*==========================================================================*/

int main (int argc, char *argv[]) {

   opt_t       opt;                      /* command line options */
   work_data_t wd;                       /* work data (array of lines) */
   int         rc;                       /* return code */

   /*
    *  Process Command Line
    */
   rc = process_commandline (argc, argv, &opt, &wd);
   if (rc)
      exit (EXIT_FAILURE);               /* no perror() here */
   else if (wd.exit_soon)                /* probably -v or -h option */
      exit (EXIT_SUCCESS);

   /*
    *  Read all the input and build work data structures
    */
   rc = read_all_input (opt, &wd);       /* allocates space for wd.lines */
   HANDLE_ERRORS (rc);

   if (!wd.output_input) {               /* always true, I think */

      /*
       *  Find a good trailer
       */
      if (opt.trailer.text) {
         wd.used_trail.text = opt.trailer.text;
         opt.trailer.text = NULL;
         wd.used_trail.len  = opt.trailer.len;
         wd.used_trail.freq = opt.trailer.freq;   /* 0 (zero), I think */
      }
      else {
         rc = find_a_good_trailer (opt, &wd);
         HANDLE_ERRORS (rc);
      }

      /*
       *  Cut off trailer from each line, remove then trailing whitespace
       */
      if (!wd.output_input) {
         rc = do_stuff_to_lines (opt, &wd);
         HANDLE_ERRORS (rc);
      }
   }

   /*
    *  Give back the fine lines of text
    */
   rc = output_aligned_lines (opt, &wd);
   HANDLE_ERRORS (rc);

   if (opt.infile != stdin)
      fclose (opt.infile);
   if (opt.outfile != stdout)
      fclose (opt.outfile);

   return EXIT_SUCCESS;
}

/*EOF*/                                                  /* vim: set sw=3: */
