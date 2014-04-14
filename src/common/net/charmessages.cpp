#include <psconfig.h>
#include "charmessages.h"


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharApprovedMessage,MSGTYPE_CHAR_CREATE_UPLOAD);

psCharApprovedMessage::psCharApprovedMessage(uint32_t clientnum)
{
    msg.AttachNew(new MsgEntry());

    msg->SetType(MSGTYPE_CHAR_CREATE_UPLOAD);
    msg->clientnum = clientnum;
}

psCharApprovedMessage::psCharApprovedMessage(MsgEntry* )
{
}

csString psCharApprovedMessage::ToString(NetBase::AccessPointers*  /*accessPointers*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("Approved");

    return msgtext;
}


//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharRejectedMessage,MSGTYPE_CHARREJECT);

psCharRejectedMessage::psCharRejectedMessage( uint32_t clientnum, 
                                              int type,
                                              const char* mesg )
{
    msg.AttachNew(new MsgEntry(sizeof(int) + strlen(mesg)+1, PRIORITY_HIGH));

    msg->SetType(MSGTYPE_CHARREJECT);
    msg->clientnum = clientnum;
    msg->Add( (uint32_t) type );
    msg->Add( mesg );
    
    valid = !(msg->overrun);
}

psCharRejectedMessage::psCharRejectedMessage(MsgEntry* msg )
{
    errorType = msg->GetUInt32();
    errorMesg = msg->GetStr();
    
    valid = !(msg->overrun);
}

csString psCharRejectedMessage::ToString(NetBase::AccessPointers*  /*accessPointers*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("Type: %d Msg: '%s'",errorType,errorMesg.GetDataSafe());

    return msgtext;
}


//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharVerificationMesg,MSGTYPE_CHAR_CREATE_VERIFY);

psCharVerificationMesg::psCharVerificationMesg( uint32_t client )
{
    runningSize = 0;
    clientnum = client;
}
    
void psCharVerificationMesg::AddStat( int value, const char* attributeName )
{
    Attribute a;
    a.value = value;
    a.name = attributeName;
    
    stats.Push( a );
    runningSize+=sizeof(int)+strlen(attributeName)+1;
}

void psCharVerificationMesg::AddSkill( int value, const char* attributeName )
{
    Attribute a;
    a.value = value;
    a.name = attributeName;
    
    skills.Push( a );
    runningSize+=sizeof(int)+strlen(attributeName)+1;
}
  
void psCharVerificationMesg::Construct()
{
    msg.AttachNew(new MsgEntry( runningSize+sizeof(int)*2 ));
    msg->clientnum  = clientnum;                        
    msg->SetType(MSGTYPE_CHAR_CREATE_VERIFY);
    msg->Add( (uint32_t)stats.GetSize() );
    msg->Add( (uint32_t)skills.GetSize() );
    
    for ( size_t z = 0; z < stats.GetSize(); z++ )
    {
        msg->Add( (uint32_t)stats[z].value );   
        msg->Add( stats[z].name );
    }
    
    for ( size_t z = 0; z < skills.GetSize(); z++ )
    {
        msg->Add( (uint32_t)skills[z].value );   
        msg->Add( skills[z].name );
    }
    valid = !(msg->overrun);    
}

psCharVerificationMesg::psCharVerificationMesg( MsgEntry* me )
{
    size_t stat = me->GetUInt32();
    size_t skill = me->GetUInt32();    
    
    for( size_t z = 0; z < stat; z++ )
    {
        Attribute a;
        a.value = me->GetUInt32();
        a.name  = me->GetStr();
        stats.Push( a );
    }
    
    for( size_t z = 0; z < skill; z++ )
    {
        Attribute a;
        a.value = me->GetUInt32();
        a.name  = me->GetStr();
        skills.Push( a );
    }
} 
    
csString psCharVerificationMesg::ToString(NetBase::AccessPointers*  /*accessPointers*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("Stats:");
    for( size_t z = 0; z < stats.GetSize(); z++ )
    {
        Attribute a = stats[z];
        msgtext.AppendFmt(" %zu('%s',%d)",z,a.name.GetDataSafe(),a.value);
    }
    msgtext.AppendFmt(" Skills:");    
    for( size_t z = 0; z < skills.GetSize(); z++ )
    {
        Attribute a = skills[z];
        msgtext.AppendFmt(" %zu('%s',%d)",z,a.name.GetDataSafe(),a.value);
    }

    return msgtext;
}


//-----------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharUploadMessage,MSGTYPE_CHAR_CREATE_UPLOAD);

psCharUploadMessage::psCharUploadMessage(bool verify,  const char* name, const char* lastname, int race, int gender, 
                         csArray<uint32_t> choices, int motherMod, int fatherMod, csArray<uint32_t> lifeEvents,
                         int selectedFace, int selectedHairStyle, int selectedBeardStyle, 
                         int selectedHairColour, int selectedSkinColour, const char* bio, const char* path )
{
    msg.AttachNew(new MsgEntry( sizeof(bool) +
                        strlen(name) * sizeof(char) + 1 +
                        strlen(lastname) * sizeof(char) + 1 +
                        sizeof(int32_t) * 11 +
                        sizeof(int32_t) * choices.GetSize() + 
                        sizeof(int32_t) * lifeEvents.GetSize() +
                        strlen(bio) * sizeof(char) + 1 +
                        strlen(path) * sizeof(char) + 1));


    msg->SetType(MSGTYPE_CHAR_CREATE_UPLOAD);
    msg->Add( verify );
    msg->Add( name );
    msg->Add( lastname );
    msg->Add( (int32_t)race );
    msg->Add( (int32_t)gender );
    
    msg->Add( (int32_t)selectedFace );
    msg->Add( (int32_t)selectedHairStyle );
    msg->Add( (int32_t)selectedBeardStyle );
    msg->Add( (int32_t)selectedHairColour );
    msg->Add( (int32_t)selectedSkinColour );
    
    msg->Add( (int32_t) choices.GetSize() );
    for ( size_t choiceIndex = 0; choiceIndex < choices.GetSize(); choiceIndex++ )
        msg->Add( (int32_t)choices[choiceIndex] );
    
    msg->Add( (int32_t) lifeEvents.GetSize() );
    for ( size_t lifeIndex = 0; lifeIndex < lifeEvents.GetSize(); lifeIndex++ )
        msg->Add( (int32_t)lifeEvents[lifeIndex] );
 
    msg->Add( (int32_t) motherMod );
    msg->Add( (int32_t) fatherMod );
    msg->Add( bio );
    msg->Add( path );

    valid = !(msg->overrun);
}
    
    
psCharUploadMessage::psCharUploadMessage( MsgEntry* me )
{
    verify = me->GetBool();
    name = me->GetStr();
    lastname = me->GetStr();
    race = me->GetUInt32();
    gender = me->GetUInt32();
    
    selectedFace = me->GetUInt32();
    selectedHairStyle = me->GetUInt32();
    selectedBeardStyle = me->GetUInt32();
    selectedHairColour = me->GetUInt32();
    selectedSkinColour = me->GetUInt32();       
    
    int32_t choiceLength = me->GetUInt32();
    for ( int x = 0; x < choiceLength; x++ )
        choices.Push( me->GetUInt32() );
 
    int32_t lifeLength = me->GetUInt32();
    for ( int y = 0; y < lifeLength; y++ )
        lifeEvents.Push( me->GetUInt32() );

    motherMod = me->GetUInt32();
    fatherMod = me->GetUInt32();
            
    bio = me->GetStr();
    path = me->GetStr();
    
    valid=!(me->overrun);
}


csString psCharUploadMessage::ToString(NetBase::AccessPointers*  /*accessPointers*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("Verify: %s",(verify?"true":"false"));
    msgtext.AppendFmt(" Name: '%s'",name.GetDataSafe());
    msgtext.AppendFmt(" Last: '%s'",lastname.GetDataSafe());
    msgtext.AppendFmt(" Race: %d",race);
    msgtext.AppendFmt(" Gender: %d",gender);
    msgtext.AppendFmt(" Face: %d",selectedFace);
    msgtext.AppendFmt(" HairStyle: %d",selectedHairStyle);
    msgtext.AppendFmt(" BeardStyle: %d",selectedBeardStyle);
    msgtext.AppendFmt(" HairColour: %d",selectedHairColour);
    msgtext.AppendFmt(" SkinColour: %d",selectedSkinColour);

    msgtext.AppendFmt(" Choices: ");
    for ( size_t x = 0; x < choices.GetSize(); x++ )
        msgtext.AppendFmt(" %d",choices[x]);
 
    for ( size_t y = 0; y < lifeEvents.GetSize(); y++ )
        msgtext.AppendFmt(" %d",lifeEvents[y]);

    msgtext.AppendFmt(" MotherMod: %d",motherMod);
    msgtext.AppendFmt(" FaderMod: %d",fatherMod);
    msgtext.AppendFmt(" Bio: '%s'",bio.GetDataSafe());
    msgtext.AppendFmt(" Path: '%s'",path.GetDataSafe());

    return msgtext;
}

//----------------------------------------------------------------------------
/*  TODO: This class is used by: 
    - MSGTYPE_CHAR_CREATE_PARENTS
    - MSGTYPE_CHAR_CREATE_CHILDHOOD
    - MSGTYPE_CHAR_CREATE_TRAITS
    Need to define subclasses for each of thise.
*/
PSF_IMPLEMENT_MSG_FACTORY(psCreationChoiceMsg,MSGTYPE_CHAR_CREATE_PARENTS);

psCreationChoiceMsg::psCreationChoiceMsg( int listener )
{
    msg.AttachNew(new MsgEntry( sizeof(int32_t) ));

    msg->clientnum  = 0;
    msg->bytes->type = listener;
    
    /// Add total choices as 0
    msg->Add( (int32_t)0 );
}

psCreationChoiceMsg::psCreationChoiceMsg( MsgEntry* me )
{    
    int totalChoices = me->GetUInt32();
    
    // sanity check!
    if(totalChoices > 30000)
    {
        valid = false;
        return;
    }

    choices.SetSize( totalChoices );
    
    for ( int x = 0; x < totalChoices; x++ )
    {
        choices[x].id           = me->GetUInt32();
        choices[x].name         = me->GetStr();
        choices[x].description  = me->GetStr();
        choices[x].choiceArea   = me->GetUInt32();
        choices[x].cpCost       = me->GetUInt32();
    }        
    
}

psCreationChoiceMsg::psCreationChoiceMsg( int clientTo, int totalChoices, int listener )
{
    // can be a big message!
    msg.AttachNew(new MsgEntry( 30000 ));

    msg->clientnum  = clientTo;
    msg->bytes->type = listener;

    msg->Add( (int32_t)totalChoices );
}

void psCreationChoiceMsg::AddChoice( int id, const char* name, const char* description, int choiceArea, int cpCost )
{
    msg->Add( (int32_t)id );
    msg->Add( name );
    msg->Add( description );
    msg->Add( (int32_t) choiceArea );
    msg->Add( (int32_t) cpCost );
}


void psCreationChoiceMsg::ConstructMessage()
{
    msg->ClipToCurrentSize();
}

csString psCreationChoiceMsg::ToString(NetBase::AccessPointers*  /*accessPointers*/)
{
    csString msgtext;
    
    for ( size_t x = 0; x < choices.GetSize(); x++ )
    {
        msgtext.AppendFmt(" %zu(Id: %d Name: '%s'"
                          " Descr: '%s' Area: %d CPCost: %d)",
                          x,choices[x].id,
                          choices[x].name.GetDataSafe(),
                          choices[x].description.GetDataSafe(),
                          choices[x].choiceArea,
                          choices[x].cpCost);
    }        
    return msgtext;
}


//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psLifeEventMsg,MSGTYPE_CHAR_CREATE_LIFEEVENTS);

psLifeEventMsg::psLifeEventMsg()
{
    msg.AttachNew(new MsgEntry( sizeof ( int32_t) ));
    msg->clientnum  = 0;
    msg->SetType(MSGTYPE_CHAR_CREATE_LIFEEVENTS);   
    
    msg->Add( (int32_t) choices.GetSize() );    
}

psLifeEventMsg::psLifeEventMsg( uint32_t client )
{
    toclient = client;
    runningSize = 0;    
}    


void psLifeEventMsg::AddEvent( LifeEventChoice* event )
{
    LifeEventChoice* choice = new LifeEventChoice;
    runningSize += sizeof(int)*3 + 
                   event->name.Length() + 1 + 
                   event->description.Length()+1;
    
    choice->id          = event->id;
    choice->name        = event->name;
    choice->description = event->description;
    choice->common      = event->common;
    choice->cpCost      = event->cpCost;
    
    for ( size_t x = 0; x < event->adds.GetSize(); x++ )
    {
        choice->adds.Push( event->adds[x] );
        runningSize+=sizeof( sizeof(int) );
    }        
        
    for ( size_t y = 0; y < event->removes.GetSize(); y++ )
    {
        choice->removes.Push( event->removes[y] );
        runningSize+=sizeof( sizeof(int) );
    }        
    
    runningSize+=sizeof(int)*2;  // for the 2 Lengths that are added
    choices.Push( choice );
}        


void psLifeEventMsg::ConstructMessage()
{
    msg.AttachNew(new MsgEntry( runningSize+sizeof(int) ));    
    msg->clientnum  = toclient;
    msg->SetType(MSGTYPE_CHAR_CREATE_LIFEEVENTS);       
    
    // Total Number of choices
    msg->Add( (int32_t) choices.GetSize() );
       
    for ( size_t x = 0; x < choices.GetSize(); x++ )
    {
        msg->Add( (uint32_t)choices[x]->id );
        msg->Add( choices[x]->name );
        msg->Add( choices[x]->description );
        msg->Add( (uint32_t) choices[x]->common );
        msg->Add( (uint32_t) choices[x]->cpCost );        
        
        msg->Add( (uint32_t)choices[x]->adds.GetSize() );        
        size_t addIndex;
        
        for ( addIndex = 0; addIndex < choices[x]->adds.GetSize(); addIndex++ )
            msg->Add( (uint32_t)choices[x]->adds[addIndex] );
  
        msg->Add( (uint32_t)choices[x]->removes.GetSize() );        
        
        size_t removesIndex;
        for ( removesIndex = 0; removesIndex < choices[x]->removes.GetSize(); removesIndex++ )
            msg->Add( (uint32_t)choices[x]->removes[removesIndex] );            
    }                
}

psLifeEventMsg::psLifeEventMsg( MsgEntry* msg )
{
    int totalChoices = msg->GetUInt32();
    
    for ( int x = 0; x < totalChoices; x++ )
    {
        LifeEventChoice *choice = new LifeEventChoice;
        choice->id          = msg->GetUInt32();
        choice->name        = msg->GetStr();
        choice->description = msg->GetStr();
        choice->common      = msg->GetUInt32();
        choice->cpCost      = msg->GetUInt32();
        
        unsigned int addsCount = msg->GetUInt32();
        for ( unsigned int addIdx = 0; addIdx < addsCount; addIdx++ )
            choice->adds.Push(msg->GetUInt32());
                    
        unsigned int removesCount = msg->GetUInt32();
        for ( unsigned int removeIdx = 0; removeIdx < removesCount; removeIdx++ )
            choice->removes.Push(msg->GetUInt32());
                               
        choices.Push( choice );            
    }    
}

csString psLifeEventMsg::ToString(NetBase::AccessPointers*  /*accessPointers*/)
{
    csString msgtext;
    
    for ( size_t x = 0; x < choices.GetSize(); x++ )
    {
        LifeEventChoice *choice = choices[x];
        msgtext.AppendFmt(" %zu(Id: %d Name: '%s'"
                          " Descr: '%s' Common: %d CPCost: %d Adds:",
                          x,choice->id,
                          choice->name.GetDataSafe(),
                          choice->description.GetDataSafe(),
                          choice->common,
                          choice->cpCost);
        for ( size_t addIdx = 0; addIdx < choice->adds.GetSize(); addIdx++ )
            msgtext.AppendFmt(" %d",choice->adds[addIdx]);
        for ( size_t removeIdx = 0; removeIdx < choice->removes.GetSize(); removeIdx++ )
            msgtext.AppendFmt(" %d",choice->removes[removeIdx]);

    }        
    return msgtext;
}

//--------------------------------------------------------------------------

