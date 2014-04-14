/*
 * pawsSketchWindow.h - Author: Keith Fulton
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_SKETCH_WINDOW_HEADER
#define PAWS_SKETCH_WINDOW_HEADER

#include "net/cmdbase.h"
#include "paws/pawswidget.h"
#include "paws/pawscolorpromptwindow.h"
#include "paws/pawsstringpromptwindow.h"

#define SKETCHBEZIER_SEGMENTS 20
/**
 * A window that shows a map or picture.
 */
class pawsSketchWindow : public pawsWidget, public psClientNetSubscriber,public iOnStringEnteredAction,public iOnColorEnteredAction
{
    class BezierWeights
    {
    private:
        float *precomputedBezierWeights; //[SKETCHBEZIER_SEGMENTS+1][4];
    public:
        BezierWeights();
        ~BezierWeights();

        // this is SLOWER than Get(int offset)
        inline float Get(int segmentIndex, int controlpointNum)
        {
            //CS_ASSERT(segmentIndex <= SKETCHBEZIER_SEGMENTS && segmentIndex >= 0);
            //CS_ASSERT(controlpointNum >= 0 && controlpointNum <= 3);
            return precomputedBezierWeights[segmentIndex * SKETCHBEZIER_SEGMENTS + controlpointNum];
        }

        // returns the precomputed value of "segmentIndex*SKETCHBEZIER_SEGMENTS + controlpointNum"
        inline float Get(int offset)
        {
            //CS_ASSERT(offset <= (SKETCHBEZIER_SEGMENTS+1)*4 && offset >= 0);
            return precomputedBezierWeights[offset];
        }
    };
    struct SketchObject
    {
        bool selected;
        int x,y;
        csString str;
        int frame;    /// This is incremented each frame when the object is selected to make it flash
        int color;    /// This is the color. -1 by default = drawing "without colours".
        bool colorSet;/// changed to true in SetColor, to false in Select. Needed to show the "new color" rather than drawing them "red"(selected)

        pawsSketchWindow *parent;

        const char *GetStr() { return str; }
        virtual void SetStr(const char *s) { str=s; }
        SketchObject()  { x=0; y=0; selected=false; color=-1; colorSet=false; }
        virtual ~SketchObject() {};

        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent)=0;
        virtual void WriteXml(csString& xml) = 0;
        virtual void Draw()=0;
        virtual bool IsHit(int mouseX, int mouseY)=0;
        virtual void Select(bool _selected)         { selected=_selected; colorSet=false; frame=0; }
        // update the RELATIVE position
        virtual void UpdatePosition(int _x, int _y) { x = _x; y = _y;     }
        virtual void SetColor(int _color) { color = _color; colorSet=true; }
    };
    struct SketchIcon : public SketchObject
    {
        csRef<iPawsImage> iconImage;
        SketchIcon() : SketchObject() {}
        SketchIcon(int _x, int _y, const char *icon, pawsSketchWindow *parent);
        virtual ~SketchIcon() {};
        bool Init(int _x, int _y, const char *icon, pawsSketchWindow *parent);
        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent);
        virtual void WriteXml(csString& xml);
        virtual void Draw();
        virtual bool IsHit(int mouseX, int mouseY);
        virtual void SetStr(const char *s);
        // update the RELATIVE position
        virtual void UpdatePosition(int _x, int _y);
    };
    struct SketchText : public SketchObject
    {
        csString font;

        SketchText(int _x,int _y,const char *value,pawsSketchWindow *_parent)
        {
            x = _x;
            y = _y;
            str = value;
            parent = _parent;
        }
        SketchText() {}
        virtual ~SketchText() {};
        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent);
        virtual void WriteXml(csString& xml);
        virtual void Draw();
        virtual bool IsHit(int mouseX, int mouseY);
        // update the RELATIVE position
        virtual void UpdatePosition(int _x, int _y);
    };
    struct SketchLine : public SketchObject
    {
        int x2,y2;
        int offsetX, offsetY;
        int dragMode;

        SketchLine(int _x,int _y, int _x2, int _y2,pawsSketchWindow *_parent)
        {
            x=_x;    y=_y;
            x2=_x2; y2=_y2;
            parent = _parent;
            dragMode = 0;
        }
        SketchLine() {};
        virtual ~SketchLine() {};
        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent);
        virtual void WriteXml(csString& xml);
        virtual void Draw();
        virtual bool IsHit(int mouseX, int mouseY);
        // update the RELATIVE position
        virtual void UpdatePosition(int _x, int _y);
    };
    struct SketchBezier : public SketchObject
    {
        /* Cubic Bézier Curve
            p1        p2 (control points)
           /            \_
        p0-----------------p3

        2 lines would actually be enough to handle the bezier curve.
        Connecting p0-p1 and p3-p2: However, the usability would suffer since we can't move the "whole line" then.
        Connecting p1-p2 and p0-p3: Displaying this doesn't look good and usability suffers.
        */

        //BezierHelper *bezierHelper;

        SketchLine p0p1;
        SketchLine p0p3;
        SketchLine p3p2;

        SketchBezier(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3, pawsSketchWindow *_parent)
        {
            this->parent = _parent;
            //this->bezierHelper = &_parent->bezierHelper;

            p0p1.parent = _parent;
            p0p1.x = x; // point 0 (also start point)
            p0p1.y = y;
            p0p1.x2 = x1; // point 1 (control point 1)
            p0p1.y2 = y1;

            p0p3.parent = _parent;
            p0p3.x = x; // point 0
            p0p3.y = y;
            p0p3.x2 = x3; // point 3 (also end point)
            p0p3.y2 = y3;

            p3p2.parent = _parent;
            p3p2.x2 = x2; // point 3 (also end point)
            p3p2.y2 = y2;
            p3p2.x = x3; // point 2 (control point 2)
            p3p2.y = y3;
        }
        SketchBezier() {};
        virtual ~SketchBezier() {};
        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent);
        virtual void WriteXml(csString& xml);
        virtual void Draw();
        virtual bool IsHit(int mouseX, int mouseY);
        // update the RELATIVE position
        virtual void UpdatePosition(int _x, int _y);
    };

protected:
    csPDelArray<SketchObject> objlist;
    size_t selectedIndex;
    int dirty;
    uint32_t currentItemID;
    csRef<iPawsImage> blackBox;
    bool editMode;
    bool mouseDown;
    int mouseDownX; // used to calculate the updatePosition diff. this value is updated OnMouseDown
    int mouseDownY;
    csStringArray iconList;
    int primCount;
    bool readOnly;
    bool stringPending;
    bool colorPending;
    BezierWeights bezierWeights;

    int frame; // incremented each frame till 10 (to update selection without clicking)

    pawsWidget* FeatherTool;
    pawsWidget* TextTool;
    pawsWidget* LineTool;
    pawsWidget* BezierTool;
    pawsWidget* PlusTool;
    pawsWidget* LeftArrowTool;
    pawsWidget* RightArrowTool;
    pawsWidget* DeleteTool;
    pawsWidget* NameTool;
    pawsWidget* SaveButton;
    pawsWidget* LoadButton;
    pawsWidget* ColorTool;

    void DrawSketch();
    bool ParseSketch(const char *xml, bool checklimits = false);
    bool ParseLimits(const char *xmlstr);
    void SetToolbarButtons();  //sets the toolbar buttons hide/show status

    void AddSketchText();
    void AddSketchIcon();
    void AddSketchLine();
    void AddSketchBezier();
    void RemoveSelected();
    void NextPrevIcon(int delta);
    void MoveObject(int dx, int dy);
    void ChangeSketchName();
    void SaveSketch();
    void LoadSketch();
    bool isBadChar(char c);

    csString toXML();

    csString filenameSafe(const csString &original);

    csString sketchName;
    
    /// Background image.
    csRef<iPawsImage> sketchBgImage;

public:
    pawsSketchWindow();
    /// TODO: Copy constructor, useless currently. Would be implemented later.
    pawsSketchWindow(const pawsSketchWindow& origin);
    virtual ~pawsSketchWindow();

    bool PostSetup();

    void HandleMessage( MsgEntry* message );

    /// inherited from iOnStringEnteredAction
    void OnStringEntered(const char *name, int param, const char *value);

    /// inherited from iOnColorEnteredAction
    void OnColorEntered(const char *name,int param,int color);

    /// inherited from iScriptableVar from pawsWidget
    double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);


    virtual bool OnMouseDown( int button, int modifiers, int x, int y );
    virtual bool OnMouseUp( int button, int modifiers, int x, int y );
    virtual bool OnKeyDown( utf32_char keyCode, utf32_char key, int modifiers );
    virtual void Hide();
    virtual void Draw();

    // SketchObject helper functions
    iGraphics2D *GetG2D();
    void DrawBlackBox(int x, int y);
    void DrawColorWidgetText(const char *text, int x, int y, int color);
    bool IsMouseDown() { return mouseDown; }

    virtual bool GetFocusOverridesControls() const { return true; }
};

CREATE_PAWS_FACTORY( pawsSketchWindow );


#endif


