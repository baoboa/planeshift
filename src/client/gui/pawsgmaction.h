/*
* pawsgmaction.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : client window for editing clickable map object actions
*
*/
#ifndef PAWS_GMACTION_HEADER
#define PAWS_GMACTION_HEADER

#include "paws/pawswidget.h"

class pawsTextBox;
class pawsEditTextBox;
class pawsMultilineEditTextBox;
class pawsComboBox;
class pawsYesNoBox;
class pawsCheckBox;


class pawsGMActionWindow : public pawsWidget
{
public:
    pawsCheckBox*                   chkActive;
    pawsGMActionWindow();
    virtual ~pawsGMActionWindow();

    bool PostSetup();
    void Show();

    void LoadAction( const char *xml );
    void LoadAction( csRef<iDocumentNode> topNode );

    bool OnButtonPressed    ( int button, int keyModifier, pawsWidget* widget );
    bool OnChange            ( pawsWidget * widget );

private:
    pawsEditTextBox*                txtID;
    pawsEditTextBox*                txtName;
    pawsEditTextBox*                txtSector;
    pawsEditTextBox*                txtMesh;
    pawsCheckBox*                   chkFullMesh;
    pawsComboBox*                   dummy;
    pawsEditTextBox*                txtdummy;
    pawsEditTextBox*                txtPolygon;
    pawsEditTextBox*                txtPosX;
    pawsEditTextBox*                txtPosY;
    pawsEditTextBox*                txtPosZ;
    pawsEditTextBox*                txtRadius;
    pawsEditTextBox*                txtInstance;
    pawsEditTextBox*                txtMasterID;
    pawsComboBox*                   cboTriggerType;
    pawsComboBox*                   cboResponseType;
    pawsMultilineEditTextBox*       txtResponse;

    pawsYesNoBox*                    confirm;

    bool                            isDirty;
    csString                        id, name, masterid,
                                    sectorName, meshName, polygon,
                                    X, Y, Z, pos_instance, radius,
                                    triggertype, responsetype, response;

};


CREATE_PAWS_FACTORY( pawsGMActionWindow );
#endif
