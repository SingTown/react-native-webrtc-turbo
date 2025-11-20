#include "framepipe.h"
#include <unordered_map>

static std::recursive_mutex mutex;
static int nextSubscriptionId = 1;
struct Subscription {
	std::string pipeId;
	FrameCallback onFrame;
	CleanupCallback onCleanup;
};
std::unordered_map<int, Subscription> subscriptions;

int subscribe(const std::string &pipeId, FrameCallback onFrame,
              CleanupCallback onCleanup) {
	std::lock_guard lock(mutex);
	int subscriptionId = nextSubscriptionId++;
	subscriptions[subscriptionId] = Subscription{pipeId, onFrame, onCleanup};
	return subscriptionId;
}

void unsubscribe(int subscriptionId) {
	std::unordered_map<int, Subscription> subscriptionsCopy;
	{
		std::lock_guard lock(mutex);
		subscriptionsCopy = subscriptions;
		subscriptions.erase(subscriptionId);
	}
	for (auto &subscription : subscriptionsCopy) {
		if (subscription.first == subscriptionId) {
			if (subscription.second.onCleanup) {
				subscription.second.onCleanup();
			}
		}
	}
}

void publish(const std::string &pipeId, std::shared_ptr<AVFrame> frame) {

	std::unordered_map<int, Subscription> subscriptionsCopy;
	{
		std::lock_guard lock(mutex);
		subscriptionsCopy = subscriptions;
	}

	for (auto &subscription : subscriptionsCopy) {
		if (subscription.second.pipeId == pipeId) {
			subscription.second.onFrame(frame);
		}
	}
}
