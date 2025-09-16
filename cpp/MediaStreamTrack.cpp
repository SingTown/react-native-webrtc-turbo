#include "MediaStreamTrack.h"
#include <mutex>

std::mutex mutex;

std::unordered_map<std::string, std::shared_ptr<VideoStreamTrack>>
    videoStreamTrackMap;

std::unordered_map<std::string, std::shared_ptr<AudioStreamTrack>>
    audioStreamTrackMap;

std::shared_ptr<MediaStreamTrack> getMediaStreamTrack(const std::string &id) {
	auto videoStreamTrack = getVideoStreamTrack(id);
	if (videoStreamTrack) {
		return videoStreamTrack;
	}
	auto audioStreamTrack = getAudioStreamTrack(id);
	if (audioStreamTrack) {
		return audioStreamTrack;
	}
	return nullptr;
}

void eraseMediaStreamTrack(const std::string &id) {
	eraseVideoStreamTrack(id);
	eraseAudioStreamTrack(id);
}

std::shared_ptr<VideoStreamTrack> getVideoStreamTrack(const std::string &id) {
	std::lock_guard lock(mutex);
	if (auto it = videoStreamTrackMap.find(id); it != videoStreamTrackMap.end())
		return it->second;
	else
		return nullptr;
}

std::string emplaceVideoStreamTrack(std::shared_ptr<VideoStreamTrack> ptr) {
	std::lock_guard lock(mutex);
	std::string id = genUUIDV4();
	videoStreamTrackMap.emplace(std::make_pair(id, ptr));
	return id;
}

void eraseVideoStreamTrack(const std::string &id) {
	std::lock_guard lock(mutex);
	auto videoStreamTrack = getVideoStreamTrack(id);
	if (videoStreamTrack) {
		videoStreamTrack->onPush(nullptr);
	}
	videoStreamTrackMap.erase(id);
}

std::shared_ptr<AudioStreamTrack> getAudioStreamTrack(const std::string &id) {
	std::lock_guard lock(mutex);
	if (auto it = audioStreamTrackMap.find(id); it != audioStreamTrackMap.end())
		return it->second;
	else
		return nullptr;
}

std::string emplaceAudioStreamTrack(std::shared_ptr<AudioStreamTrack> ptr) {
	std::lock_guard lock(mutex);
	std::string id = genUUIDV4();
	audioStreamTrackMap.emplace(std::make_pair(id, ptr));
	return id;
}

void eraseAudioStreamTrack(const std::string &id) {
	std::lock_guard lock(mutex);
	auto audioStreamTrack = getAudioStreamTrack(id);
	if (audioStreamTrack) {
		audioStreamTrack->onPush(nullptr);
	}
	audioStreamTrackMap.erase(id);
}