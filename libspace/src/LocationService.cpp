// Copyright (c) 2009 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/space/LocationService.hpp>
#include <sirikata/core/command/Commander.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/odp/SST.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::LocationUpdatePolicyFactory);
AUTO_SINGLETON_INSTANCE(Sirikata::LocationServiceFactory);

namespace Sirikata {

LocationServiceListener::~LocationServiceListener() {
}



LocationUpdatePolicyFactory& LocationUpdatePolicyFactory::getSingleton() {
    return AutoSingleton<LocationUpdatePolicyFactory>::getSingleton();
}

void LocationUpdatePolicyFactory::destroy() {
    AutoSingleton<LocationUpdatePolicyFactory>::destroy();
}


LocationUpdatePolicy::LocationUpdatePolicy()
 : mLocService(NULL)
{
}

void LocationUpdatePolicy::initialize(LocationService* locservice)
{
    mLocService = locservice;
    mLocService->addListener(this, true);
    mLocMessageRouter = mLocService->context()->serverRouter()->createServerMessageService("loc-update");
}

LocationUpdatePolicy::~LocationUpdatePolicy() {
    delete mLocMessageRouter;
}


LocationServiceFactory& LocationServiceFactory::getSingleton() {
    return AutoSingleton<LocationServiceFactory>::getSingleton();
}

void LocationServiceFactory::destroy() {
    AutoSingleton<LocationServiceFactory>::destroy();
}

LocationService::LocationService(SpaceContext* ctx, LocationUpdatePolicy* update_policy)
 : PollingService(ctx->mainStrand, "LocationService Poll", Duration::milliseconds((int64)10)),
   mContext(ctx),
   mUpdatePolicy(update_policy)
{
    mProfiler = mContext->profiler->addStage("Location Service");

    mUpdatePolicy->initialize(this);

    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_LOCATION, this);
    mContext->objectSessionManager()->addListener(this);

    // Implementations may add more commands, but these should always be
    // available. They get dispatched to the main strand so implementations only
    // need to worry about processing them.
    if (mContext->commander()) {
        // Get basic properties about this implementation, e.g. the type, the
        // number of objects tracked, etc.
        mContext->commander()->registerCommand(
            "space.loc.properties",
            mContext->mainStrand->wrap(
                std::tr1::bind(&LocationService::commandProperties, this, _1, _2, _3)
            )
        );

        // Get all stored properties of a given object.
        mContext->commander()->registerCommand(
            "space.loc.object",
            mContext->mainStrand->wrap(
                std::tr1::bind(&LocationService::commandObjectProperties, this, _1, _2, _3)
            )
        );
    }
}

LocationService::~LocationService() {
    delete mProfiler;
    delete mUpdatePolicy;

    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_LOCATION, this);
    mContext->objectSessionManager()->removeListener(this);
}

void LocationService::newSession(ObjectSession* session) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    SST::Stream<SpaceObjectReference>::Ptr strm = session->getStream();

    SST::Connection<SpaceObjectReference>::Ptr conn = strm->connection().lock();
    assert(conn);

    SpaceObjectReference sourceObject = conn->remoteEndPoint().endPoint;

    strm->listenSubstream(OBJECT_PORT_LOCATION,
        std::tr1::bind(
            &LocationService::handleLocationUpdateSubstream, this,
            sourceObject.object().getAsUUID(),
            std::tr1::placeholders::_1,std::tr1::placeholders::_2
        )
    );

}

void LocationService::handleLocationUpdateSubstream(const UUID& source, int err, SSTStreamPtr s) {
    s->registerReadCallback(
        std::tr1::bind(
            &LocationService::handleLocationUpdateSubstreamRead, this,
            source, s, new std::stringstream(),
            std::tr1::placeholders::_1,std::tr1::placeholders::_2
        )
    );
}

void LocationService::handleLocationUpdateSubstreamRead(const UUID& source, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length) {
    prevdata->write((const char*)buffer, length);
    String payload(prevdata->str());
#ifdef SIRIKATA_SPACE_DELAY_APPLY_LOC_UPDATE
    static Duration delay = Duration::seconds(SIRIKATA_SPACE_DELAY_APPLY_LOC_UPDATE);
    mContext->mainStrand->post(
        delay,
        std::tr1::bind(
            &LocationService::tryHandleLocationUpdate, this,
            source, s, payload, prevdata
        )
    );
#else
    tryHandleLocationUpdate(source, s, payload, prevdata);
#endif
}

void LocationService::tryHandleLocationUpdate(const UUID& source, SSTStreamPtr s, const String& payload, std::stringstream* prevdata) {
    if (locationUpdate(source, (void*)payload.c_str(), payload.size())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s, and close the stream
        s->registerReadCallback(0);
        s->close(false);
    }
}

void LocationService::start() {
    PollingService::start();
    mUpdatePolicy->start();
}

void LocationService::stop() {
    mUpdatePolicy->stop();
    PollingService::stop();
}

void LocationService::poll() {
    mProfiler->started();
    service();
    mProfiler->finished();
}

void LocationService::addListener(LocationServiceListener* listener, bool want_aggregates) {
    ListenerInfo info;
    info.listener = listener;
    info.wantAggregates = want_aggregates;
    mListeners.insert(info);
}

void LocationService::removeListener(LocationServiceListener* listener) {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++) {
        if (it->listener == listener) {
            mListeners.erase(it);
            break;
        }
    }
}



// Server Subscriptions

void LocationService::subscribe(ServerID remote, const UUID& uuid, SeqNoPtr seqnoptr) {
    mUpdatePolicy->subscribe(remote, uuid, seqnoptr);
}

void LocationService::subscribe(ServerID remote, const UUID& uuid, ProxIndexID index_id, SeqNoPtr seqnoptr) {
    mUpdatePolicy->subscribe(remote, uuid, index_id, seqnoptr);
}

void LocationService::unsubscribe(ServerID remote, const UUID& uuid) {
    mUpdatePolicy->unsubscribe(remote, uuid);
}

void LocationService::unsubscribe(ServerID remote, const UUID& uuid, ProxIndexID index_id) {
    mUpdatePolicy->unsubscribe(remote, uuid, index_id);
}

void LocationService::unsubscribe(ServerID remote, const std::tr1::function<void()>&callback) {
    mUpdatePolicy->unsubscribe(remote, callback);
}


// OH Subscriptions

void LocationService::subscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    mUpdatePolicy->subscribe(remote, uuid);
}

void LocationService::subscribe(const OHDP::NodeID& remote, const UUID& uuid, ProxIndexID index_id) {
    mUpdatePolicy->subscribe(remote, uuid, index_id);
}

void LocationService::unsubscribe(const OHDP::NodeID& remote, const UUID& uuid) {
    mUpdatePolicy->unsubscribe(remote, uuid);
}

void LocationService::unsubscribe(const OHDP::NodeID& remote, const UUID& uuid, ProxIndexID index_id) {
    mUpdatePolicy->unsubscribe(remote, uuid, index_id);
}

void LocationService::unsubscribe(const OHDP::NodeID& remote, const std::tr1::function<void()>&callback) {
    mUpdatePolicy->unsubscribe(remote, callback);
}



// Object Subscriptions

void LocationService::subscribe(const UUID& remote, const UUID& uuid) {
    mUpdatePolicy->subscribe(remote, uuid);
}

void LocationService::subscribe(const UUID& remote, const UUID& uuid, ProxIndexID index_id) {
    mUpdatePolicy->subscribe(remote, uuid, index_id);
}

void LocationService::unsubscribe(const UUID& remote, const UUID& uuid) {
    mUpdatePolicy->unsubscribe(remote, uuid);
}

void LocationService::unsubscribe(const UUID& remote, const UUID& uuid, ProxIndexID index_id) {
    mUpdatePolicy->unsubscribe(remote, uuid, index_id);
}

void LocationService::unsubscribe(const UUID& remote, const std::tr1::function<void()>&callback) {
    mUpdatePolicy->unsubscribe(remote, callback);
}




void LocationService::notifyLocalObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& physics, const String& query_data) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
          it->listener->localObjectAdded(uuid, agg, loc, orient, bounds, mesh, physics, query_data);
}
class CallOnDestruct {
    std::tr1::function<void()>mCallback;
public:
    CallOnDestruct(const std::tr1::function<void()>&callback):mCallback(callback) {}
    static void nop(const std::tr1::shared_ptr<CallOnDestruct>&data){
        if (!data)
            SILOG(space,error,"Invalid data passed to COD nop");
    };
    ~CallOnDestruct() {
        mCallback();
    }
};
void LocationService::notifyLocalObjectRemoved(const UUID& uuid, bool agg, const LocationServiceListener::RemovalCallback&callback) const {
    bool hasCallbacked=false;
    ListenerList::const_iterator begin = mListeners.begin();
    LocationServiceListener::RemovalCallback oneToManyCallback;
    std::tr1::shared_ptr<CallOnDestruct> originalCallbackCaller;
    ListenerList::const_iterator it = begin;
    while (it != mListeners.end()) {
        ListenerList::const_iterator next = it;
         ++next;
         if (!agg || it->wantAggregates) {
             hasCallbacked=true;
             if (it==begin) {
                 if (next==mListeners.end()) {//size == 1, can call the callback straight away
                     it->listener->localObjectRemoved(uuid, agg, callback);
                 }else {//many callbacks, need to refcount all of them and only deliver the callback when needed
                     std::tr1::shared_ptr<CallOnDestruct> tmp(new CallOnDestruct(callback));
                     originalCallbackCaller = tmp;
                     oneToManyCallback=std::tr1::bind(&CallOnDestruct::nop,originalCallbackCaller);
                     it->listener->localObjectRemoved(uuid, agg, oneToManyCallback);
                 }
             }else {
                 it->listener->localObjectRemoved(uuid, agg, oneToManyCallback);
             }
         }
         it=next;
    }
    if (!hasCallbacked) {
        callback();
    }
}


void LocationService::notifyLocalLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localLocationUpdated(uuid, agg, newval);
}

void LocationService::notifyLocalOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localOrientationUpdated(uuid, agg, newval);
}

void LocationService::notifyLocalBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localBoundsUpdated(uuid, agg, newval);
}


void LocationService::notifyLocalMeshUpdated(const UUID& uuid, bool agg, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localMeshUpdated(uuid, agg, newval);
}

void LocationService::notifyLocalPhysicsUpdated(const UUID& uuid, bool agg, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localPhysicsUpdated(uuid, agg, newval);
}

void LocationService::notifyLocalQueryDataUpdated(const UUID& uuid, bool agg, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        if (!agg || it->wantAggregates)
            it->listener->localQueryDataUpdated(uuid, agg, newval);
}


void LocationService::notifyReplicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const AggregateBoundingInfo& bounds, const String& mesh, const String& physics, const String& query_data) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
      it->listener->replicaObjectAdded(uuid, loc, orient, bounds, mesh, physics, query_data);
}

void LocationService::notifyReplicaObjectRemoved(const UUID& uuid) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaObjectRemoved(uuid);
}

void LocationService::notifyReplicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaLocationUpdated(uuid, newval);
}

void LocationService::notifyReplicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaOrientationUpdated(uuid, newval);
}

void LocationService::notifyReplicaBoundsUpdated(const UUID& uuid, const AggregateBoundingInfo& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaBoundsUpdated(uuid, newval);
}

void LocationService::notifyReplicaMeshUpdated(const UUID& uuid, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaMeshUpdated(uuid, newval);
}

void LocationService::notifyReplicaPhysicsUpdated(const UUID& uuid, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaPhysicsUpdated(uuid, newval);
}

void LocationService::notifyReplicaQueryDataUpdated(const UUID& uuid, const String& newval) const {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->replicaQueryDataUpdated(uuid, newval);
}

void LocationService::notifyOnLocationUpdateFromServer(const ServerID sid, const Sirikata::Protocol::Loc::LocationUpdate& update) {
    for(ListenerList::const_iterator it = mListeners.begin(); it != mListeners.end(); it++)
        it->listener->onLocationUpdateFromServer(sid, update);
}

} // namespace Sirikata
