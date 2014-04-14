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

/**
 * This is an Interface Class to the Crystalspace Soundrenderer.
 * It works like a Wrapper and has some additional functionalities but
 * no own data/objects. All it does is simplifying access to the Soundrenderer.  
 */

class SoundSystem
{
    public:
    /**
     * initializes this object and tries to load the soundrenderer.
     * returns true or false
     * @param objectReg ps objectreg where we find our soundrenderer
     */
    bool Initialize(iObjectRegistry* objectReg);

    /**
     * Creates a stream out of the given snddata.
     * @param snddata valid iSndSysData object
     * @param loop LOOP or DONT_LOOP
     * @param type 3dtype of this sound CS_SND3D_DISABLE=0 CS_SND3D_RELATIVE=1 or CS_SND3D_ABSOLUTE=2
     * @param sndstream your iSndSysStream object
     */
    bool CreateStream (csRef<iSndSysData> &snddata, int loop, int type,
                       csRef<iSndSysStream> &sndstream);

    /**
     * Removes a stream.
     * @param sndstream iSndSysStream object to remove
     */
    void RemoveStream(csRef<iSndSysStream> &sndstream);

    /**
     * Create a Source associated to your Stream.
     * @param sndstream your iSndSysStream object
     * @param sndsource your iSndSysSource object
     */
    bool CreateSource (csRef<iSndSysStream> &sndstream,
                       csRef<iSndSysSource> &sndsource);

    /**
     * Removes a Source.
     * @param sndsource iSndSysSource object to remove
     */
    void RemoveSource (csRef<iSndSysSource> &sndsource);

    /**
     * Creates a 3d Source on top of a 2d source.
     * Doesnt work of your stream type is CS_SND3D_DISABLE
     * @param sndsource your iSndSysSource object
     * @param sndsource3d your iSndSysSource3D object
     * @param mindist distance when volume is at max
     * @param maxdist distance when volume is at min
     * @param pos 3d position of this source
     */
    void Create3dSource (csRef<iSndSysSource> &sndsource,
                         csRef<iSndSysSource3D> &sndsource3d,
                         float mindist, float maxdist, csVector3 pos);

    /**
     * Creates a directional source on top of a 3d source.
     * @param sndsource3d your iSndSysSource3D object
     * @param sndsourcedir your iSndSysSource3DDirectionalSimple object
     * @param direction direction this source is emitting to
     * @param rad radiation of the directional cone
     */
    void CreateDirectional3dSource (csRef<iSndSysSource3D> &sndsource3d,
                                    csRef<iSndSysSource3DDirectionalSimple> &sndsourcedir,
                                    csVector3 direction, float rad);

    /**
     * Updates listener position
     * @param v viewpoint or for that matter hearpoint
     * @param f front
     * @param t top
     */
    void UpdateListener( csVector3 v, csVector3 f, csVector3 t );

    private:
    csRef<iSndSysRenderer> sndrenderer; ///< soundrenderer were using
    csRef<iSndSysListener> listener;    ///< our listener object
};

#endif /*_SOUND_SYSTEM_H_*/
