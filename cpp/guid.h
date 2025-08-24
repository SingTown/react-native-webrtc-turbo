#pragma once
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

std::string genUUIDV4() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

	uint32_t d1 = dis(gen);
	uint16_t d2 = dis(gen) & 0xFFFF;
	uint16_t d3 = dis(gen) & 0xFFFF;
	uint16_t d4 = dis(gen) & 0xFFFF;
	uint64_t d5 = (((uint64_t)dis(gen) << 32) | dis(gen));

	d3 = (d3 & 0x0FFF) | 0x4000;
	d4 = (d4 & 0x3FFF) | 0x8000;

	std::ostringstream oss;
	oss << std::hex << std::setfill('0') << std::nouppercase << std::setw(8)
	    << d1 << "-" << std::setw(4) << d2 << "-" << std::setw(4) << d3 << "-"
	    << std::setw(4) << d4 << "-" << std::setw(12)
	    << (d5 & 0xFFFFFFFFFFFFULL);

	return oss.str();
}