#include "../include/multimap_fib.hh"
#include <iostream>

void MultimapFib::addSubscription(const MediaNet::ShortName& name, SubscriberInfo subscriberInfo) {
	fibStore.insert(std::make_pair(name, std::move(subscriberInfo)));
	std::clog << name << "-has-" << fibStore.count(name) << "\n";
}

void MultimapFib::removeSubscription(const MediaNet::ShortName& name, const SubscriberInfo& subscriberInfo) {
	if(!fibStore.count(name)) {
		return;
	}

	auto entries = fibStore.equal_range(name);
	auto it = std::find_if(entries.first, entries.second,
												[&subscriberInfo](auto const& entry){
		return MediaNet::IpAddr::toString(entry.second.face)
		== MediaNet::IpAddr::toString(subscriberInfo.face);
	});

	if(it != fibStore.end()) {
		fibStore.erase(it);
	}
}

std::list<SubscriberInfo> MultimapFib::lookupSubscription(const MediaNet::ShortName& name) const{
	// this logic is too trivial, but starting here for now.
	auto result = std::list<SubscriberInfo>{};
	auto lookup = [&](const MediaNet::ShortName& name) {
		auto entries = fibStore.equal_range(name);
		std::for_each(entries.first, entries.second, [&result](auto& entry)
		{
			result.push_back(entry.second);
		});
	};

	assert(name.resourceID);

	lookup(MediaNet::ShortName{name.resourceID});

	if(name.senderID)
		lookup(MediaNet::ShortName{name.resourceID, name.senderID});

	if(name.sourceID)
		lookup(MediaNet::ShortName{name.resourceID, name.senderID, name.sourceID});

	return result;

}