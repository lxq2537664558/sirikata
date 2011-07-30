

#ifndef __EMERSON_MESSAGING_MANAGER_HPP__
#define __EMERSON_MESSAGING_MANAGER_HPP__

#include <map>
#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/network/ObjectMessage.hpp>
#include <string>
#include <sstream>
#include <sirikata/core/util/Liveness.hpp>

namespace Sirikata{
namespace JS{

class EmersonScript;

typedef Stream<SpaceObjectReference> SSTStream;
typedef SSTStream::Ptr SSTStreamPtr;

// NOTE: virtual on Liveness because JSObjectScript also uses it
class EmersonMessagingManager : public virtual Liveness
{
public:
    EmersonMessagingManager(Network::IOService* ios);
    virtual ~EmersonMessagingManager();

    //EmersonScript must know what to do with messages that we receive.
    /*
      Returns true if decoded payload as a scripting communication message,
      false otherwise.

      Payload should be able to be parsed into JS::Proctocol::JSMessage.  If it
      can be, and deserialization is successful, processes the scripting
      message. (via deserializeMsgAndDispatch.)
     */
    virtual bool handleScriptCommRead(const SpaceObjectReference& src, const SpaceObjectReference& dst, const std::string& payload) = 0;


    bool sendScriptCommMessageReliable(const SpaceObjectReference& sender, const SpaceObjectReference& receiver, const String& msg);


    void presenceConnected(const SpaceObjectReference& connPresSporef);
    void presenceDisconnected(const SpaceObjectReference& disconnPresSporef);

private:
    // Possibly save the new stream to mStreams for later use. Since both sides
    // might initiate, we always save the stream initiated by the object with
    // smaller if we identify a conflict.
    void setupNewStream(SSTStreamPtr sstStream);
    // Get a saved stream for the given destination object, or NULL if one isn't
    // available.
    SSTStreamPtr getStream(const SpaceObjectReference& pres, const SpaceObjectReference& remote);

    //reading helpers
    void createScriptCommListenerStreamCB(Liveness::Token alive, const SpaceObjectReference& toListenFrom, int err, SSTStreamPtr sstStream);
    void handleIncomingSubstream(Liveness::Token alive, int err, SSTStreamPtr streamPtr);
    void handleScriptCommStreamRead(Liveness::Token alive, SSTStreamPtr sstptr, String* prevdata, uint8* buffer, int length);

    //writing helper
    void scriptCommWriteStreamConnectedCB(Liveness::Token alive, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver, int err, SSTStreamPtr streamPtr);

    // Writes a message to a *substream* of the given stream
    void writeMessage(Liveness::Token alive, SSTStreamPtr streamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver);
    void writeMessageSubstream(Liveness::Token alive, int err, SSTStreamPtr subStreamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver);
    void writeData(Liveness::Token alive, SSTStreamPtr streamPtr, const String& msg, const SpaceObjectReference& sender, const SpaceObjectReference& receiver);

    //map of existing presences.  value doesn't matter, just want a quick way of
    //checking if particular presences are connected.
    std::map<SpaceObjectReference, bool> allPres;

    // Streams to be reused for sending messages
    typedef std::tr1::unordered_map<SpaceObjectReference, SSTStreamPtr, SpaceObjectReference::Hasher> StreamMap;
    typedef std::tr1::unordered_map<SpaceObjectReference, StreamMap, SpaceObjectReference::Hasher> PresenceStreamMap;
    PresenceStreamMap mStreams;

    //used for retrying messages that haven't completely sent.
    Network::IOTimerPtr retryTimer;

};

} //end namespace js
} //end namespace sirikata

#endif
