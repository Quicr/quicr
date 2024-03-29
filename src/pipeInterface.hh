#pragma once

#include <memory>
#include <string>
#include <chrono>

#include "quicr/packet.hh"

namespace MediaNet {

class PipeInterface
{
public:
  enum struct StatName : uint16_t
  {
    none = 0, // must be first
    mtu,
    minRTTms,
    bigRTTms,
    ppsTargetUp,
    lossPerMillionUp,
    lossPerMillionDown,
    bitrateUp,
    bitrateDown,
    jitterUpMs,
    jitterDownMs,
    bad // must be last
  };

  virtual bool start(uint16_t port, const std::string& server,
                     PipeInterface *upStream);
  [[nodiscard]] virtual bool ready() const;
  virtual void stop();

  virtual bool send(std::unique_ptr<Packet>);

  /// non blocking, return nullptr if no buffer
  virtual std::unique_ptr<Packet> recv();

  virtual bool fromDownstream(std::unique_ptr<Packet>);
  virtual std::unique_ptr<Packet> toDownstream();

  // tells upstream things the stat
  virtual void updateStat(StatName stat, uint64_t value);

  // tells upstream things name was received
  virtual void ack(MediaNet::ShortName name);

  // tells downstream things the current RTT
  virtual void updateRTT(uint16_t minRttMs, uint16_t bigRttMs);

  virtual void updateMTU(uint16_t mtu, uint32_t pps);

  virtual void updateBitrateUp(uint64_t minBps,
                               uint64_t startBps,
                               uint64_t maxBps);

  virtual void
  runUpdates(const std::chrono::time_point<std::chrono::steady_clock>& now);
	virtual ~PipeInterface();

protected:
  explicit PipeInterface(PipeInterface *downStream);

  PipeInterface *nextPipe;
  PipeInterface *prevPipe;
};

} // namespace MediaNet
