/*

   wnrtl.c - global variables used by WordNet Run Time Library

*/

#include <stdio.h>
#include "wn.h"

/*static char *Id = "$Id: wnrtl.c,v 1.2 2007/09/22 17:37:46 magodra Exp $";*/

/* Search code variables and flags */

SearchResults wnresults;	/* structure containing results of search */

int fnflag = 0;			/* if set, print lex filename after sense */
int dflag = 1;			/* if set, print definitional glosses */
int saflag = 1;			/* if set, print SEE ALSO pointers */
int fileinfoflag = 0;		/* if set, print lex file info on synsets */
int frflag = 0;			/* if set, print verb frames */
int abortsearch = 0;		/* if set, stop search algorithm */
int offsetflag = 0;		/* if set, print byte offset of each synset */
int wnsnsflag = 0;		/* if set, print WN sense # for each word */

/* File pointers for database files */

int OpenDB = 0;			/* if non-zero, database file are open */
FILE *datafps[NUMPARTS + 1] = { NULL, NULL, NULL, NULL, NULL } , 
     *indexfps[NUMPARTS + 1] = { NULL, NULL, NULL, NULL, NULL } ,
     *sensefp = NULL,
     *cntlistfp = NULL,
     *keyindexfp = NULL,
     *revkeyindexfp = NULL,
     *vsentfilefp = NULL, *vidxfilefp = NULL;

/* Method for interface to check for events while search is running */

void (*interface_doevents_func)(void) = NULL;
                        /* callback function for interruptable searches */
                        /* in single-threaded interfaces */

/* General error message handler - can be defined by interface.
   Default function provided in library returns -1 */

int default_display_message(char *);
int (*display_message)(char *) = default_display_message;

/*
   Revsion log: 

   $Log: wnrtl.c,v $
   Revision 1.2  2007/09/22 17:37:46  magodra
   - Added /path command to admin commands. This command will edit paths directly
     in db. Subcommands are
      display : Display all path points in current sector.
      hide    : Hide all displayed path points
      point   : Add a new point
      create  : Set the waypoint link id to be used for the new path.
      help    : Display a short help description.
   - Compiler warning fixes
   - New class psPath and psPathNetwork to handle the new predefined path objects.
     - Moved most of the waypoint function from npcclient to the new Path Network class.
     - Added points to paths from the waypoint object to point to each path connecting
       that waypoint to other waypoints. It is created a default path if no path points
       are in the db that goes linear between the two waypoints. If this looks bad just
       add some point in the path with the /path point command.
   - Added new utility function psGetRandom and psGetRandom(uint32 limit) easy
     to use instead of creating your own random objects all over the place.
   - Added pathlist command to npcclient til list all current paths. Needed for debug.
   - Added replacement $name, $race, $tribe in the npc client locate behaviour operation.
     This allow for behaviors like <locate obj="home:$name" />, name will be replaced with
     the npcs name. This will allow to generate a generic npc types like citizen that could
     have each npc go to different home and work locations but still using the same script.
   - Modified the MovePath npc client behaviour to use the new paths.
     - Added a new name attribute to each waypoint_link in order to find the path this behaviour
       should walk.
     - Added new direction attribute that could be either forward or revers.
   - Removed old import of npcdefs.xml file. All data has been moved to db.
   - Improved the performance of the item detection perception stuff in the npc client.
   => DB VERSION BUMPED!.

   Revision 1.1  2005/12/23 22:39:48  khakilord
   Added the wordnet library to the project.

   Revision 1.8  2005/01/27 17:33:54  wn
   cleaned up includes

   Revision 1.7  2005/01/27 16:31:17  wn
   removed cousinfp for 1.6

   Revision 1.6  2002/03/22 20:29:58  wn
   added revkeyindexfp

   Revision 1.5  2001/10/11 18:03:02  wn
   initialize keyindexfp

   Revision 1.4  2001/03/27 18:48:15  wn
   added cntlistfp

   Revision 1.3  2001/03/13 17:45:48  wn
   *** empty log message ***

   Revision 1.2  2000/08/14 16:05:06  wn
   added tcflag

   Revision 1.1  1997/09/02 16:31:18  wn
   Initial revision


*/
