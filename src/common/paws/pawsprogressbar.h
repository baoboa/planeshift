/*
 * pawsprogressbar.h - Author: Andrew Craig
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
#ifndef PAWS_PROGRESS_BAR_HEADER
#define PAWS_PROGRESS_BAR_HEADER

#include "pawswidget.h"

/**
 * \addtogroup common_paws
 * @{ */

class pawsProgressBar : public pawsWidget
{
public:
    pawsProgressBar();
    pawsProgressBar(const pawsProgressBar &origin);
    ~pawsProgressBar();

    float GetTotalValue() const
    {
        return totalValue;
    }
    void SetTotalValue(float newValue)
    {
        totalValue = newValue;
    }
    void Completed()
    {
        complete = true;
    }
    void SetCurrentValue(float newValue);
    float GetCurrentValue()
    {
        return currentValue;
    }
    virtual void Draw();

    static void DrawProgressBar(const csRect &rect, iGraphics3D* graphics3D, float percent,
                                int start_r, int start_g, int start_b,
                                int diff_r,  int diff_g,  int diff_b,
                                int alpha = 255);

    bool IsDone()
    {
        return complete;
    }
    bool Setup(iDocumentNode* node);

    void OnUpdateData(const char* dataname,PAWSData &value);

private:
    float totalValue;
    float currentValue;
    float percent;
    bool  complete;

    int   start_r,start_g,start_b;
    int   diff_r,diff_g,diff_b;
};

CREATE_PAWS_FACTORY(pawsProgressBar);

/** @} */

#endif
