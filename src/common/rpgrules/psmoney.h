/*
 * psmoney.h by Anders Reggestad <andersr@pvv.org>
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
 *
 */
#ifndef PS_MONEY
#define PS_MONEY

//CS includes
#include <csutil/csstring.h>

// Value of coins in other coins
#define CIRCLES_VALUE_TRIAS 250
#define OCTAS_VALUE_TRIAS    50
#define HEXAS_VALUE_TRIAS    10

#define CIRCLES_VALUE_HEXAS  (CIRCLES_VALUE_TRIAS / HEXAS_VALUE_TRIAS)
#define CIRCLES_VALUE_OCTAS   (CIRCLES_VALUE_TRIAS / OCTAS_VALUE_TRIAS)
#define OCTAS_VALUE_HEXAS     (OCTAS_VALUE_TRIAS / HEXAS_VALUE_TRIAS)

typedef enum
{
    MONEY_TRIAS,
    MONEY_HEXAS,
    MONEY_OCTAS,
    MONEY_CIRCLES
} Money_Slots;


class psMoney 
{
public:
    psMoney();
    psMoney(int trias);
    psMoney(int circles, int octas, int hexas, int trias);

    /** Construct a psMoney based on a string 
     *  Format: "C,O,H,T"
     */
    psMoney(const char * moneyString);
    
   
    void Set(const char * moneyString);
    void Set(int type, int value);
    void Set(int circles, int octas, int hexas, int trias);

    /** Set the number of circles
     */
    void SetCircles(int c) { circles = c; }
    void AdjustCircles( int c );
    
    /** Get the number of circles
     * @return circles
     */
    int GetCircles() const { return circles; }

    /** Set the number of octas
     */
    void SetOctas(int o) { octas = o; }
    void AdjustOctas(int o);
    /** Get the number of octas
     * @return octas
     */
    int GetOctas() const { return octas; }

    /** Set the number of hexas
     */
    void SetHexas(int h) { hexas = h; }
    void AdjustHexas( int h );
    /** Get the number of hexas
     * @return hexas
     */
    int GetHexas() const { return hexas; }

    /** Set the number of trias
     */
    void SetTrias(int t) { trias = t; }   
    void AdjustTrias( int t );

    /** Get the number of trias
     * @return trias
     */
    int GetTrias() const { return trias; }   
   
    /** Get the total in trias
     * @return Trias
     */
    int GetTotal() const;

    /** Convert psMoney to a string
     * @return "C,O,H,T"
     */
    csString ToString() const;

    /** Convert psMoney to user-friendly string
     * @return "12 Circles, 3 Hexas and 14 Trias"
     */
    csString ToUserString() const;

    /** Normalize to have the total match the highest possible number of high value coins. E.g. 11 trias would normalized be 1 Hexa and 1 Tria.
     * @return The normalized money without modifying the original object.
     */
    psMoney Normalized() const;

    bool operator > (const psMoney& other) const;
    psMoney operator - (const psMoney& other) const;
    psMoney operator - (void) const;
    psMoney operator + (const psMoney& other) const;
    psMoney operator +=(const psMoney& other);
    psMoney operator * (const int mult) const;
    
    void Adjust( int type, int value );
    int Get( int type ) const;

	bool EnsureTrias(int minValue);
	bool EnsureHexas(int minValue);
	bool EnsureOctas(int minValue);
	bool EnsureCircles(int minValue);

protected:
    int circles;
    int octas;
    int hexas;
    int trias;
};


#endif
