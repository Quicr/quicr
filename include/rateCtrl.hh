#pragma once

#include <chrono>
#include <cstdint>

#include "packet.hh"
#include "pipeInterface.hh"

using namespace MediaNet;

namespace MediaNet {

struct PacketUpstreamStatus {
    uint32_t seqNum;
    uint16_t sizeBits;

    uint32_t localSendTimeUs;
    uint32_t remoteReceiveTimeUs;
    uint32_t localAckTimeUs;

    uint32_t sendPhaseCount; // cycleCount * numPhasePerCycle + phase

    bool notLost;
    // bool congested;

    MediaNet::ShortName shortName;
};

struct PacketDownstreamStatus {
  uint32_t remoteSeqNum;
  uint16_t sizeBits;

  uint32_t remoteSendTimeUs;
  uint32_t localReceiveTimeUs;
  uint32_t remoteAckTimeUs;

  uint32_t sendPhaseCount; // cycleCount * numPhasePerCycle + phase

  bool notLost;
  // bool congested;
};

class RateCtrl {
public:
  explicit RateCtrl(PipeInterface *pacerPipeRef);

  void sendPacket(uint32_t seqNum, uint32_t sendTimeUs, uint16_t sizeBits,
                  ShortName shortName);

  void recvPacket(uint32_t relaySeqNum, uint32_t remoteSendTimeUs,
                  uint32_t localRecvTimeUs, uint16_t sizeBits);
  void recvAck(uint32_t seqNum, uint32_t remoteAckTimeUs,
               uint32_t localRecvAckTimeUs);


  [[nodiscard]] uint32_t getPhase() const { return (phaseCycleCount % numPhasePerCycle); }

  [[nodiscard]] uint64_t bwUpTarget() const;   // in bits per second
  [[nodiscard]] uint64_t bwDownTarget() const; // in bits per second

private:
  PipeInterface *pacerPipe;

  uint32_t upHistorySeqOffset;
  std::vector<PacketUpstreamStatus> upstreamHistory;

  uint32_t downHistorySeqOffset;
  std::vector<PacketDownstreamStatus> downstreamHistory;

  void updatePhase();
  static const uint32_t phaseTimeUs = 33333 / 2; // 0.5 frames at 30 fps
  static const uint32_t numPhasePerCycle = 10;

  void startNewPhase();
  uint32_t phaseCycleCount; // does *not* reset to zero with each new cycle
  uint32_t bitsSentThisPhase;

  void startNewCycle();
  std::chrono::steady_clock::time_point cycleStartTime;

  void updateRTT(uint32_t valUs, int32_t remoteTimeOffsetUs);
  uint32_t minCycleRTTUs;
  uint32_t maxCycleAckTimeUs;
  uint32_t estRTTUs;
  uint32_t estAckTimeUs;
  int32_t cycleRelayTimeOffsetUs;
  int32_t estRelayTimeOffsetUs;

  void estUpstreamBw();
  float upstreamPacketLossRate; // probability between 0 and 1

  void updateUpstreamBwFilter(float bps, float lossRate, float delayMs);
  float upstreamCycleMaxBw; // in bps
  float upstreamBwEst;      // in bps

  void estDownstreamBw();
  float downstreamPacketLossRate; // probability between 0 and 1

  void updateDownstreamBwFilter(float bps, float lossRate, float delayMs);
  float downstreamCycleMaxBw; // in bps
  float downstreamBwEst;      // in bps

  // void sendDownstreamRateMsg();
};

} // namespace MediaNet
