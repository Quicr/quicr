#pragma once

#include <cstdint>
#include <memory>
#include <string>
//#include <utility> // for pair

#include "connectionPipe.hh"
#include "crazyBitPipe.hh"
#include "encryptPipe.hh"
#include "fecPipe.hh"
#include "fragmentPipe.hh"
#include "pacerPipe.hh"
#include "packet.hh"
#include "pipeInterface.hh"
#include "priorityPipe.hh"
#include "retransmitPipe.hh"
#include "subscribePipe.hh"
#include "udpPipe.hh"
#include "fakeLossPipe.hh"

namespace MediaNet {

// class UdpPipe;
// class PacerPipe;

class QuicRClient {
public:
  QuicRClient();
  virtual ~QuicRClient();
  virtual bool open(uint32_t clientID, const std::string relayName,
                    const uint16_t port, uint64_t token);
  virtual bool ready() const;
  virtual void close();

  void setEncryptionKey(std::vector<uint8_t> salt, std::vector<uint8_t> key,
                        int authTagLen);
  void setDecryptionKey(uint32_t clientID, std::vector<uint8_t> salt,
                        std::vector<uint8_t> key);

  virtual std::unique_ptr<Packet> createPacket(const Packet::ShortName &name,
                                               int reservedPayloadSize = 1200);
  virtual bool publish(std::unique_ptr<Packet>);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

  uint64_t getTargetUpstreamBitrate(); // in bps
  // uint64_t getTargetDownstreamBitrate(); // in bps
  // uint64_t getMaxBandwidth();

  // void subscribe( const std::string origin,
  //               const std::string resource,
  //               uint32_t senderID=0, uint8_t sourceID=0 );

private:
  UdpPipe udpPipe;
  FakeLossPipe fakeLossPipe;
  CrazyBitPipe crazyBitPipe;
  ConnectionPipe connectionPipe;
  PacerPipe pacerPipe;
  PriorityPipe priorityPipe;
  RetransmitPipe retransmitPipe;
  FecPipe fecPipe;
  SubscribePipe subscribePipe;
  FragmentPipe fragmentPipe;
  EncryptPipe encryptPipe;
  PipeInterface *firstPipe;

  uint32_t pubClientID;
  uint64_t secToken;
};

} // namespace MediaNet