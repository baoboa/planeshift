/*
 * pawsconfigspellchecker.cpp.cpp - Author: Fabian Stock (AiwendilH@googlemail.com)
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//CS INCLUDES
#include <psconfig.h>
#include <iutil/plugin.h>

//PAWS INCLUDES
#include "pawsconfigspellchecker.h"
#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"


//CLIENT INCLUDES
#include "../globals.h"
#include "gui/chatwindow.h"

pawsConfigSpellChecker::pawsConfigSpellChecker()
{
    personalDictBox = NULL;
    enabled = NULL;
    colorR = NULL;
    colorG = NULL;
    colorB = NULL;
}

bool pawsConfigSpellChecker::PostSetup()
{
    // setup the widget. Needed in the xml file is one MultiLine Edit Box for the words,a checkbox to enable/disable the spellchecker
    // and three EditTextBoxes for the color components of Typos
    chatWindow = dynamic_cast<pawsChatWindow*>(PawsManager::GetSingleton().FindWidget("ChatWindow"));
    if(!chatWindow)
    {
        Error1("Couldn't find ChatWindow!");
        return false;
    }
    personalDictBox = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget("PersonalDictionaryBox"));
    if (!personalDictBox) {
           Error1("Could not locate PersonalDictionaryBox widget!");
           return false;
    }
    enabled = dynamic_cast<pawsCheckBox*>(FindWidget("enabled"));
    if (!enabled) {
           Error1("Could not locate enabled widget!");
           return false;
    }
    colorR = dynamic_cast<pawsEditTextBox*>(FindWidget("typoColorR"));
    if (!colorR) {
           Error1("Could not locate typoColorR widget!");
           return false;
    }
    colorG = dynamic_cast<pawsEditTextBox*>(FindWidget("typoColorG"));
    if (!colorG) {
           Error1("Could not locate typoColorG widget!");
           return false;
    }
    colorB = dynamic_cast<pawsEditTextBox*>(FindWidget("typoColorB"));
    if (!colorB) {
           Error1("Could not locate typoColorB widget!");
           return false;
    }

    csRef<iSpellChecker> spellChecker = csQueryRegistryOrLoad<iSpellChecker>(PawsManager::GetSingleton().GetObjectRegistry(), "crystalspace.planeshift.spellchecker");

    if (spellChecker)
    {
        // if no dictionaires are loaded hide the config widgets and show an error message
        if (!spellChecker->hasDicts())
        {
            personalDictBox->Hide();
            enabled->Hide();
            colorR->Hide();
            colorG->Hide();
            colorB->Hide();
            pawsTextBox* personalDictionaryLabel = 0;
            personalDictionaryLabel =  dynamic_cast<pawsTextBox*>(FindWidget("PersonalDictionaryLabel"));
            if (personalDictionaryLabel) personalDictionaryLabel->Hide();
            pawsTextBox* typoColorLabel = 0;
            typoColorLabel =  dynamic_cast<pawsTextBox*>(FindWidget("typoColorLabel"));
            if (typoColorLabel) typoColorLabel->Hide();
            pawsTextBox* errorLabel1 = 0;
            errorLabel1 = dynamic_cast<pawsTextBox*>(FindWidget("ErrorLabel1"));
            if (errorLabel1) errorLabel1->Show();
            pawsTextBox* errorLabel2 = 0;
            errorLabel2 = dynamic_cast<pawsTextBox*>(FindWidget("ErrorLabel2"));
            if (errorLabel2) errorLabel2->Show();
            pawsTextBox* errorLabel3 = 0;
            errorLabel3= dynamic_cast<pawsTextBox*>(FindWidget("ErrorLabel3"));
            if (errorLabel3) errorLabel3->Show();
        }
    }
    else
    {
        // show an error message if the spellchecker plugin is not available
        personalDictBox->Hide();
        enabled->Hide();
        colorR->Hide();
        colorG->Hide();
        colorB->Hide();
        pawsTextBox* personalDictionaryLabel = 0;
        personalDictionaryLabel =  dynamic_cast<pawsTextBox*>(FindWidget("PersonalDictionaryLabel"));
        if (personalDictionaryLabel) personalDictionaryLabel->Hide();
        pawsTextBox* typoColorLabel = 0;
        typoColorLabel =  dynamic_cast<pawsTextBox*>(FindWidget("typoColorLabel"));
        if (typoColorLabel) typoColorLabel->Hide();
        pawsTextBox* errorLabel1 = 0;
        errorLabel1 = dynamic_cast<pawsTextBox*>(FindWidget("ErrorLabel1"));
        if (errorLabel1) errorLabel1->Show();
        pawsTextBox* errorLabel2 = 0;
        errorLabel2 = dynamic_cast<pawsTextBox*>(FindWidget("ErrorLabel2"));
        if (errorLabel2) errorLabel2->Show();
        pawsTextBox* errorLabel3 = 0;
        errorLabel3= dynamic_cast<pawsTextBox*>(FindWidget("ErrorLabel3"));
        if (errorLabel3) errorLabel3->Show();
        if (errorLabel1) errorLabel1->SetText("Sorry, this version of PlaneShift was compiled without");
        if (errorLabel2) errorLabel2->SetText("support for the spell checker.");
        if (errorLabel3) errorLabel3->SetText("");
    }

    return true;
}

bool pawsConfigSpellChecker::Initialize()
{
    if ( ! LoadFromFile("configspellchecker.xml"))
    {
        return false;
    }
    return true;
}

bool pawsConfigSpellChecker::LoadConfig()
{
    // set the checkbox according to the current state of the spellchecker
    enabled->SetState(chatWindow->getInputTextBox()->getSpellChecked());
    // set the color of typos
    csString r, g, b;
    int ir, ig, ib;

    //PawsManager::GetSingleton().GetGraphics2D()->
    graphics2D->GetRGB(chatWindow->getInputTextBox()->getTypoColour(), ir, ig, ib);
    r.Format("%d", ir);
    g.Format("%d", ig);
    b.Format("%d", ib);

    colorR->SetText(r);
    colorG->SetText(g);
    colorB->SetText(b);

    csRef<iSpellChecker> spellChecker = csQueryRegistryOrLoad<iSpellChecker>(PawsManager::GetSingleton().GetObjectRegistry(), "crystalspace.planeshift.spellchecker");

    if (spellChecker)
    {
        //fill the text box with the words by putting them all in a single string separated by newlines
        csString words("");
        for (size_t i = 0; i < spellChecker->getPersonalDict().GetSize(); i++)
        {
            words.Append(spellChecker->getPersonalDict()[i]).Append("\n");
        }
        personalDictBox->SetText(words);
    }
    dirty = false;
    return true;
}

bool pawsConfigSpellChecker::SaveConfig()
{
    // save if the spellchecker is enabled or not
    chatWindow->getInputTextBox()->setSpellChecked(enabled->GetState());

    // save the color for typos
    chatWindow->getInputTextBox()->setTypoColour(graphics2D->FindRGB(atoi(colorR->GetText()), atoi(colorG->GetText()), atoi(colorB->GetText())));

    csRef<iSpellChecker> spellChecker = csQueryRegistryOrLoad<iSpellChecker>(PawsManager::GetSingleton().GetObjectRegistry(), "crystalspace.planeshift.spellchecker");

    if (spellChecker)
    {
        // remove die previously defined words from the list and hunspell
        spellChecker->clearPersonalDict();

        // then refill it with the values from the text box
        // first we remove all unnecessary spaces
        csString words = personalDictBox->GetText();
        words.Collapse();
        // then we exchange the remaining spaces with newlines as we need single words
        words.FindReplace(" ", "\n");
        // last but not least we splitting the content of the textbox in single strings again
        csString tmpWord;
        size_t oldNewLine = 0;
        size_t foundNewLine = words.Find("\n", oldNewLine);
        while (foundNewLine != (size_t)-1)
        {
            words.SubString(tmpWord, oldNewLine, foundNewLine-oldNewLine);
            tmpWord.Collapse();
            if((!tmpWord.IsEmpty()) && (!WordExists(spellChecker, tmpWord)))
            {
                spellChecker->addWord(tmpWord);
            }
            oldNewLine = foundNewLine;
            foundNewLine = words.Find("\n", oldNewLine+1);
        }
        words.SubString(tmpWord, oldNewLine, words.Length()-oldNewLine);
        tmpWord.Collapse();
        if((!tmpWord.IsEmpty()) && (!WordExists(spellChecker, tmpWord)))
        {
            spellChecker->addWord(tmpWord);
        }
    }

    // Save to file
    chatWindow->SaveChatSettings();
    // and reload the config again so that difference made by the save function are displayed (for example the removed special chars from the words)
    LoadConfig();
    return true;
}

bool pawsConfigSpellChecker::WordExists(csRef<iSpellChecker> spellChecker, csString word)
{
    if (spellChecker)
    {
        for (size_t i = 0; i < spellChecker->getPersonalDict().GetSize(); i++)
        {
            if (word == spellChecker->getPersonalDict()[i])
            {
                return true;
            }
        }
    }
    return false;
}

void pawsConfigSpellChecker::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_CHAT_FILE_NAME);
    LoadConfig();
}
