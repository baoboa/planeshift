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

#ifndef EEDIT_EDIT_EFFECT_TOOLBOX_HEADER
#define EEDIT_EDIT_EFFECT_TOOLBOX_HEADER

#include "eedittoolbox.h"
#include "paws/pawswidget.h"

#include "eeditselectstring.h"
#include "eeditselectvec3.h"
#include "eeditselectfloat.h"
#include "eeditselectyesno.h"
#include "eeditselectnewanchor.h"

class pawsButton;
class pawsListBox;

class psEffect;
class psEffectAnchor;
class psEffectAnchorSocket;
class psEffectObj;
class pawsTree;
class pawsSimpleTreeNode;
class pawsSeqTreeNode;

/**
 * \addtogroup eedit
 * @{ */

/**
 * This allows you to edit the effect.
 */
class EEditEditEffectToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditEditEffectToolbox>
{
public:
    EEditEditEffectToolbox();
    virtual ~EEditEditEffectToolbox();

    /**
     * Loads the given effect tree.
     * 
     * @param effect the effect to load
     */
    void LoadEffect(psEffect * effect);
    
    // inheritted from EEditToolbox
    virtual void Update(unsigned int elapsed);
    virtual size_t GetType() const;
    virtual const char * GetName() const;
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

    /**
     * Called whenever a widget is selected.
     *
     * @param widget The selected widget.
     * @return false.
     */
    virtual bool OnSelected( pawsWidget* widget);
    virtual void OnUpdateData(const char *dataname,PAWSData& data);

    enum ANCHOR_TYPE
    {
        AT_NONE = 0,
        AT_BASIC,
        AT_SPLINE,
        AT_SOCKET,

        AT_COUNT
    };

private:

    /** Updates the given effect according to the given data.
     */
    void UpdateEffectData(psEffect * effect, char owner, char attr, int index, int index2, int index3, PAWSData & data);

    /** Builds the tree root nodes with anchors and object nodes (deletes the old ones if they exist).
     */
    void RebuildTreeRoot();
    
    /** Displays anchor data in the given tree node.
     *   @param node the tree node that will display the data.
     *   @param anchor the anchor to visualize.
     *   @param anchorType the type of anchor we're visualizing.
     *                      One Of:
     *                      <ul>
     *                        <li>AT_BASIC
     *                        <li>AT_SPLINE
     *                        <li>AT_SOCKET
     *                      </ul>
     */
    void BuildAnchorNode(pawsSimpleTreeNode * node, psEffectAnchor * anchor, unsigned char anchorType);
    
    /** Displays direction anchor attribute data in the given tree node.
     *   @param node the tree node that will display the data.
     *   @param anchor the anchor to visualize.
     *   @param index the index of the anchor so that it can be accessed with psEffect->GetAnchor(index).
     */
    void BuildDirectionNode(pawsSeqTreeNode * node, psEffectAnchor * anchor, size_t index);
    
    /** Displays socket anchor attribute data in the given tree node.
     *   @param node the tree node that will display the data.
     *   @param anchor the anchor to visualize.
     *   @param index the index of the anchor so that it can be accessed with psEffect->GetAnchor(index).
     */
    void BuildSocketNode(pawsSeqTreeNode * node, psEffectAnchorSocket * anchor, size_t index);
        
    /** Displays anchor keyframe data in the given tree node.
     *   @param node the tree node that will display the data.
     *   @param anchor the anchor to visualize.
     *   @param index the index of the anchor so that it can be accessed with psEffect->GetAnchor(index).
     *   @param keyIndex the index of the keyframe so that it can be accessed with psEffectAnchor->GetKeyFrame(index).
     */
    void BuildAnchorKeyFrameNode(pawsSeqTreeNode * node, psEffectAnchor * anchor, size_t index, size_t keyIndex);
    
    /** Places the given effect anchor into the effect items tree.
     *   @param anchor the anchor to put in the tree.
     *   @param index the index of this anchor according to the parent effect.
     */
    void LoadEffectAnchor(psEffectAnchor * anchor, size_t index);

    /** Places the given effect object into the effect items tree.
     *   @param obj the object to put in the tree.
     *   @param index the index of this object according to the parent effect.
     */
    void LoadEffectObject(psEffectObj * obj, size_t index);

    // store a reference to the effect we're currently editing
    psEffect * currEffect;
    csString currEffectName;

    // effect tree stuff
    pawsTree * effectItemsTree;
    pawsSimpleTreeNode * effectItemsRoot;
    pawsSimpleTreeNode * effectAnchorsRoot;
    pawsSimpleTreeNode * effectObjsRoot;
};

CREATE_PAWS_FACTORY(EEditEditEffectToolbox);

/** @} */

#endif 
