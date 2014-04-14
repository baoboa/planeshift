/*
 * skillcache.h
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PS_SKILL_CACHE_H
#define PS_SKILL_CACHE_H

#include <csutil/list.h>
#include <csutil/csstring.h>

class MsgEntry;

/**
 * \addtogroup common_util
 * @{ */

/**
 * @class psSkillCacheItem
 *
 * psSkillCacheItem item is one stat or skill in the skill cache.
 */
class psSkillCacheItem {

    public:

        /**
         * Operations with this skill item:
         */
        enum Oper {
            UPDATE_OR_ADD, ///< The item is updated or added
            REMOVE         ///< The item should be removed from the list
        };

        /**
         * Constructs the cache item using values from another cache item.
         */
        psSkillCacheItem(psSkillCacheItem *item);

        /**
         * @brief Constructs an empty cache item where nameId and skillId are both set
         * to the nameId value.
         *
         * This constructor is used on the client side and is followed be the
         * read() function that reads the rest of values from the message entry.
         */
        psSkillCacheItem(unsigned int nameId);

        /**
         * @brief Constructs the cache item with the given values.
         *
         * This constructor is used on the server side.
         */
        psSkillCacheItem(int skillId,
                         unsigned int nameId,
                         unsigned short R,
                         unsigned short AS,
                         unsigned short Y,
                         unsigned short YC,
                         unsigned short Z,
                         unsigned short ZC,
                         unsigned short CAT,
                         bool stat = false);

        ~psSkillCacheItem();

        /**
          * The update() function updates the cache item and sets the modified state if
          * needed.
         */
        void update(unsigned short R,
                    unsigned short AS,
                    unsigned short Y,
                    unsigned short YC,
                    unsigned short Z,
                    unsigned short ZC);

        /**
         * Updates the cache item with values form another cache item
         */
        void update(psSkillCacheItem *item);

        /**
         * Calculates the number of bytes needed for the message entry
         */
        unsigned short size() const;

        /**
         * Writes the cache item to the message entry and changes
         * the modified state to false.
         */
        void write(MsgEntry *);

        /**
         * Reads the cache item from the message entry and sets the
         * modified state to true.
         */
        void read(MsgEntry *);

        bool isModified() const { return modified; }
        void setModified(bool modified) { this->modified = modified; }
        bool isRemoved() const { return removed; };
        void setRemoved(bool value) {removed = value; modified = true; }

        bool isStat() const { return stat; }
        int getSkillId() const { return skillId; }
        unsigned int getNameId() const { return nameId; }
        unsigned short getCategory() const { return category; }
        unsigned short getRank() const { return rank; }
        unsigned short getActualStat() const { return actualStat; }
        unsigned short getKnowledge() const { return knowledge; }
        unsigned short getKnowledgeCost() const { return knowledgeCost; }
        unsigned short getPractice() const { return practice; }
        unsigned short getPracticeCost() const { return practiceCost; }

    private:
        bool removed;
        bool modified;
        
        unsigned int nameId;
        int skillId;

        unsigned short rank;
        unsigned short actualStat;
        unsigned short knowledge;
        unsigned short practice;
        unsigned short knowledgeCost;
        unsigned short practiceCost;
        unsigned short category;
        bool stat;

};

/**
 * Skill cache iterator.
 */
typedef csList<psSkillCacheItem *>::Iterator psSkillCacheIter;

/**
 * @class psSkillCache
 *
 * The psSkillCache class implements the skill cache both on the server and on the client.
 */
class psSkillCache {

    public:
        psSkillCache();
        ~psSkillCache();

        /**
         * @brief Updates the cache.
         *
         * The received list can contain new skills, updates to the existing skills or
         * skills that should be removed from the cache.
         */
        void apply(psSkillCache *list);

        /**
         * @brief Adds the skill to the cache using the skillId value as the key.
         *
         * The skill item shall be allocated in the heap because the skill
         * cache will take ownership of the item and delete when necessary.
         */
        void addItem(int skillId, psSkillCacheItem *item);

        /**
         * @brief Searches for the skill item with the given ID value and returns a
         * pointer to it or NULL if not found.
         *
         * The returned skill item is owned by the cache.
         */
        psSkillCacheItem *getItemBySkillId(uint id);

        psSkillCacheIter iterBegin() { return psSkillCacheIter(skillCache); }

        /**
         * Clears the skill cache and deletes all the psSkillCacheItem objects
         * in it.
         */
        void clear();

        /**
         * Sets the modified state for the cache and all the items in the cache.
         */
        void setModified(bool modified);

        /**
         * Returns true if the cache or at least one of the items in the cache
         * is modified.
         */
        bool isModified();
        
        /**
         * Sets the removed state for the cache.
         */
        void setRemoved(bool removed);
        
        /**
         * Returns true if least one item has been removed from the cache.
         */
        bool hasRemoved();

        void setProgressionPoints(unsigned int points);
        unsigned int getProgressionPoints() const { return progressionPoints; }

        void setNew(bool value) { newList = value; }
        bool isNew() const { return newList; }

        /**
         * Calculates the number of bytes needed for the message entry.
         */
        unsigned short size() const;

        /**
         * Returns the number of modified items in the cache.
         */
        unsigned short count() const;

        /**
         * @brief Writes the cache and all the modified items to the message entry.
         *
         * Make sure that no items in the cache are modified between size() and
         * write() function calls.
         */
        void write(MsgEntry *);

        /**
         * Reads new values for the cache and for items in it from the
         * message entry.
         */
        void read(MsgEntry *);

        /**
         * Convert cache to string.
         */
        csString ToString() const;

    private:
        csList<psSkillCacheItem *> skillCache;
        bool modified;
        bool removed;
        unsigned int progressionPoints;
        bool newList;

        /**
         * Removes the item from the skill cache.
         */
        bool deleteItem(psSkillCacheItem *item);

};

/** @} */

#endif
