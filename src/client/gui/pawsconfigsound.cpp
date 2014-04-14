/*
 * pawsconfigsound.cpp - Author: Christian Svensson
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/vfs.h>

// COMMON INCLUDES
#include "util/log.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawsconfigsound.h"
#include "paws/pawsmanager.h"
#include "paws/pawscrollbar.h"
#include "paws/pawscheckbox.h"

pawsConfigSound::pawsConfigSound()
{
    loaded= false;
}

bool pawsConfigSound::Initialize()
{
    if ( ! LoadFromFile("configsound.xml"))
        return false;

    return true;
}

bool pawsConfigSound::PostSetup()
{
    generalVol = (pawsScrollBar*)FindWidget("generalVol");
    if(!generalVol)
        return false;

    musicVol = (pawsScrollBar*)FindWidget("musicVol");
    if(!musicVol)
        return false;

    ambientVol = (pawsScrollBar*)FindWidget("ambientVol");
    if(!ambientVol)
        return false;

    actionsVol = (pawsScrollBar*)FindWidget("actionsVol");
    if(!actionsVol)
        return false;

    effectsVol = (pawsScrollBar*)FindWidget("effectsVol");
    if(!effectsVol)
        return false;

    guiVol = (pawsScrollBar*)FindWidget("guiVol");
    if(!guiVol)
        return false;

    voicesVol = (pawsScrollBar*)FindWidget("voicesVol");
    if(!voicesVol)
        return false;

    instrumentsVol = (pawsScrollBar*)FindWidget("instrumentsVol");
    if(!instrumentsVol)
        return false;

    instruments = (pawsCheckBox*)FindWidget("instruments");
    if(!instruments)
        return false;

    voices = (pawsCheckBox*)FindWidget("voices");
    if(!voices)
        return false;

    gui = (pawsCheckBox*)FindWidget("gui");
    if(!gui)
        return false;

    ambient = (pawsCheckBox*)FindWidget("ambient");
    if(!ambient)
        return false;

    actions = (pawsCheckBox*)FindWidget("actions");
    if(!actions)
        return false;

    effects = (pawsCheckBox*)FindWidget("effects");
    if(!effects)
        return false;

    music = (pawsCheckBox*)FindWidget("music");
    if(!music)
        return false;

    muteOnFocusLoss = (pawsCheckBox*) FindWidget("muteOnFocusLoss");
    if (!muteOnFocusLoss)
        return false;

    loopBGM = (pawsCheckBox*) FindWidget("loopBGM");
    if (!loopBGM)
        return false;

    combatMusic = (pawsCheckBox*) FindWidget("combatMusic");
    if (!combatMusic)
        return false;

    chatSound = (pawsCheckBox*) FindWidget("chatSound");
    if (!chatSound)
        return false;

    soundLocation = (pawsComboBox*) FindWidget("soundLocation");
    soundLocation->NewOption("Player");
    soundLocation->NewOption("Camera");
    
    generalVol->SetMaxValue(200);
    generalVol->SetTickValue(10);
    generalVol->EnableValueLimit(true);

    musicVol->SetMaxValue(100);
    musicVol->SetTickValue(10);
    musicVol->EnableValueLimit(true);

    ambientVol->SetMaxValue(100);
    ambientVol->SetTickValue(10);
    ambientVol->EnableValueLimit(true);

    actionsVol->SetMaxValue(100);
    actionsVol->SetTickValue(10);
    actionsVol->EnableValueLimit(true);

    effectsVol->SetMaxValue(100);
    effectsVol->SetTickValue(10);
    effectsVol->EnableValueLimit(true);

    guiVol->SetMaxValue(100);
    guiVol->SetTickValue(10);
    guiVol->EnableValueLimit(true);

    voicesVol->SetMaxValue(100);
    voicesVol->SetTickValue(10);
    voicesVol->EnableValueLimit(true);

    instrumentsVol->SetMaxValue(100);
    instrumentsVol->SetTickValue(10);
    instrumentsVol->EnableValueLimit(true);

    return true;
}
bool pawsConfigSound::LoadConfig()
{
    generalVol->SetCurrentValue(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MAIN_SNDCTRL)->GetVolume()*100,false);
    musicVol->SetCurrentValue(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->GetVolume()*100,false);
    ambientVol->SetCurrentValue(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->GetVolume()*100,false);
    guiVol->SetCurrentValue(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->GetVolume()*100,false);
    voicesVol->SetCurrentValue(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->GetVolume()*100,false);
    actionsVol->SetCurrentValue(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->GetVolume()*100,false);
    effectsVol->SetCurrentValue(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->GetVolume()*100,false);
    instrumentsVol->SetCurrentValue(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->GetVolume() * 100, false);

    ambient->SetState(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->GetToggle());
    actions->SetState(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->GetToggle());
    effects->SetState(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->GetToggle());
    music->SetState(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->GetToggle());
    gui->SetState(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->GetToggle());
    voices->SetState(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->GetToggle());
    instruments->SetState(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->GetToggle());

    muteOnFocusLoss->SetState(psengine->GetMuteSoundsOnFocusLoss());
    loopBGM->SetState(psengine->GetSoundManager()->IsLoopBGMToggleOn());
    combatMusic->SetState(psengine->GetSoundManager()->IsCombatMusicToggleOn());
    chatSound->SetState(psengine->GetSoundManager()->IsChatToggleOn());
    
    if (psengine->GetSoundManager()->IsListenerOnCameraToggleOn() == true)
    {
        soundLocation->Select("Camera");
    }
    else
    {
        soundLocation->Select("Player");
    }

    loaded= true;
    dirty = false;
    return true;
}

bool pawsConfigSound::SaveConfig()
{
    csString xml;
    xml = "<sound>\n";
    xml.AppendFmt("<ambient on=\"%s\" />\n",
                     ambient->GetState() ? "yes" : "no");
    xml.AppendFmt("<actions on=\"%s\" />\n",
                     actions->GetState() ? "yes" : "no");
    xml.AppendFmt("<effects on=\"%s\" />\n",
                     effects->GetState() ? "yes" : "no");
    xml.AppendFmt("<music on=\"%s\" />\n",
                     music->GetState() ? "yes" : "no");
    xml.AppendFmt("<gui on=\"%s\" />\n",
                     gui->GetState() ? "yes" : "no");
    xml.AppendFmt("<voices on=\"%s\" />\n",
                     voices->GetState() ? "yes" : "no");
    xml.AppendFmt("<instruments on=\"%s\" />\n",
                     instruments->GetState() ? "yes" : "no");
    xml.AppendFmt("<volume value=\"%d\" />\n",
                     int(generalVol->GetCurrentValue()));
    xml.AppendFmt("<musicvolume value=\"%d\" />\n",
                     int(musicVol->GetCurrentValue()));
    xml.AppendFmt("<ambientvolume value=\"%d\" />\n",
                     int(ambientVol->GetCurrentValue()));
    xml.AppendFmt("<actionsvolume value=\"%d\" />\n",
                     int(actionsVol->GetCurrentValue()));
    xml.AppendFmt("<effectsvolume value=\"%d\" />\n",
                     int(effectsVol->GetCurrentValue()));
    xml.AppendFmt("<guivolume value=\"%d\" />\n",
                     int(guiVol->GetCurrentValue()));
    xml.AppendFmt("<voicesvolume value=\"%d\" />\n",
                     int(voicesVol->GetCurrentValue()));
    xml.AppendFmt("<instrumentsvolume value=\"%d\" />\n",
                     int(instrumentsVol->GetCurrentValue()));
    xml.AppendFmt("<muteonfocusloss on=\"%s\" />\n",
                     muteOnFocusLoss->GetState() ? "yes" : "no");
    xml.AppendFmt("<loopbgm on=\"%s\" />\n",
                     loopBGM->GetState() ? "yes" : "no");
    xml.AppendFmt("<combatmusic on=\"%s\" />\n",
                     combatMusic->GetState() ? "yes" : "no");
    xml.AppendFmt("<chatsound on=\"%s\" />\n",
                     chatSound->GetState() ? "yes" : "no");
               
    if (csStrCaseCmp(soundLocation->GetSelectedRowString(), "Camera") == 0)
    {
        xml.AppendFmt("<usecamerapos on=\"%s\" />\n", "yes");
    }
    else
    {
        xml.AppendFmt("<usecamerapos on=\"%s\" />\n", "no");
    }
    xml += "</sound>\n";

    dirty = false;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/sound.xml",
                                         xml,xml.Length());
}

void pawsConfigSound::SetDefault()
{
    psengine->LoadSoundSettings(true);
    LoadConfig();
}

bool pawsConfigSound::OnScroll(int /*scrollDir*/, pawsScrollBar* wdg)
{
    dirty = true;
    if(wdg == generalVol && loaded)
    {
        if(generalVol->GetCurrentValue() < 1)
            generalVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MAIN_SNDCTRL)->SetVolume(generalVol->GetCurrentValue()/100);
    }
    else if(wdg == musicVol && loaded)
    {
        if(musicVol->GetCurrentValue() < 1)
            musicVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->SetVolume(musicVol->GetCurrentValue()/100);
    }
    else if(wdg == ambientVol && loaded)
    {
        if(ambientVol->GetCurrentValue() < 1)
            ambientVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->SetVolume(ambientVol->GetCurrentValue()/100);
    }
    else if(wdg == actionsVol && loaded)
    {
        if(actionsVol->GetCurrentValue() < 1)
            actionsVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->SetVolume(actionsVol->GetCurrentValue()/100);
    }
    else if(wdg == effectsVol && loaded)
    {
        if(effectsVol->GetCurrentValue() < 1)
            effectsVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->SetVolume(effectsVol->GetCurrentValue()/100);
    }
    else if(wdg == guiVol && loaded)
    {
        if(guiVol->GetCurrentValue() < 1)
            guiVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->SetVolume(guiVol->GetCurrentValue()/100);
    }
    else if(wdg == voicesVol && loaded)
    {
        if(voicesVol->GetCurrentValue() < 1)
            voicesVol->SetCurrentValue(1,false);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->SetVolume(voicesVol->GetCurrentValue()/100);
    }
    else if(wdg == instrumentsVol && loaded)
    {
        if(instrumentsVol->GetCurrentValue() < 1)
            instrumentsVol->SetCurrentValue(1, false);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->SetVolume(instrumentsVol->GetCurrentValue() / 100);
    }
    else
    {
        return false;
    }
    
    SaveConfig();
    return true;
}

bool pawsConfigSound::OnButtonPressed(int /*button*/, int /*mod*/, pawsWidget* wdg)
{
    dirty = true;

    if(wdg == ambient)
    {
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->SetToggle(ambient->GetState());
    }
    else if(wdg == actions)
    {
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->SetToggle(actions->GetState());
    }
    else if(wdg == effects)
    {
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->SetToggle(effects->GetState());
    }
    else if(wdg == music)
    {
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->SetToggle(music->GetState());
    }
    else if(wdg == gui)
    {
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->SetToggle(gui->GetState());
    }
    else if(wdg == voices)
    {
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->SetToggle(voices->GetState());
    }
    else if(wdg == instruments)
    {
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->SetToggle(instruments->GetState());
    }
    else if(wdg == loopBGM)
    {
        psengine->GetSoundManager()->SetLoopBGMToggle(loopBGM->GetState());
    }
    else if(wdg == combatMusic)
    {
        psengine->GetSoundManager()->SetCombatMusicToggle(combatMusic->GetState());
    }
    else if(wdg == muteOnFocusLoss)
    {
        psengine->SetMuteSoundsOnFocusLoss(muteOnFocusLoss->GetState());
    }
    else if(wdg == chatSound)
    {
        psengine->GetSoundManager()->SetChatToggle(chatSound->GetState());
    }
    else
    {
        return false;
    }

    SaveConfig();    
    return true;
}

void pawsConfigSound::OnListAction(pawsListBox* /*selected*/, int /*status*/)
{
    pawsComboBox* soundlocation = (pawsComboBox*)FindWidget("soundLocation");
    csString _selected = soundlocation->GetSelectedRowString();
   
    if (_selected.Compare("Camera"))
    {
        psengine->GetSoundManager()->SetListenerOnCameraToggle(true);
    }
    else
    {
        psengine->GetSoundManager()->SetListenerOnCameraToggle(false);
    }
    SaveConfig();
}


void pawsConfigSound::Show()
{
    oldambient = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->GetToggle();
    oldmusic = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->GetToggle();
    oldactions = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->GetToggle();
    oldeffects = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->GetToggle();
    oldgui = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->GetToggle();
    oldvoices = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->GetToggle();
    oldInstruments = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->GetToggle();
    oldchatsound = psengine->GetSoundManager()->IsChatToggleOn();
    oldlisteneroncamerapos = psengine->GetSoundManager()->IsListenerOnCameraToggleOn();

    oldvol = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MAIN_SNDCTRL)->GetVolume();
    oldmusicvol = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->GetVolume();
    oldambientvol = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->GetVolume();
    oldactionsvol = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->GetVolume();
    oldeffectsvol = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->GetVolume();
    oldguivol = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->GetVolume();
    oldvoicesvol = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->GetVolume();
    oldInstrumentsVol = psengine->GetSoundManager()->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->GetVolume();

    pawsWidget::Show();
}

void pawsConfigSound::Hide()
{
    if(dirty)
    {
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->SetToggle(oldambient);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->SetToggle(oldactions);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->SetToggle(oldeffects);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->SetToggle(oldmusic);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->SetToggle(oldgui);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->SetToggle(oldvoices);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->SetToggle(oldInstruments);
        psengine->GetSoundManager()->SetChatToggle(oldchatsound);
        psengine->GetSoundManager()->SetListenerOnCameraToggle(oldlisteneroncamerapos);

        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MAIN_SNDCTRL)->SetVolume(oldvol);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->SetVolume(oldmusicvol);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->SetVolume(oldambientvol);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->SetVolume(oldactionsvol);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->SetVolume(oldeffectsvol);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->SetVolume(oldguivol);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->SetVolume(oldvoicesvol);
        psengine->GetSoundManager()->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->SetVolume(oldInstrumentsVol);
    }

    pawsWidget::Hide();
}

