//
// Database Abstraction Layer Interface definition
// Keith Fulton <keith@paqrat.com>
// 03/07/02
//


#ifndef __IDAL_H__
#define __IDAL_H__


#include <csutil/scf.h>
#include <csutil/scf_implementation.h>
#include <csutil/csstring.h>
#include "util/stringarray.h"

#define QUERY_FAILED 0xFFFFFFFF

// Forward decls
class iResultSet;
class iResultRow;
class iRecord;
class psDBProfiles;
class LogCSV;

struct iDataConnection : public virtual iBase
{
public:
    SCF_INTERFACE(iDataConnection, 0, 0, 1);

    /// Returns whether this object is actually connected to the database.
    virtual int IsValid(void)=0;

    /// Connects to the database and logs in.  Sets IsValid=true if successful.
    virtual bool Initialize(const char *host, unsigned int port, const char *database,
                            const char *user, const char *pwd, LogCSV* logcsv)=0;

    /// Disconnect from the database
    virtual bool Close()=0;

    /// Prepares string variables for use within query strings.
    virtual void Escape(csString& to, const char *from)=0;
    
    /**
     * Returns a pointer to a ResultSet if successful, or NULL if not.
     * All db accessor functions should store the query somehow to 
     * return it in GetLastQuery, and set an explanatory error 
     * string accessible by GetLastError if unsuccessful.
     */
    virtual iResultSet *Select(const char *sql,...)=0;

    /**
     * Special function for handling database selects which result in a single
     * value returned, since this behavior is so common.
     */
    virtual int SelectSingleNumber(const char *sql, ...)=0;

    /**
     * Function to insert, update or delete rows from the database.
     * Returns the number of rows affected by the command.  0 will usually
     * signify an error.
     */
    virtual unsigned long Command(const char *sql,...)=0;
    virtual unsigned long CommandPump(const char *sql,...) = 0;

    /**
     * This dynamically builds an insert sql statement
     * from the supplied table name, field name array,
     * and value array.  It runs and returns the LastInsertID
     * resulting from that insert.
     */
    virtual uint64 GenericInsertWithID(const char *table,const char **fieldnames,psStringArray& fieldvalues)=0;

    /**
     * This dynamically builds an update sql statement
     * from the supplied table name, field name array,
     * and value array.  It runs and returns True if
     * successful, false if not.
     */
    virtual bool GenericUpdateWithID(const char *table,const char *idfield,const char *id,const char **fieldnames,psStringArray& fieldvalues)=0;

    /**
     * Returns an explanatory error message for whatever error was
     * last encountered by a command or select, or NULL for a successful command.
     */
    virtual const char *GetLastError(void)=0;

    /**
     * Returns the string used in the last query or command issued.  Mostly
     * useful for debugging if an error is encountered.
     */
    virtual const char *GetLastQuery(void)=0;
    
    /**
     * Returns the last sequence number used in an Insert statement.
     */
    virtual uint64 GetLastInsertID()=0;

    /**
     * Convert a uint64 value to a string for printing or saving.
     */
    virtual const char *uint64tostring(uint64 value,csString& recv)=0;
    
    virtual const char* DumpProfile()=0;
    virtual void ResetProfile()=0;
    
    virtual iRecord* NewUpdatePreparedStatement(const char* table, const char* idfield, unsigned int count, const char* file, unsigned int line) =0;
    virtual iRecord* NewInsertPreparedStatement(const char* table, unsigned int count, const char* file, unsigned int line) = 0;
};


class iResultRow
{
public:
    /**
     * The iResultSet should call this to set the maximum number of fields
     * returned by the query, so the [] operator can do bounds checking.
     */
    virtual void SetMaxFields(int fields)=0;

    /**
     * The Row needs some accessor pointer in order to return the proper fields.
     * We don't know in the Interface what types that native db ptr will be, but
     * it can be set here.  This should be called by the iResultSet ctor.
     */
    virtual void SetResultSet(void * resultsettoken)=0;
    
    /**
     * This command should tell the row to populate itself with data from the
     * specified row in the result set.  The data should stay accessible there
     * until a new row is fetched or the dtor is called.  This function will be
     * called by the [] operator of the iResultSet most likely.
     */
    virtual int Fetch(int row)=0;

    /**
     * This will return a ptr to the value in the specified field, starting
     * with the zero-based index of the columns in the result set, or NULL
     * for an out of bounds value.
     */
    virtual const char *operator[](int whichfield)=0;
    
    /**
     * This should return a ptr to the value, like the previous [] operator
     * but use the names of the columns specified in the query.
     */
    virtual const char *operator[](const char *fieldname)=0;

    virtual const char* GetString(int whichfield)=0;
    virtual const char* GetString(const char *fieldname)=0;

    virtual int GetInt(int whichfield)=0;
    virtual int GetInt(const char *fieldname)=0;

    virtual unsigned long GetUInt32(int whichfield)=0;
    virtual unsigned long GetUInt32(const char *fieldname)=0;

    virtual float GetFloat(int whichfield)=0;
    virtual float GetFloat(const char *fieldname)=0;

    virtual uint64 GetUInt64(int whichfield)=0;
    virtual uint64 GetUInt64(const char *fieldname)=0;

    virtual uint64 stringtouint64(const char *stringbuf)=0;

    virtual ~iResultRow() {}
};

class iResultSet
{
public:
    /**
     * This function handles prefetching and caching of the specified
     * row in preparation for field access.  It is intended to be used
     * in concert with ResultRow::operator[] so that iResultSet[][] can
     * return values directly to the caller.
     */
    virtual iResultRow& operator[](unsigned long whichrow)=0;

    /**
     * Returns the number of rows in the result set, so that a for () loop
     * or other control structure can be used to access the data.
     */
    virtual unsigned long Count(void)=0;

    /**
     * Deletes itself.  This must be a member of the plugin class rather
     * than letting the caller delete it due to a Windows assertion error
     * if a different module frees memory from the one which allocated it.
     */
    virtual void Release(void)=0;

    virtual ~iResultSet() {}
};

// A class that encapsulates the facilities for prepared statements
class iRecord
{
public:
    
    virtual void AddField(const char* fname, float fValue)=0;
    virtual void AddField(const char* fname, int iValue)=0;
    virtual void AddField(const char* fname, unsigned int uiValue)=0;
    virtual void AddField(const char* fname, unsigned short usValue)=0;
    virtual void AddField(const char* fname, const char* sValue)=0;
    virtual void AddFieldNull(const char* fname)=0;

    virtual bool Execute(uint32 uid)=0;
    
    virtual void Reset()=0;
    
    virtual ~iRecord() {}
};

#endif

