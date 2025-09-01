#include "MediaStreamTrack.h"

std::unordered_map<std::string, std::shared_ptr<MediaStreamTrack>>
    mediaStreamTrackMap;

std::shared_ptr<MediaStreamTrack> getMediaStreamTrack(const std::string &id) {
	if (auto it = mediaStreamTrackMap.find(id); it != mediaStreamTrackMap.end())
		return it->second;
	else
		throw std::invalid_argument("Stream ID does not exist");
}

std::string emplaceMediaStreamTrack(std::shared_ptr<MediaStreamTrack> ptr) {
	std::string id = genUUIDV4();
	mediaStreamTrackMap.emplace(std::make_pair(id, ptr));
	return id;
}

void eraseMediaStreamTrack(const std::string &id) {
	mediaStreamTrackMap.erase(id);
}
