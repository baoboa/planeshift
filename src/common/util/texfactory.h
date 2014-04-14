/*
 * texfactory.h by Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __TEXFACT_H__
#define __TEXFACT_H__

#include <csutil/parray.h>
#include <igraphic/image.h>
#include <iutil/objreg.h>

#include "util/psxmlparser.h"


class psImageRegion;

/**
 * \addtogroup common_util
 * @{ */

/**
 * This class reads in the xml file defining regions and maintains
 * the preload list of textures.  When a new player texture is required,
 * the xml string specifying the parts to be merged is passed to this
 * class and this class fabricates the new texture from it.
 */
class psTextureFactory
{
protected:

    iObjectRegistry* object_reg;

    csPDelArray<psImageRegion>  regions;    // List of psImageRegions, loaded from racelib.xml file.
    csPDelArray<iImage>         imagecache; // List of iImage's, loaded on demand and saved in a list for future use

    /**
     * Loads an XML section of zero or more region tags.  Each region is
     * added to the regions list for fast future access.
     */
    bool LoadRace(iDocumentNode * raceNode);

    /**
     * Returns a pointer to a race-specific texture region.
     * This pointer can be passed into psTextureFactory to overlay the image.
     */
    psImageRegion *GetRegion(const char *race,const char *part);

    /**
     * Returns a pointer to an iImage specified by the race and filename.
     * This function searches the existing image file list for the name.
     * If it is found in memory already, the ptr is returned immediately,
     * otherwise the image is loaded, added to the cache list, and returned.
     */
    csPtr<iImage> GetImage(const char *race,const char *filename);

public:

    psTextureFactory();
    ~psTextureFactory();

    /**
     * This function loads the xml file and cycles through the race
     * tags, calling LoadRace() for each one to parse the region lists.
     */
    bool Initialize(iObjectRegistry* object_reg,const char *xmlfilename);

    /**
     * Takes an xml string specifying race textures and part textures,
     * finds the relevant regions and images, and overlays the regions
     * onto the base texture.  If this is all successful, a new iImage
     * ptr is returned to the caller.  Otherwise, NULL.
     */
    csPtr<iImage> CreateTextureImage(const char *xmlspec);
};


/**
 * This struct just stores the left and right side of each line
 * in a psRegion.
 */
struct psScanline
{
    int x1,x2,y;

    psScanline() { x1=x2=y=0; }

    psScanline(int l,int r, int h)
    {
        x1=l;
        x2=r;
        y=h;
    };
};



/**
 * This class stores all required info for a particular region.
 * A region is defined as a list of scanlines.  They do not have to be
 * contiguous or convex.  OverlayRegion copies the pixels from
 * one specified image in the region into the other image.  This is
 * the main workhorse function here.
 */
class psImageRegion
{
protected:
    csPDelArray<psScanline> scanlines;

public:
    csString race;
    csString part;

    psImageRegion(const char *racename,const char *partname);
    psImageRegion() { };
    
    ~psImageRegion();

    /**
     * Scans through the xml string and pulls out all \<line\> tags
     * making a list of psScanlines from the tags.
     */
    bool CreateRegion(iDocumentNode* node);
    void AddScanline( psScanline* line ) { scanlines.Push(line); }

    psScanline* GetLine(int x);

    /**
     * Creates a list of scanlines to make a region from
     * the rectangle specified by the coordinates.
     */
    bool CreateRectRegion(int left, int top, int right, int bottom);

    /**
     * Copies the pixels from the src image, in this region
     * as specified by the list of scanlines, into the dest pixels.
     *
     * Bitmode is intended to allow different types of blending
     * but is not used at this time.
     */
    void OverlayRegion(iImage *dest, iImage *src,int bitmode=0);
};

/** @} */

#endif

