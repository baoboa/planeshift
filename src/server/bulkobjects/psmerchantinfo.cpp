/*
 * psmerchantinfo.h
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/psdatabase.h"

#include "../psserver.h"
#include "../cachemanager.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psmerchantinfo.h"


bool psMerchantInfo::Load(PID pid)
{
    bool is_merchant = false;

    Result merchant_categories(db->Select("SELECT * from merchant_item_categories where player_id=%u", pid.Unbox()));
    if(merchant_categories.IsValid())
    {
        int i;
        int count=merchant_categories.Count();

        for(i=0; i<count; i++)
        {
            psItemCategory* category = FindCategory(atoi(merchant_categories[i]["category_id"]));
            if(!category)
            {
                Error1("Error! Category could not be loaded. Skipping.\n");
                continue;
            }
            categories.Push(category);
            is_merchant = true;
        }
    }

    return is_merchant;
}

psItemCategory* psMerchantInfo::FindCategory(int id)
{
    return psserver->GetCacheManager()->GetItemCategoryByID(id);
}

psItemCategory* psMerchantInfo::FindCategory(const csString &name)
{
    return psserver->GetCacheManager()->GetItemCategoryByName(name);
}
