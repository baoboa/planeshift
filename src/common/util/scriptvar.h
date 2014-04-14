/*
 * mathscript.h by Keith Fulton <keith@planeshift.it>
 *
 * Copyright (C) 2004 PlaneShift Team (info@planeshift.it,
 * http://www.planeshift.it)
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
#ifndef __MATHSCRIPTVAR_H__
#define __MATHSCRIPTVAR_H__

class MathEnvironment;

/**
 * \addtogroup common_util
 * @{ */

class iScriptableVar
{
public:
    virtual double GetProperty(MathEnvironment*, const char *ptr)=0;
    virtual double CalcFunction(MathEnvironment*, const char * functionName, const double * params) = 0;
    virtual const char* ToString() = 0;
    virtual ~iScriptableVar() {};
};

/** @} */

#endif

