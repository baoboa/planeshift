#ifndef PAWS_SLOT_HEADER
#define PAWS_SLOT_HEADER


#include "paws/pawswidget.h"

//=============================================================================
// Forward Declarations
//=============================================================================
class psSlotManager;
class pawsTextBox;


//=============================================================================
// Classes
//=============================================================================


class pawsSlot : public pawsWidget
{
public:
    pawsSlot();
    ~pawsSlot();

    void SetDrag( bool isDragDrop ) { dragDrop = isDragDrop; }
    void SetEmptyOnZeroCount( bool emptyOnZeroCount ) { this->emptyOnZeroCount = emptyOnZeroCount; }
    bool Setup( iDocumentNode* node );
    void SetContainer( int id ) { containerID  = id; }

    bool OnMouseDown( int button, int modifiers, int x, int y );
    void Draw();
    void SetToolTip( const char* text );

    int StackCount() { return stackCount; }
    void StackCount( int newCount );

    int GetPurifyStatus();
    void SetPurifyStatus(int status);

    void PlaceItem( const char* imageName, const char* meshFactName,
        const char* matName = NULL, int count = 0 );
    iPawsImage* Image() { return image; }
    const char *ImageName();

    const char *GetMeshFactName()
    {
        return meshfactName;
    }

    const char *GetMaterialName()
    {
        return materialName;
    }

    const csString & SlotName() const { return slotName; }
    void SetSlotName(const csString & name) { slotName = name; }

    int ContainerID() { return containerID; };
    int ID() { return slotID; }
    void SetSlotID( int id ) { slotID = id; }

    virtual void Clear();
    bool IsEmpty() { if(!reserved) return empty; else return false; }

    void Reserve() { reserved = true; }

    /** Sets if this widget has to draw the stack count.
     *  @param value Sets if the widget has to draw the stack count. True to do so.
     */
    void DrawStackCount(bool value);

    /** Tells if this widget is currently drawing the stack count.
     *  @return BOOL TRUE if the widget is drawing the stack count.
     */
    bool IsDrawingStackCount()  { return drawStackCount; }

    bool SelfPopulate( iDocumentNode *node);

    void OnUpdateData(const char *dataname,PAWSData& value);

    void ScalePurifyStatus();

    bool IsBartender() { return isBartender; }
    void SetBartender( bool t) { isBartender = t; }

    void SetBartenderAction(csString& act) { action = act; }
    csString &GetAction() { return action; }
    void clearBartenderAction() { action.Empty(); }

    bool GetLock() { return locked; }
    void SetLock( bool state ) { locked = state; }
protected:
    psSlotManager*   mgr;
    csString         meshfactName;
    csString         materialName;
    csString         slotName;
    int              containerID;
    int              slotID;
    int              stackCount;
    int              purifyStatus;
    bool empty;
    bool dragDrop;
    bool drawStackCount;
    bool reserved;      ///< implemented to fix dequip behaviour. Cleared on PlaceItem and Clear
    bool locked;

    csRef<iPawsImage> image;
    pawsWidget* purifySign;
    pawsTextBox* stackCountLabel;
    bool handleMouseClicks;
    bool emptyOnZeroCount;      ///< should the slot clear itself when the stackcount hits 0 ?

    bool isBartender;       ///< Flag on if this a special bartender slot.
    csString action;        ///< This is the action to do if this a bartender button.
};

CREATE_PAWS_FACTORY( pawsSlot );

#endif

