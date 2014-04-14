/*
 * dal.cpp by Keith Fulton
 * Database abstraction layer (mysql)
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// SCF definitions

CS_PLUGIN_NAMESPACE_BEGIN(dbmysql)
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
        mysql_close(conn);
        conn = NULL;
    }

    bool psMysqlConnection::Initialize (iObjectRegistry *objectreg)
    {
        objectReg = objectreg;

        pslog::Initialize (objectreg);

        return true;
    }

    bool psMysqlConnection::Initialize(const char *host, unsigned int port, const char *database,
                                  const char *user, const char *pwd, LogCSV* logcsv)
    {
        this->logcsv = logcsv;
        // Create a mydb
        mysql_library_init(0, NULL, NULL);
        mysql_thread_init();
        conn=mysql_init(NULL);
        // Conn is the valid connection to be used for mydb. Have to store the mydb to get
        // errors if this call fails.
        MYSQL *conn_check = mysql_real_connect(conn,host,user,pwd,database,port,NULL,CLIENT_FOUND_ROWS);
        if(!conn_check)
            return false;
        my_bool my_true = true;

    #if MYSQL_VERSION_ID >= 50000
        mysql_options(conn_check, MYSQL_OPT_RECONNECT, &my_true);
    #endif

    #ifdef USE_DELAY_QUERY
        dqm.AttachNew(new DelayedQueryManager(host, port, database, user, pwd));
        dqmThread.AttachNew(new Thread(dqm));
        dqmThread->Start();
        dqmThread->SetPriority(THREAD_PRIO_HIGH);
    #endif

        return (conn == conn_check);
    }

    bool psMysqlConnection::Close()
    {
        mysql_close(conn);
        conn = NULL;

    #ifdef USE_DELAY_QUERY
        mysql_thread_end();

        dqm->Stop();
        dqmThread->Stop();
    #endif

        mysql_library_end();
        return true;
    }


    int psMysqlConnection::IsValid()
    {
        return (conn) ? 1 : 0;
    }

    const char *psMysqlConnection::GetLastError()
    {
        return mysql_error(conn);
    }

    // Sets *to to the escaped value
    void psMysqlConnection::Escape(csString& to, const char *from)
    {
        size_t len = strlen(from);
        char* buff = new char[len*2+1];

        mysql_escape_string(buff, from, (unsigned long)len);
        to = buff;
        delete[] buff;
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
        if (!mysql_query(conn, querystr))
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
            return (unsigned long) mysql_affected_rows(conn);
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
        if (!mysql_query(conn, querystr))
        {
            if(timer.Stop() > 1000)
            {
                csString status;
                status.Format("SQL query %s, has taken %u time to process.\n", querystr.GetData(), timer.Stop());
                if(logcsv)
                    logcsv->Write(CSV_STATUS, status);
            }
            profs.AddSQLTime(querystr, timer.Stop());
            return (unsigned long) mysql_affected_rows(conn);
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
        if (!mysql_query(conn, querystr))
        {
            if(timer.Stop() > 1000)
            {
                csString status;
                status.Format("SQL query %s, has taken %u time to process.\n", querystr.GetData(), timer.Stop());
                if(logcsv)
                    logcsv->Write(CSV_STATUS, status);
            }
            profs.AddSQLTime(querystr, timer.Stop());
            iResultSet *rs = new psResultSet(conn);
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
        if (!mysql_query(conn, querystr))
        {
            if(timer.Stop() > 1000)
            {
                csString status;
                status.Format("SQL query %s, has taken %u time to process.\n", querystr.GetData(), timer.Stop());
                if(logcsv)
                    logcsv->Write(CSV_STATUS, status);
            }
            profs.AddSQLTime(querystr, timer.Stop());
            psResultSet *rs = new psResultSet(conn);

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
        return mysql_insert_id(conn);
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

        //printf("%s\n",command.GetData());

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

        //printf("%s\n",command.GetData());

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

    psResultSet::psResultSet(MYSQL *conn)
    {
        rs = mysql_store_result(conn);
        if (rs)
        {
            rows    = (unsigned long) mysql_num_rows(rs);
            fields  = mysql_num_fields(rs);
            row.SetMaxFields(fields);
            row.SetResultSet(rs);

            current = (unsigned long) -1;
        }
        else
            rows = 0;
    }

    psResultSet::~psResultSet()
    {
        mysql_free_result(rs);
    }

    iResultRow& psResultSet::operator[](unsigned long whichrow)
    {
        if (whichrow != current)
        {
            if (!row.Fetch(whichrow))
                current = whichrow;
        }
        return row;
    }

    void psResultRow::SetResultSet(void * resultsettoken)
    {
        rs = (MYSQL_RES *)resultsettoken;
    }

    int psResultRow::Fetch(int row)
    {
        mysql_data_seek(rs, row);

        rr = mysql_fetch_row(rs);

        if (rr)
            return 0;   // success
        else
        {
            max = 0;    // no fields will make operator[]'s safe
            return 1;
        }
    }

    const char *psResultRow::operator[](int whichfield)
    {
        if (whichfield >= 0 && whichfield < max)
            return rr[whichfield];
        else
            return "";
    }

    const char *psResultRow::operator[](const char *fieldname)
    {
        MYSQL_FIELD *field;
        CS_ASSERT(fieldname);
        CS_ASSERT(max); // trying to access when no fields returned in row! probably empty resultset.

        int i;

        for ( i=last_index; (field=mysql_fetch_field(rs)); i++)
        {
            if (field->name && !strcasecmp(field->name,fieldname))
            {
                last_index = i+1;
                return rr[i];
            }
        }
        mysql_field_seek(rs, 0);
        for (i=0; (i<last_index) && (field=mysql_fetch_field(rs)); i++)
        {
            if (field->name && !strcasecmp(field->name,fieldname))
            {
                last_index = i+1;
                return rr[i];
            }
        }
        CPrintf(CON_BUG, "Could not find field %s!. Exiting.\n",fieldname);
        CS_ASSERT(false);
        return ""; // Illegal name.
    }

    const char* psResultRow::GetString(int whichfield)
    {
        return this->operator [](whichfield);
    }

    const char* psResultRow::GetString(const char *fieldname)
    {
        return this->operator [](fieldname);
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
        //command.FormatPush("%s='%1.2f'", fname, fValue);
        AddToStatement(fname);
        temp[index].fValue = fValue;
        bind[index].buffer_type = MYSQL_TYPE_FLOAT;
        bind[index].buffer = (void *)&(temp[index].fValue);
        index++;
    }

    void dbRecord::AddField(const char* fname, int iValue)
    {
        //command.FormatPush("%s='%u'", fname, iValue);
        AddToStatement(fname);
        temp[index].iValue = iValue;
        bind[index].buffer_type = MYSQL_TYPE_LONG;
        bind[index].buffer = (void *)&(temp[index].iValue);
        index++;
    }

    void dbRecord::AddField(const char* fname, unsigned int uiValue)
    {
        //command.FormatPush("%s='%u'", fname, iValue);
        AddToStatement(fname);
        temp[index].uiValue = uiValue;
        bind[index].buffer_type = MYSQL_TYPE_LONG;
        bind[index].is_unsigned = true;
        bind[index].buffer = (void *)&(temp[index].uiValue);
        index++;
    }

    void dbRecord::AddField(const char* fname, unsigned short usValue)
    {
        //csString escape;
        //db->Escape(escape, sValue);
        //command.FormatPush("%s='%s'", fname, escape.GetData());
        AddToStatement(fname);
        temp[index].usValue = usValue;
        bind[index].buffer_type = MYSQL_TYPE_SHORT;
        bind[index].is_unsigned = true;
        bind[index].buffer = &(temp[index].usValue);
        index++;
    }

    void dbRecord::AddField(const char* fname, const char* sValue)
    {
        //csString escape;
        //db->Escape(escape, sValue);
        //command.FormatPush("%s='%s'", fname, escape.GetData());
        AddToStatement(fname);
        temp[index].string = sValue;
        temp[index].length = (unsigned long)temp[index].string.Length();

        bind[index].buffer_type = MYSQL_TYPE_STRING;
        bind[index].buffer = const_cast<char *> (temp[index].string.GetData());

        bind[index].buffer_length = temp[index].length;
        bind[index].length = &(temp[index].length);
        index++;
    }

    void dbRecord::AddFieldNull(const char* fname)
    {
        AddToStatement(fname);
        bind[index].buffer_type = MYSQL_TYPE_NULL;
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

        CS_ASSERT(count == mysql_stmt_param_count(stmt));

        if(mysql_stmt_bind_param(stmt, bind) != 0)
            return false;

        bool result = (mysql_stmt_execute(stmt) == 0);
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

        prepared = (mysql_stmt_prepare(stmt, statement, (unsigned long)statement.Length()) == 0);

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

        prepared = (mysql_stmt_prepare(stmt, statement, (unsigned long)statement.Length()) == 0);

        return prepared;
    }


    #ifdef USE_DELAY_QUERY

    DelayedQueryManager::DelayedQueryManager(const char *host, unsigned int port, const char *database,
                                  const char *user, const char *pwd)
    {
        start=end=0;
        m_Close = false;
        m_host = csString(host);
        m_port = port;
        m_db = csString(database);
        m_user = csString(user);
        m_pwd = csString(pwd);
    }

    void DelayedQueryManager::Stop()
    {
        m_Close = true;
        datacondition.NotifyOne();
        CS::Threading::Thread::Yield();
    }

    void DelayedQueryManager::Run()
    {
        mysql_thread_init();
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
        mysql_thread_end();
    }

    void DelayedQueryManager::Push(csString query)
    {
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
        datacondition.NotifyOne();
    }
    #endif
}
CS_PLUGIN_NAMESPACE_END(dbmysql)
