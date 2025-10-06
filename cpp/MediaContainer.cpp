#include "MediaContainer.h"
#include <mutex>

static std::recursive_mutex mutex;

std::unordered_map<std::string, std::shared_ptr<VideoContainer>>
    videoContainerMap;

std::unordered_map<std::string, std::shared_ptr<AudioContainer>>
    audioContainerMap;

std::shared_ptr<MediaContainer> getMediaContainer(const std::string &id) {
	auto videoContainer = getVideoContainer(id);
	if (videoContainer) {
		return videoContainer;
	}
	auto audioContainer = getAudioContainer(id);
	if (audioContainer) {
		return audioContainer;
	}
	return nullptr;
}

void eraseMediaContainer(const std::string &id) {
	eraseVideoContainer(id);
	eraseAudioContainer(id);
}

std::shared_ptr<VideoContainer> getVideoContainer(const std::string &id) {
	std::lock_guard lock(mutex);
	if (auto it = videoContainerMap.find(id); it != videoContainerMap.end())
		return it->second;
	else
		return nullptr;
}

std::string emplaceVideoContainer(std::shared_ptr<VideoContainer> ptr) {
	std::lock_guard lock(mutex);
	std::string id = genUUIDV4();
	videoContainerMap.emplace(std::make_pair(id, ptr));
	return id;
}

void eraseVideoContainer(const std::string &id) {
	std::lock_guard lock(mutex);
	auto videoContainer = getVideoContainer(id);
	if (videoContainer) {
		videoContainer->onPush(nullptr);
	}
	videoContainerMap.erase(id);
}

std::shared_ptr<AudioContainer> getAudioContainer(const std::string &id) {
	std::lock_guard lock(mutex);
	if (auto it = audioContainerMap.find(id); it != audioContainerMap.end())
		return it->second;
	else
		return nullptr;
}

std::string emplaceAudioContainer(std::shared_ptr<AudioContainer> ptr) {
	std::lock_guard lock(mutex);
	std::string id = genUUIDV4();
	audioContainerMap.emplace(std::make_pair(id, ptr));
	return id;
}

void eraseAudioContainer(const std::string &id) {
	std::lock_guard lock(mutex);
	auto audioContainer = getAudioContainer(id);
	if (audioContainer) {
		audioContainer->onPush(nullptr);
	}
	audioContainerMap.erase(id);
}