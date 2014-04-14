/*
 * pawstree.h - Author: Ondrej Hurt
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

#ifndef PAWS_TREE_HEADER
#define PAWS_TREE_HEADER

#include <csutil/parray.h>
#include <iutil/document.h>
#include "pawswidget.h"
#include "pawstextbox.h"
#include "pawsmanager.h"
#include "pawscrollbar.h"

/**
 * \addtogroup common_paws
 * @{ */

#define TREE_MOUSE_SCROLL_AMOUNT 20
/**
    How to create tree by xml:
    root widget must be pawsTreeNode
    it can have childs as pawsTreeNode, pawsWidgetTreeNode and pawsSeqTreeNode.
    pawsTreeNode is a simple node that can have it's own childs, but don't contain any widgets
    pawsWidgetTreeNode is a tree node that contains one widget and child nodes
    pawsSeqTreeNode is a tree node with sequence of widgets, placed in sequence. it also can have child nodes.
    Widgets in pawsWidgetTreeNode and pawsSeqTreeNode must be placed beetween <nodewidget>..</nodewidget>
    example for tree creation:

    <widget name="treeRoot" factory="pawsTreeNode" <!-- some attrs -->>
    <!-- setup environment -->
        <widget name="firstNode" factory="pawsTreeNode">
            <widget name="seqNode1" factory="pawsSeqTreNode">
                <nodewidget>
                    <widget name="someWidget1" factroy="paws***">
                    </widget>
                    <widget name="someWidget2" factroy="paws***">
                    </widget>
                    <!-- some oter widgets -->
                </nodewidget>
                <!-- node children -->
            </widget>
            <widget name="seqNode2" factory="pawsSeqTreNode">
                    <widget name="someWidget3" factroy="paws***">
                    </widget>
            </widget>
        </widget>
        <widget name="secondNode">
            <widget name="widTreeNode" factory="pawsWidgetTreeNode">
                <nodewidget>
                    <!-- some widget -->
                </nodewidget>
                <!-- node children -->
            </widget>
        </widget>
    </widget>
*/

class pawsTreeNode;

class pawsITreeStruct
{
public:
    virtual void NodeChanged()=0;
    // this is called by tree nodes when they change
    virtual void NewNode(pawsTreeNode* node)=0;
    // OWNERSHIP of 'node' goes to callee
    virtual void RemoveNode(pawsTreeNode* node)=0;
    // OWNERSHIP of 'node' goes to caller
    virtual ~pawsITreeStruct() {};
};

//////////////////////////////////////////////////////////////////////
//                            pawsTreeNode
//////////////////////////////////////////////////////////////////////

struct TreeNodeAttribute
{
    csString name,value;
};


// pawsTreeNode summarizes everything that is common to all types of tree
// control nodes

class pawsTreeNode : public pawsWidget
{
public:
    pawsTreeNode();
    pawsTreeNode(const pawsTreeNode &origin);
    virtual ~pawsTreeNode();

    // from pawsWidget:
    virtual bool Load(iDocumentNode* node);


    // Sets the tree that the node belongs to
    void SetTree(pawsITreeStruct* tree);

    // Set first children. ONLY CALLED BY PAWSTREE'S COPY CONSTRUCTOR!!!
    void SetFirstChild(pawsTreeNode* child);

    // Attributes - you can set and retrieve named string values for each node.
    csString GetAttr(const csString &name);
    void SetAttr(const csString &name, const csString &value);

    void SetParent(pawsTreeNode* parent);
    void SetPrevSibling(pawsTreeNode* node);
    void SetNextSibling(pawsTreeNode* node);

    pawsTreeNode* GetParent();
    pawsTreeNode* GetFirstChild();
    pawsTreeNode* FindLastChild();
    pawsTreeNode* GetPrevSibling();
    pawsTreeNode* GetNextSibling();
    pawsTreeNode* FindLastSibling();

    pawsTreeNode* FindNodeAbove();
    // Finds node that is positioned right above our node
    pawsTreeNode* FindNodeBelow();
    // Finds node that is positioned right below our node
    pawsTreeNode* FindLowestSubtreeNode();
    // Finds lowest node in subtree where our node is root

    virtual void InsertChild(pawsTreeNode* node, pawsTreeNode* nextSibling = NULL);
    // inserts 'node' before 'nextSibling' (NULL=append)
    // OWNERSHIP of 'node' goes to pawsTreeNode
    virtual void MoveChild(pawsTreeNode* node, pawsTreeNode* nextSibling = NULL);
    // moves 'node' before 'nextSibling' (NULL=append)
    virtual void RemoveChild(pawsTreeNode* node);
    // removes 'node' from children (without destructing it)
    // OWNERSHIP of 'node' goes to CALLER
    virtual void DeleteChild(pawsTreeNode* node);
    // deletes 'node' from children and destructs it (so the pointer is no longer valid)
    // children of 'node' are deleted too
    virtual void Clear();
    // DeleteChild() for all children

    pawsTreeNode* FindChildByName(const csString &name, bool indirectToo);
    // Finds child nu its name
    pawsTreeNode* FindNodeByPath(const csString &path);
    // Finds node using slash-separated list of names of the node and its parents.
    // e.g. "options/controls/keyboard"

    virtual void SetCollapsable(bool collapsable);
    // sets if node can be collapsed
    virtual bool IsCollapsable()
    {
        return collapsable;
    }
    virtual void Expand();
    virtual void ExpandAll();
    virtual void Collapse();
    virtual void CollapseAll();
    virtual bool IsCollapsed();
    virtual bool BuriedInRuins();
    //determines if this node is hidden because of collapsed parents (including indirect parents)


//    virtual pawsTreeNode * Copy() = 0;

    int GetRowNum();
    // Calculates position of our node in tree (positions are starting at 0)

protected:
    void SetChildrenVisibAfterCollapseChange(bool expanded);
    // calls either Show() or Hide() method for all children

    pawsITreeStruct* tree;            //tree our node belongs to
    pawsTreeNode* parent;             //NULL in root node
    pawsTreeNode* firstChild;
    pawsTreeNode* prevSibling, * nextSibling;         //NULL in first/last sibling

    csArray<TreeNodeAttribute> attrList;          //attributes associated with node
    bool collapsable;                 //can node by collapsed ?
    bool collapsed;                   //is node currently collapsed ?
};


//////////////////////////////////////////////////////////////////////
//                         pawsTreeStruct
//////////////////////////////////////////////////////////////////////




// pawsTreeStruct maintains tree structure
// it is NOT able not draw itself on screen etc.

class pawsTreeStruct : public pawsITreeStruct
{
public:
    pawsTreeStruct();
    virtual ~pawsTreeStruct();

    // from pawsWidget:
    virtual bool Load(iDocumentNode* node);

    // from pawsITreeStruct:
    virtual void NodeChanged();
    virtual void NewNode(pawsTreeNode* node)=0;
    virtual void RemoveNode(pawsTreeNode* node)=0;

    pawsTreeNode* GetRoot();
    virtual void SetRoot(pawsTreeNode* root);
    //OWNERSHIP of 'root' goes to pawsTreeStruct

    // Manipulation with nodes using pointers:
    virtual void InsertChild(pawsTreeNode* parent, pawsTreeNode* node, pawsTreeNode* nextSibling = NULL);
    virtual void MoveChild(pawsTreeNode* node, pawsTreeNode* nextSibling = NULL);
    virtual void RemoveChild(pawsTreeNode* node);
    virtual void DeleteChild(pawsTreeNode* node);
    virtual void Clear();

    // Manipulation with nodes using their names:
    virtual void InsertChild(const csString &parent, pawsTreeNode* node, const csString &nextSibling);
    virtual void InsertChild(const csString &parent, pawsTreeNode* node);
    virtual void MoveChild(const csString &name, const csString &nextSibling);
    virtual void DeleteChild(const csString &name);


    pawsTreeNode* FindNodeByName(const csString &name);
    // searches for first node of name 'name' in whole tree, NULL means not found

    pawsTreeNode* FindNodeAt(pawsTreeNode* parent, int x, int y);
    // returns the node that the [x, y] coordinates point to
    // NULL means no such node
    int GetVersion()
    {
        return version;
    }

protected:
    pawsTreeNode* root;
    int version;    // This number is increased every time the NodeChanged() method is called.
    // Its purpose is to detect changes made in tree.
};


//////////////////////////////////////////////////////////////////////
//                            pawsTree
//////////////////////////////////////////////////////////////////////

class pawsITreeLayout;
class pawsITreeDecorator;


// pawsTree is customisable tree widget
// You will probably want to use pawsSimpleTree instead because it's easier.

class pawsTree : public pawsWidget, public pawsTreeStruct
{
public:
    pawsTree();
    pawsTree(const pawsTree &origin);
    virtual ~pawsTree();

    // from pawsWidget:
    virtual bool Setup(iDocumentNode* node);
    virtual bool PostSetup();

    virtual void Draw();
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers);
    virtual bool OnScroll(int scrollDirection, pawsScrollBar* widget);
    virtual bool LoadChildren(iDocumentNode* node);

    // from pawsITreeStruct:
    virtual void NewNode(pawsTreeNode* node);
    virtual void RemoveNode(pawsTreeNode* node);

    // from pawsTreeStruct:
    virtual void SetRoot(pawsTreeNode* root);


    void SetTreeLayout(pawsITreeLayout* layout);
    // OWNERSHIP of 'layout' goes to pawsTree
    void SetTreeDecorator(pawsITreeDecorator* decor);
    // decor==NULL disables decorations
    // OWNERSHIP of 'decor' goes to pawsTree

    void SetScrollBars(bool horiz, bool vert);
    // sets visibility of scrollbars

    // Multiselect is not implemented directly, use pawsICheckTreeNode tree nodes instead
    virtual void Select(pawsTreeNode*);
    virtual void Deselect();
    pawsTreeNode* GetSelected();

    void SetNotify(pawsWidget* _notificationTarget);
    // sets widget that is target of OnSelected() events

protected:
    void SetScrollBarMax();
    void cloneTreeNodes(const pawsTree &origin);

    pawsITreeLayout* layout;
    pawsITreeDecorator* decor;

    pawsTreeNode* selected;            // selected node
    pawsWidget* notificationTarget;    // target of tree events
    pawsScrollBar* horizScrollBar, * vertScrollBar;
};


//////////////////////////////////////////////////////////////////////
//                            tree layout
//////////////////////////////////////////////////////////////////////

// pawsITreeLayout classes calculate and set position of tree nodes
class pawsITreeLayout
{
public:
    virtual void SetLayout() = 0;
    // Sets position of 'node' and all nodes beneath
    virtual void SetHorizScroll(int horizScroll) = 0;
    virtual void SetVertScroll(int vertScroll) = 0;
    virtual void GetTreeSize(int &width, int &height) = 0;
    virtual ~pawsITreeLayout() {};
};

class pawsStdTreeLayout : public pawsITreeLayout
{
public:
    pawsStdTreeLayout(pawsTree* tree, int rowSpacing, int levelSpacing);
    virtual ~pawsStdTreeLayout();
    // rowSpacing   = space between rows
    // levelSpacing = difference in X coordinate of two levels

    // from pawsITreeLayout:
    virtual void SetLayout();
    virtual void SetHorizScroll(int horizScroll);
    virtual void SetVertScroll(int vertScroll);
    virtual void GetTreeSize(int &width, int &height);

protected:
    void SetSubtreeLayout(pawsTreeNode* subtreeRoot, int x, int y, int &maxX, int &maxY);
    // Moves 'subtreeRoot' to [x , y] and all children accordingly
    // 'maxX' and 'maxY' are the greates coordinates where the subtree reaches on screen

    pawsTree* tree;
    int rowSpacing, levelSpacing;
    int horizScroll, vertScroll;

    int width, height;      // size of tree calculated by object during last SetLayout()

    int lastVersion;        // value of pawsTreeStruct::version at the time the SetLayout() method was last called
};


//////////////////////////////////////////////////////////////////////
//                            tree decorator
//////////////////////////////////////////////////////////////////////

// pawsITreeDecorator classes are used to paint all graphics, except graphics of nodes
// e.g. lines connecting nodes.
// It also receives user input related to graphics painted by itself.

class pawsITreeDecorator
{
public:
    virtual ~pawsITreeDecorator() {};
    virtual void Decorate() = 0;
    virtual bool OnMouseDown(int button, int modifiers, int x, int y) = 0;
};

class pawsStdTreeDecorator : public pawsITreeDecorator
{
public:
    pawsStdTreeDecorator(pawsTree* tree, iGraphics2D* g2d, int selectedColor, int lineColor, int collSpacing);
    virtual ~pawsStdTreeDecorator();

    // from pawsITreeDecorator;
    virtual void Decorate();
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);

protected:
    void DecorateSubtree(pawsTreeNode* node);
    // paints graphics around nodes of subtree with root 'subtreeRoot'
    pawsTreeNode* FindCollapsingNodeInSubtree(pawsTreeNode* subtreeRoot, int x, int y);
    // finds collapse/expand sign that is being pointed to by [x, y] in subtree with root 'subtreeRoot'
    void GetCollapseSignFrame(pawsTreeNode* node, csRect &rect);
    // calculates position of collapse/expand sign of 'node' on screen

    pawsTree* tree;
    iGraphics2D* g2d;
    int selectedColor;    // background color of selected node
    int lineColor;        // color of lines
    int collSpacing;      // distance between node and expand/collapse sign

    csRef<iPawsImage> collImage;
    csRef<iPawsImage> expandImage;
};



//////////////////////////////////////////////////////////////////////
//                  miscellaneous tree node types
//////////////////////////////////////////////////////////////////////

// class pawsWidgetTreeNode
class pawsWidgetTreeNode : public pawsTreeNode
{
public:
    pawsWidgetTreeNode();
    pawsWidgetTreeNode(const pawsWidgetTreeNode &origin);
    //from pawsWidget:
    virtual bool Load(iDocumentNode* node);

    void SetWidget(pawsWidget* widget);
    // Assigns the widget that makes main body of the node.
    // OWNERSHIP of 'widget' goes to pawsWidgetTreeNode
protected:
    pawsWidget* widget;
};

// iCheckTreeNode is common interface for tree nodes that have some kind of checkbox inside
// (something that is represented by boolean value)

class pawsICheckTreeNode
{
public:
    virtual bool GetCheck() = 0;
    virtual void SetCheck(bool ch) = 0;
    virtual ~pawsICheckTreeNode() {};
};

// pawsCheckTreeNode is standard implementation of iCheckTreeNode
// The node is made of a checkbox and arbitrary additional widget following the checkbox

class pawsCheckTreeNode : public pawsICheckTreeNode, public pawsTreeNode
{
public:
    pawsCheckTreeNode();
    pawsCheckTreeNode(const pawsCheckTreeNode &origin);
    virtual ~pawsCheckTreeNode() {};

    //from pawsICheckTreeNode:
    virtual bool GetCheck();
    virtual void SetCheck(bool ch);


    virtual void SetWidget(pawsWidget* widget);
    // set widget that follows the checkbox
    // OWNERSHIP of widget goes to pawsCheckTreeNode

protected:
//    pawsCheckbox * checkbox;
    pawsWidget* widget;
};



class pawsSeqTreeNode_widget
{
public:
    pawsSeqTreeNode_widget(pawsWidget* w, int width);
    pawsWidget* widget;
    int width;
};

// pawsSeqTreeNode node is sequence of arbitrary widgets that are drawn from left to right.

class pawsSeqTreeNode : public pawsTreeNode
{
public:
    pawsSeqTreeNode()
    {
        factory = "pawsSeqTreeNode";
    }
    pawsSeqTreeNode(const pawsSeqTreeNode &origin);
    //from pawsWidget:
    virtual bool Load(iDocumentNode* node);

    virtual void AddSeqWidget(pawsWidget* widget, int width);
    // adds 'widget' and reserves 'width' space for it (this does not have to be equal to widget width)
    // OWNERSHIP of widget goes to pawsSeqTreeNode
    virtual void AddSeqWidget(pawsWidget* widget);
    // calls AddWidget() with width of widget
    // OWNERSHIP of widget goes to pawsSeqTreeNode
    virtual pawsWidget* GetSeqWidget(int index);

    virtual void Draw();

protected:
    csList<pawsSeqTreeNode_widget> widgets;
};

//////////////////////////////////////////////////////////////////////
//                         pawsSimpleTree
//////////////////////////////////////////////////////////////////////

// pawsSimpleTreeNode is tree node which can comprise of checkbox, image and text label.
// (checkbox is drawn first, image second, label third)
// All parts are optional.


enum {showCheckBox=1, showImage=2, showLabel=4};

class pawsSimpleTreeNode : public pawsCheckTreeNode
{
public:
    pawsSimpleTreeNode();
    pawsSimpleTreeNode(const pawsSimpleTreeNode &origin);
    virtual ~pawsSimpleTreeNode();

    // from pawsWidget:
    virtual bool Load(iDocumentNode* node);


    virtual void Set(int mode, bool checked, const csString &imageName, const csString &label);
    // This method sets content of the node.
    // 'mode' is something like "showCheckBox | showLabel"
    // Node must already be in a tree before you can call this method.

protected:
    pawsTextBox* textBox;
    pawsWidget* image;

};



// class pawsSimpleTree is tree with nodes of type pawsSimpleTreeNode
// Its purpose is to provide simple tree control that will fit for most situations

class pawsSimpleTree : public pawsTree
{
public:
    pawsSimpleTree();
    pawsSimpleTree(const pawsSimpleTree &origin);
    // from pawsWidget:
    virtual bool Setup(iDocumentNode* node);

    // Those methods set the default color used by nodes of the tree:
    int  GetDefaultColor();
    void SetDefaultColor(int);

    virtual void InsertChildL(const csString &parent, const csString &name, const csString &label, const csString &nextSibling);
    // creates and inserts node that contains label
    virtual void InsertChildI(const csString &parent, const csString &name, const csString &image, const csString &nextSibling);
    // creates and inserts node that contains image
    virtual void InsertChildIL(const csString &parent, const csString &name, const csString &image, const csString &label, const csString &nextSibling);
    // creates and inserts node that contains label and image
protected:
    int defaultColor;
};


//////////////////////////////////////////////////////////////////////
//                         class factories
//////////////////////////////////////////////////////////////////////


CREATE_PAWS_FACTORY(pawsTree);
CREATE_PAWS_FACTORY(pawsSimpleTree);
CREATE_PAWS_FACTORY(pawsSimpleTreeNode);
CREATE_PAWS_FACTORY(pawsTreeNode);
CREATE_PAWS_FACTORY(pawsSeqTreeNode);
CREATE_PAWS_FACTORY(pawsWidgetTreeNode);

/** @} */

#endif

