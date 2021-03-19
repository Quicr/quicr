
#include <cassert>

#include "quicr/encode.hh"
#include "quicr/quicRClient.hh"

#include "quicr/connectionPipe.hh"
#include "quicr/crazyBitPipe.hh"
#include "quicr/encryptPipe.hh"
#include "quicr/fecPipe.hh"
#include "quicr/fragmentPipe.hh"
#include "quicr/pacerPipe.hh"
#include "quicr/priorityPipe.hh"
#include "quicr/retransmitPipe.hh"
#include "quicr/subscribePipe.hh"
#include "quicr/udpPipe.hh"

using namespace MediaNet;

QuicRClient::QuicRClient()
    : udpPipe(), fakeLossPipe(&udpPipe), crazyBitPipe(&fakeLossPipe),
      connectionPipe(&crazyBitPipe), pacerPipe(&connectionPipe),
      priorityPipe(&pacerPipe), retransmitPipe(&priorityPipe),
      fecPipe(&retransmitPipe), subscribePipe(&fecPipe),
      fragmentPipe(&subscribePipe), encryptPipe(&fragmentPipe),
      statsPipe(&encryptPipe),
      firstPipe(&statsPipe) {

  // TODO - get rid of all other places were defaults get set for mtu, rtt, pps
  firstPipe->updateMTU(1280, 480);
  firstPipe->updateRTT(20, 50);
}

QuicRClient::~QuicRClient() { firstPipe->stop(); }

bool QuicRClient::ready() const { return firstPipe->ready(); }

void QuicRClient::close() {
	shutDown = true;
	firstPipe->stop();
}

void QuicRClient::setCurrentTime(const std::chrono::time_point<std::chrono::steady_clock> &now) {
  firstPipe->timepoint_now(now);
}

void QuicRClient::setCryptoKey(sframe::MLSContext::EpochID epoch,
                               const sframe::bytes &mls_epoch_secret) {
  encryptPipe.setCryptoKey(epoch, mls_epoch_secret);
}

bool QuicRClient::publish(std::unique_ptr<Packet> packet) {
  size_t payloadSize = packet->size();
  assert(payloadSize < 63 * 1200);

  ClientData clientData;
  clientData.clientSeqNum=0;

  NamedDataChunk namedDataChunk;
  namedDataChunk.shortName = packet->shortName();
  namedDataChunk.lifetime = toVarInt(0); // TODO

  DataBlock dataBlock;
  dataBlock.metaDataLen = toVarInt(0);
  dataBlock.dataLen = toVarInt(packet->size());

  packet << dataBlock;
  packet << namedDataChunk;
  packet << clientData;

  return firstPipe->send(move(packet));
}

std::unique_ptr<Packet> QuicRClient::recv() {

  auto packet = std::unique_ptr<Packet>(nullptr);

  bool bad = true;
  while (bad) {

    packet = firstPipe->recv();

    if (!packet) {
      return packet;
    }

    if (packet->fullSize() <= 0) {
    	std::clog << "quicr recv very bad size = " << packet->fullSize() << std::endl;
    	continue;
    }

		if (packet->size() == 0) {
			//std::clog << "quicr recv very zero buffer size, tag = " << ((uint16_t)(nextTag(packet)) >> 8) << std::endl;
			return packet;
		}

    if (nextTag(packet) == PacketTag::header) {
    	// Packet::Header header;
    	// packet >> header;
      //std::clog << "quicr empty message, header tag: " << ((uint16_t)(header.tag) >> 8) << std::endl;
      continue;
    }

    // implies NamedDataChunk
    if (nextTag(packet) != PacketTag::shortName) {
      // TODO log bad data
      std::clog << "quicr recv bad tag: " << (((uint16_t)(nextTag(packet))) >> 8)
                << std::endl;
      continue;
    }

    if (packet->size() < 2) {
      // TODO log bad data
      std::clog << "quicr recv bad size=" << packet->size() << std::endl;
      continue;
    }

    NamedDataChunk namedDataChunk;
    DataBlock dataBlock;

    bool ok = true;

    ok &= packet >> namedDataChunk;
    ok &= packet >> dataBlock;

    if (!ok) {
      // TODO log bad data
      std::clog << "quicr recv bad relay data ="  << std::endl;
      continue;
    }

    assert( dataBlock.metaDataLen == toVarInt(0) ); // TODO implement

    // TODO - set packet lifetime

    packet->name = namedDataChunk.shortName;

    size_t payloadSize = fromVarInt( dataBlock.dataLen );

    if ( payloadSize > packet->size()) {
      std::clog << "quicr recv bad data size " << payloadSize << " "
                << packet->size() << std::endl;
      continue;
    }

    packet->headerSize = (int)(packet->fullSize()) - payloadSize;

    bad = false;
  }

  //std::clog << "QuicR received packet size=" << packet->size() << std::endl;
  return packet;
}

bool QuicRClient::open(uint32_t clientID, const std::string relayName,
                       const uint16_t port, uint64_t token) {
  (void)clientID; // TODO
  (void)token;    // TODO
  connectionPipe.setAuthInfo(clientID, token);


  auto ret = firstPipe->start(port, relayName, nullptr);
  if (ret) {
		// kick off timer thread
		timerThread = std::thread([this]() { this->runTimerThread(); });
	}

  return ret;
}

uint64_t QuicRClient::getTargetUpstreamBitrate() {
  return pacerPipe.getTargetUpstreamBitrate(); // TODO - move to stats
}

std::unique_ptr<Packet> QuicRClient::createPacket(const ShortName &shortName,
                                                  int reservedPayloadSize) {
  auto packet = std::make_unique<Packet>();
  assert(packet);
  packet->name = shortName;
  packet->reserve(reservedPayloadSize + 20); // TODO - tune the 20

	auto hdr = Packet::Header(PacketTag::headerData);
  packet << hdr;
  packet->headerSize = (int)(packet->buffer.size());

  // std::clog << "Create empty packet " << *packet << std::endl;

  return packet;
}

bool QuicRClient::subscribe(ShortName name) {
  return subscribePipe.subscribe(name);
}

void QuicRClient::setPacketsUp(uint16_t pps, uint16_t mtu) {
  assert(firstPipe);
  assert(pps >= 10);
  assert(mtu >= 56);
  firstPipe->updateMTU(mtu, pps);
}

void QuicRClient::setRttEstimate(uint32_t minRttMs, uint32_t bigRttMs) {
  assert(firstPipe);
  if (bigRttMs == 0) {
    bigRttMs = minRttMs * 3 / 2;
  }
  assert(minRttMs > 0);
  assert(bigRttMs > 0);
  firstPipe->updateRTT(minRttMs, bigRttMs);
}

void QuicRClient::setBitrateUp(uint64_t minBps, uint64_t startBps,
                               uint64_t maxBps) {
  assert(firstPipe);
  firstPipe->updateBitrateUp(minBps, startBps, maxBps);
}

///
/// Private implementation
///
void QuicRClient::runTimerThread() {
	while(!shutDown) {
		firstPipe->timepoint_now(std::chrono::steady_clock::now());
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
