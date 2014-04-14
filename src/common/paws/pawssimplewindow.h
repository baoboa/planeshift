/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_SIMPLE_WINDOW_H
#define PAWS_SIMPLE_WINDOW_H

#include "pawswidget.h"

/**
 * \addtogroup common_paws
 * @{ */

/**
 * This is meant as a blank window that can be used for windows that are completely
 * specified through data (some combination of scripts and pub/subs).
 */
class pawsSimpleWindow : public pawsWidget
{
public:
    pawsSimpleWindow();
};

CREATE_PAWS_FACTORY(pawsSimpleWindow);

/** @} */

#endif // PAWS_SIMPLE_WINDOW_H

