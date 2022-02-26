#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <exception>
#include <limits>

namespace Wp81CodeScanner
{
	class CodaBarReader {
	private:
		std::vector<int> setCounters(uint8_t* pRead, int width);
		int findStartPattern(std::vector<int> counters);
		int toNarrowWidePattern(int counterIndex, std::vector<int> counters);
	public:
		std::string read(uint8_t* pRead, int width);
	};
}