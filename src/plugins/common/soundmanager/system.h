/*
 * system.h
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

#ifndef _SOUND_SYSTEM_H_
#define _SOUND_SYSTEM_H_

#include <cssysdef.h>
#include <iutil/objreg.h>
#include <isndsys.h>

#define DEFAULT_LISTENER_ROLL_OFF 1.0


/**
 * This is an interface class to the CrystalSpace sound renderer. It works like a
 * wrapper and has some additional functionalities but at the moment it hasn't own
 * data/objects. All it does is simplifying access to the sound renderer. In the
 * future this will be the class that controls the number of active sources.
 */
class SoundSystem
{
public:
    /**
     * Initializes this object by loading the CS sound renderer and creating a
     * global listener.
     * @param objectReg ps iObjectRegistry used to get the sound renderer.
     * @return true on success, false otherwise.
     */
    bool Initialize(iObjectRegistry* objectReg);

    /**
     * Creates a stream out of the given sound data.
     * @param sndData valid iSndSysData object.
     * @param loop true if the sound must loop, false otherwise.
     * @param type 3D type of this sound; it can have these values CS_SND3D_DISABLE=0,
     * CS_SND3D_RELATIVE=1 or CS_SND3D_ABSOLUTE=2.
     * @param sndStream the iSndSysStream object that will contain the stream.
     * @return true on success, false if SoundSystem hasn't been initialized correctly.
     */
    bool CreateStream(csRef<iSndSysData> &sndData, bool loop, int type,
                      csRef<iSndSysStream> &sndStream);

    /**
     * Removes a stream from the sound renderer.
     * @param sndStream iSndSysStream object to remove.
     */
    void RemoveStream(csRef<iSndSysStream> &sndStream);

    /**
     * Create a source associated to your stream with volume 0.
     * @param sndStream your iSndSysStream object.
     * @param sndSource the iSndSysSource object that will contain the source.
     * @return true on success, false if SoundSystem hasn't been initialized correctly.
     */
    bool CreateSource(csRef<iSndSysStream> &sndStream,
                      csRef<iSndSysSource> &sndSource);

    /**
     * Removes a source from the sound renderer.
     * @attention removing the source doesn't remove its stream! Call RemoveStream for
     * that.
     * @param sndSource iSndSysSource object to remove.
     */
    void RemoveSource(csRef<iSndSysSource> &sndSource);

    /**
     * Creates a 3D source on top of a 2D source. It doesn't work if your stream type
     * is CS_SND3D_DISABLE. To remove the 3D source it is enough to remove the original
     * 2D source.
     * @param sndSource your 2D iSndSysSource object.
     * @param sndSource3D the iSndSysSource3D that will contain the 3D source.
     * @param minDist greatest distance at which the sound is played at maximum volume.
     * @param maxDist maximum distance at which the sound can be heard.
     * @param pos 3D position of this source.
     */
    void Create3DSource(csRef<iSndSysSource> &sndSource,
                        csRef<iSndSysSource3D> &sndSource3D,
                        float minDist, float maxDist, csVector3 pos);

    /**
     * Creates a directional source on top of a 3D source. To remove the 3D source it
     * is enough to remove the original 2D source.
     * @param sndSource3D your iSndSysSource3D object.
     * @param sndSourceDir the iSndSysSource3DDirectionalSimple object that will contain
     * the directional source.
     * @param direction direction this source emits to.
     * @param rad radiation of the directional cone.
     */
    void CreateDirectional3DSource(csRef<iSndSysSource3D> &sndSource3D,
                                   csRef<iSndSysSource3DDirectionalSimple> &sndSourceDir,
                                   csVector3 direction, float rad);

    /**
     * Updates listener's position.
     * @param v viewpoint or for that matter hearpoint.
     * @param f front.
     * @param t top.
     */
    void UpdateListener(csVector3 v, csVector3 f, csVector3 t);

    /**
     * Gets the current listener's position.
     * @return a vector containing the listener position. If SoundSystem has not been
     * initialized correctly it returns a null vector (0, 0, 0).
     */
    csVector3 GetListenerPosition() const;

private:
    csRef<iSndSysRenderer> sndRenderer; ///< CrystalSpace sound renderer
    csRef<iSndSysListener> listener;    ///< global listener
};

#endif /*_SOUND_SYSTEM_H_*/
