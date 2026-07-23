#pragma once

#include "BinaryData.h"

class CLZ
{
public:
	static BinaryData Decompress(uint8_t* pInput, int length);
	static BinaryData Decompress(uint8_t* pInput, int offset, int length);
	static BinaryData Decompress(const char* pFileName);

	static bool IsCompressed(uint8_t* pInput, int length);
	static bool IsCompressed(uint8_t* pInput, int offset, int length);
};
