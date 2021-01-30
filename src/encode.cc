#include <cassert>
#include <iostream>

#include "encode.hh"
#include "packet.hh"

using namespace MediaNet;

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const Packet::ShortName &msg) {

    int startSize = p->size();
  p << msg.resourceID; // size = 8
  p << msg.senderID; // size = 4
  p << msg.sourceID; // size = 1
  p << msg.mediaTime; // size = 4
  p << msg.fragmentID; // size = 1
    int endSize = p->size();

    assert( (endSize-startSize) == 18 );

  p << PacketTag::shortName;

  return p;
}

bool operator>>(std::unique_ptr<Packet> &p, Packet::ShortName  &msg)
{
    if (nextTag(p) != PacketTag::shortName) {
        std::cerr << "Did not find expected PacketTag::shortName" << std::endl;
        return false;
    }

    PacketTag tag = PacketTag::none;
    bool ok = true;
    ok &= p >> tag;
    ok &= p >> msg.fragmentID;
    ok &= p >> msg.mediaTime;
    ok &= p >> msg.sourceID;
    ok &= p >> msg.senderID;
    ok &= p >> msg.resourceID;

    if (!ok) {
        std::cerr << "problem parsing shortName" << std::endl;
    }

    return ok;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetClientSeqNum &msg) {
  p << msg.clientSeqNum;
  p << PacketTag::clientSeqNum;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetClientSeqNum &msg) {
  if (nextTag(p) != PacketTag::clientSeqNum) {
    std::cerr << "Did not find expected PacketTag::clientSeqNum" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.clientSeqNum;

  if (!ok) {
    std::cerr << "problem parsing NetClientSeqNum" << std::endl;
  }

  return ok;
}

/********* RelaySeqNum TAG ********/

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetRelaySeqNum &msg) {

  p << msg.relaySeqNum;
  p << msg.remoteSendTimeMs;

  p << PacketTag::relaySeqNum;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetRelaySeqNum &msg) {

  if (nextTag(p) != PacketTag::relaySeqNum) {
    std::cerr << "Did not find expected PacketTag::relaySeqNum" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.remoteSendTimeMs;
  ok &= p >> msg.relaySeqNum;

  if (!ok) {
    std::cerr << "problem parsing NetRelaySeqNum" << std::endl;
  }

  return ok;
}

/********* ACK TAG ***************/

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetAck &msg) {
  p << msg.netAckSeqNum;
  p << msg.netRecvTimeUs;
  p << PacketTag::ack;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetAck &msg) {
  if (nextTag(p) != PacketTag::ack) {
    // std::cerr << "Did not find expected PacketTag::ack" << std::endl;
    return false;
  }

  PacketTag tag = PacketTag::none;
  bool ok = true;
  ok &= p >> tag;
  ok &= p >> msg.netRecvTimeUs;
  ok &= p >> msg.netAckSeqNum;

  if (!ok) {
    std::cerr << "problem parsing NetAck" << std::endl;
  }

  return ok;
}

/*************** TAG types *************************/

PacketTag MediaNet::nextTag(std::unique_ptr<Packet> &p) {
    if (p->buffer.empty()) {
        return PacketTag::none;
    }
    uint8_t trucTag = p->buffer.back(); // TODO - support varint size tags
    return nextTag( trucTag);
}

PacketTag MediaNet::nextTag(uint16_t trucTag) {
  PacketTag tag = PacketTag::badTag;
  switch (trucTag) {
  case packetTagTrunc(PacketTag::none):
    tag = PacketTag::none;
    break;

      case packetTagTrunc(PacketTag::appData):
          tag = PacketTag::appData;
          break;
      case packetTagTrunc(PacketTag::appDataFrag):
          tag = PacketTag::appDataFrag;
          break;
  case packetTagTrunc(PacketTag::clientSeqNum):
    tag = PacketTag::clientSeqNum;
    break;
  case packetTagTrunc(PacketTag::ack):
    tag = PacketTag::ack;
    break;

  case packetTagTrunc(PacketTag::sync):
    tag = PacketTag::sync;
    break;
  case packetTagTrunc(PacketTag::shortName):
    tag = PacketTag::shortName;
    break;
  case packetTagTrunc(PacketTag::relaySeqNum):
    tag = PacketTag::relaySeqNum;
    break;
  case packetTagTrunc(PacketTag::relayRateReq):
    tag = PacketTag::relayRateReq;
    break;

  case packetTagTrunc(PacketTag::headerMagicData):
    tag = PacketTag::headerMagicData;
    break;
  case packetTagTrunc(PacketTag::headerMagicSyn):
    tag = PacketTag::headerMagicSyn;
    break;
  case packetTagTrunc(PacketTag::headerMagicRst):
    tag = PacketTag::headerMagicRst;
    break;
  case packetTagTrunc(PacketTag::headerMagicDataCrazy):
    tag = PacketTag::headerMagicDataCrazy;
    break;
  case packetTagTrunc(PacketTag::headerMagicRstCrazy):
    tag = PacketTag::headerMagicRstCrazy;
    break;
  case packetTagTrunc(PacketTag::headerMagicSynCrazy):
    tag = PacketTag::headerMagicSynCrazy;
    break;

  case packetTagTrunc(PacketTag::extraMagicVer1):
    tag = PacketTag::extraMagicVer1;
    break;

  case packetTagTrunc(PacketTag::badTag):
    tag = PacketTag::badTag;
    break;
  default:
    assert(0); // TODO remove
    tag = PacketTag::badTag;
  }

  return tag;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              PacketTag tag) {
  uint16_t t = MediaNet::packetTagTrunc(tag);
  assert(t < 127); // TODO var len encode
  p->buffer.push_back((uint8_t)t);
  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, PacketTag &tag) {
  if (p->buffer.empty()) {
    tag = PacketTag::none;
    return false;
  }

  tag = MediaNet::nextTag(p);

  p->buffer.pop_back();
  return true;
}

/******************    atomic types *************************/

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint64_t val) {
  // TODO - memcpy version for little endian machines optimization

  // buffer on wire is little endian (that is *not* network byte order)
  p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 8) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 16) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 24) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 32) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 40) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 48) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 56) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint32_t val) {
  // buffer on wire is little endian (that is *not* network byte order)
  p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 8) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 16) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 24) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint16_t val) {
  // buffer on wire is little endian (that is *not* network byte order)
  p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
  p->buffer.push_back(uint8_t((val >> 8) & 0xFF));

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              uint8_t val) {
  p->buffer.push_back(val);

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uint64_t &val) {
  uint8_t byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  bool ok = true;
  ok &= p >> byte[7];
  ok &= p >> byte[6];
  ok &= p >> byte[5];
  ok &= p >> byte[4];
  ok &= p >> byte[3];
  ok &= p >> byte[2];
  ok &= p >> byte[1];
  ok &= p >> byte[0];

  val = (uint64_t(byte[0]) << 0) + (uint64_t(byte[1]) << 8) +
        (uint64_t(byte[2]) << 16) + (uint64_t(byte[3]) << 24) +
        (uint64_t(byte[4]) << 32) + (uint64_t(byte[5]) << 40) +
        (uint64_t(byte[6]) << 48) + (uint64_t(byte[7]) << 56);

  return ok;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uint32_t &val) {
  uint8_t byte[4] = {0, 0, 0, 0};

  bool ok = true;
  ok &= p >> byte[3];
  ok &= p >> byte[2];
  ok &= p >> byte[1];
  ok &= p >> byte[0];

  val = (byte[3] << 24) + (byte[2] << 16) + (byte[1] << 8) + (byte[0] << 0);

  return ok;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uint16_t &val) {
  uint8_t byte[2] = {0, 0};

  bool ok = true;
  ok &= p >> byte[1];
  ok &= p >> byte[0];

  val = (byte[1] << 8) + (byte[0] << 0);

  return ok;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uint8_t &val) {
  if (p->buffer.empty()) {
    return false;
  }
  val = p->buffer.back();
  p->buffer.pop_back();
  return true;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetSyncReq &msg) {
  p << msg.senderId;
  p << msg.clientTimeMs;
  p << msg.versionVec;

  p << PacketTag::sync;

  return p;
}

std::unique_ptr<Packet> &MediaNet::operator<<(std::unique_ptr<Packet> &p,
                                              const NetRateReq &msg) {
  p << msg.bitrateKbps;
  p << PacketTag::relayRateReq;

  return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, NetRateReq &msg) {
  if (nextTag(p) != PacketTag::relayRateReq) {
    // std::cerr << "Did not find expected PacketTag::relayRateReq" <<
    // std::endl;
    return false;
  }

  PacketTag tag = PacketTag::relayRateReq;
  bool ok = true;

  ok &= p >> tag;
  ok &= p >> msg.bitrateKbps;

  if (!ok) {
    std::cerr << "problem parsing relayRateReq" << std::endl;
  }

  return ok;
}


std::unique_ptr<Packet>& MediaNet::operator<<(std::unique_ptr<Packet> &p, uintVar_t v)
{
    uint64_t val = fromVarInt(v);

    assert( val < ( (uint64_t)1<<61) );

    if ( val <= ((uint64_t)1<<7) ) {
        p->buffer.push_back(uint8_t( ((val >> 0) & 0x7F)) | 0x00 );
        return p;
    }

    if ( val <= ((uint64_t)1<<14) ) {
        p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
        p->buffer.push_back(uint8_t( ((val >> 8) & 0x3F) | 0x80 ) );
        return p;
    }

    if ( val <= ((uint64_t)1<<29) ) {
        p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
        p->buffer.push_back(uint8_t((val >> 8) & 0xFF));
        p->buffer.push_back(uint8_t((val >> 16) & 0xFF));
        p->buffer.push_back(uint8_t( ((val >> 24) & 0x1F) | 0x80 | 0x40 ) );
        return p;
    }

    p->buffer.push_back(uint8_t((val >> 0) & 0xFF));
    p->buffer.push_back(uint8_t((val >> 8) & 0xFF));
    p->buffer.push_back(uint8_t((val >> 16) & 0xFF));
    p->buffer.push_back(uint8_t((val >> 24) & 0xFF));
    p->buffer.push_back(uint8_t((val >> 32) & 0xFF));
    p->buffer.push_back(uint8_t((val >> 40) & 0xFF));
    p->buffer.push_back(uint8_t((val >> 48) & 0xFF));
    p->buffer.push_back(uint8_t( ((val >> 56) & 0x0F) | 0x80 | 0x40 | 0x20 ) );

    return p;
}

bool MediaNet::operator>>(std::unique_ptr<Packet> &p, uintVar_t &v) {
    uint8_t byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    bool ok = true;

    if (p->buffer.empty()) {
        return false;
    }
    uint8_t first = p->buffer.back();

    if ((first & (0x80)) == 0 ) {
        ok &= p >> byte[0];
        uint8_t val = ( (byte[0]& 0x7F) << 0);
        v = toVarInt(val);
        return ok;
    }

    if ((first & (0x80 | 0x40)) == 0x80) {
        ok &= p >> byte[1];
        ok &= p >> byte[0];
        uint16_t val = ( ( (uint16_t)byte[1] & 0x3F) << 8)
                        + ( (uint16_t)byte[0] << 0);
        v = toVarInt(val);
        return ok;
    }

    if ((first & (0x80 | 0x40 | 0x20) ) == (0x80|0x40) ) {
        ok &= p >> byte[3];
        ok &= p >> byte[2];
        ok &= p >> byte[1];
        ok &= p >> byte[0];
        uint32_t val = ( (uint32_t)(byte[3] & 0x1F) << 24)
                + ( (uint32_t)byte[2] << 16)
                + ( (uint32_t)byte[1] << 8)
                + ( (uint32_t)byte[0] << 0);
        v = toVarInt(val);
        return ok;
    }

    ok &= p >> byte[7];
    ok &= p >> byte[6];
    ok &= p >> byte[5];
    ok &= p >> byte[4];
    ok &= p >> byte[3];
    ok &= p >> byte[2];
    ok &= p >> byte[1];
    ok &= p >> byte[0];
    uint64_t val = ( (uint64_t)(byte[3] & 0x0F) << 56)
            + ((uint64_t)(byte[2]) << 48)
            + ((uint64_t)(byte[1]) << 40)
            + ((uint64_t)(byte[0]) << 32)
            + ((uint64_t)(byte[2]) << 24)
            + ((uint64_t)(byte[2]) << 16)
            + ((uint64_t)(byte[1]) << 8)
            + ( (uint64_t)(byte[0]) << 0);
    v = toVarInt(val);
    return ok;
}

uintVar_t MediaNet::toVarInt( uint64_t v)
{
    assert( v < ( (uint64_t)0x1 <<61 ));
    return static_cast<uintVar_t>(v);
}

uint64_t MediaNet::fromVarInt(  uintVar_t v )
{
    return static_cast<uint64_t >(v);
}


std::ostream& MediaNet::operator<<(std::ostream& stream, const Packet& packet)
{
    int ptr = packet.buffer.size()-1;
    while( ptr >= 0 ) {
        const uint8_t* data = packet.buffer.data();
        MediaNet::PacketTag tag = nextTag( data[ptr--] );
        uint16_t len = ((uint16_t)tag) & 0xFF;
        if ( len == 255 ) {
            if ( ptr >= 2 ) {
                uint16_t lenHigh = data[ptr--];
                uint16_t lenLow = data[ptr--];
                len = (lenHigh<<8) + lenLow;
            }
        }

        ptr -= len;

        switch (tag) {
            case PacketTag::headerMagicData:
            case PacketTag::headerMagicDataCrazy:
                stream << " magicData";
                break;
            case PacketTag::headerMagicSyn:
            case PacketTag::headerMagicSynCrazy:
                stream << " magicSync";
                break;
            case PacketTag::headerMagicRst:
            case PacketTag::headerMagicRstCrazy:
                stream << " magicReset";
                break;
            case PacketTag::shortName:
                stream << " shortName";
                break;
            case PacketTag::appData:
                stream << " appData(" << len << ")";
                break;
            case PacketTag::appDataFrag:
                stream << " appDataFrag(" << len << ")";
                break;
            default:
                stream << " tag:" << (((uint16_t)tag) >>8) << "(" << len << ")";
        }
        if ( ((uint16_t)tag) == 0 ) { ptr=-1; }
    }
    return stream;
}

