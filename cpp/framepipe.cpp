#include "framepipe.h"
#include <unordered_map>

static std::recursive_mutex mutex;
static int nextSubscriptionId = 1;
struct Subscription {
	std::vector<std::string> pipeIds;
	FrameCallback onFrame;
	CleanupCallback onCleanup;
};
std::unordered_map<int, Subscription> subscriptions;

int subscribe(const std::vector<std::string> &pipeIds, FrameCallback onFrame,
              CleanupCallback onCleanup) {
	std::lock_guard lock(mutex);
	int subscriptionId = nextSubscriptionId++;
	subscriptions[subscriptionId] = Subscription{pipeIds, onFrame, onCleanup};
	return subscriptionId;
}

void unsubscribe(int subscriptionId) {
	std::lock_guard lock(mutex);
	// Make a copy to avoid iterator invalidation
	auto subscriptionsCopy = subscriptions;
	subscriptions.erase(subscriptionId);

	for (auto &subscription : subscriptionsCopy) {
		if (subscription.first == subscriptionId) {
			if (subscription.second.onCleanup) {
				subscription.second.onCleanup(subscriptionId);
			}
		}
	}
}

void publish(const std::string &pipeId, std::shared_ptr<AVFrame> frame) {
	std::unordered_map<int, Subscription> subscriptionsCopy;
	{
		std::lock_guard lock(mutex);
		// Make a copy to speed up
		subscriptionsCopy = subscriptions;
	}

	for (auto &subscription : subscriptionsCopy) {
		for (const auto &pipeIdInSubscription : subscription.second.pipeIds) {
			if (pipeId == pipeIdInSubscription) {
				int subscriptionId = subscription.first;
				if (subscription.second.onFrame) {
					subscription.second.onFrame(pipeId, subscriptionId, frame);
				}
			}
		}
	}
}
