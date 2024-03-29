#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

#include "pipeInterface.hh"
#include "quicr/packet.hh"

namespace MediaNet {

class FragmentPipe : public PipeInterface
{
public:
  explicit FragmentPipe(PipeInterface* t);

  bool send(std::unique_ptr<Packet> packet) override;

  void updateStat(StatName stat, uint64_t value) override;

  /// non blocking, return nullptr if no buffer
  std::unique_ptr<Packet> recv() override;

  void updateMTU(uint16_t mtu, uint32_t pps) override;

  std::unique_ptr<Packet> processRxPacket(std::unique_ptr<Packet> packet);

private:
  uint16_t mtu;

  std::mutex fragListMutex;
  std::map<MediaNet::ShortName, std::unique_ptr<Packet>> fragList;
};

} // namespace MediaNet
