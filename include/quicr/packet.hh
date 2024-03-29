#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>
#if defined(__linux__) || defined(__APPLE__)
#include <netinet/in.h>
#include <sys/socket.h>
#elif defined(_WIN32)
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

#include "shortName.hh"

#include "../../src/packetTag.hh"

namespace MediaNet {

class QuicRClient;
class QuicRServer;
class SubscribePipe;
class FragmentPipe;
class EncryptPipe; // Needed to manipulate buffer directly

struct IpAddr
{
  struct sockaddr_in addr;
  socklen_t addrLen;

  static std::string toString(const IpAddr&);
  bool operator<(const IpAddr& rhs) const;
};

static constexpr int QUICR_HEADER_SIZE_BYTES =
  6; // (1) magic + (4) pathToken + (1) tag

class Packet
{
  // friend std::ostream &operator<<(std::ostream &os, const Packet &dt);
  // friend MediaNet::PacketTag MediaNet::nextTag(std::unique_ptr<Packet> &p);

  // friend UdpPipe;
  // friend FecPipe;
  // friend CrazyBitPipe;
  friend QuicRClient;
  friend QuicRServer;
  friend SubscribePipe;
  friend FragmentPipe;
  friend EncryptPipe;

public:
  struct Header
  {
    PacketTag tag;
    uint32_t pathToken;

    Header() {}
    explicit Header(PacketTag tag);
    Header(PacketTag tag, uint32_t token);
  };

  Packet();
  void copy(const Packet& p);
  [[nodiscard]] std::unique_ptr<Packet> clone() const;

  uint8_t& data() { return buffer.at(headerSize); }
  uint8_t& fullData() { return buffer.at(0); }

  [[nodiscard]] size_t size() const;
  [[nodiscard]] size_t fullSize() const { return buffer.size(); }
  void resize(int size) { buffer.resize(headerSize + size); }
  void resizeFull(int size) { buffer.resize(size); }

  void reserve(int s) { buffer.reserve(headerSize + s); }

  void setReliable(bool reliable = true);
  [[nodiscard]] bool isReliable() const;

  void setFEC(bool doFec = true);
  bool getFEC() const;

  uint32_t getPathToken() const;
  void setPathToken(uint32_t token);

  [[nodiscard]] const IpAddr& getSrc() const;
  [[maybe_unused]] void setSrc(const IpAddr& src);
  [[maybe_unused]] [[nodiscard]] const IpAddr& getDst() const;
  void setDst(const IpAddr& dst);

  [[nodiscard]] ShortName shortName() const { return name; };

  void setFragID(uint8_t fragmentID, bool lastFrag);

  void push_back(const std::vector<uint8_t>& data)
  {
    buffer.insert(buffer.end(), data.begin(), data.end());
  }
  void push_back(uint8_t t) { buffer.push_back(t); };
  void pop_back() { buffer.pop_back(); };
  uint8_t back() { return buffer.back(); };
  std::vector<uint8_t> back(uint16_t len)
  {
    assert(len <= buffer.size());
    auto vec = std::vector<uint8_t>(len);
    auto detla = buffer.size() - len;
    std::copy(buffer.begin() + detla, buffer.end(), vec.begin());
    buffer.erase(buffer.begin() + detla, buffer.end());
    return vec;
  }
  [[maybe_unused]] [[nodiscard]] uint8_t getPriority() const;
  void setPriority(uint8_t priority);

  // Handy debugging function
  std::string to_hex();

private:
  std::vector<uint8_t> buffer;
  int headerSize = QUICR_HEADER_SIZE_BYTES;

  MediaNet::ShortName name;

  // 1 is highest (control), 2 critical audio, 3 critical
  // video, 4 important, 5 not important - make enum
  uint8_t priority;

  bool reliable;
  bool useFEC;

  MediaNet::IpAddr src;
  MediaNet::IpAddr dst;
};

bool
operator<(const MediaNet::ShortName& a, const MediaNet::ShortName& b);

} // namespace MediaNet
