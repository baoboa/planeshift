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

#ifndef EEDIT_INPUTBOX_MANAGER_HEADER
#define EEDIT_INPUTBOX_MANAGER_HEADER

#include <csutil/csstring.h>
#include <csgeom/vector3.h>


class PawsManager;

class EEditSelectFloat;
class EEditSelectString;
class EEditSelectVec3;
class EEditSelectYesNo;
class EEditSelectList;
class EEditSelectNewAnchor;
class EEditSelectEditAnchor;
class EEditSelectNewAnchorKeyFrame;

/**
 * \addtogroup eedit
 * @{ */
/** A class that manages the inputbox windows.
 */
class EEditInputboxManager
{
public:
    EEditInputboxManager();
    ~EEditInputboxManager();

    /**
     * Loads all the toolbox widgets.
     *
     * @param paws a pointer to a valid PawsManager
     * @return true on success, false otherwise.
     */
    bool LoadWidgets(PawsManager * paws);

    /**
     * Registers all the factories needed by the toolboxes.
     *
     * @return true on success, false otherwise.
     */
    bool RegisterFactories() const;

    // Select Float
    class iSelectFloat
    {
    public:
        virtual void Select(float value) = 0;
        virtual ~iSelectFloat() {};
    };
    void SelectFloat(float startValue, iSelectFloat * callback);

    // Select String
    class iSelectString
    {
    public:
        virtual void Select(const csString & value) = 0;
        virtual ~iSelectString() {};
    };
    void SelectString(const csString & startValue, iSelectString * callback);

    // Select 3D Vector
    class iSelectVec3
    {
    public:
        virtual void Select(const csVector3 & value) = 0;
        virtual ~iSelectVec3() {};
    };
    void SelectVec3(const csVector3 & startValue, iSelectVec3 * callback);
    
    // Select YesNo
    class iSelectYesNo
    {
    public:
        virtual void Select(bool value) = 0;
        virtual ~iSelectYesNo() {};
    };
    void SelectYesNo(bool startValue, iSelectYesNo * callback);
    
    // Select List
    class iSelectList
    {
    public:
        virtual void Select(const csString & value, size_t index) = 0;
        virtual ~iSelectList() {};
    };
    void SelectList(csString * list, size_t listCount, const csString & defaultValue, iSelectList * callback);
    
    // Select NewAnchor
    class iSelectNewAnchor
    {
    public:
        virtual void Select(const csString & newName, const csString & newAnchorType) = 0;
        virtual ~iSelectNewAnchor() {};
    };
    void SelectNewAnchor(iSelectNewAnchor * callback);

    // Select EditAnchor
    class iSelectEditAnchor
    {
    public:
        virtual void ChangeName(const csString & newName) = 0;
        virtual void DeleteAnchor() = 0;
        virtual ~iSelectEditAnchor() {};
    };
    void SelectEditAnchor(const csString & currName, iSelectEditAnchor * callback);
    
    // Select NewAnchorKeyFrame
    class iSelectNewAnchorKeyFrame
    {
    public:
        virtual void Select(float newTime) = 0;
        virtual ~iSelectNewAnchorKeyFrame() {};
    };
    void SelectNewAnchorKeyFrame(iSelectNewAnchorKeyFrame * callback);

    class iSelectEditAnchorKeyFrame
    {
    public:
        virtual void Select(float newTime) = 0;
        virtual ~iSelectEditAnchorKeyFrame() {};
    };

private:
  
    EEditSelectFloat             * selectFloat;
    EEditSelectString            * selectString;
    EEditSelectVec3              * selectVec3;
    EEditSelectYesNo             * selectYesNo;
    EEditSelectList              * selectList;
    EEditSelectNewAnchor         * selectNewAnchor;
    EEditSelectEditAnchor        * selectEditAnchor;
    EEditSelectNewAnchorKeyFrame * selectNewAnchorKeyFrame;
};

/** @} */

#endif
