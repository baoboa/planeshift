/*
 * dal.cpp by Stefano Angeleri
 * Database Abstraction Layer (sqlite3)
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/stringarray.h>

#include "util/log.h"
#include "util/consoleout.h"

#include "dal.h"

//not implemented
#ifdef USE_DELAY_QUERY
#undef USE_DELAY_QUERY
#endif

// SCF definitions
CS_PLUGIN_NAMESPACE_BEGIN(dbsqlite3)
{
    SCF_IMPLEMENT_FACTORY(psMysqlConnection)

    /**
     * Actual class which does all the work now.
     */

    psMysqlConnection::psMysqlConnection(iBase *iParent) : scfImplementationType(this, iParent)
    {
        conn = NULL;
    }

    psMysqlConnection::~psMysqlConnection()
    {
        Close();
    }

    bool psMysqlConnection::Initialize (iObjectRegistry *objectreg)
    {
        objectReg = objectreg;

        pslog::Initialize (objectreg);

        return true;
    }

    bool psMysqlConnection::Initialize(const char* /*host*/, unsigned int /*port*/, const char* database,
                                  const char* /*user*/, const char* /*pwd*/, LogCSV* logcsv)
    {
        this->logcsv = logcsv;
        // Create a mydb
        if(sqlite3_open(database, &conn) != SQLITE_OK)
            return false;

    #ifdef USE_DELAY_QUERY
        dqm.AttachNew(new DelayedQueryManager(host, port, database, user, pwd));
        dqmThread.AttachNew(new Thread(dqm));
        dqmThread->Start();
        dqmThread->SetPriority(THREAD_PRIO_HIGH);
    #endif

        return true;
    }

    bool psMysqlConnection::Close()
    {
        //waits for sqlite to complete and close.
        if(conn)
        {
            while(sqlite3_close(conn) != SQLITE_OK);
            conn = NULL;
        }
        return true;
    }


    int psMysqlConnection::IsValid()
    {
        return (conn) ? 1 : 0;
    }

    const char *psMysqlConnection::GetLastError()
    {
        return sqlite3_errmsg(conn);
    }

    // Sets *to to the escaped value
    void psMysqlConnection::Escape(csString& to, const char *from)
    {
        char *buff = sqlite3_mprintf("%q", from);
        to = buff;
        sqlite3_free(buff);
    }

    unsigned long psMysqlConnection::CommandPump(const char *sql,...)
    {
        
    #ifdef USE_DELAY_QUERY
        csString querystr;
        va_list args;

        va_start(args, sql);
        querystr.FormatV(sql, args);
        va_end(args);
        dqm->Push(querystr);

        return 1;
    #else
        psStopWatch timer;
        csString querystr;
        va_list args;

        va_start(args, sql);
        querystr.FormatV(sql, args);
        va_end(args);

        lastquery = querystr;


        timer.Start();
        if(!sqlite3_exec(conn, querystr, NULL, NULL, NULL))
        {
            if(timer.Stop() > 1000)
            {
                csString status;
                status.Format("SQL query %s, has taken %u time to process.\n", querystr.GetData(), timer.Stop());
                if(logcsv)
                    logcsv->Write(CSV_STATUS, status);
            }
            //csString status;
            //status.Format("%s, %d", querystr.GetData(), timer.Stop());
            //logcsv->Write(CSV_SQL, status);
            profs.AddSQLTime(querystr, timer.Stop());
            return (unsigned long) sqlite3_changes(conn);
        }
        else
            return QUERY_FAILED;

    #endif
    }

    unsigned long psMysqlConnection::Command(const char *sql,...)
    {
        psStopWatch timer;
        csString querystr;
        va_list args;

        va_start(args, sql);
        querystr.FormatV(sql, args);
        va_end(args);

        lastquery = querystr;

        timer.Start();
        if(!sqlite3_exec(conn, querystr, NULL, NULL, NULL))
        {
            if(timer.Stop() > 1000)
            {
                csString status;
                status.Format("SQL query %s, has taken %u time to process.\n", querystr.GetData(), timer.Stop());
                if(logcsv)
                    logcsv->Write(CSV_STATUS, status);
            }
            profs.AddSQLTime(querystr, timer.Stop());
            return (unsigned long) sqlite3_changes(conn);
        }
        else
            return QUERY_FAILED;
    }

    iResultSet *psMysqlConnection::Select(const char *sql, ...)
    {
        psStopWatch timer;
        csString querystr;
        va_list args;

        va_start(args, sql);
        querystr.FormatV(sql, args);
        va_end(args);

        lastquery = querystr;

        timer.Start();
        int rows = 0;
        int columns = 0;
        char **result;

        if(!sqlite3_get_table(conn, querystr.GetData(), &result, &rows, &columns, NULL))
        {
            if(timer.Stop() > 1000)
            {
                csString status;
                status.Format("SQL query %s, has taken %u time to process.\n", querystr.GetData(), timer.Stop());
                if(logcsv)
                    logcsv->Write(CSV_STATUS, status);
            }
            profs.AddSQLTime(querystr, timer.Stop());
            iResultSet *rs = new psResultSet(result, rows, columns);
            return rs;
        }
        else
            return NULL;
    }

    int psMysqlConnection::SelectSingleNumber(const char *sql, ...)
    {
        psStopWatch timer;
        csString querystr;
        va_list args;

        va_start(args, sql);
        querystr.FormatV(sql, args);
        va_end(args);

        lastquery = querystr;

        timer.Start();

        int rows = 0;
        int columns = 0;
        char **result;

        if(!sqlite3_get_table(conn, querystr, &result, &rows, &columns, NULL))
        {
            if(timer.Stop() > 1000)
            {
                csString status;
                status.Format("SQL query %s, has taken %u time to process.\n", querystr.GetData(), timer.Stop());
                if(logcsv)
                    logcsv->Write(CSV_STATUS, status);
            }
            profs.AddSQLTime(querystr, timer.Stop());
            iResultSet *rs = new psResultSet(result, rows, columns);

            if (rs->Count())
            {
                const char *value = (*rs)[0][0];
                int result = atoi( value?value:"0" );
                delete rs;
                return result;
            }
            else
            {
                delete rs;  // return err code below
            }
        }
        return QUERY_FAILED;
    }

    uint64 psMysqlConnection::GetLastInsertID()
    {
        return sqlite3_last_insert_rowid(conn);
    }

    uint64 psMysqlConnection::GenericInsertWithID(const char *table,const char **fieldnames,psStringArray& fieldvalues)
    {
        csString command;
        const size_t count = fieldvalues.GetSize();
        uint i;
        command = "INSERT INTO ";
        command.Append(table);
        command.Append(" (");
        for (i=0;i<count;i++)
        {
            if (i>0)
                command.Append(",");
            command.Append(fieldnames[i]);
        }

        command.Append(") VALUES (");
        for (i=0;i<count;i++)
        {
            if (i>0)
                command.Append(",");
            if (fieldvalues[i]!=NULL)
            {
                command.Append("'");
                csString escape;
                Escape( escape, fieldvalues[i] );
                command.Append(escape);
                command.Append("'");
            }
            else
            {
                command.Append("NULL");
            }
        }
        command.Append(")");

        if (Command("%s", command.GetDataSafe())!=1)
            return 0;

        return GetLastInsertID();
    }

    bool psMysqlConnection::GenericUpdateWithID(const char *table,const char *idfield,const char *id,psStringArray& fields)
    {
        uint i;
        const size_t count = fields.GetSize();
        csString command;

        command.Append("UPDATE ");
        command.Append(table);
        command.Append(" SET ");

        for (i=0;i<count;i++)
        {
            if (i>0)
                command.Append(",");
            command.Append(fields[i]);
        }

        command.Append(" where ");
        command.Append(idfield);
        command.Append("='");
        csString escape;
        Escape( escape, id );
        command.Append(escape);
        command.Append("'");

        if (CommandPump("%s", command.GetDataSafe())==QUERY_FAILED)
        {
            return false;
        }

        return true;
    }

    bool psMysqlConnection::GenericUpdateWithID(const char *table,const char *idfield,const char *id,const char **fieldnames,psStringArray& fieldvalues)
    {
        uint i;
        const size_t count = fieldvalues.GetSize();
        csString command;

        command.Append("UPDATE ");
        command.Append(table);
        command.Append(" SET ");

        for (i=0;i<count;i++)
        {
            if (i>0)
                command.Append(",");
            command.Append(fieldnames[i]);
            if (fieldvalues[i]!=NULL)
            {
                command.Append("='");
                csString escape;
                Escape(escape, fieldvalues[i]);
                command.Append(escape);
                command.Append("'");
            }
            else
            {
                command.Append("=NULL");
            }

        }

        command.Append(" where ");
        command.Append(idfield);
        command.Append("='");
        csString escape;
        Escape( escape, id );
        command.Append(escape);
        command.Append("'");

        if (CommandPump("%s", command.GetDataSafe())==QUERY_FAILED)
        {
            return false;
        }

        return true;
    }

    const char *psMysqlConnection::uint64tostring(uint64 value,csString& recv)
    {
        recv.Clear();

        while (value>0)
        {
            recv.Insert(0,(char)'0'+(value % 10) );
            value/=10;
        }
        return recv;
    }

    const char* psMysqlConnection::DumpProfile()
    {
        profileDump = profs.Dump();
        return profileDump;
    }

    void psMysqlConnection::ResetProfile()
    {
        profs.Reset();
        profileDump.Empty();
    }

    iRecord* psMysqlConnection::NewUpdatePreparedStatement(const char* table, const char* idfield, unsigned int count, const char* file, unsigned int line)
    {
        return new dbUpdate(conn, table, idfield, count, logcsv, file, line);
    }

    iRecord* psMysqlConnection::NewInsertPreparedStatement(const char* table, unsigned int count, const char* file, unsigned int line)
    {
        return new dbInsert(conn, table, count, logcsv, file, line);
    }

    psResultSet::psResultSet(char **result, int rowNum, int columns)
    {
        rs = result;
        if(rs)
        {
            rows    = rowNum;
            fields  = columns;
            row.SetMaxFields(columns);
            row.SetResultSet(rs);

            current = (unsigned long) -1;
        }
        else
            rows = 0;
    }

    psResultSet::~psResultSet()
    {
        sqlite3_free_table(rs);
    }

    iResultRow& psResultSet::operator[](unsigned long whichrow)
    {
        if (whichrow != current)
        {
            if(whichrow <= rows)
            {
                row.Fetch(whichrow);
                current = whichrow;
                row.SetMaxFields(fields);
            }
            else
                row.SetMaxFields(0); // no fields will make operator[]'s safe
        }
        
        return row;
    }

    const char* psResultRow::GetString(int whichfield)
    {
        return this->operator [](whichfield);
    }

    const char* psResultRow::GetString(const char *fieldname)
    {
        return this->operator [](fieldname);
    }

    void psResultRow::SetResultSet(void * resultsettoken)
    {
        rs = (char **)resultsettoken;
    }

    int psResultRow::Fetch(int row)
    {
        rowNum = row;
        return 0;
    }

    const char *psResultRow::operator[](int whichfield)
    {
        if (whichfield >= 0 && whichfield < max)
            return rs[max+(rowNum*max)+whichfield];
        else
            return "";
    }

    const char *psResultRow::operator[](const char *fieldname)
    {
        CS_ASSERT(fieldname);
        CS_ASSERT(max); // trying to access when no fields returned in row! probably empty resultset.

        for (int i=0; i <= max; i++)
        {
            if (rs[i] && !strcasecmp(rs[i],fieldname))
            {
                return rs[max+(rowNum*max)+i];
            }
        }

        CPrintf(CON_BUG, "Could not find field %s!. Exiting.\n",fieldname);
        CS_ASSERT(false);
        return ""; // Illegal name.
    }

    int psResultRow::GetInt(int whichfield)
    {
        const char *ptr = this->operator [](whichfield);
        return (ptr)?atoi(ptr):0;
    }

    int psResultRow::GetInt(const char *fieldname)
    {
        const char *ptr = this->operator [](fieldname);
        return (ptr)?atoi(ptr):0;
    }

    unsigned long psResultRow::GetUInt32(int whichfield)
    {
        const char *ptr = this->operator [](whichfield);
        return (ptr)?strtoul(ptr,NULL,10):0;
    }

    unsigned long psResultRow::GetUInt32(const char *fieldname)
    {
        const char *ptr = this->operator [](fieldname);
        return (ptr)?strtoul(ptr,NULL,10):0;
    }

    float psResultRow::GetFloat(int whichfield)
    {
        const char *ptr = this->operator [](whichfield);
        return (ptr)?atof(ptr):0;
    }

    float psResultRow::GetFloat(const char *fieldname)
    {
        const char *ptr = this->operator [](fieldname);
        return (ptr)?atof(ptr):0;
    }

    uint64 psResultRow::GetUInt64(int whichfield)
    {
        const char *ptr = this->operator [](whichfield);
        return (ptr)?stringtouint64(ptr):0;
    }

    uint64 psResultRow::GetUInt64(const char *fieldname)
    {
        const char *ptr = this->operator [](fieldname);
        return (ptr)?stringtouint64(ptr):0;
    }

    uint64 psResultRow::stringtouint64(const char *stringbuf)
    {
        /* Our goal is to handle a conversion from a string of base-10 digits into most any numeric format.
         *  This has very specific use and so we presume a few things here:
         *
         *  The value in the string can actually fit into the type being used
         *  This function can be slow compared to the smaller ato* and strto* counterparts.
         */
        uint64 result=0;
        CS_ASSERT(stringbuf!=NULL);
        while (*stringbuf!=0x00)
        {
            CS_ASSERT(*stringbuf >='0' && *stringbuf<='9');
            result=result * 10;
            result = result +  (uint64)(*stringbuf - (char)'0');
            stringbuf++;
        }

        return result;
    }

    void dbRecord::AddField(const char* fname, float fValue)
    {
        AddToStatement(fname);
        temp[index].fValue = fValue;
        temp[index].type = SQL_TYPE_FLOAT;
        index++;
    }

    void dbRecord::AddField(const char* fname, int iValue)
    {
        AddToStatement(fname);
        temp[index].iValue = iValue;
        temp[index].type = SQL_TYPE_INT;
        index++;
    }

    void dbRecord::AddField(const char* fname, unsigned int uiValue)
    {
        AddToStatement(fname);
        temp[index].iValue = uiValue;
        temp[index].type = SQL_TYPE_INT;
        index++;
    }

    void dbRecord::AddField(const char* fname, unsigned short usValue)
    {
        AddToStatement(fname);
        temp[index].iValue = usValue;
        temp[index].type = SQL_TYPE_INT;
        index++;
    }

    void dbRecord::AddField(const char* fname, const char* sValue)
    {
        char *buff = sqlite3_mprintf("%q", sValue);
        AddToStatement(fname);
        temp[index].sValue = buff;
        temp[index].type = SQL_TYPE_STRING;
        index++;
        sqlite3_free(buff);
    }

    void dbRecord::AddFieldNull(const char* fname)
    {
        AddToStatement(fname);
        temp[index].type = SQL_TYPE_NULL;
        index++;
    }

    bool dbRecord::Execute(uint32 uid)
    {
        SetID(uid);
        CS_ASSERT_MSG("Error: wrong number of expected fields", index == count);

        if(!prepared)
            Prepare();

        psStopWatch timer;
        timer.Start();

        CS_ASSERT(count == (unsigned int)sqlite3_bind_parameter_count(stmt));
        CS_ASSERT(count != index);

        for(unsigned int i = 0; i < index; i++)
        {
            switch(temp[i].type)
            {
                case SQL_TYPE_FLOAT:
                    sqlite3_bind_double(stmt,i+1,temp[i].fValue);
                    break;
                case SQL_TYPE_INT:
                    sqlite3_bind_int(stmt, i+1,temp[i].iValue);
                    break;
                case SQL_TYPE_STRING:
                    sqlite3_bind_text(stmt, i+1,temp[i].sValue,-1,SQLITE_STATIC);
                    break;
                case SQL_TYPE_NULL:
                    sqlite3_bind_null(stmt,i+1);
                    break;
            }

        }

        bool result = (sqlite3_step(stmt) == SQLITE_DONE);
        if(result && timer.Stop() > 1000)
        {
            csString status;
            status.Format("SQL query in file %s line %d, has taken %u time to process.\n", file, line, timer.Stop());
            if(logcsv)
                logcsv->Write(CSV_STATUS, status);
        }
        return result;
    }

    bool dbInsert::Prepare()
    {
        csString statement;

        // count fields to update
        statement.Format("INSERT INTO %s (", table);
        for (unsigned int i=0;i<count;i++)
        {
            if (i>0)
                statement.Append(", ");
            statement.Append(command[i]);
        }

        statement.Append(") VALUES (");
        for (unsigned int i=0;i<count;i++)
        {
            if (i>0)
                statement.Append(", ");
            statement.Append("?");
        }

        statement.Append(")");

        prepared = (sqlite3_prepare_v2(conn, statement, (int)statement.Length(), &stmt, NULL) == SQLITE_OK);

        return prepared;
    }

    bool dbUpdate::Prepare()
    {
       csString statement;

        // count - 1 fields to update
        statement.Format("UPDATE %s SET ", table);
        for (unsigned int i=0;i<(count-1);i++)
        {
            if (i>0)
                statement.Append(",");
            statement.Append(command[i]);
        }
        statement.Append(" where ");
        statement.Append(idfield);
        // field count is the idfield
        statement.Append("= ?");

        prepared = (sqlite3_prepare_v2(conn, statement, (int)statement.Length(), &stmt, NULL) == SQLITE_OK);

        return prepared;
    }

    //NOT IMPLEMENTED
    #ifdef USE_DELAY_QUERY

    DelayedQueryManager::DelayedQueryManager(const char *host, unsigned int port, const char *database,
                                  const char *user, const char *pwd)
    {
        /*start=end=0;
        m_Close = false;
        m_host = csString(host);
        m_port = port;
        m_db = csString(database);
        m_user = csString(user);
        m_pwd = csString(pwd);*/
    }

    void DelayedQueryManager::Stop()
    {
        /*m_Close = true;
        datacondition.NotifyOne();
        CS::Threading::Thread::Yield();*/
    }

    void DelayedQueryManager::Run()
    {
        /*mysql_thread_init();
        MYSQL *conn=mysql_init(NULL);
        printf("Starting secondary connection with params: %s %s %s %d", m_host.GetData(), m_user.GetData(), m_db.GetData(), m_port);
        m_conn = mysql_real_connect(conn,m_host,m_user,m_pwd,m_db,m_port,NULL,CLIENT_FOUND_ROWS);
        if (!m_conn)
        {
            printf("Failed to connect to database: Error: %s\n", mysql_error(conn));
            return;
        }

        my_bool my_true = true;

    #if MYSQL_VERSION_ID >= 50000
        mysql_options(m_conn, MYSQL_OPT_RECONNECT, &my_true);
    #endif

        psStopWatch timer;
        while(!m_Close)
        {
            CS::Threading::MutexScopedLock lock(mutex);
            datacondition.Wait(mutex);
            while (start != end)
            {
                csString currQuery;
                {
                    CS::Threading::RecursiveMutexScopedLock lock(mutexArray);
                    currQuery = arr[end];
                    end = (end+1) % THREADED_BUFFER_SIZE;
                }
                timer.Start();
                if (!mysql_query(m_conn, currQuery))
                    profs.AddSQLTime(currQuery, timer.Stop());
            }
        }
        mysql_close(m_conn);
        mysql_thread_end();*/
    }

    void DelayedQueryManager::Push(csString query)
    {/*
        {
            CS::Threading::RecursiveMutexScopedLock lock(mutexArray);
            size_t tstart = (start+1) % THREADED_BUFFER_SIZE;
            if (tstart == end)
            {
                return;
            }
            arr[start] = query;
            start = tstart;
        }
        datacondition.NotifyOne();*/
    }
    #endif
}CS_PLUGIN_NAMESPACE_END(dbsqlite3)
