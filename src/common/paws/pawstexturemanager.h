/*
 * pawstexturemanager.h - Author: Andrew Craig
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
// pawstexturemanager.h: interface for the pawsTextureManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_TEXTURE_MANAGER
#define PAWS_TEXTURE_MANAGER

#include <csutil/ref.h>
#include <csutil/csstring.h>
#include <csutil/objreg.h>
#include <csutil/parray.h>
#include <cstool/cspixmap.h>
#include <igraphic/image.h>
#include <csgeom/csrectrg.h>

#include <ivideo/texture.h>
#include <iutil/vfs.h>
#include <iutil/document.h>

class pawsTextureManager;

/**
 * \addtogroup common_paws
 * @{ */

struct iPawsImage : public virtual iBase
{
    SCF_INTERFACE(iPawsImage, 1, 0, 0);
    virtual const char* GetName() const = 0;

    virtual void Draw(int x, int y, int alpha=-1) = 0;
    virtual void Draw(csRect rect, int alpha=-1) = 0;
    virtual void Draw(int x, int y, int newWidth, int newHeight, int alpha=-1) = 0;

    virtual int  GetWidth() const = 0;
    virtual int  GetHeight() const = 0;
    virtual void ExpandClipRect(csRect &clipRect) = 0;

    virtual int GetDefaultAlpha() const = 0;
    virtual bool IsLoaded() const = 0;
};

class pawsTextureManager
{
public:
    pawsTextureManager(iObjectRegistry* objectReg);
    virtual ~pawsTextureManager();

    /**
     * Loads an image list from xml.
     *
     * The format for an image is:
     * \<image file="/this/path/to/image.png" resource="commonName"\>
     *       \<texturerect x="0" y="0" width="32" height="32" />
     *       \<alpha level="128" /\>
     *       \<trans r="255" g="0" b="255" /\>
     * \</image\>
     *
     * @param listName The VFS path to the list file.
     *
     * @return true if the list was loaded properly.
     */
    bool LoadImageList(const char* listName);

    /**
     * Remove a description from the list.
     *
     * This removes an image from the list once we are totally finished with it.
     * ie there will not be any more refs to it.
     *
     * @param name The description to remove from the list.
     */
    void Remove(const char* name);

    csPtr<iPawsImage> GetPawsImage(const char* name);
    csPtr<iPawsImage> GetOrAddPawsImage(const char* name);
    bool AddImage(const char* resource);
    void AddPawsImage(csRef<iPawsImage> &element);

public:
    iObjectRegistry* objectReg;

    csRef<iVFS> vfs;
    csRef<iDocumentSystem>  xml;

    csHash<csRef<iPawsImage>, csString> elementList;
};

/** @} */

#endif


