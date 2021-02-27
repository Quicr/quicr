
#include <iostream>
#include <thread>

#include "encode.hh"
#include "qbroadcast.hh"
#include "quicRServer.hh"

using namespace MediaNet;

Connection::Connection(uint32_t randNum, const MediaNet::IpAddr &addr)
    : relaySeqNum(randNum), address(addr) {}

BroadcastRelay::BroadcastRelay(uint16_t port) : qServer() {
  qServer.open(port);
  prevAckSeqNum = 0;
  prevRecvTimeUs = 0;

  std::random_device randDev;
  randomGen.seed(randDev()); // TODO - should use crypto random
  getRandom = std::bind(randomDist, randomGen);
}

void BroadcastRelay::process() {
  std::unique_ptr<Packet> packet = qServer.recv();

  if (!packet) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return;
  }

  if (packet->fullSize() < 1) {
    // log bad packet
    return;
  }

  if (nextTag(packet) == PacketTag::clientData) {
    processAppMessage(packet);
    return;
  }

  if (nextTag(packet) == PacketTag::relayRateReq) {
    // std::clog << std::endl << "Got relayRateReq" << std::endl;
    processRate(packet);
    return;
  }

  std::clog << std::endl
            << "Got bad packet nextTag=" << int(nextTag(packet)) / 256
            << std::endl;
}

void BroadcastRelay::processRate(std::unique_ptr<MediaNet::Packet> &packet) {
  NetRateReq rateReq;
  packet >> rateReq;
  std::clog << std::endl
            << "ReqRate: " << float(rateReq.bitrateKbps) / 1000.0 << " mbps"
            << std::endl;
}

void BroadcastRelay::processAppMessage(
    std::unique_ptr<MediaNet::Packet> &packet) {
  ClientData seqNumTag{};
  packet >> seqNumTag;

  // auto tag = PacketTag::none;
  // packet >> tag;
  auto tag = nextTag(packet);
  if (tag == PacketTag::pubData) {
    return processPub(packet, seqNumTag);
  } else if (tag == PacketTag::subscribeReq) {
    return processSub(packet, seqNumTag);
  }

  std::clog << "Bad App message:" << (int)tag << "\n";
}

void BroadcastRelay::processSub(std::unique_ptr<MediaNet::Packet> &packet,
                                ClientData &clientSeqNumTag) {
  std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration dn = tp.time_since_epoch();
  uint32_t nowUs =
      (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn)
          .count();

  // ack the packet
  auto ack = std::make_unique<Packet>();
  ack->setDst(packet->getSrc());
  ack << PacketTag::headerMagicData;
  NetAck ackTag{};
  ackTag.clientSeqNum = clientSeqNumTag.clientSeqNum;
  ackTag.netRecvTimeUs = nowUs;
  ack << ackTag;

  // save the subscription
  PacketTag tag;
  packet >> tag;
  ShortName name;
  packet >> name;
  std::clog << "Adding Subscription for: " << name << std::endl;
  auto conIndex = connectionMap.find(name);
  if (conIndex == connectionMap.end()) {
    // new connection
    connectionMap[name] =
        std::make_unique<Connection>(getRandom(), packet->getSrc());
  }

  std::unique_ptr<Connection> &con = connectionMap[name];
  con->lastSyn = std::chrono::steady_clock::now();
}

void BroadcastRelay::processPub(std::unique_ptr<MediaNet::Packet> &packet,
                                ClientData &seqNumTag) {
  std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now();
  std::chrono::steady_clock::duration dn = tp.time_since_epoch();
  uint32_t nowUs =
      (uint32_t)std::chrono::duration_cast<std::chrono::microseconds>(dn)
          .count();

  std::clog << ".";

  // save the name for publish
  PacketTag tag;
  packet >> tag;
  ShortName name;
  packet >> name;

  uint16_t payloadSize;
  packet >> payloadSize;
  if (payloadSize > packet->size()) {
    std::clog << "relay recv bad data size " << payloadSize << " "
              << packet->size() << std::endl;
    return;
  }

  auto ack = std::make_unique<Packet>();
  ack->setDst(packet->getSrc());

  ack << PacketTag::headerMagicData;

  // TODO - get rid of prev Ack tag and use ack vector
  if (prevAckSeqNum > 0) {
    NetAck prevAckTag{};
    prevAckTag.clientSeqNum = prevAckSeqNum;
    prevAckTag.netRecvTimeUs = prevRecvTimeUs;
    ack << prevAckTag;
  }

  NetAck ackTag{};
  ackTag.clientSeqNum = seqNumTag.clientSeqNum;
  ackTag.netRecvTimeUs = nowUs;
  ack << ackTag;

  qServer.send(move(ack));

  prevAckSeqNum = ackTag.clientSeqNum;
  prevRecvTimeUs = ackTag.netRecvTimeUs;

  // TODO - loop over connections and remove ones with old last Syn time

  // TODO: push this into server class
  packet << payloadSize;
  packet << name;
  packet << tag;

  // for each connection, make copy and forward
  for (auto const &[addr, con] : connectionMap) {
    auto subData = packet->clone(); // TODO - just clone header stuff

    subData->setDst(con->address);

    RelayData netRelaySeqNum;
    netRelaySeqNum.relaySeqNum = con->relaySeqNum++;
    netRelaySeqNum.remoteSendTimeUs = nowUs;

    subData << netRelaySeqNum;

    // std::clog << "Relay send: " << *subData << std::endl;

    bool simLoss = false;

    if (false) {
      //  simulate 10% packet loss
      if ((netRelaySeqNum.relaySeqNum % 10) == 7) {
        simLoss = true;
      }
    }

    if (!simLoss) {
      qServer.send(move(subData));
      std::clog << "*";
    } else {
      std::clog << "-";
    }
  }
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4068)
#endif

#pragma ide diagnostic ignored "EndlessLoop"
#pragma ide diagnostic ignored "UnreachableCode"

int main() {
  auto qRelay = BroadcastRelay(5004);
  while (true) {
    qRelay.process();
  }
  return 0;
}
