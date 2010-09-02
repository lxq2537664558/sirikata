/*  Sirikata
 *  Object.cpp
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

#include "Object.hpp"
#include <sirikata/core/network/ObjectMessage.hpp>
#include <sirikata/core/util/Random.hpp>
#include "ObjectHostContext.hpp"
#include "ObjectHost.hpp"
#include "ObjectFactory.hpp"
#include <sirikata/core/trace/Trace.hpp>
#include "Trace.hpp"
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <boost/bind.hpp>

#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"


#define OBJ_LOG(level,msg) SILOG(object,level,"[OBJ] " << msg)

namespace Sirikata {

void datagramSendDoneCallback(int, void*) {
}

void datagramCallback(uint8* buffer, int length) {
  printf( "Received datagram %s\n", buffer );
}



class ReadClass {
public:
 int numBytesReceived;

 ReadClass():numBytesReceived(0) {}

 void childStreamReadCallback(uint8* buffer, int length) {

   for (int i=0; i<length; i++) {
     if ( buffer[i] != (i+numBytesReceived) % 255  ) {
       std::cout << i << "\n";
       std::cout << (i+numBytesReceived) << "\n";
       fflush(stdout);
       assert(false);
     }
   }

   numBytesReceived += length;

   printf("numBytesReceived=%d in object \n", numBytesReceived);

   fflush(stdout);
 }
};

void delayed_register( boost::shared_ptr< Stream<UUID> > s) {
  boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
  ReadClass *c = new ReadClass();
  s->registerReadCallback( std::tr1::bind( &ReadClass::childStreamReadCallback, c, _1, _2)  ) ;
}

void func4(int err, boost::shared_ptr< Stream<UUID> > s) {
  if (err != 0) {
    printf("stream could not be successfully created\n");
    fflush(stdout);
    return;
  }

  std::cout << "func4 called with endpoint " << s->connection().lock()->localEndPoint().endPoint.toString() << "\n";
  fflush(stdout);

  int length = 1000000;

  uint8* f = new uint8[length];
  for (int i=0; i<length; i++) {
    f[i] = i % 255;
  }

  //if (s->connection().lock()->localEndPoint().endPoint.toString() == "02000000-0000-0000-0200-000000000000") {
    s->write(f, length);
  //}
}

void func6(int err, boost::shared_ptr< Stream<UUID> > s)
{
  std::cout << "func6 called with endpoint " << s->connection().lock()->localEndPoint().endPoint.toString() << "\n";
  ReadClass *c = new ReadClass();
  s->registerReadCallback( std::tr1::bind( &ReadClass::childStreamReadCallback, c, _1, _2)  ) ;

}

void func3(int err, boost::shared_ptr< Stream<UUID> > s) {
  static bool called_once = false;
  if (s == boost::shared_ptr< Stream<UUID> >()) {
      std::cout << "func3: Could not initiate stream\n";
      fflush(stdout);
      return;
  }
  
  s->listenSubstream(21, func6 );
  new Thread(boost::bind(delayed_register, s));

  if (called_once) {
    return;
  }

  called_once = true;

  if (err != 0) {
    printf("Failed to create new top level stream\n");
    return;
  }

  std::cout << "func3 called with endpoint " << s->connection().lock()->localEndPoint().endPoint.toString() << "\n";
  fflush(stdout);

  ReadClass *c = new ReadClass();
  s->registerReadCallback( std::tr1::bind( &ReadClass::childStreamReadCallback, c, _1, _2)  ) ;



  s->connection().lock()->registerReadDatagramCallback(20, datagramCallback);

  /*for (int i=0 ;i<1000; i++) {
    s->connection().lock()->datagram( (uint8*)("foobar\n\0"), 8, 20, 20, datagramSendDoneCallback );
  }*/

  s->createChildStream( func4,  NULL, 0, 22, 22 );
}

void func5(int err, boost::shared_ptr< Stream<UUID> > s) {
  if (s == boost::shared_ptr< Stream<UUID> >()) {
    std::cout << "func5: Could not initiate stream\n";
    fflush(stdout);
    return;
  }

  s->listenSubstream(22, func6 );

  std::cout << "func5 called with endpoint " << s->connection().lock()->localEndPoint().endPoint.toString() << "\n";
  fflush(stdout);

  s->connection().lock()->registerReadDatagramCallback(20, datagramCallback);

  new Thread(boost::bind(delayed_register, s));

  s->createChildStream( func4,  NULL, 0, 21, 21  );
}



float64 MaxDistUpdatePredicate::maxDist = 3.0;

Object::Object(ObjectFactory* obj_factory, const UUID& id, MotionPath* motion, const BoundingSphere3f& bnds, bool regQuery, SolidAngle queryAngle, const ObjectHostContext* ctx)
 : mID(id),
   mContext(ctx),
   mObjectFactory(obj_factory),
   mBounds(bnds),
   mLocation(motion->initial()),
   mMotion(motion),
   mLocationExtrapolator(mMotion->initial(), MaxDistUpdatePredicate()),
   mRegisterQuery(regQuery),
   mQueryAngle(queryAngle),
   mConnectedTo(0),
   mMigrating(false),
   mQuitting(false),
   mLocUpdateTimer( Network::IOTimer::create(mContext->ioService) ),
   mSSTTestTimer( Network::IOTimer::create(mContext->ioService) )
{
  mSSTDatagramLayer = BaseDatagramLayer<UUID>::createDatagramLayer(mID, this, this);
  Stream<UUID>::listen(func5, EndPoint<UUID>(mID, 51000) );

  if (mID.toString() == "02000000-0000-0000-0200-000000000000") {
    mSSTTestTimer->wait(
                      Duration::milliseconds( (int64)50000),
                      mContext->mainStrand->wrap(
                                                 std::tr1::bind(&Object::testSST, this)
                                                 )
                        );
  }
}

void Object::testSST() {
  std::cout << "testSST\n";
  fflush(stdout);
  Stream<UUID>::connectStream(this,
              EndPoint<UUID>(mID, 50000),
              EndPoint<UUID>( UUID( String("bf0b0000-0000-0000-bf0b-000000000000"), UUID::HumanReadable()), 51000),
              func3
              );

}

Object::~Object() {
    stop();
    disconnect();
    mObjectFactory->notifyDestroyed(mID);
}

void Object::start() {
    connect();
}

void Object::stop() {
    mQuitting = true;
    mLocUpdateTimer->cancel();
}

void Object::scheduleNextLocUpdate() {
    const Time tnow = mContext->simTime();

    TimedMotionVector3f curLoc = location();
    const TimedMotionVector3f* update = mMotion->nextUpdate(tnow);
    if (update != NULL) {

        mLocUpdateTimer->wait(
            update->time() - tnow,
            mContext->mainStrand->wrap(
                std::tr1::bind(&Object::handleNextLocUpdate, this, *update)
            )
        );
    }
}

void Object::handleNextLocUpdate(const TimedMotionVector3f& up) {
    if (mQuitting) {
        disconnect();
        return;
    }

    const Time tnow = mContext->simTime();

    TimedMotionVector3f curLoc = up;
    mLocation = curLoc;

    if (!mMigrating && mLocationExtrapolator.needsUpdate(tnow, curLoc.extrapolate(tnow))) {
        mLocationExtrapolator.updateValue(curLoc.time(), curLoc.value());

        // Generate and send an update to Loc
        Sirikata::Protocol::Loc::Container container;
        Sirikata::Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
        Sirikata::Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t(curLoc.updateTime());
        requested_loc.set_position(curLoc.position());
        requested_loc.set_velocity(curLoc.velocity());

        std::string payload = serializePBJMessage(container);

	      boost::shared_ptr<Stream<UUID> > spaceStream = mContext->objectHost->getSpaceStream(mID);
        if (spaceStream != boost::shared_ptr<Stream<UUID> >()) {
          boost::shared_ptr<Connection<UUID> > conn = spaceStream->connection().lock();
          assert(conn);

          conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
                          OBJECT_PORT_LOCATION, NULL);
	      }

        // XXX FIXME do something on failure
        CONTEXT_OHTRACE_NO_TIME(objectGenLoc, tnow, mID, curLoc, bounds());
    }

    scheduleNextLocUpdate();
}

bool Object::send( uint16 src_port,  UUID src,  uint16 dest_port,  UUID dest, std::string payload) {
  bool val = mContext->objectHost->send(
			     src_port, src,
			     dest_port, dest,
			     payload
			     );

  return val;
}
void Object::sendNoReturn( uint16 src_port,  UUID src,  uint16 dest_port,  UUID dest, std::string payload) {
    send(src_port, src, dest_port, dest, payload);
}
bool Object::route(Sirikata::Protocol::Object::ObjectMessage* msg) {
  mContext->mainStrand->post(std::tr1::bind(
			     &Object::sendNoReturn, this,
			     msg->source_port(), msg->source_object(),
			     msg->dest_port(), msg->dest_object(),
			     msg->payload())
			    );

  delete msg;

  return true;
}

const TimedMotionVector3f Object::location() const {
    return mLocation.read();
}

const BoundingSphere3f Object::bounds() const {
    return mBounds.read();
}

void Object::connect() {
    if (connected()) {
        OBJ_LOG(warning,"Tried to connect when already connected " << mID.toString());
        return;
    }

    TimedMotionVector3f curMotion = mMotion->at(mContext->simTime());

//     if (mRegisterQuery)
//         mContext->objectHost->connect(
//             this,
//             mQueryAngle,
//             mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceConnection, this, _1) ),
//             mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceMigration, this, _1) ),
// 	    mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceStreamCreated, this) )
//         );
//     else
//         mContext->objectHost->connect(
//             this,
//             mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceConnection, this, _1) ),
//             mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceMigration, this, _1) ),
// 	    mContext->mainStrand->wrap( boost::bind(&Object::handleSpaceStreamCreated, this ) )
//         );

    if (mRegisterQuery)
        mContext->objectHost->connect(
            this,
            mQueryAngle,
            mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceConnection, this, std::tr1::placeholders::_1) ),
            mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceMigration, this, std::tr1::placeholders::_1) ),
	    mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceStreamCreated, this) )
        );
    else
        mContext->objectHost->connect(
            this,
            mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceConnection, this,std::tr1::placeholders::_1) ),
            mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceMigration, this, std::tr1::placeholders::_1) ),
	    mContext->mainStrand->wrap( std::tr1::bind(&Object::handleSpaceStreamCreated, this ) )
        );



}

void Object::disconnect() {
    // FIXME send if a connection has been requested but not completed
    if (connected())
        mContext->objectHost->disconnect(this);
}

void Object::handleSpaceConnection(ServerID sid) {
    if (sid == 0) {
        OBJ_LOG(error,"Failed to open connection for object " << mID.toString());
        return;
    }

    OBJ_LOG(insane,"Got space connection callback");
    mConnectedTo = sid;

    // Always record our initial position, may be the only "update" we ever send
    const Time tnow = mContext->simTime();
    TimedMotionVector3f curLoc = location();
    BoundingSphere3f curBounds = bounds();
    CONTEXT_OHTRACE_NO_TIME(objectConnected, tnow, mID, sid);
    CONTEXT_OHTRACE_NO_TIME(objectGenLoc, tnow, mID, curLoc, curBounds);

    // Start normal processing
    mContext->mainStrand->post(
        std::tr1::bind(&Object::scheduleNextLocUpdate, this)
    );
}

void Object::handleSpaceMigration(ServerID sid) {
    OBJ_LOG(insane,"Migrated to new space server: " << sid);
    mConnectedTo = sid;
}

void Object::handleSpaceStreamCreated() {  
  boost::shared_ptr<Stream<UUID> > sstStream = mContext->objectHost->getSpaceStream(mID);

  if (sstStream != boost::shared_ptr<Stream<UUID> >() ) {
    boost::shared_ptr<Connection<UUID> > sstConnection = sstStream->connection().lock();
    assert(sstConnection);

    sstConnection->registerReadDatagramCallback(OBJECT_PORT_LOCATION,
						std::tr1::bind(&Object::locationMessage, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2)
						);
    sstConnection->registerReadDatagramCallback(OBJECT_PORT_PROXIMITY,
                                                std::tr1::bind(&Object::proximityMessage, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2)
                                                );

  }
}


bool Object::connected() {
    return (mConnectedTo != NullServerID);
}

void Object::receiveMessage(const Sirikata::Protocol::Object::ObjectMessage* msg) {
    assert( msg->dest_object() == uuid() );

    
    dispatchMessage(*msg);
    delete msg;
}

void Object::locationMessage(uint8* buffer, int len) {
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    bool parse_success = contents.ParseFromArray(buffer, len);
    assert(parse_success);

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

        Sirikata::Protocol::TimedMotionVector update_loc = update.location();
        TimedMotionVector3f loc(update_loc.t(), MotionVector3f(update_loc.position(), update_loc.velocity()));

        CONTEXT_OHTRACE(objectLoc,
            mID,
            update.object(),
            loc
        );

        // FIXME do something with the data
    }
}

void Object::proximityMessage(uint8* buffer, int len) {
    //assert(msg.source_object() == UUID::null()); // Should originate at space server
    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromArray(buffer, len);
    assert(parse_success);


    for(int32 idx = 0; idx < contents.addition_size(); idx++) {
        Sirikata::Protocol::Prox::ObjectAddition addition = contents.addition(idx);

        TimedMotionVector3f loc(addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()));

        CONTEXT_OHTRACE(prox,
            mID,
            addition.object(),
            true,
            loc
        );
    }

    for(int32 idx = 0; idx < contents.removal_size(); idx++) {
        Sirikata::Protocol::Prox::ObjectRemoval removal = contents.removal(idx);

        CONTEXT_OHTRACE(prox,
            mID,
            removal.object(),
            false,
            TimedMotionVector3f()
        );
    }
}

} // namespace Sirikata