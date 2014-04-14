/*
 * pawsmoney.h - Author: Ondrej Hurt
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


#ifndef PAWS_MONEY_HEADER
#define PAWS_MONEY_HEADER

#include <csutil/list.h>
#include <iutil/document.h>
#include "paws/pawswidget.h"
#include "net/subscriber.h"
#include "gui/inventorywindow.h"
#include "pawsslot.h"

/**
 * pawsMoney is widget that holds four items slots - for each coin.
 * It is connected to drag'n'drop system.
 */

class pawsMoney : public pawsWidget
{
public:
    pawsMoney();

    //from pawsWidget:
    virtual bool Setup(iDocumentNode * node);
    virtual bool PostSetup();

    /** Get and set amounts of different coins */
    void Set(int circles, int octas, int hexas, int trias);
    void Set(int coinType, int count);
    void Set(const psMoney & money);
    void Get(int & circles, int & octas, int & hexas, int & trias);
    int  Get(int coinType);

    /** Returns the amount of money in trias */
    int  GetAmount() { return amount; }

    psMoney Get();

    /** Tells if the amount of money is Zero (null) */
    bool IsNoAmount();

    void SetContainer( int containerID );
    void Drag( bool dragOn );

    void Draw();
    void OnUpdateData(const char *dataname,PAWSData& value);

protected:
    /** Loads pawsMoney widget content from file */
    bool CreateGUI();

    bool border;    ///< should the slot widgets have border ?
    int spacing;    ///< distance between slots

    int amount;     ///< The total amount of money in trias

    pawsSlot * circles;
    pawsSlot * octas;
    pawsSlot * hexas;
    pawsSlot * trias;

    /// Called when amount of a coin (hexa, octa ... ) is changed
    void RecalculateAmount();
};

CREATE_PAWS_FACTORY(pawsMoney);

#endif

