#pragma once

#include <stdint.h>

namespace Wp81CodeScanner
{
	class Binarizer {
	private:
		int estimateBlackPoint(int* buckets, int numBuckets);
	public:
		void binarizeRow(uint8_t* pLum, uint8_t* pBinary, uint8_t* pThrshld, int width);
	};
}