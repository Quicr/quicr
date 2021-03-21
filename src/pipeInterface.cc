
#include <cassert>

#include "pipeInterface.hh"

using namespace MediaNet;

PipeInterface::PipeInterface(PipeInterface *t)
    : downStream(t), upStream(nullptr) {}

PipeInterface::~PipeInterface() {
  if (downStream) {
    delete downStream;
    downStream = nullptr;
  }
}

bool PipeInterface::start(const uint16_t port, const std::string server,
                          PipeInterface *upStreamLink) {
  assert(downStream);

  upStream = upStreamLink;
  if (downStream) {
    return downStream->start(port, server, this);
  }

  return true;
}

bool PipeInterface::ready() const {
  if (downStream) {
    return downStream->ready();
  }
  return true;
}

void PipeInterface::stop() {
  if (downStream) {
    downStream->stop();
  }
}

bool PipeInterface::send(std::unique_ptr<Packet> packet) {
  assert(downStream);
  return downStream->send(move(packet));
}

std::unique_ptr<Packet> PipeInterface::recv() {
  assert(downStream);
  return downStream->recv();
}

bool PipeInterface::fromDownstream(std::unique_ptr<Packet> packet) {
  assert(upStream);
  return upStream->fromDownstream(move(packet));
}

void PipeInterface::updateStat(PipeInterface::StatName stat, uint64_t value) {
  if (upStream) {
    upStream->updateStat(stat, value);
  }
}

void PipeInterface::ack(ShortName name) {
  if (upStream) {
    upStream->ack(name);
  }
}

void PipeInterface::updateRTT(uint16_t minRtMs, uint16_t bigRtMs) {
  if (downStream) {
    downStream->updateRTT(minRtMs, bigRtMs);
  }
}

std::unique_ptr<Packet> PipeInterface::toDownstream() {
  return std::unique_ptr<Packet>(nullptr);
}

void PipeInterface::updateMTU(uint16_t mtu, uint32_t pps) {
  if (downStream) {
    downStream->updateMTU(mtu, pps);
  }
}

void PipeInterface::updateBitrateUp(uint64_t minBps, uint64_t startBps,
                                    uint64_t maxBps) {
  if (downStream) {
    downStream->updateBitrateUp(minBps, startBps, maxBps);
  }
}
