/*
 * pawstree.cpp - Author: Ondrej Hurt
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

// CS INCLUDES
#include <psconfig.h>
#include <csgfx/rgbpixel.h>
#include <iutil/event.h>

// COMMON INCLUDES
#include "util/log.h"

// PAWS INCLUDES
#include "pawsborder.h"
#include "pawstree.h"
#include "pawsmanager.h"

#define SCROLLBAR_WIDTH 15




////////////////////////////////////////////////////////////
//        this should be in some library
////////////////////////////////////////////////////////////

// converts one hexadecimal digit to decimal value
int HexToDec4(char digit)
{
    if((digit >= '0') && (digit <= '9'))
        return digit - '0';
    else if((digit >= 'A') && (digit <= 'Z'))
        return digit - 'A' + 10;
    else if((digit >= 'a') && (digit <= 'z'))
        return digit - 'a' + 10;
    else
        return 0;
}

// converts two hexadecimal digits to decimal value
int HexToDec8(char first, char second)
{
    return (HexToDec4(first) << 4) + HexToDec4(second);
}

// converts hexadecimal string to color in current pixel format
int ParseColor(const csString &str, iGraphics2D* g2d)
{
    int r, g, b;

    if(str.Length() != 6)
        return false;

    r = HexToDec8(str.GetAt(0), str.GetAt(1));
    g = HexToDec8(str.GetAt(2), str.GetAt(3));
    b = HexToDec8(str.GetAt(4), str.GetAt(5));

    return g2d->FindRGB(r, g, b);
}


////////////////////////////////////////////////////////////
//
//                    pawsTreeNode
//
////////////////////////////////////////////////////////////

pawsTreeNode::pawsTreeNode()
{
    tree = NULL;
    parent = NULL;
    firstChild = NULL;
    prevSibling = NULL;
    nextSibling = NULL;
    collapsable = true;
    collapsed = false;
    factory = "pawsTreeNode";
}
pawsTreeNode::pawsTreeNode(const pawsTreeNode &origin)
    :pawsWidget(origin),
     tree(0),
     collapsable(origin.collapsable),
     collapsed(origin.collapsed)
{
    for(unsigned int i = 0 ; i < origin.attrList.GetSize(); i++)
        attrList.Push(origin.attrList[i]);

    parent = 0;
    prevSibling = 0;
    nextSibling = 0;
    firstChild = 0;

    /*
    if(origin.firstChild)
    {
        pawsTreeNode * tmp = origin.firstChild;
        firstChild = dynamic_cast<pawsTreeNode*>(PawsManager::GetSingleton().CreateWidget(tmp->factory,tmp));
        firstChild->parent = this;
        firstChild->prevSibling = 0;

        pawsTreeNode * pr = firstChild;

        while (tmp->nextSibling)//loop through all siblings [Depth First Search]
        {
            pawsTreeNode * t = dynamic_cast<pawsTreeNode*>(PawsManager::GetSingleton().CreateWidget(tmp->nextSibling->factory,tmp->nextSibling));
            pr->nextSibling = t;
            t->prevSibling = pr;
            t->parent = this;
            pr = t;
            tmp = tmp->nextSibling;
        }
        pr->nextSibling = 0;
    }*/
}
pawsTreeNode::~pawsTreeNode()
{
    Clear();
}

void pawsTreeNode::SetTree(pawsITreeStruct* _tree)
{
    tree = _tree;
}

csString pawsTreeNode::GetAttr(const csString &name)
{
    for(size_t x = 0; x < attrList.GetSize(); x++)
    {
        if(attrList[x].name == name)
            return attrList[x].value;
    }
    return "";
}

void pawsTreeNode::SetAttr(const csString &name, const csString &value)
{
    TreeNodeAttribute attr;

    attr.name  = name;
    attr.value = value;
    attrList.Push(attr);

    if(tree != NULL) tree->NodeChanged();
}

pawsTreeNode* pawsTreeNode::GetParent()
{
    return parent;
}

void pawsTreeNode::SetParent(pawsTreeNode* _parent)
{
    parent = _parent;
}

pawsTreeNode* pawsTreeNode::GetFirstChild()
{
    return firstChild;
}

pawsTreeNode* pawsTreeNode::FindNodeByPath(const csString & /*path*/)
{
    //not implemented yet
    return NULL;
}

pawsTreeNode* pawsTreeNode::FindChildByName(const csString &name, bool indirectToo)
{
    pawsTreeNode* child, * foundIndirect;

    child = firstChild;
    while(child != NULL)
    {
        if(name == child->name)
            return child;
        if(indirectToo)
        {
            foundIndirect = child->FindChildByName(name, true);
            if(foundIndirect != NULL)
                return foundIndirect;
        }

        child = child->GetNextSibling();
    }
    return NULL;
}

pawsTreeNode* pawsTreeNode::GetPrevSibling()
{
    return prevSibling;
}

pawsTreeNode* pawsTreeNode::GetNextSibling()
{
    return nextSibling;
}

void pawsTreeNode::SetPrevSibling(pawsTreeNode* node)
{
    prevSibling = node;
}

void pawsTreeNode::SetNextSibling(pawsTreeNode* node)
{
    nextSibling = node;
}

void pawsTreeNode::SetFirstChild(pawsTreeNode* child)
{
    firstChild = child;
}

pawsTreeNode* pawsTreeNode::FindLastChild()
{
    return firstChild->FindLastSibling();
}

void pawsTreeNode::InsertChild(pawsTreeNode* node, pawsTreeNode* nextSibling)
{
    pawsTreeNode* prevSibling;

    if(nextSibling != NULL)
        prevSibling = nextSibling->GetPrevSibling();
    else
        prevSibling = FindLastChild();

    node->SetNextSibling(nextSibling);
    node->SetPrevSibling(prevSibling);

    if(nextSibling != NULL)
        nextSibling->SetPrevSibling(node);
    if(prevSibling != NULL)
        prevSibling->SetNextSibling(node);
    else
        firstChild = node;
    node->SetParent(this);
    node->SetTree(tree);

    tree->NewNode(node);

    if(tree != NULL)
        tree->NodeChanged();
}

void pawsTreeNode::MoveChild(pawsTreeNode* node, pawsTreeNode* nextSibling)
{
    RemoveChild(node);
    InsertChild(node, nextSibling);

    if(tree != NULL) tree->NodeChanged();
}

void pawsTreeNode::RemoveChild(pawsTreeNode* node)
{
    pawsTreeNode* prev = node->GetPrevSibling(),
                  * next = node->GetNextSibling();
    if(prev != NULL)
        prev->SetNextSibling(next);
    else
        firstChild = next;
    if(next != NULL)
        next->SetPrevSibling(prev);

    if(tree != NULL)
        tree->RemoveNode(node);
}

void pawsTreeNode::DeleteChild(pawsTreeNode* node)
{
    RemoveChild(node);
    delete node;
}

void pawsTreeNode::Clear()
{
    while(firstChild != NULL)
        DeleteChild(firstChild);
}

void pawsTreeNode::SetCollapsable(bool _collapsable)
{
    collapsable = _collapsable;
    if(!collapsable && collapsed)
        Expand();
}

void pawsTreeNode::Expand()
{
    collapsed = false;
    SetChildrenVisibAfterCollapseChange(true);

    if(tree != NULL) tree->NodeChanged();
}

void pawsTreeNode::ExpandAll()
{
    pawsTreeNode* child;

    Expand();
    child = firstChild;
    while(child != NULL)
    {
        child->ExpandAll();
        child = child->GetNextSibling();
    }
}

void pawsTreeNode::Collapse()
{
    collapsed = true;
    SetChildrenVisibAfterCollapseChange(false);

    if(tree != NULL) tree->NodeChanged();
}

void pawsTreeNode::CollapseAll()
{
    pawsTreeNode* child;

    Collapse();
    child = firstChild;
    while(child != NULL)
    {
        child->CollapseAll();
        child = child->GetNextSibling();
    }
}

bool pawsTreeNode::IsCollapsed()
{
    return collapsed;
}

bool pawsTreeNode::BuriedInRuins()
{
    if(parent == NULL)
        return false;
    return parent->IsCollapsed() || parent->BuriedInRuins();
}

void pawsTreeNode::SetChildrenVisibAfterCollapseChange(bool expanded)
{
    pawsTreeNode* child;

    child = firstChild;
    while(child != NULL)
    {
        if(expanded)
        {
            child->Show();
        }
        else
        {
            child->Hide();
        }
        if(! child->IsCollapsed())
            child->SetChildrenVisibAfterCollapseChange(expanded);
        child = child->GetNextSibling();
    }
}

int pawsTreeNode::GetRowNum()
{
    if(prevSibling != NULL)
        return prevSibling->FindLowestSubtreeNode()->GetRowNum() + 1;
    else if(parent != NULL)
        return parent->GetRowNum() + 1;
    else
        return 0;
}

pawsTreeNode* pawsTreeNode::FindLastSibling()
{
    pawsTreeNode* sibling;

    sibling = this;
    while((sibling != NULL) && (sibling->GetNextSibling() != NULL))
        sibling = sibling->GetNextSibling();
    return sibling;
}

pawsTreeNode* pawsTreeNode::FindLowestSubtreeNode()
{
    pawsTreeNode* lastChild;

    lastChild = FindLastChild();
    if(lastChild == NULL)
        return this;
    else
        return lastChild->FindLowestSubtreeNode();
}

pawsTreeNode* pawsTreeNode::FindNodeAbove()
{
    if(prevSibling != NULL)
        return prevSibling->FindLowestSubtreeNode();
    else
        return parent;
}

pawsTreeNode* pawsTreeNode::FindNodeBelow()
{
    pawsTreeNode* node;

    if(firstChild != NULL)
        return firstChild;
    else if(nextSibling != NULL)
        return nextSibling;
    else
    {
        node = parent;
        while((node != NULL) && (node->GetNextSibling() == NULL))
            node = node->GetParent();
        if(node != NULL)
            return node->GetNextSibling();
        else
            return NULL;
    }
}

bool pawsTreeNode::Load(iDocumentNode* node)
{
    csRef<iDocumentNodeIterator> xmlChildren, xmlAttrList;
    csRef<iDocumentNode> xmlChild, xmlAttr;
    csString factory;
    pawsWidget* childAsWidget;
    pawsTreeNode* childNode;

    Clear();

    name = node->GetAttributeValue("name");
    xmlChildren = node->GetNodes("widget");
    while(xmlChildren->HasNext())
    {
        xmlChild = xmlChildren->Next();

        factory = xmlChild->GetAttributeValue("factory");
        childAsWidget = PawsManager::GetSingleton().CreateWidget(factory);
        if(!childAsWidget)
        {
            Error2("Could not create node from factory: %s", factory.GetData());
            return false;
        }
        childNode = dynamic_cast<pawsTreeNode*>(childAsWidget);
        if(childNode == NULL)
        {
            Error1("Created node is not pawsTreeNode");
            return false;
        }
        InsertChild(childNode);

        if(!childNode->Load(xmlChild))
        {
            Error1("Node failed to load");
            return false;
        }
    }


    attrList.DeleteAll();

    xmlAttrList = node->GetNodes("attr");
    while(xmlAttrList->HasNext())
    {
        xmlAttr = xmlAttrList->Next();
        SetAttr(xmlAttr->GetAttributeValue("name"), xmlAttr->GetAttributeValue("value"));
    }

    if(tree != NULL)
        tree->NodeChanged();

    csString collapsed = node->GetAttributeValue("collapsed");
    if(collapsed == "yes")
        CollapseAll();

    return true;
}

//////////////////////////////////////////////////////////////////////
//
//                        pawsTreeStruct
//
//////////////////////////////////////////////////////////////////////

pawsTreeStruct::pawsTreeStruct()
{
    root = NULL;
    version = 0;
}

pawsTreeStruct::~pawsTreeStruct()
{
    if(root != NULL)
        delete root;
}

void pawsTreeStruct::NodeChanged()
{
    version++;
}

void pawsTreeStruct::SetRoot(pawsTreeNode* _root)
{
    if(root != NULL)
        delete root;

    root = _root;
}

pawsTreeNode* pawsTreeStruct::FindNodeByName(const csString &name)
{
    if(root != NULL)
        return root->FindChildByName(name, true);
    else
        return NULL;
}

void pawsTreeStruct::InsertChild(pawsTreeNode* parent, pawsTreeNode* node, pawsTreeNode* nextSibling)
{
    parent->InsertChild(node, nextSibling);
}

void pawsTreeStruct::MoveChild(pawsTreeNode* node, pawsTreeNode* nextSibling)
{
    pawsTreeNode* parent;

    parent = node->GetParent();
    if(parent != NULL)
        parent->MoveChild(node, nextSibling);
}

void pawsTreeStruct::RemoveChild(pawsTreeNode* node)
{
    pawsTreeNode* parent;

    parent = node->GetParent();
    if(parent != NULL)
        parent->RemoveChild(node);
}

void pawsTreeStruct::DeleteChild(pawsTreeNode* node)
{
    pawsTreeNode* parent;

    parent = node->GetParent();
    if(parent != NULL)
        parent->DeleteChild(node);
}

void pawsTreeStruct::Clear()
{
    if(root != NULL)
        root->Clear();
}





void pawsTreeStruct::InsertChild(const csString &parent, pawsTreeNode* node, const csString &nextSibling)
{
    pawsTreeNode* parentNode, * nextSiblingNode;

    parentNode = FindNodeByName(parent);
    // Allows a root to be added automatically.
    if(!parentNode)
    {
        if(!root)
        {
            SetRoot(node);
            node->SetTree(this);
            return;
        }
        else
        {
            parentNode = root;
        }
    }
    nextSiblingNode = FindNodeByName(nextSibling);
    parentNode->InsertChild(node, nextSiblingNode);
}

void pawsTreeStruct::InsertChild(const csString &parent, pawsTreeNode* node)
{
    pawsTreeNode* parentNode;

    parentNode = FindNodeByName(parent);
    if(parentNode == NULL) return;

    parentNode->InsertChild(node, NULL);
}

void pawsTreeStruct::MoveChild(const csString &name, const csString &nextSibling)
{
    pawsTreeNode* node, * parentNode, * nextSiblingNode;

    node = FindNodeByName(name);
    if(node == NULL) return;
    nextSiblingNode = FindNodeByName(nextSibling);
    if(nextSiblingNode == NULL) return;

    parentNode = node->GetParent();
    if(parentNode != NULL)
        parentNode->MoveChild(node, nextSiblingNode);
}

void pawsTreeStruct::DeleteChild(const csString &name)
{
    pawsTreeNode* node, * parentNode;

    node = FindNodeByName(name);
    if(node == NULL) return;

    parentNode = node->GetParent();
    if(parentNode != NULL)
        parentNode->DeleteChild(node);
    else
        Clear();
}

bool pawsTreeStruct::Load(iDocumentNode* node)
{
    csRef<iDocumentNode> xmlRoot;
    csString factory;
    pawsWidget* rootAsWidget;
    pawsTreeNode* newRoot;

    version++;

    xmlRoot = node->GetNode("widget");
    if(xmlRoot == NULL)
    {
        Error1("<widget> tag not found");
        return true;
    }

    factory = xmlRoot->GetAttributeValue("factory");
    rootAsWidget = PawsManager::GetSingleton().CreateWidget(factory);
    if(rootAsWidget == NULL)
    {
        Error2("Could not create root from factory: %s", factory.GetData());
        return false;
    }

    newRoot = dynamic_cast<pawsTreeNode*>(rootAsWidget);
    if(newRoot == NULL)
    {
        Error2("Root(%s) is not pawsTreeNode",rootAsWidget->GetType());
        delete rootAsWidget;
        return false;
    }
    SetRoot(newRoot);

    root->SetTree(this);
    return root->Load(xmlRoot);
}

pawsTreeNode* pawsTreeStruct::FindNodeAt(pawsTreeNode* parent, int x, int y)
{
    pawsTreeNode* child, *foundIndirect;
    csRect frame;

    if(parent->IsCollapsed())
        return NULL;

    child = parent->GetFirstChild();
    while(child != NULL)
    {
        frame = child->GetScreenFrame();
        if(frame.Contains(x, y))
            return child;
        foundIndirect = FindNodeAt(child, x, y);
        if(foundIndirect != NULL)
            return foundIndirect;

        child = child->GetNextSibling();
    }
    return NULL;
}

pawsTreeNode* pawsTreeStruct::GetRoot()
{
    return root;
}

//////////////////////////////////////////////////////////////////////
//
//                    pawsStdTreeLayout
//
//////////////////////////////////////////////////////////////////////


pawsStdTreeLayout::pawsStdTreeLayout(pawsTree* _tree, int _rowSpacing, int _levelSpacing)
{
    tree           = _tree;
    rowSpacing     = _rowSpacing;
    levelSpacing   = _levelSpacing;
    horizScroll    = 0;
    vertScroll     = 0;
    lastVersion    = -1;
    width          = 0;
    height         = 0;
}

pawsStdTreeLayout::~pawsStdTreeLayout()
{
}

void pawsStdTreeLayout::SetLayout()
{
    pawsTreeNode* root;
    csRect treeFrame;
    int treeX, treeY, maxX, maxY;

    // if the tree hasn't changed, we have nothing to update
    if(lastVersion == tree->GetVersion())
        return;

    root = tree->GetRoot();
    if(root == NULL)
        return;

    treeFrame = tree->GetScreenFrame();
    treeX = treeFrame.xmin - horizScroll;
    treeY = treeFrame.ymin - vertScroll;
    maxX  = treeX;
    maxY  = treeY;
    root->SetSize(0, 0);
    SetSubtreeLayout(root, treeX, treeY, maxX, maxY);
    width  = maxX - treeX + 1;
    height = maxY - treeY + 1;

    lastVersion = tree->GetVersion();
}

void pawsStdTreeLayout::SetSubtreeLayout(pawsTreeNode* subtreeRoot, int x, int y, int &maxX, int &maxY)
{
    pawsTreeNode* child;
    csRect frame;

    subtreeRoot->MoveTo(x, y);
    frame = subtreeRoot->GetScreenFrame();
    if(frame.xmax > maxX)
        maxX = frame.xmax;
    maxY = frame.ymax;

    if(! subtreeRoot->IsCollapsed())
    {
        child = subtreeRoot->GetFirstChild();
        while(child != NULL)
        {
            SetSubtreeLayout(child, x + levelSpacing, maxY + rowSpacing, maxX, maxY);
            child = child->GetNextSibling();
        }
    }
}

void pawsStdTreeLayout::SetHorizScroll(int _horizScroll)
{
    horizScroll = _horizScroll;
    SetLayout();
}
void pawsStdTreeLayout::SetVertScroll(int _vertScroll)
{
    vertScroll = _vertScroll;
    SetLayout();
}

void pawsStdTreeLayout::GetTreeSize(int &_width, int &_height)
{
    _width = width;
    _height = height;
}

//////////////////////////////////////////////////////////////////////
//
//                        pawsStdTreeDecorator
//
//////////////////////////////////////////////////////////////////////

pawsStdTreeDecorator::pawsStdTreeDecorator(pawsTree* _tree, iGraphics2D* _g2d,
        int _selectedColor, int _lineColor, int _collSpacing)
{
    tree              = _tree;
    g2d               = _g2d;
    selectedColor     = _selectedColor;
    lineColor         = _lineColor;
    collSpacing       = _collSpacing;

    collImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage("treecollapse");
    expandImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage("treeexpand");
}

pawsStdTreeDecorator::~pawsStdTreeDecorator()
{
}

void pawsStdTreeDecorator::Decorate()
{
    pawsTreeNode* root;

    root = tree->GetRoot();
    if(root != NULL)
        DecorateSubtree(root);
}

void pawsStdTreeDecorator::DecorateSubtree(pawsTreeNode* node)
{
    csRef<iPawsImage> currImage;
    pawsTreeNode* child;
    csRect nodeFrame, childFrame, collSignFrame;
    int vertLineX, vertLineYMin, vertLineYMax;
    //coordinates of lines painted between siblings
    int horizLineXMin, horizLineY;
    //coordinates of lines painted between nodes and their collapse signs
    bool hasCollSign;    // has the currently processed node collapse/expand sign ?


    nodeFrame = node->GetScreenFrame();

    if(node != tree->GetRoot())
    {
        // paints collapse/expand sign
        hasCollSign  =  node->IsCollapsable() && (node->GetFirstChild() != NULL);
        if(hasCollSign)
        {
            GetCollapseSignFrame(node, collSignFrame);
            if(node->IsCollapsed())
                currImage = expandImage;
            else
                currImage = collImage;

            if(currImage)
                currImage->Draw(collSignFrame);
        }

        // paints node selection box
        if(node == tree->GetSelected())
            g2d->DrawBox(nodeFrame.xmin-2, nodeFrame.ymin, nodeFrame.Width()+4, nodeFrame.Height(), selectedColor);
    }

    if(node->IsCollapsed())
        return;

    // paints children of node
    child = node->GetFirstChild();
    if(child != NULL)
    {
        childFrame = child->GetScreenFrame();
        vertLineX = (nodeFrame.xmin + childFrame.xmin) / 2;
        vertLineYMin = childFrame.ymin;

        do
        {
            hasCollSign = child->IsCollapsable() && (child->GetFirstChild() != NULL);
            childFrame = child->GetScreenFrame();
            GetCollapseSignFrame(child, collSignFrame);

            horizLineY = (childFrame.ymin + childFrame.ymax) / 2;
            horizLineXMin = (hasCollSign) ? collSignFrame.xmax : vertLineX;

            vertLineYMax = (hasCollSign) ? collSignFrame.ymin-1 : horizLineY;

            if(child != tree->GetRoot())
                g2d->DrawLine(vertLineX, vertLineYMin, vertLineX, vertLineYMax, lineColor);

            g2d->DrawLine(horizLineXMin, horizLineY, childFrame.xmin - 3, horizLineY, lineColor);

            // These draw an outline on the bounding box of the node widget to check bounds
            //g2d->DrawLine(childFrame.xmin, childFrame.ymin, childFrame.xmax, childFrame.ymin, lineColor);
            //g2d->DrawLine(childFrame.xmin, childFrame.ymin, childFrame.xmin, childFrame.ymax, lineColor);
            //g2d->DrawLine(childFrame.xmin, childFrame.ymax, childFrame.xmax, childFrame.ymax, lineColor);
            //g2d->DrawLine(childFrame.xmax, childFrame.ymin, childFrame.xmax, childFrame.ymax, lineColor);

            DecorateSubtree(child);

            // determine vertLineYMin for next sibling
            vertLineYMin = (hasCollSign) ? collSignFrame.ymax+1 : horizLineY;

            child = child->GetNextSibling();
        }
        while(child != NULL);
    }
}

bool pawsStdTreeDecorator::OnMouseDown(int /*button*/, int /*modifiers*/, int x, int y)
{
    pawsTreeNode* target, * root;

    root = tree->GetRoot();
    if(root != NULL)
    {
        target = FindCollapsingNodeInSubtree(root, x, y);
        if(target != NULL)
        {
            // has the currently processed node collapse/expand sign ?
            bool hasCollSign  =  target->IsCollapsable() &&
                                 (target->GetFirstChild() != NULL);
            if(hasCollSign)
            {
                if(target->IsCollapsed())
                    target->Expand();
                else
                    target->Collapse();
            }
        }
    }
    return true;
}

void pawsStdTreeDecorator::GetCollapseSignFrame(pawsTreeNode* node, csRect &rect)
{
    csRect nodeFrame = node->GetScreenFrame();
    rect.SetPos(nodeFrame.xmin - collSpacing - 2,
                nodeFrame.ymin + nodeFrame.Height()/2 - 4);
    rect.SetSize(10, 10);
}

pawsTreeNode* pawsStdTreeDecorator::FindCollapsingNodeInSubtree(pawsTreeNode* subtreeRoot, int x, int y)
{
    csRect signFrame;
    pawsTreeNode* child, * found;


    // has the currently processed node collapse/expand sign ?
    bool hasCollSign  =  subtreeRoot->IsCollapsable() &&
                         (subtreeRoot->GetFirstChild() != NULL);
    if(hasCollSign)
    {
        GetCollapseSignFrame(subtreeRoot, signFrame);
        if(signFrame.Contains(x, y))
            return subtreeRoot;
    }

    child = subtreeRoot->GetFirstChild();
    while(child != NULL)
    {
        found = FindCollapsingNodeInSubtree(child, x, y);
        if(found != NULL)
            return found;
        child = child->GetNextSibling();
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////
//
//                            pawsTree
//
//////////////////////////////////////////////////////////////////////


pawsTree::pawsTree()
{
    selected = NULL;
    notificationTarget = NULL;
    factory = "pawsTree";

    layout = 0;
    SetTreeLayout(new pawsStdTreeLayout(this, 5, 20));

    decor = 0;
    SetTreeDecorator(new pawsStdTreeDecorator(this, graphics2D, 0x666666, 0x999999, 13));

    horizScrollBar = NULL;
    vertScrollBar = NULL;
}
void pawsTree::cloneTreeNodes(const pawsTree &origin)
{
    csArray<pawsTreeNode*> oTreeNodes; //original tree nodes
    csArray<pawsTreeNode*> TreeNodes; //this tree's tree nodes
    for(unsigned int i = 0 ; i< origin.children.GetSize(); i++)
    {
        pawsTreeNode* tmp = dynamic_cast<pawsTreeNode*>(origin.children[i]);
        if(tmp)
        {
            oTreeNodes.Push(tmp);
            TreeNodes.Push(dynamic_cast<pawsTreeNode*>(children[i]));
        }
    }
    for(unsigned int i = 0 ; i< oTreeNodes.GetSize() ; i++)
    {
        pawsTreeNode* ocur = oTreeNodes[i]; //original node
        pawsTreeNode* cur = TreeNodes[i]; //current node

        cur->SetTree(this);

        for(unsigned int j = 0 ; j < oTreeNodes.GetSize() ; j++)
        {
            if(ocur->GetParent() == oTreeNodes[j])
                cur->SetParent(TreeNodes[j]);
            else if(ocur->GetPrevSibling() == oTreeNodes[j])
                cur->SetPrevSibling(TreeNodes[j]);
            else if(ocur->GetNextSibling() == oTreeNodes[j])
                cur->SetNextSibling(TreeNodes[j]);
            else if(ocur->GetFirstChild() == oTreeNodes[j])
                cur->SetFirstChild(TreeNodes[j]);
        }

        if(origin.root == oTreeNodes[i])
            root = TreeNodes[i];
    }
}
pawsTree::pawsTree(const pawsTree &origin)
    :pawsWidget(origin)
{
    layout = 0;
    SetTreeLayout(new pawsStdTreeLayout(this, 5, 20));

    decor = 0;
    SetTreeDecorator(new pawsStdTreeDecorator(this, graphics2D, 0x666666, 0x999999, 13));

    this->root = 0;

    horizScrollBar = NULL;
    vertScrollBar = NULL;
    notificationTarget = origin.notificationTarget;

    if(origin.root)
        cloneTreeNodes(origin);

    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.horizScrollBar == origin.children[i])
            horizScrollBar = dynamic_cast<pawsScrollBar*>(children[i]);
        else if(origin.vertScrollBar == origin.children[i])
            vertScrollBar = dynamic_cast<pawsScrollBar*>(children[i]);

        if(horizScrollBar != 0 && vertScrollBar!= 0)
            break;
    }
}

pawsTree::~pawsTree()
{
    if(layout != NULL)
        delete layout;
    if(decor != NULL)
        delete decor;
    if(root != NULL)
    {
        pawsWidget::DeleteChild(root);
        root = NULL;
    }
}

void pawsTree::SetRoot(pawsTreeNode* _root)
{
    if(root != NULL)
    {
        if(selected == root)
            Deselect();
        pawsWidget::DeleteChild(root);
    }
    root = _root;
    AddChild(root);
}

void pawsTree::SetScrollBars(bool horiz, bool vert)
{
    if(horiz && !horizScrollBar)
    {
        horizScrollBar = new pawsScrollBar;
        AddChild(horizScrollBar);
        horizScrollBar->SetRelativeFrame(0, defaultFrame.Width()-GetActualHeight(SCROLLBAR_WIDTH),
                                         defaultFrame.Width(), GetActualHeight(SCROLLBAR_WIDTH));
        horizScrollBar->PostSetup();
        horizScrollBar->SetTickValue(20);
        horizScrollBar->Show();
    }
    if(!horiz && horizScrollBar)
    {
        pawsWidget::DeleteChild(horizScrollBar);
        horizScrollBar = NULL;
        if(layout)
            layout->SetHorizScroll(0);
    }

    if(vert && !vertScrollBar)
    {
        csString widgetStr;

        widgetStr.Format("<widget factory=\"pawsScrollBar\" name=\"scrollbar\" style=\"Standard Scrollbar\" direction=\"vertical\" tick=\"20\" minValue=\"0\" ><frame x=\"%d\" y=\"%d\" width=\"20\" height=\"%d\" /></widget>",
                         defaultFrame.Width()-20,0,defaultFrame.Height());

        vertScrollBar = dynamic_cast<pawsScrollBar*>(PawsManager::GetSingleton().LoadWidgetFromString(widgetStr));
        AddChild(vertScrollBar);

//        vertScrollBar = new pawsScrollBar;
//        if (vertScrollBar == NULL)
//        {
//            Error1("Could not created pawsScrollBar");
//            return;
//        }
//        vertScrollBar->SetRelativeFrame(defaultFrame.Width()-GetActualWidth(SCROLLBAR_WIDTH), 0,
//                                        GetActualWidth(SCROLLBAR_WIDTH), defaultFrame.Height());
//        vertScrollBar->PostSetup();
//        vertScrollBar->SetTickValue(20);
//        vertScrollBar->SetAttachFlags(ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT );
        vertScrollBar->Show();
    }
    if(!vert && (vertScrollBar != NULL))
    {
        pawsWidget::DeleteChild(vertScrollBar);
        vertScrollBar = NULL;
        if(layout)
            layout->SetVertScroll(0);
    }
    SetScrollBarMax();
}


void pawsTree::Select(pawsTreeNode* node)
{
    selected = node;

    //we have to check if the selected element is outside sight. If so move the scrollbar
    //correctly in order to have it on screen.

    if(selected->GetScreenFrame().ymin < GetScreenFrame().ymin) //the element is hidden in the top
    {
        //get the current position and move it relatively of the part which is hidden
        if(vertScrollBar) //just to be safe
            vertScrollBar->SetCurrentValue(vertScrollBar->GetCurrentValue()-(GetScreenFrame().ymin-selected->GetScreenFrame().ymin));
    }
    else if(selected->GetScreenFrame().ymax > GetScreenFrame().ymax) //the element is hidden in the bottom
    {
        //get the current position and move it relatively of the part which is hidden
        if(vertScrollBar) //just to be safe
            vertScrollBar->SetCurrentValue(vertScrollBar->GetCurrentValue()+(selected->GetScreenFrame().ymax-GetScreenFrame().ymax));
    }

    if(notificationTarget != NULL)
    {
        notificationTarget->OnSelected(node);
    }
}

void pawsTree::Deselect()
{
    selected = NULL;
}

pawsTreeNode* pawsTree::GetSelected()
{
    return selected;
}

bool pawsTree::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> decorator = node->GetNode("decorator");
    if(decorator)
    {
        csRGBcolor selColour = csRGBcolor(102,102,102);
        csRGBcolor lineColour = csRGBcolor(153,153,153);
        int colSpacing = 13;

        csRef<iDocumentAttribute> selR = decorator->GetAttribute("selR");
        if(selR) selColour.red = selR->GetValueAsInt();
        csRef<iDocumentAttribute> selG = decorator->GetAttribute("selG");
        if(selG) selColour.green = selG->GetValueAsInt();
        csRef<iDocumentAttribute> selB = decorator->GetAttribute("selB");
        if(selB) selColour.blue = selB->GetValueAsInt();

        csRef<iDocumentAttribute> lineR = decorator->GetAttribute("lineR");
        if(lineR) lineColour.red = lineR->GetValueAsInt();
        csRef<iDocumentAttribute> lineG = decorator->GetAttribute("lineG");
        if(lineG) lineColour.green = lineG->GetValueAsInt();
        csRef<iDocumentAttribute> lineB = decorator->GetAttribute("lineB");
        if(lineB) lineColour.blue = lineB->GetValueAsInt();

        csRef<iDocumentAttribute> colSpace = decorator->GetAttribute("colSpacing");
        if(colSpace) colSpacing = colSpace->GetValueAsInt();

        SetTreeDecorator(new pawsStdTreeDecorator(this, graphics2D,
                         graphics2D->FindRGB(selColour.red, selColour.blue, selColour.green),
                         graphics2D->FindRGB(lineColour.red, lineColour.blue, lineColour.green),
                         colSpacing));
    }

    csRef<iDocumentNode> layout = node->GetNode("layout");
    if(layout)
    {
        int rowSpacingVal = 5;
        int levelSpacingVal = 20;
        csRef<iDocumentAttribute> rowSpacing = layout->GetAttribute("rowSpacing");
        if(rowSpacing)
            rowSpacingVal = rowSpacing->GetValueAsInt();
        csRef<iDocumentAttribute> levelSpacing = layout->GetAttribute("levelSpacing");
        if(levelSpacing)
            levelSpacingVal = levelSpacing->GetValueAsInt();
        SetTreeLayout(new pawsStdTreeLayout(this, rowSpacingVal, levelSpacingVal));
    }

    return true;
}

bool pawsTree::PostSetup()
{
    return true;
}

void pawsTree::Draw()
{
    csRect oldClip;

    if(layout)
    {
        layout->SetLayout();
        SetScrollBarMax();
    }

    ClipToParent(true);
    DrawBackground();

    graphics2D->SetClipRect(screenFrame.xmin, screenFrame.ymin, screenFrame.xmax, screenFrame.ymax);
    if(decor)
        decor->Decorate();

    ClipToParent(false);
    DrawChildren();
}

void pawsTree::SetTreeLayout(pawsITreeLayout* _layout)
{
    if(layout)
    {
        delete layout;
        layout = 0;
    }
    layout = _layout;
    layout->SetLayout();
}

void pawsTree::SetTreeDecorator(pawsITreeDecorator* _decor)
{
    if(decor)
    {
        delete decor;
        decor = 0;
    }
    decor = _decor;
}

bool pawsTree::OnMouseDown(int button, int modifiers, int x, int y)
{
    if(button == csmbWheelUp)
    {
        if(vertScrollBar)
            vertScrollBar->SetCurrentValue(vertScrollBar->GetCurrentValue() - TREE_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if(button == csmbWheelDown)
    {
        if(vertScrollBar)
            vertScrollBar->SetCurrentValue(vertScrollBar->GetCurrentValue() + TREE_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if(button == csmbHWheelLeft)
    {
        if(horizScrollBar)
            horizScrollBar->SetCurrentValue(horizScrollBar->GetCurrentValue() - TREE_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if(button == csmbHWheelRight)
    {
        if(horizScrollBar)
            horizScrollBar->SetCurrentValue(horizScrollBar->GetCurrentValue() + TREE_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else
    {
        pawsTreeNode* node = NULL;

        if(root != NULL)
        {
            node = FindNodeAt(root, x, y);
            if(node != NULL)
            {
                Select(node);
                return true;
            }
        }

        if(decor != NULL)
            if(decor->OnMouseDown(button, modifiers, x, y))
                return true;
    }
    return false;
}

bool pawsTree::OnKeyDown(utf32_char /*keyCode*/, utf32_char keyChar, int /*modifiers*/)
{
    pawsTreeNode* node;

    switch(keyChar)
    {
        case CSKEY_UP:
            if(selected != NULL)
            {
                node = selected;
                do
                {
                    node = node->FindNodeAbove();
                }
                while((node != NULL) && node->BuriedInRuins());
                if((node != NULL) && (node != root))
                    Select(node);
            }
            return true;

        case CSKEY_DOWN:
            if(selected != NULL)
            {
                node = selected;
                do
                {
                    node = node->FindNodeBelow();
                }
                while((node != NULL) && node->BuriedInRuins());
                if(node != NULL)
                    Select(node);
            }
            return true;

        case CSKEY_ENTER:
        case CSKEY_SPACE:
            if(selected != NULL)
            {
                //Is the selected node expandable or collapsable?
                if(selected->IsCollapsable() && selected->GetFirstChild() != NULL)
                {
                    //If so expand or collapse it depending on its status
                    if(selected->IsCollapsed())
                        selected->Expand();
                    else
                        selected->Collapse();
                }
            }
            //some notify?
            return true;
    }
    return false;
}

bool pawsTree::LoadChildren(iDocumentNode* node)
{
    return pawsTreeStruct::Load(node);
}

void pawsTree::SetScrollBarMax()
{
    int width, height;

    if(layout != NULL)
    {
        layout->GetTreeSize(width, height);
        if(horizScrollBar != NULL)
        {
            if(width > screenFrame.Width())
            {
                horizScrollBar  -> SetMaxValue(csMax(0, width  - screenFrame.Width()));
                if(!horizScrollBar->IsVisible()) //as this function is called continually we have to check if
                    horizScrollBar->Show(); //the widget is already shown to avoid flickering
            }
            else
            {
                if(horizScrollBar->IsVisible())
                    horizScrollBar->Hide();
                horizScrollBar->SetMaxValue(0);
            }
        }
        if(vertScrollBar != NULL)
        {
            if(height > screenFrame.Height())
            {
                vertScrollBar   -> SetMaxValue(csMax(0, height - screenFrame.Height()));
                if(!vertScrollBar->IsVisible())
                    vertScrollBar->Show();
            }
            else
            {
                if(vertScrollBar->IsVisible())
                    vertScrollBar->Hide();
                vertScrollBar->SetMaxValue(0);
            }
        }
    }
}

void pawsTree::SetNotify(pawsWidget* _notificationTarget)
{
    notificationTarget = _notificationTarget;
}

bool pawsTree::OnScroll(int /*scrollDirection*/, pawsScrollBar* widget)
{
    int scroll;

    scroll = (int)widget->GetCurrentValue();
    version++;
    if(widget == horizScrollBar)
        layout->SetHorizScroll(scroll);
    else if(widget == vertScrollBar)
        layout->SetVertScroll(scroll);

    return true;
}

void pawsTree::NewNode(pawsTreeNode* node)
{
    pawsWidget::AddChild(node);
}
void pawsTree::RemoveNode(pawsTreeNode* node)
{
    pawsWidget::RemoveChild(node);
}


//////////////////////////////////////////////////////////////////////
//
//                        pawsSeqTreeNode
//
//////////////////////////////////////////////////////////////////////

pawsSeqTreeNode_widget::pawsSeqTreeNode_widget(pawsWidget* _w, int _width)
{
    widget = _w;
    width = _width;
}

pawsSeqTreeNode::pawsSeqTreeNode(const pawsSeqTreeNode &origin)
    :pawsTreeNode(origin)
{
    csList<pawsSeqTreeNode_widget>::Iterator iter(origin.widgets);
    csArray<pawsSeqTreeNode_widget> arr;
    while((iter.HasNext()))
    {
        iter.Next();
        arr.Push(*iter);
    }
    for(unsigned int i = 0 ; i < arr.GetSize(); i++)
    {
        for(unsigned int j = 0 ; j < origin.children.GetSize(); j++)
        {
            if(arr[i].widget == origin.children[j])
            {
                arr[i].widget = children[j];
                break;
            }
        }
    }
    for(unsigned int i = 0 ; i< arr.GetSize(); i++)
        widgets.PushBack(arr[i]);
}
void pawsSeqTreeNode::AddSeqWidget(pawsWidget* w, int width)
{
    csRect widgetFrame;

    widgets.PushBack(pawsSeqTreeNode_widget(w, width));

    AddChild(w);
    w->MoveTo(screenFrame.xmax+1, screenFrame.ymin);
    w->Show();

    int newWidth = width + defaultFrame.Width();
    int newHeight = csMax(w->GetDefaultFrame().Height(), defaultFrame.Height());

    SetRelativeFrameSize(newWidth, newHeight);
}

void pawsSeqTreeNode::AddSeqWidget(pawsWidget* widget)
{
    int width = widget->GetScreenFrame().Width();
    AddSeqWidget(widget, width);
}

pawsWidget* pawsSeqTreeNode::GetSeqWidget(int index)
{
    csList<pawsSeqTreeNode_widget>::Iterator iter(widgets);
    int currIndex=0;

    while((iter.HasNext()))
    {
        iter.Next();
        if(currIndex == index)
            return iter->widget;

        currIndex++;
    }

    return NULL;
}

bool pawsSeqTreeNode::Load(iDocumentNode* node)
{
    csRef<iDocumentNodeIterator> nodeWidgetChildren;
    csRef<iDocumentNode> nodewidgetNode, widgetNode;
    csString factory;
    pawsWidget* newWidget;

    name = node->GetAttributeValue("name");

    nodewidgetNode = node->GetNode("nodewidget");
    if(nodewidgetNode != NULL)
    {
        nodeWidgetChildren = nodewidgetNode->GetNodes("widget");

        while(nodeWidgetChildren->HasNext())
        {
            //TODO: attributes for widgets like reserved width
            widgetNode = nodeWidgetChildren->Next();

            factory = widgetNode->GetAttributeValue("factory");
            newWidget = PawsManager::GetSingleton().CreateWidget(factory);
            if(newWidget == NULL)
                return false;
            if(! newWidget->Load(widgetNode))
            {
                delete newWidget;
                return false;
            }
            //TODO: AddSeqWidget(pawsWidget*, int) if necessary
            AddSeqWidget(newWidget);
        }
    }
    return pawsTreeNode::Load(node);
}

void pawsSeqTreeNode::Draw()
{
    pawsWidget::Draw();
}

//////////////////////////////////////////////////////////////////////
//
//                        pawsCheckTreeNode
//
//////////////////////////////////////////////////////////////////////

pawsCheckTreeNode::pawsCheckTreeNode()
{
    widget = NULL;
//    checkbox = NULL;
}
pawsCheckTreeNode::pawsCheckTreeNode(const pawsCheckTreeNode &origin)
    :pawsTreeNode(origin)
{
    widget = 0;
    if(origin.widget)
    {
        for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
        {
            if(origin.widget == origin.children[i])
                widget = children[i];

            if(widget != 0) break;
        }
    }
}

bool pawsCheckTreeNode::GetCheck()
{
    return false;
}

void pawsCheckTreeNode::SetCheck(bool /*ch*/)
{
    //checkbox->Set(ch);
}

void pawsCheckTreeNode::SetWidget(pawsWidget* _widget)
{
    csRect widgetFrame;

    if(widget != NULL)
    {
        pawsWidget::DeleteChild(widget);
        widget =  NULL;
    }

    widget = _widget;
    widgetFrame = widget->GetScreenFrame();
    AddChild(widget);
    widget->MoveTo(screenFrame.xmin, screenFrame.ymin);
    widget->Show();
    SetSize(widgetFrame.Width(), widgetFrame.Height());
}


//////////////////////////////////////////////////////////////////////
//
//                        pawsWidgetTreeNode
//
//////////////////////////////////////////////////////////////////////

pawsWidgetTreeNode::pawsWidgetTreeNode()
{
    widget = NULL;
    factory = "pawsWidgetTreeNode";
}
pawsWidgetTreeNode::pawsWidgetTreeNode(const pawsWidgetTreeNode &origin)
    :pawsTreeNode(origin)
{
    widget = 0;
    if(origin.widget)
    {
        for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
        {
            if(origin.widget == origin.children[i])
                widget = children[i];

            if(widget != 0) break;
        }
    }
}
void pawsWidgetTreeNode::SetWidget(pawsWidget* _widget)
{
    csRect widgetFrame;
    if(widget != NULL)
        pawsWidget::DeleteChild(widget);

    widget = _widget;
    widget->MoveTo(screenFrame.xmin, screenFrame.ymin);
    widgetFrame = widget->GetScreenFrame();
    SetRelativeFrameSize(widgetFrame.Width(), widgetFrame.Height());
    AddChild(widget);
}

bool pawsWidgetTreeNode::Load(iDocumentNode* node)
{
    csRef<iDocumentNode> nodewidgetNode, widgetNode;
    csString factory;
    pawsWidget* newWidget;

    name = node->GetAttributeValue("name");

    nodewidgetNode = node->GetNode("nodewidget");
    if(nodewidgetNode == NULL)
        return false;
    widgetNode = nodewidgetNode->GetNode("widget");
    if(widgetNode == NULL)
        return false;

    factory = widgetNode->GetAttributeValue("factory");
    newWidget = PawsManager::GetSingleton().CreateWidget(factory);
    if(newWidget == NULL)
        return false;
    if(! newWidget->Load(widgetNode))
    {
        delete newWidget;
        return false;
    }
    SetWidget(newWidget);

    return pawsTreeNode::Load(node);
}

//////////////////////////////////////////////////////////////////////
//
//                            pawsSimpleTree
//
//////////////////////////////////////////////////////////////////////


pawsSimpleTreeNode::pawsSimpleTreeNode()
{
    textBox = NULL;
    image = NULL;
    SetWidget(new pawsWidget());
    factory = "pawsSimpleTreeNode";
}

pawsSimpleTreeNode::pawsSimpleTreeNode(const pawsSimpleTreeNode &origin)
    :pawsCheckTreeNode(origin)
{
    image = 0;
    textBox = 0;
    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.image == origin.children[i])
            image = children[i];
        else if(origin.textBox == origin.children[i])
            textBox = dynamic_cast<pawsTextBox*>(children[i]);

        if(image != 0 && textBox != 0) break;
    }
}

pawsSimpleTreeNode::~pawsSimpleTreeNode()
{
}

void pawsSimpleTreeNode::Set(int mode, bool /*checked*/, const csString &imageName, const csString &label)
{
    csRect frame;
    int lastX;

    assert(tree != NULL);

    if(textBox != NULL)
    {
        widget->DeleteChild(textBox);
        textBox = NULL;
    }
    if(image != NULL)
    {
        widget->DeleteChild(image);
        image = NULL;
    }

    lastX = screenFrame.xmin - 1;
    if(mode & showCheckBox)
    {
        assert("checkbox nodes are not currently implemented"==0);
    }
    if(mode & showImage)
    {
        image = new pawsWidget();
        image->SetSize(20, 20);
        image->SetBackground(imageName);
        image->MoveTo(lastX+1, screenFrame.ymin);
        widget->AddChild(image);
        lastX += 20;
    }
    if(mode & showLabel)
    {
        textBox = new pawsTextBox;
        textBox->SetParent(widget);
        textBox->SetText(label);
        textBox->SetSizeByText(5,5);
        textBox->VertAdjust(pawsTextBox::vertCENTRE);
        textBox->MoveTo(lastX+1,0);

        widget->AddChild(textBox);
        lastX += textBox->GetScreenFrame().Width();
    }
    widget->SetRelativeFrame(0,0,lastX-screenFrame.xmin,20);
    widget->SetSize(lastX-screenFrame.xmin, 20);
    widget->MoveTo(0,0);
    SetRelativeFrameSize(lastX - screenFrame.xmin, 20);
}

bool pawsSimpleTreeNode::Load(iDocumentNode* node)
{
    csRef<iDocumentAttribute> labelAttr, imageAttr, checkedAttr;
    csString labelValue, imageValue;
    bool checkedValue = false;
    int mode;

    if(!pawsCheckTreeNode::Load(node))
        return false;

    mode = 0;

    name = node->GetAttributeValue("name");

    checkedAttr  = node->GetAttribute("checked");
    if(checkedAttr != NULL)
    {
        checkedValue = (checkedAttr->GetValueAsInt() == 1);
        mode |= showCheckBox;
    }
    imageAttr = node->GetAttribute("image");
    if(imageAttr != NULL)
    {
        imageValue = imageAttr->GetValue();
        mode |= showImage;
    }
    labelAttr = node->GetAttribute("label");
    if(labelAttr != NULL)
    {
        labelValue = PawsManager::GetSingleton().Translate(labelAttr->GetValue());
        mode |= showLabel;
    }

    Set(mode, checkedValue, imageValue, labelValue);
    return true;
}

pawsSimpleTree::pawsSimpleTree()
{
    factory = "pawsSimpleTree";
}

pawsSimpleTree::pawsSimpleTree(const pawsSimpleTree &origin)
    :pawsTree(origin),
     defaultColor(origin.defaultColor)
{
}

bool pawsSimpleTree::Setup(iDocumentNode* node)
{
    csString colorStr;

    colorStr  =  node->GetAttributeValue("colour");
    name      = node->GetAttributeValue("name");
    if(colorStr.GetData() != NULL)
        defaultColor = ParseColor(colorStr.GetData(), graphics2D);
    else
        defaultColor = 0xffffff;

    return true;
}

int  pawsSimpleTree::GetDefaultColor()
{
    return defaultColor;
}

void pawsSimpleTree::SetDefaultColor(int _color)
{
    defaultColor = _color;
}

void pawsSimpleTree::InsertChildL(const csString &parent, const csString &name, const csString &label, const csString &nextSibling)
{
    pawsSimpleTreeNode* node = new pawsSimpleTreeNode();
    pawsTreeStruct::InsertChild(parent, node, nextSibling);
    node->SetColour(defaultColor);
    node->Set(showLabel, false, "", label);
    node->SetName(name);
}
void pawsSimpleTree::InsertChildI(const csString &parent, const csString &name, const csString &image, const csString &nextSibling)
{
    pawsSimpleTreeNode* node = new pawsSimpleTreeNode();
    pawsTreeStruct::InsertChild(parent, node, nextSibling);
    node->SetColour(defaultColor);
    node->Set(showImage, false, image, "");
    node->SetName(name);
}
void pawsSimpleTree::InsertChildIL(const csString &parent, const csString &name, const csString &image, const csString &label, const csString &nextSibling)
{
    pawsSimpleTreeNode* node = new pawsSimpleTreeNode();
    pawsTreeStruct::InsertChild(parent, node, nextSibling);
    node->SetColour(defaultColor);
    node->Set(showLabel | showImage, false, image, label);
    node->SetName(name);
}

