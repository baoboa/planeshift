/*
 * pawsdndbutton.h - Author: Joe Lyon
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
// pawsbutton.h: interface for the pawsDnDButton class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_DND_BUTTON_HEADER
#define PAWS_DND_BUTTON_HEADER

#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "psslotmgr.h"

//#include "globals.h"
//#include "pscelclient.h"
//#include "net/clientmsghandler.h"
//#include "net/cmdhandler.h"
//#include "paws/pawsnumberpromptwindow.h"

#define DNDBUTTON_DRAGGING 9999

/**
 * \addtogroup common_paws
 * @{ */

/** A Drag-and-Drop capable button widget.
 */
class pawsDnDButton : public pawsButton
{
public:
    pawsDnDButton();
    virtual ~pawsDnDButton();

    virtual bool Setup(iDocumentNode* node);
    bool SelfPopulate(iDocumentNode* node);

    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool OnMouseUp(int button, int modifiers, int x, int y);
    virtual void MouseOver(bool value);

    iPawsImage* GetMaskingImage();

    void SetTextOffsetY( int offset ) 
    {
        upTextOffsetY = offset;
        downTextOffsetY = offset;
    }
    int GetTextOffsetY()
    {
        return upTextOffsetY;
    }
    void SetTextOffsetX( int offset ) 
    {
        upTextOffsetX = offset;
        downTextOffsetX = offset;
    }
    int GetTextOffsetX()
    {
        return upTextOffsetX;
    }
    void SetDrag(int isDragDrop)
    {
        dragDrop = isDragDrop;
    }
    bool PlaceItem(const char* imageName, const char* Name, const char* toolTip, const char* action);

    csArray<csString>* GetImageNameCallback()
    {
        return ImageNameCallback;
    }
    void SetImageNameCallback(csArray<csString>* in)
    {
        ImageNameCallback=in;
    }

    csArray<csString>* GetNameCallback()
    {
        return NameCallback;
    }
    void SetNameCallback(csArray<csString>* in)
    {
        NameCallback=in;
    }
    const char* GetName()
    {
        if(NameCallback)
        {
            if(NameCallback->Get(id-indexBase).Length()>0)
            {
                return NameCallback->Get(id-indexBase).GetData();
            }
        }
        if(GetText())
            return GetText();
        return NULL;
    }

    csArray<csString>* GetActionCallback()
    {
        return ActionCallback;
    }
    void SetActionCallback(csArray<csString>* in)
    {
        ActionCallback=in;
    }

    void SetIndexBase(int indexBase)
    {
        this->indexBase=indexBase;
    }
    int GetIndexBase()
    {
        return indexBase;
    }

    void SetMaskingImage(const char* image);

    int ContainerID()
    {
        return containerID;
    }
    void SetAction(const char* act)
    {
        SetAction(csString(act));
    }
    void SetAction(csString act)
    {
        if(ActionCallback)
        {
            ActionCallback->Get(id-indexBase).Replace(act);
        }
        action = act;
    }
    const char* GetAction()
    {
        if(action)
        {
            if(!action.IsEmpty())
            {
                return action.GetData();
            }
        }
        return NULL;
    }

    void SetDnDLock(bool locked)
    {
        DnDLock = locked;
    }
    bool GetDnDLock()
    {
        return DnDLock;
    }

    /***
     * Remove all contents of button.
     */
    void Clear();

    /***
     * Get Button index number
     */
    int GetButtonIndex()
    {
        return id-indexBase;
    }

    void SetDragDropInProgress(int val);
    int IsDragDropInProgress();

    void DrawMask();

   /**
    * return the name of the font
    */
    char const * GetFontName()
    {
        return fontName;
    }

    void EnableBackground( bool mode );
 

protected:
    psSlotManager*       mgr;
    int                  dragDrop;
    int                  dragDropInProgress;
    csString             action;
    int                  containerID;
    int                  indexBase;
    int                  editMode;
    pawsWidget*          Callback;
    csArray<csString>*   ImageNameCallback;
    csArray<csString>*   NameCallback;
    csArray<csString>*   ActionCallback;
    csString              backgroundBackup;

    virtual bool CheckKeyHandled(int keyCode);

    bool             DnDLock; // true = locked, false = enabled.
};

//----------------------------------------------------------------------
CREATE_PAWS_FACTORY(pawsDnDButton);

/** @} */

#endif
