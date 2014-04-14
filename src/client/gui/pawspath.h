/*
 * pawspath.h - Author: Andrew Craig
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
#ifndef PAWS_PATH_WINDOW_HEADER
#define PAWS_PATH_WINDOW_HEADER
 
#include "paws/pawswidget.h"
#include "psclientchar.h"
#include "pawscharcreatemain.h"
 
class pawsPathWindow : public pawsWidget
{
public:
    pawsPathWindow();
    ~pawsPathWindow(); 
    bool OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget );
    bool PostSetup();

private:
    int chosenPath;

    psCreationManager* createManager;
    pawsCreationMain* charCreateMain;
    void SetPath(int i);
    void ClearPath(void);
    /** Sets the label headers visibility in this window.
     *  @param visible TRUE if you wish to show the label headers.
     */
    void LabelHeaderVisibility(bool visible);
};

CREATE_PAWS_FACTORY( pawsPathWindow ); 
 
#endif

