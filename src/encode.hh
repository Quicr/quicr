#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "packetTag.hh"
#include "quicr/packet.hh"
#include "quicr/shortName.hh"

namespace MediaNet {

std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, uint64_t val);
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, uint32_t val);
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, uint16_t val);
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, uint8_t val);
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const std::string& val);
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const std::vector<uint8_t>& val);
template<std::size_t SIZE>
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const std::array<uint8_t, SIZE>& val)
{
  for (auto v = val.begin(); v != val.end(); ++v) {
    p << *v;
  }
  return p;
}

bool
operator>>(std::unique_ptr<Packet>& p, uint64_t& val);
bool
operator>>(std::unique_ptr<Packet>& p, uint32_t& val);
bool
operator>>(std::unique_ptr<Packet>& p, uint16_t& val);
bool
operator>>(std::unique_ptr<Packet>& p, uint8_t& val);
bool
operator>>(std::unique_ptr<Packet>& p, std::string& val);
bool
operator>>(std::unique_ptr<Packet>& p, std::vector<uint8_t>& val);
template<std::size_t SIZE>
bool
operator>>(std::unique_ptr<Packet>& p, std::array<uint8_t, SIZE>& val)
{
  bool ok = true;
  for (auto v = val.rbegin(); v != val.rend(); ++v) {
    ok &= (p >> *v);
  }
  return ok;
}

enum class uintVar_t : uint64_t
{
};
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, uintVar_t val);
bool
operator>>(std::unique_ptr<Packet>& p, uintVar_t& val);
uintVar_t toVarInt(uint64_t);
uint64_t fromVarInt(uintVar_t);

PacketTag
nextTag(std::unique_ptr<Packet>& p);
PacketTag
nextTag(uint16_t truncTag);

bool
operator>>(std::unique_ptr<Packet>& p, PacketTag& tag);
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, PacketTag tag);

/* Message Header */
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const Packet::Header& msg);

bool
operator>>(std::unique_ptr<Packet>& p, Packet::Header& msg);

/* ShortName */
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const ShortName& msg);

bool
operator>>(std::unique_ptr<Packet>& p, ShortName& msg);

/* SYNC Request */
struct NetSyncReq
{
  uint32_t cookie;
  std::string origin;
  uint32_t senderId;
  uint64_t clientTimeMs;
  uint64_t supportedFeaturesVec;
};
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const NetSyncReq& msg);
bool
operator>>(std::unique_ptr<Packet>& p, NetSyncReq& msg);

/* SYN ACK */
struct NetSyncAck
{
  uint64_t serverTimeMs;
  uint64_t useFeaturesVec;
};

std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const NetSyncAck& msg);
bool
operator>>(std::unique_ptr<Packet>& p, NetSyncAck& msg);

/* NetReset*/
struct NetReset
{
  uint32_t cookie;
};

struct NetResetRetry
{
  uint32_t cookie;
};

struct NetResetRedirect
{
  uint32_t cookie;
  std::string origin;
  uint16_t port;
};

std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const NetResetRetry& msg);
bool
operator>>(std::unique_ptr<Packet>& p, NetResetRetry& msg);
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const NetResetRedirect& msg);
bool
operator>>(std::unique_ptr<Packet>& p, NetResetRedirect& msg);

/* Rate Request */
struct NetRateReq
{
  uintVar_t bitrateKbps; // in kilo bits pers second
};
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const NetRateReq& msg);
bool
operator>>(std::unique_ptr<Packet>& p, NetRateReq& msg);

/* NetAck */
struct NetAck
{
  uint32_t clientSeqNum;
  uint32_t recvTimeUs;
  uint32_t ackVec;
  uint32_t ecnVec;
};

std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const NetAck& msg);
bool
operator>>(std::unique_ptr<Packet>& p, NetAck& msg);

/* NetNack */
struct NetNack
{
  uint32_t relaySeqNum;
};

std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const NetNack& msg);
bool
operator>>(std::unique_ptr<Packet>& p, NetNack& msg);

/* SubscribeRequest */
struct Subscribe
{
  ShortName name;
};

std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const Subscribe& msg);
bool
operator>>(std::unique_ptr<Packet>& p, Subscribe& msg);

///
/// ClientData
///
struct ClientData
{
  uint32_t clientSeqNum;
};

std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const ClientData& msg);
bool
operator>>(std::unique_ptr<Packet>& p, ClientData& msg);

///
/// RelayData
///
struct RelayData
{
  uint32_t relaySeqNum;
  uint32_t relaySendTimeUs;
};
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const RelayData& msg);
bool
operator>>(std::unique_ptr<Packet>& p, RelayData& msg);

///
/// EncDataBlock
///
struct EncryptedDataBlock
{
  uint8_t authTagLen;
  uintVar_t metaDataLen;
  uintVar_t cipherDataLen; // total len following data including tag and meta
};
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const EncryptedDataBlock& data);
bool
operator>>(std::unique_ptr<Packet>& p, EncryptedDataBlock& msg);

///
/// DataBlock
///
struct DataBlock
{
  uintVar_t metaDataLen;
  uintVar_t dataLen; // total length of follow data including meta
};
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const DataBlock& data);
bool
operator>>(std::unique_ptr<Packet>& p, DataBlock& msg);

///
/// NamedDataChunk and friends
///
struct NamedDataChunk
{
  ShortName shortName;
  uintVar_t lifetime;
};
std::unique_ptr<Packet>&
operator<<(std::unique_ptr<Packet>& p, const NamedDataChunk& msg);
bool
operator>>(std::unique_ptr<Packet>& p, NamedDataChunk& msg);

std::ostream&
operator<<(std::ostream& stream, Packet& packet);

} // namespace MediaNet
