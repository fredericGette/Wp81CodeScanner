/**
* See https://github.com/glassechidna/zxing-cpp/blob/master/core/src/zxing/common/GlobalHistogramBinarizer.cpp
*/

#include "pch.h"
#include "Binarizer.h"

void Wp81CodeScanner::Binarizer::binarizeRow(uint8_t* pLum, uint8_t* pBinary, uint8_t* pThrshld, int width)
{
	int localBuckets[32] = {}; // 32=256/8
	
	for (int x = 0; x < width; x++) {
		int luminance = pLum[x] & 0xff;
		// divide value by 8
		localBuckets[luminance >> 3]++;
	}

	int blackPoint = estimateBlackPoint(localBuckets, _countof(localBuckets));

	for (int x = 0; x < width; x++) {
		int luminance = pLum[x] & 0xff;
		// 0 = white bit, 1 = black bit
		pBinary[x] = luminance < blackPoint;
		pThrshld[x] = blackPoint;
	}
}

int Wp81CodeScanner::Binarizer::estimateBlackPoint(int* buckets, int numBuckets) {
	// Find tallest peak in histogram
	int maxBucketCount = 0;
	int firstPeak = 0;
	int firstPeakSize = 0;

	for (int x = 0; x < numBuckets; x++) {
		if (buckets[x] > firstPeakSize) {
			firstPeak = x;
			firstPeakSize = buckets[x];
		}
		if (buckets[x] > maxBucketCount) {
			maxBucketCount = buckets[x];
		}
	}

	// Find second-tallest peak -- well, another peak that is tall and not
	// so close to the first one
	int secondPeak = 0;
	int secondPeakScore = 0;
	for (int x = 0; x < numBuckets; x++) {
		int distanceToBiggest = x - firstPeak;
		// Encourage more distant second peaks by multiplying by square of distance
		int score = buckets[x] * distanceToBiggest * distanceToBiggest;
		if (score > secondPeakScore) {
			secondPeak = x;
			secondPeakScore = score;
		}
	}

	if (firstPeak > secondPeak) {
		int temp = firstPeak;
		firstPeak = secondPeak;
		secondPeak = temp;
	}

	// Kind of arbitrary; if the two peaks are very close, then we figure there is
	// so little dynamic range in the image, that discriminating black and white
	// is too error-prone.
	// Decoding the image/line is either pointless, or may in some cases lead to
	// a false positive for 1D formats, which are relatively lenient.
	// We arbitrarily say "close" is
	// "<= 1/16 of the total histogram buckets apart"
	// std::cerr << "! " << secondPeak << " " << firstPeak << " " << numBuckets << std::endl;
	if (secondPeak - firstPeak <= numBuckets >> 4) {
		throw "Not enough contrast";
	}

	// Find a valley between them that is low and closer to the white peak
	int bestValley = secondPeak - 1;
	int bestValleyScore = -1;
	for (int x = secondPeak - 1; x > firstPeak; x--) {
		int fromFirst = x - firstPeak;
		// Favor a "valley" that is not too close to either peak -- especially not
		// the black peak -- and that has a low value of course
		int score = fromFirst * fromFirst * (secondPeak - x) *
			(maxBucketCount - buckets[x]);
		if (score > bestValleyScore) {
			bestValley = x;
			bestValleyScore = score;
		}
	}

	// Multiply value by 8
	return bestValley << 3;
}


