/*
 * queue.h
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _SOUND_QUEUE_H_
#define _SOUND_QUEUE_H_

#include <csutil/csstring.h>
#include <csutil/array.h>

class SoundHandle;
class SoundControl;

/**
 * Queue object for @see SoundQueue.
 * These objects are used by @see SoundQueue and represent a single sound to be
 * be be played.
 */

class SoundQueueItem
{
    public:
    csString        filename;   ///< a filename of a file which may exist within our vfs */
    SoundHandle     *handle;    ///< pointer to its SoundHandle - not guaranteed to be valid */

    /**
     * Constructor
     * @param file name of a file which may exist within our vfs
     */
    SoundQueueItem (const char *file);
    /**
     * Destructor
     */
    ~SoundQueueItem ();
};


/**
 * Used to put Sounds in a Queue and play them in the order they have been added.
 */  

class SoundQueue
{
    private:
    csArray<SoundQueueItem*>    queue;              ///< a array that contains all queued SoundQueueItem(s) */
    float                       volume;             ///< volume for this queue */
    SoundControl               *sndCtrl;            ///< SoundControl to control volume of this queue */

    public:
    /**
     * Constructor
     * @param ctrl a valid SoundControl
     * @param vol volume as float
     */
    SoundQueue (SoundControl* &ctrl, float vol);
    /**
     * Destructor
     */
    ~SoundQueue ();
    /**
     * Used to create a new @see SoundQueueItem.
     * Will create a new @see SoundQueueItem and adds it to the Queue
     * @param filename a unique (not enforced) filename  
     */
    void AddItem (const char *filename);
    /**
     * Used to remove a @see SoundQueueItem from the Queue.
     * Will remove that @see SoundQueueItem and deletes it from the Queue.
     * @param item a valid SoundQueueItem
     */  
    void DeleteItem (SoundQueueItem* &item);
    /**
     * Checks if there are queued items which can be played.
     * Walks over @see queue and checks if there are items to play or to delete.
     * Faulty Items will be removed.
     */
    void Work ();
    /// Stops playback and purges all SoundQueueItem(s).
    void Purge ();

    /** Returns elements still in the queue to be played
     *  @return The amount of elements still to be played which are in the queue.
     */
    size_t GetSize(); 
};

#endif /*_SOUND_QUEUE_H_*/
