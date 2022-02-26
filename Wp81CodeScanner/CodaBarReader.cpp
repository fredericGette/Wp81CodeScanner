/**
* See https://github.com/glassechidna/zxing-cpp/blob/master/core/src/zxing/oned/CodaBarReader.cpp
*/

#include "pch.h"
#include "CodaBarReader.h"

/**
* These represent the encodings of characters, as patterns of wide and narrow bars. The 7 least-significant bits of
* each int correspond to the pattern of wide and narrow, with 1s representing "wide" and 0s representing narrow.
*/
const int CHARACTER_ENCODINGS[] = {
	0x003, 0x006, 0x009, 0x060, 0x012, 0x042, 0x021, 0x024, 0x030, 0x048, // 0-9
	0x00c, 0x018, 0x045, 0x051, 0x054, 0x015, 0x01A, 0x029, 0x00B, 0x00E, // -$:/.+ABCD
};

/** Characters corresponding to the CHARACTER_ENCODINGS */
char const ALPHABET[] = "0123456789-$:/.+ABCD";

/** official start and end patterns */
const char STARTEND_ENCODING[] = { 'A', 'B', 'C', 'D', 0 };

std::string Wp81CodeScanner::CodaBarReader::read(uint8_t * pRead, int width)
{
	std::string result = "";

	// Convert the 0,1 to counters
	// Each counter indicates the width of a bar or a space
	// The first counter is a space.
	std::vector<int> counters = setCounters(pRead, width);

	// Find the index of the first counter of the start character.
	int startOffset = findStartPattern(counters);
	int nextStart = startOffset;

	std::vector<int> characterIndexes = {};
	do {
		int charOffset = toNarrowWidePattern(nextStart, counters);
		if (charOffset == -1) {
			throw "next character not found";
		}
		// Hack: We store the position in the alphabet table into a
		// StringBuilder, so that we can access the decoded patterns in
		// validatePattern. We'll translate to the actual characters later.
		characterIndexes.push_back(charOffset);
		nextStart += 8;
		// Stop as soon as we see the end character.
		if (characterIndexes.size() > 1 &&
			strchr(STARTEND_ENCODING, ALPHABET[charOffset]) != nullptr) {
			break;
		}
	} while (nextStart < counters.size()); // no fixed end pattern so keep on reading while data is available

	// remove stop/start characters character and check if a long enough string is contained
	if (characterIndexes.size() < 3) {
		throw "Not enough characters";
	}
	characterIndexes.erase(characterIndexes.begin());
	characterIndexes.pop_back();

	// Translate character table offsets to actual characters.
	for (int i : characterIndexes) {
		result += ALPHABET[i];
	}

	return result;
}

/**
* Records the size of all runs of white and black pixels, starting with white.
*/
std::vector<int> Wp81CodeScanner::CodaBarReader::setCounters(uint8_t * pRead, int width)
{
	std::vector<int> counters = {};

	// Start from the first white bit (0 value).
	int x = 0;
	while (x < width && pRead[x]) x++;
	if (x >= width) {
		throw "first white bit not found";
	}
	bool isWhite = true;
	int count = 0;
	for (; x < width; x++) {
		if (pRead[x] ^ isWhite) { // the current pixel has the value of "isWhite"
			count++;
		}
		else { // the current pixel is different than "isWhite"
			counters.push_back(count);
			count = 1; // Count the current pixel
			isWhite = !isWhite;
		}
	}
	counters.push_back(count); // Store the last counter.

	return counters;
}

/**
* Find the index of the first counter of the start character.
*/
int Wp81CodeScanner::CodaBarReader::findStartPattern(std::vector<int> counters)
{
	// Each iteration starts from a black bar
	for (int i = 1; i < counters.size(); i += 2) {
		int charOffset = toNarrowWidePattern(i, counters);
		if (charOffset != -1 && strchr(STARTEND_ENCODING, ALPHABET[charOffset]) != nullptr) {
			// Look for whitespace before start pattern, >= 50% of width of start pattern
			// We make an exception if the whitespace is the first element.
			int patternSize = 0;
			for (int j = i; j < i + 7; j++) {
				patternSize += counters[j];
			}
			if (i == 1 || counters[i - 1] >= patternSize / 2) {
				return i;
			}
		}
	}
	throw "start pattern not found";
}

/**
* Find the index in CHARACTER_ENCODINGS (or ALPHABET) of the character represented by 7 counters.
* @param counterIndex 
*	index of the first counter (a bar).
* @param counters
*   array of counters. Each counter corresponds to the number of pixel of a bar or of a space.
* @return the index of the character or -1 when the counters don't match a character
*/
int Wp81CodeScanner::CodaBarReader::toNarrowWidePattern(int counterIndex, std::vector<int> counters)
{
	int end = counterIndex + 7; // CodaBar character has 7 bits (4 black bars and 3 white spaces).
	if (end >= counters.size()) {
		return -1;
	}

	// Find the width of narrow (min) and wide (max) bars
	int maxBar = 0;
	int minBar = std::numeric_limits<int>::max();
	for (int j = counterIndex; j < end; j += 2) {
		int currentCounter = counters[j];
		if (currentCounter < minBar) {
			minBar = currentCounter;
		}
		if (currentCounter > maxBar) {
			maxBar = currentCounter;
		}
	}
	int thresholdBar = (minBar + maxBar) / 2;

	// Find the width of narrow (min) and wide (max) spaces
	int maxSpace = 0;
	int minSpace = std::numeric_limits<int>::max();
	for (int j = counterIndex + 1; j < end; j += 2) {
		int currentCounter = counters[j];
		if (currentCounter < minSpace) {
			minSpace = currentCounter;
		}
		if (currentCounter > maxSpace) {
			maxSpace = currentCounter;
		}
	}
	int thresholdSpace = (minSpace + maxSpace) / 2;

	// Create the pattern corresponding to the counters.
	// pattern positions (bit 8 is unused): 7=bar,6=space,5=bar,4=space,3=bar,2=space,1=bar
	// 0=narrow, 1=wide
	int bitmask = 1 << 7; 
	int pattern = 0;
	for (int i = 0; i < 7; i++) {
		// Select the correct threshold (bar or space).
		int threshold = (i & 1) == 0 ? thresholdBar : thresholdSpace;
		// move to the next bit (from high to low)
		bitmask >>= 1;
		if (counters[counterIndex + i] > threshold) {
			// Wide bar or space corresponds to a 1 bit in the pattern
			pattern |= bitmask;
		}
	}

	for (int i = 0; i < sizeof(CHARACTER_ENCODINGS); i++) {
		if (CHARACTER_ENCODINGS[i] == pattern) {
			return i;
		}
	}
	return -1;
}
