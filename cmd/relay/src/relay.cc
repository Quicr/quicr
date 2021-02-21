
#include <chrono>
#include <thread>
#include <random>

#include "../include/relay.hh"
#include "../include/multimap_fib.hh"

using namespace MediaNet;

///
/// Connection
///

Connection::Connection(uint32_t relaySeq, uint64_t cookie_in)
				: relaySeqNum(relaySeq)
				, cookie(cookie_in) {}


Relay::Relay(uint16_t port)
		: transport(* new UdpPipe)
		  , fib(std::make_unique<MultimapFib>()) {
	transport.start(port, "", nullptr);
	std::random_device randDev;
	randomGen.seed(randDev()); // TODO - should use crypto random
	getRandom = std::bind(randomDist, randomGen);
}

void Relay::process() {
	auto packet = transport.recv();

	if(!packet) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		return;
	}

	auto tag = nextTag(packet);

	switch (tag) {
		case PacketTag::sync:
			return processSyn(packet);
		case PacketTag::clientSeqNum:
			return processAppMessage(packet);
		case PacketTag::relayRateReq:
			return processRateRequest(packet);
		default:
			std::clog << "unknown tag :" << (int) tag << "\n";
	}

}


///
/// Private Implementation
///

void Relay::processSyn(std::unique_ptr<MediaNet::Packet> &packet) {
	std::clog << "Got a Syn"
						<< " from=" << IpAddr::toString(packet->getSrc())
						<< " len=" << packet->fullSize() << std::endl;

	NetSyncReq sync = {};
	packet >> sync;

	auto conIndex = connectionMap.find(packet->getSrc());
	if(conIndex != connectionMap.end()) {
		// existing connection
		std::unique_ptr<Connection> &con = connectionMap[packet->getSrc()];
		con->lastSyn = std::chrono::steady_clock::now();
		std::clog << "existing connection\n";
		sendSyncRequest(packet->getSrc(), sync.authSecret);
		return;
	}

	// new connection or retry with cookie
	auto it = cookies.find(packet->getSrc());
	if (it == cookies.end()) {
		// send a reset with retry cookie
		auto cookie = random();
		cookies.emplace(packet->getSrc(),
										std::make_tuple(std::chrono::steady_clock::now(), cookie));

		auto rstPkt = std::make_unique<Packet>();
		NetRstRetry rstRetry{};
		rstRetry.cookie = cookie;
		rstPkt << PacketTag::headerMagicRst;
		rstPkt << rstRetry;
		rstPkt->setDst(packet->getSrc());
		transport.send(std::move(rstPkt));
		std::clog << "new connection attempt, generate cookie:" << cookie << std::endl;
		return;
	}

	// cookie exists, verify it matches
	auto& [when, cookie] = it->second;
	// TODO: verify now() - when is within in acceptable limits
	if (sync.cookie != cookie) {
		// bad cookie, reset the connection
		auto rstPkt = std::make_unique<Packet>();
		rstPkt << PacketTag::headerMagicRst;
		rstPkt->setDst(packet->getSrc());
		transport.send(std::move(rstPkt));
		std::clog << "incorrect cookie: found:" << sync.cookie << ", expected:" << cookie<< std::endl;
		return;
	}

	// good sync new connection
	connectionMap[packet->getSrc()] = std::make_unique<Connection>(getRandom(), cookie);
	cookies.erase(it);
	std::clog << "Added connection\n";
	sendSyncRequest(packet->getSrc(), sync.authSecret);

}

void Relay::processRst(std::unique_ptr<MediaNet::Packet> &packet) {
	auto conIndex = connectionMap.find(packet->getSrc());
	if (conIndex == connectionMap.end()) {
		std::clog << "Reset recieved for unknown connection\n";
		return;
	}
	connectionMap.erase(conIndex);
	std::clog << "Reset recieved for connection: " << IpAddr::toString(packet->getSrc()) << "\n";
}

void Relay::processAppMessage(std::unique_ptr<MediaNet::Packet>& packet) {
	NetClientSeqNum seqNumTag{};
	packet >> seqNumTag;

	//auto tag = PacketTag::none;
	//packet >> tag;
	auto tag = nextTag(packet);
	if(tag == PacketTag::appData) {
		return processPub(packet, seqNumTag);
	} else if(tag  == PacketTag::subscribeReq) {
		return processSub(packet, seqNumTag);
	}

	std::clog << "Bad App message:" << (int) tag << "\n";
}

/// Subscribe Request
void Relay::processSub(std::unique_ptr<MediaNet::Packet> &packet, NetClientSeqNum& clientSeqNumTag) {
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
	ackTag.netAckSeqNum = clientSeqNumTag.clientSeqNum;
	ackTag.netRecvTimeUs = nowUs;
	ack << ackTag;

	// save the subscription
	PacketTag tag;
	packet >> tag;
  ShortName name;
  packet >> name;
  std::clog << "Adding Subscription for: " << name << std::endl;
  fib->addSubscription(name, SubscriberInfo{name, packet->getSrc()});
}

void Relay::processPub(std::unique_ptr<MediaNet::Packet> &packet, NetClientSeqNum& clientSeqNumTag) {
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

	// TODO: refactor ack logic
	auto ack = std::make_unique<Packet>();
	ack->setDst(packet->getSrc());

	ack << PacketTag::headerMagicData;

	// TODO - get rid of prev Ack tag and use ack vector
	if (prevAckSeqNum > 0) {
		NetAck prevAckTag{};
		prevAckTag.netAckSeqNum = prevAckSeqNum;
		prevAckTag.netRecvTimeUs = prevRecvTimeUs;
		ack << prevAckTag;
	}

	NetAck ackTag{};
	ackTag.netAckSeqNum = clientSeqNumTag.clientSeqNum;
	ackTag.netRecvTimeUs = nowUs;
	ack << ackTag;

	transport.send(move(ack));

	prevAckSeqNum = ackTag.netAckSeqNum;
	prevRecvTimeUs = ackTag.netRecvTimeUs;

  // find the matching subscribers
  auto subscribers = fib->lookupSubscription(name);

  // std::clog << "Name:" << name << " has:" << subscribers.size() << " subscribers\n";

  packet << payloadSize;
  packet << name;
  packet << tag;

  for(auto const& subscriber : subscribers) {
		auto subData = packet->clone(); // TODO - just clone header stuff
		auto con = connectionMap.find(subscriber.face);
		assert(con != connectionMap.end());
		subData->setDst(subscriber.face);

		NetRelaySeqNum netRelaySeqNum{};
		netRelaySeqNum.relaySeqNum = con->second->relaySeqNum++;
		netRelaySeqNum.remoteSendTimeUs = nowUs;

		subData << netRelaySeqNum;

		// std::clog << "Relay send: " << subData->size() << std::endl;

		bool simLoss = false;

		if (false) {
			//  simulate 10% packet loss
			if ((netRelaySeqNum.relaySeqNum % 10) == 7) {
				simLoss = true;
			}
		}

		if (!simLoss) {
			transport.send(move(subData));
			std::clog << "*";
		} else {
			std::clog << "-";
		}
	}

}

void Relay::processRateRequest(std::unique_ptr<MediaNet::Packet> &packet) {
	NetRateReq rateReq{};
	packet >> rateReq;
	std::clog << std::endl
						<< "ReqRate: " << float(rateReq.bitrateKbps) / 1000.0 << " mbps"
						<< std::endl;
}

void Relay::stop() {
  assert(0);
  // TODO
}

// TODO: make it static ?
void Relay::sendSyncRequest(const MediaNet::IpAddr& to, uint64_t authSecret) {
	auto syncAckPkt = std::make_unique<Packet>();
	auto syncAck = NetSyncAck{};
	const auto now = std::chrono::system_clock::now();
	const auto duration = now.time_since_epoch();
	syncAck.serverTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	syncAck.authSecret = authSecret;
	syncAckPkt << PacketTag::headerMagicSynAck;
	syncAckPkt << syncAck;
	syncAckPkt->setDst(to);
	transport.send(std::move(syncAckPkt));
}