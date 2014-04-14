/*
 * pawscharbirth.h - Author: Andrew Dai
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
#ifndef PAWS_CHAR_BIRTH_HEADER
#define PAWS_CHAR_BIRTH_HEADER
 
#include "paws/pawswidget.h"
#include "psclientchar.h"

class pawsTextBox;

/// Struct for the zodiacs
struct Zodiac
{
    csString name;
    csString img;
    csString desc;
    unsigned int month;
};

class pawsCharBirth : public pawsWidget
{
public:
    pawsCharBirth();
    ~pawsCharBirth(); 
    pawsCharBirth(const pawsCharBirth& origin){}
    void OnListAction( pawsListBox* widget, int status );
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    bool PostSetup();
    void Show();
    void Draw();
    void Randomize();

    Zodiac* GetZodiac(const char* name);
    Zodiac* GetZodiac(unsigned int month);

private:
    void PopulateFields();

    int lastSiblingsChoice;
    int lastZodiacChoice;
    int sibCount;
    bool dataLoaded;

    // Contains the zodiacs
    csPDelArray<Zodiac> zodiacs;

    psCreationManager* createManager;
    pawsTextBox* cpBox;
    void UpdateCP();

    pawsComboBox* months;
    pawsComboBox* days;
};

CREATE_PAWS_FACTORY( pawsCharBirth ); 
 
#endif

