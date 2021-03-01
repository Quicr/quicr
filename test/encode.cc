#include <doctest/doctest.h>
#include <memory>

#include "encode.hh"
#include "name.hh"
#include "packet.hh"

using namespace MediaNet;

///
/// Atomic Types encode/decode tests
///

TEST_CASE("string encode/decode") {
  std::string origin_in = "This is a string with : , * () + - !";
  auto packet = std::make_unique<Packet>();
  std::string origin_out;
  packet >> origin_out;
  CHECK_EQ(origin_in, origin_in);
}

TEST_CASE("ShortName encode/decode") {
  ShortName name{};
  name.resourceID = 0xAA;
  name.senderID = 0xBB;
  name.sourceID = 0xCC;
  name.mediaTime = 0xDD;
  name.fragmentID = 0xEE;

  auto packet = std::make_unique<Packet>();
  packet << name;

  ShortName name_out{};
  packet >> name_out;

  CHECK_EQ(name, name_out);
}

TEST_CASE("RelayData encode/decode") {
  auto packet = std::make_unique<Packet>();

  RelayData data_in{};
  data_in.relaySeqNum = 0x1000;
  data_in.relaySendTimeUs = 0;

  packet << data_in;

  RelayData data_out{};
  packet >> data_out;

  CHECK_EQ(data_in.relaySendTimeUs, data_out.relaySendTimeUs);
  CHECK_EQ(data_in.relaySeqNum, data_out.relaySeqNum);
}

TEST_CASE("ClientData encode/decode") {
  auto packet = std::make_unique<Packet>();

  ClientData data_in{};
  data_in.clientSeqNum = 0x1000;

  packet << data_in;

  ClientData data_out{};
  packet >> data_out;

  CHECK_EQ(data_in.clientSeqNum, data_out.clientSeqNum);
}

///
/// Protocol messages encode/decode
///
// TODO: match hex encoding

TEST_CASE("NetSyncReq encode/decode") {
  NetSyncReq req_in;
  req_in.cookie = 0xC001E;
  req_in.origin = "example.com";
  req_in.senderId = 0x1234;
  req_in.supportedFeaturesVec = 0xABCD;
  req_in.clientTimeMs = 0xA1B1C1D1;

  auto packet = std::make_unique<Packet>();
  packet << req_in;

  NetSyncReq req_out;
  packet >> req_out;

  REQUIRE(req_in.cookie == req_out.cookie);
  REQUIRE(req_in.origin == req_out.origin);
  REQUIRE(req_in.senderId == req_out.senderId);
  REQUIRE(req_in.supportedFeaturesVec == req_out.supportedFeaturesVec);
  REQUIRE(req_in.clientTimeMs == req_out.clientTimeMs);
}

TEST_CASE("NetSyncAck encode/decode") {
  NetSyncAck ack_in;
  ack_in.useFeaturesVec = 0x1111;
  ack_in.serverTimeMs = 0x2222;

  auto packet = std::make_unique<Packet>();
  packet << ack_in;

  NetSyncAck ack_out;
  packet >> ack_out;

  CHECK_EQ(ack_in.serverTimeMs, ack_out.serverTimeMs);
  CHECK_EQ(ack_in.useFeaturesVec, ack_out.useFeaturesVec);
}

TEST_CASE("NetRstRetry encode/decode") {
  NetRstRetry retry_in;
  retry_in.cookie = 0x1234;

  auto packet = std::make_unique<Packet>();
  packet << retry_in;

  NetRstRetry retry_out;
  packet >> retry_out;

  CHECK_EQ(retry_in.cookie, retry_out.cookie);
}

TEST_CASE("NetRstRedirect encode/decode") {
  NetRstRedirect redirect_in;
  redirect_in.port = 0x1000;
  redirect_in.origin = "example.com";
  redirect_in.cookie = 0x1234;

  auto packet = std::make_unique<Packet>();
  packet << redirect_in;

  NetRstRedirect redirect_out;
  packet >> redirect_out;

  CHECK_EQ(redirect_in.cookie, redirect_out.cookie);
  CHECK_EQ(redirect_in.origin, redirect_out.origin);
  CHECK_EQ(redirect_in.port, redirect_out.port);
}

TEST_CASE("NetRateReq encode/decode") {
  NetRateReq req_in;
  req_in.bitrateKbps = toVarInt(0x1000);

  auto packet = std::make_unique<Packet>();
  packet << req_in;

  NetRateReq req_out;
  packet >> req_out;

  CHECK_EQ(req_in.bitrateKbps, req_out.bitrateKbps);
}

TEST_CASE("NetAck encode/decode") {
  NetAck ack_in;
  ack_in.ecnVec = 0x1;
  ack_in.ackVec = 0x4;
  ack_in.clientSeqNum = 0x1000;
  ack_in.recvTimeUs = 0x2000;

  auto packet = std::make_unique<Packet>();
  packet << ack_in;

  NetAck ack_out;
  packet >> ack_out;

  CHECK_EQ(ack_in.ecnVec, ack_out.ecnVec);
  CHECK_EQ(ack_in.ackVec, ack_out.ackVec);
  CHECK_EQ(ack_in.recvTimeUs, ack_out.recvTimeUs);
  CHECK_EQ(ack_in.clientSeqNum, ack_out.clientSeqNum);
}

TEST_CASE("PubData encode/decode") {
  auto cipherText = std::vector<uint8_t>{0x1, 0x2, 0x3, 0x4, 0x5, 0xA};
  PubData data_in;
  data_in.name = ShortName(1, 2, 3);
  data_in.lifetime = toVarInt(0x1000);
  data_in.encryptedDataBlock = EncryptedDataBlock{1, cipherText};

  auto packet = std::make_unique<Packet>();
  // TODO packet << ciperText;
  packet << data_in;

  PubData data_out{};
  packet >> data_out;

  CHECK(data_in.name == data_out.name);
  CHECK_EQ(data_in.lifetime, data_out.lifetime);
  CHECK_EQ(data_in.encryptedDataBlock.authTagLen,
           data_out.encryptedDataBlock.authTagLen);
  CHECK_EQ(data_in.encryptedDataBlock.cipherText,
           data_out.encryptedDataBlock.cipherText);
}

TEST_CASE("PSubData encode/decode") {
  auto cipherText = std::vector<uint8_t>{0x1, 0x2, 0x3, 0x4, 0x5, 0xA};
  SubData data_in;
  data_in.name = ShortName(1, 2, 3);
  data_in.lifetime = toVarInt(0x1000);
  data_in.encryptedDataBlock = EncryptedDataBlock{1, cipherText};

  auto packet = std::make_unique<Packet>();
  packet << data_in;

  SubData data_out{};
  packet >> data_out;

  CHECK(data_in.name == data_out.name);
  CHECK_EQ(data_in.lifetime, data_out.lifetime);
  CHECK_EQ(data_in.encryptedDataBlock.authTagLen,
           data_out.encryptedDataBlock.authTagLen);
  CHECK_EQ(data_in.encryptedDataBlock.cipherText,
           data_out.encryptedDataBlock.cipherText);
}
