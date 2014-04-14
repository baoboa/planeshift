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


#ifndef PAWS_CONFIG_CAMERA_HEADER
#define PAWS_CONFIG_CAMERA_HEADER

// CS INCLUDES
#include <csutil/array.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "pawsconfigwindow.h"

class psCamera;

//////////////////////////////////////////////////////////////////////////////
//                        pawsConfigCamera
//////////////////////////////////////////////////////////////////////////////


class pawsConfigCamera : public pawsConfigSectionWindow
{
public:
    pawsConfigCamera();
    virtual ~pawsConfigCamera();
    
    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

     // from pawsWidget:
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
   
protected:

    /** Grabs a camera setting from the camera
     *   @param settingName the name of the setting
     *   @param mode the camera mode
     *   @return the value of the specific camera setting (for bool: +ve means true, -ve means false)
     */
    float GetCameraSetting(const csString& settingName, int mode) const;

    /** applies a camera setting to the camera
     *   @param settingName the name of the setting
     *   @param value the new value (for bool: +ve means true, -ve means false)
     *   @param mode the camera mode
     */
    void SetCameraSetting(const csString& settingName, float value, int mode) const;

    /** a recursive function to apply the tree's settings to the camera
     *   @param subtreeRoot the root of the current subtree
     */
    void SaveConfigByTreeNodes(pawsTreeNode * subtreeRoot);

    /** Creates the tree that makes the whole GUI.
     * @return  success.
     */
    bool CreateTree();
     
    // Adds widgets to all nodes in subtree with root 'subtreeRoot'
    //   -- command label, key combination label and button that opens FingeringWindow
    void CreateTreeWidgets(pawsTreeNode * subtreeRoot);

    // Sets all labels (in subtree with root 'subtreeRoot')
    void SetLabels(pawsTreeNode * subtreeRoot);

	/** 
     * Sets all values to the ones that have the camera (in subtree with root 'subtreeRoot')
     */
    void SetCameraValues(pawsTreeNode * subtreeRoot);

    float labelWidth;
    float inputWidth;

    pawsTree * tree;                    // the tree that makes whole window GUI
};




class pawsConfigCameraFactory : public pawsWidgetFactory
{
public:
    pawsConfigCameraFactory()
    {
        Register( "pawsConfigCamera" );
    }
    pawsWidget* Create()
    {
        return new pawsConfigCamera();
    }
};



#endif 
