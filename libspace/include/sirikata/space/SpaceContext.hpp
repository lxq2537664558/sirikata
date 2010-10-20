/*  Sirikata
 *  SpaceContext.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_SPACE_CONTEXT_HPP_
#define _SIRIKATA_SPACE_CONTEXT_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/space/Trace.hpp>
#include <sirikata/core/network/SSTImpl.hpp>

namespace Sirikata {

class ServerMessageRouter;
class ObjectMessageRouter;
class ServerMessageDispatcher;
class ObjectMessageDispatcher;

class Forwarder;
class MockForwarder;

class CoordinateSegmentation;

/** SpaceContext holds a number of useful items that are effectively global
 *  for each space node and used throughout the system -- ServerID, time information,
 *  MessageRouter (sending messages), MessageDispatcher (subscribe/unsubscribe
 *  from messages), and a Trace object.
 */
class SIRIKATA_SPACE_EXPORT SpaceContext : public Context {
public:
    SpaceContext(ServerID _id, Network::IOService* ios, Network::IOStrand* strand, const Time& epoch, Trace::Trace* _trace, const Duration& duration = Duration::zero());
    ~SpaceContext();

    ServerID id() const {
        return mID.read();
    }

    ServerMessageRouter* serverRouter() const {
        return mServerRouter.read();
    }

    ObjectMessageRouter* objectRouter() const {
        return mObjectRouter.read();
    }

    ServerMessageDispatcher* serverDispatcher() const {
        return mServerDispatcher.read();
    }
    ObjectMessageDispatcher* objectDispatcher() const {
        return mObjectDispatcher.read();
    }

    CoordinateSegmentation* cseg() const {
        return mCSeg.read();
    }

    typedef Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;

    void newStream(int err, SSTStreamPtr s);

    SSTStreamPtr getObjectStream(const ObjectReference& objid) {
      if (mObjectStreams.find(objid) != mObjectStreams.end()) {
        return mObjectStreams[objid];
      }

      return SSTStreamPtr();
    }

    SpaceTrace* spacetrace() const { return mSpaceTrace; }

private:
    friend class Forwarder; // Allow forwarder to set mRouter and mDispatcher
    friend class MockForwarder; // Same for mock forwarder
    friend class Server;

    Sirikata::AtomicValue<ServerID> mID;

    Sirikata::AtomicValue<ServerMessageRouter*> mServerRouter;
    Sirikata::AtomicValue<ObjectMessageRouter*> mObjectRouter;
    Sirikata::AtomicValue<ServerMessageDispatcher*> mServerDispatcher;
    Sirikata::AtomicValue<ObjectMessageDispatcher*> mObjectDispatcher;

    Sirikata::AtomicValue<CoordinateSegmentation*> mCSeg;

    std::map<ObjectReference, SSTStreamPtr>  mObjectStreams;

    SpaceTrace* mSpaceTrace;
}; // class SpaceContext

} // namespace Sirikata

#endif //_SIRIKATA_SPACE_CONTEXT_HPP_
