/*  Sirikata
 *  SQLiteAuthenticator.hpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SIRIKATA_SPACE_SQLITE_AUTHENTICATOR_HPP_
#define _SIRIKATA_SPACE_SQLITE_AUTHENTICATOR_HPP_

#include <sirikata/space/Authenticator.hpp>
#include <sirikata/sqlite/SQLite.hpp>

namespace Sirikata {

class SQLiteAuthenticator : public Authenticator {
public:
    SQLiteAuthenticator(SpaceContext* ctx, const String& dbfile, const String& select_stmt, const String& delete_stmt);
    virtual ~SQLiteAuthenticator() {}

    virtual void start();
    virtual void stop();

    virtual void authenticate(const UUID& obj_id, MemoryReference auth, Callback cb);

private:
    // Helper that checks and logs errors, then returns bool indicating
    // success/failure
    bool checkSQLiteError(int rc, const String& msg) const;

    // Check if the ticket is valid
    bool checkTicket(const String& ticket);
    // Delete a ticket from the db
    void deleteTicket(const String& ticket);
    // Generate the response to the auth request
    void respond(Callback cb, bool result);

    SpaceContext* mContext;
    String mDBFile;
    String mDBGetSessionStmt;
    String mDBDeleteSessionStmt;

    SQLiteDBPtr mDB;
};

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_SQLITE_AUTHENTICATOR_HPP_
