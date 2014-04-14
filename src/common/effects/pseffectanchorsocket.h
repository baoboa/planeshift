/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
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

#ifndef PS_EFFECT_ANCHOR_SOCKET_HEADER
#define PS_EFFECT_ANCHOR_SOCKET_HEADER

#include "pseffectanchor.h"

struct iSpriteCal3DState;
struct iSpriteCal3DSocket;

/**
 * \addtogroup common_effects
 * @{ */

class psEffectAnchorSocket : public psEffectAnchor
{
public:

    psEffectAnchorSocket();
    ~psEffectAnchorSocket();

    // inheritted function overloads
    bool Load(iDocumentNode* node);
    bool Create(const csVector3 &offset, iMeshWrapper* posAttach, bool rotateWithMesh = false);
    bool Update(csTicks elapsed);
    psEffectAnchor* Clone() const;

    // these functions are overridden, because the parent mesh takes care of position/sectoring
    // we want to override these functions, because the default behaviour is for the effect to manage it
    void SetRotBase(const csMatrix3 & /*newRotBase*/) {}
    void SetPosition(const csVector3 & /*basePos*/, iSector* /*sector*/, const csMatrix3 & /*transf*/) {}
    void SetPosition(const csVector3 & /*basePos*/, iSectorList* /*sectors*/, const csMatrix3 & /*transf*/) {}

    /** Gets the name of the socket where this anchor is attached.
     *   @return the name of the socket.
     */
    const csString &GetSocketName() const
    {
        return socketName;
    }

    /** Sets the socket where this anchor is attached.
     *   @param name the name of the socket.
     */
    void SetSocket(const char* name);

private:

    csString socketName;
    iSpriteCal3DSocket* socket;
    size_t meshID;
    csVector3 initPos;
    csRef<iSpriteCal3DState> cal3d;
};

/** @} */

#endif
