/*
 * autoexec.h
 *
 * Author: Fabian Stock (Aiwendil)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef AUTOEXEC_HEADER
#define AUTOEXEC_HEADER

// Crystal Space Includes
#include <csutil/csstring.h>
#include <csutil/array.h>

struct AutoexecCommand
{
    /** Charname associated to the player.
     *  It rappresents a null string if it's a general one (valid for all chars).
     */
    csString name;
    csString cmd; ///< The commands to run.
};

/// class handling autoexecution of commands at the startup.
class Autoexec
{
  public:
    Autoexec(); ///< Constructor.
    ~Autoexec(); ///< Destructor.
    void execute();                                 ///< Triggers the autoexec to run the commands.
    /** Gets if the autoexec is enabled and will trigger.
     *  @return True if the autoexec functionality is enabled.
     */
    bool GetEnabled() { return enabled; };
    /** Sets if the autoexec is enabled and will trigger.
     *  @param en Set this true if you want to activate the autoexec functionality.
     */
    void SetEnabled(const bool en) {enabled = en;};
    void SaveCommands();                            ///< Save all the autoexec data to file.
    void LoadDefault();                           ///< Overrides the current autoexec data with default
    /** Gets commands of one character name.
     *  @param name The name for which to search for the commands.
     *              An empty string for the general ones.
     *  @return The string with all the commands of the requested type.
     */
    csString getCommands(csString name);
    /** Adds commands to the autoexec list.
     *  @param name The character name for which the cmd string will be executed.
     *  @param cmd The string of commands to execute automatically.
     */
    void addCommand(csString name, csString cmd);
  protected:
    /** Loads the autoexec data with the provided file name.
     *  @param fileName The vfs filename where to find the command list
     */
    void LoadCommands(const char * fileName);
    bool enabled;                                 ///< Stores if the autoexec functionality is enabled
    csArray<AutoexecCommand> cmds;                ///< Stores all the commands which are in the data file.
};


#endif // AUTOEXEC_HEADER

