/*
 * basemusicscore.h, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef BASE_MUSIC_SCORE_H
#define BASE_MUSIC_SCORE_H


//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================

//====================================================================================
// Local Includes
//====================================================================================
#include "scoreelements.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------

/**
 * Implements a musical score. Central object of this class is the cursor which is part
 * of BaseMusicalScore's interface to all intent and purposes. The score can be in edit
 * or in play mode. When in play mode, it is read-only.
 *
 * Though the content of the score can be read with BaseMusicalScore::GetMeasure(size_t),
 * using the cursor is the only way to modify it. After you set the score mode through
 * BaseMusicalScore::Set*Mode(), a reference of the score can be retrieved by using the
 * methods BaseMusicalScore::Get*Cursor().
 *
 * The read-only property of the score in play mode is enforced by exploiting the Cursor
 * class const-correctness. The drawback of this design is that you will not be able to
 * move the cursor by calling its methods in play mode. To circumvent this, work-around
 * methods are provided in BaseMusicalScore. You can use BaseMusicalScore::AdvanceCursor()
 * instead of BaseMusicalScore::Cursor::Advance() for example.
 */
class BaseMusicalScore
{

public:
    class Cursor;

    /**
     * Create an empty score in edit mode.
     */
    BaseMusicalScore();

    /**
     * Destructor.
     */
    ~BaseMusicalScore();

    /**
     * @copydoc BaseMusicalScore::Cursor::Advance()
     */
    bool AdvanceCursor(bool ignoreEndOfMeasure);

    /**
     * Return the cursor if the score is in edit mode.
     *
     * @retrn The cursor if the score is in edit mode, 0 otherwise.
     */
    Cursor* GetEditCursor();

    /**
     * Return the n-th measure in the score.
     *
     * @param n The index of the measure to retrieve. Must be valid.
     * @return The n-th measure in the score.
     */
    const Measure* GetMeasure(size_t n) const;

    /**
     * Return the number of measures in the score.
     *
     * @return The number of measures in the score.
     */
    size_t GetNMeasures() const { return measures.GetSize(); }

    /**
     * Return the cursor if the score is in play mode.
     *
     * @retrn The cursor if the score is in play mode, 0 otherwise.
     */
    const Cursor* GetPlayCursor() const;

    /**
     * Set the mode to edit. If the score is in play mode, the previous cursor is
     * invalidated and the new one starts from the beginning of the score.
     *
     * @return The cursor of the score.
     */
    Cursor* SetEditMode();

    /**
     * Set the mode to play. If the score is in edit mode, the previous cursor is
     * invalidated and the new one starts from the beginning of the score.
     *
     * @return The cursor of the score.
     */
    const Cursor* SetPlayMode();

private:
    enum ScoreMode
    {
        EDIT,
        PLAY
    };

    Cursor* cursor; ///< The cursor of this score.
    ScoreMode mode; ///< The mode of this cursor.

    /**
     * All the measures of this musical score. End-of-measure and end-of-score variables
     * are not actually represented. They are just a nice abstraction for the user.
     */
    csArray<Measure> measures;
};

//------------------------------------------------------------------------------------

/**
 * This is part of the BaseMusicalScore API. Only a BaseMusicalScore object can create an
 * instance of this object.
 *
 * In order to grasp how the cursor works, it is useful to understand that, similarly to
 * the end-of-file character in files, musical scores use a couple of special measure
 * elements. The end of a measure is marked by an end-of-measure element. Each measure in
 * the score contains at least that one. Moreover, the last element of the score is a
 * special end-of-score element. This is positioned after the last measure and it does not
 * belong to any of them. An empty measure has only the end-of-measure element. Similarly,
 * an empty score contains only the end-of-score element.
 *
 * Though the user is free to add and remove elements to an existing measure by using the
 * methods provided by the Measure class, it must be noticed that by doing so the cursor
 * is not updated in any way. This may cause undesired effects (e.g. the cursor may point
 * to a non-existent measure). For this reason it is safer to use the methods of this
 * class which clearly state their effect on the cursor. If you still want to use the
 * methods in Measure, be sure to call Cursor::Validate() after.
 */
class BaseMusicalScore::Cursor
{
public:
    /**
     * Destructor.
     */
    ~Cursor();

    /**
     * Move the cursor to the next element. This takes into account repeat sections only
     * if the score is in play mode. If the current element is the last one of the score,
     * false is returned and the cursor is moved to the end-of-score element. Any
     * subsequent call will not affect the cursor and return false.
     *
     * @param ignoreEndOfMeasure Skip end-of-measure elements if true.
     * @return False if the score is in edit mode and there is no element written after
     * the current one or if the score is in play mode and there is no element to play
     * after the current one, true otherwise.
     */
    bool Advance(bool ignoreEndOfMeasure);

    /**
     * Get the element currently pointed by the cursor.
     *
     * @return The current measure element or a null pointer in case the cursor is on a
     * end-of-measure or the end-of-score element.
     */
    MeasureElement* GetCurrentElement();

    /**
     * @copydoc BaseMusicalScore::Cursor::GetCurrentElement()
     */
    const MeasureElement* GetCurrentElement() const;

    /**
     * Get the measure currently pointed by the cursor.
     *
     * @return The current measure or a null pointer if the cursor is on a end-of-measure
     * or the end-of-score element.
     */
    Measure* GetCurrentMeasure();

    /**
     * @copydoc BaseMusicalScore::Cursor::GetCurrentMeasure()
     */
    const Measure* GetCurrentMeasure() const;

    /**
     * Check if the element after the current one is the end-of-score element. This takes
     * into account repeat sections only if the score is in play mode.
     *
     * @param ignoreEndOfMeasure If this is false, end-of-measure elements are considered
     * valid next elements, otherwise they are ignored.
     * @return True if the score is in edit mode and there is another note written on the
     * score after the current one or if the score is in play mode and there is another
     * note that must be played after the current one, false otherwise.
     */
    bool HasNext(bool ignoreEndOfMeasure) const;

    /**
     * Insert a copy of the given element before the current one. The cursor does not
     * change position. If the cursor points to the end-of-score element, a new measure
     * is pushed at the end of the score and the cursor is moved to the element just
     * inserted. If the cursor points to an end-of-measure element, the element is
     * inserted before it and the cursor is moved to the element just inserted.
     *
     * @param element The element to insert.
     */
    void InsertElementAfter(const MeasureElement &element);

    /**
     * Insert a copy of the given element before the current one. The cursor does not
     * change position. If the cursor point to the end-of-score element, a new measure
     * is pushed at the end of the score and the cursor is moved to the end-of-measure
     * element of the new measure.
     *
     * @param element The element to insert.
     */
    void InsertElementBefore(const MeasureElement &element);

    /**
     * Insert a copy of the given measure after the current one. The cursor does not
     * change position. If the cursor is on the end-of-score element, the measure is
     * pushed at the end of the score and the cursor is moved to the first element of the
     * new measure (which is end-of-measure if the given measure is empty).
     *
     * @param measure The measure to insert.
     */
    void InsertMeasureAfter(const Measure &measure);

    /**
     * Insert a copy of the given measure before the current one. The cursor does not
     * change position. If the cursor is on the end-of-score element, the measure is
     * pushed at the end of the score and the cursor is kept on end-of-score.
     *
     * @param measure The measure to insert.
     */
    void InsertMeasureBefore(const Measure &measure);

    /**
     * Check that the cursor is on an end-of-measure element.
     *
     * @return True if the cursor is on an end-of-measure element, false otherwise.
     */
    bool IsEndOfMeasure() const;

    /**
     * Check that the cursor is on the end-of-score element. Notice that this is different
     * than checking if the current element is the last one of the score. You can use
     * Cursor::HasNext()for that.
     *
     * @return True if the cursor is on an end-of-score element, false otherwise.
     */
    bool IsEndOfScore() const;

    /**
     * Check that the cursor is in a valid position which means that it is not on a
     * special element.
     *
     * @return True if the cursor is valid, false otherwise.
     */
    bool IsValid() const;

    /**
     * Remove the current element. The cursor is moved to the previous element within the
     * same measure. If the removed element is the first one of the measure, the cursor is
     * moved to the next element. If the cursor is on a end-of-measure or the end-of-score
     * element, nothing happens and false is returned.
     *
     * @return True if the element was removed, false if the cursor is on a end-of-measure
     * or the end-of-score element.
     */
    bool RemoveCurrentElement();

    /**
     * Remove the current measure. The cursor is moved to the first element of the previous
     * measure. If the removed measure is the first one of the score, the cursor is moved
     * to the first element of the next measure or to the end-of-score element in case the
     * removed measure was the only one in the score. If the cursor is already on the
     * end-of-score element, nothing happens and false is returned.
     *
     * @return True if the measure was removed, false if the cursor is on the end-of-score
     * element.
     */
    bool RemoveCurrentMeasure();

    /**
     * If the cursor is pointing to a non-existent element, this moves it to the
     * end-of-measure character in the same measure. You have no reason to use this method
     * unless you modify add/remove elements in a Measure by using its methods.
     */
    void Validate();

private:
    // In this way only BaseMusicalScore can access the private constructor
    friend BaseMusicalScore::BaseMusicalScore();
    friend Cursor* BaseMusicalScore::SetEditMode();
    friend const Cursor* BaseMusicalScore::SetPlayMode();

    size_t currElementIdx; ///< Index of the current element.
    size_t currMeasureIdx; ///< Index of the current measure.
    ScoreContext* context; ///< The context is valid only if it is in play mode.
    BaseMusicalScore* score; ///< The musical score this cursor refers to.

    /**
     * Create a cursor pointing to the first element of the score. Only the score can
     * create a cursor.
     *
     * @param score The musical score this cursor refers to.
     * @param mode If the mode is play, a context is created and mantained.
     */
    Cursor(BaseMusicalScore* score, BaseMusicalScore::ScoreMode mode);

    /**
     * Check if an eventual repeat section must be repeated after this measure. The
     * context and the measure index must be valid. This make sense only in play mode.
     *
     * @return True if a repeat must be performed, false otherwise.
     */
    bool CheckRepeat() const;

    /**
     * Check if the cursor is placed on the last written element of the score. This does
     * not take into account repeat sections. The cursor cannot be on the end-of-score
     * element.
     *
     * @param ignoreEndOfMeasure If this is false, end-of-measure elements are considered
     * valid next elements, otherwise they are ignored.
     * @return False if the current element is the last one in the score, true otherwise.
     */
    bool HasNextWritten(bool ignoreEndOfMeasure) const;

    /**
     * Move the cursor to the first element of the score.
     */
    void Reset();
};

#endif // BASE_MUSIC_SCORE_H
