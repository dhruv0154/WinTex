#pragma once
#include "Utilities.h"

class CSQZ
{
public:
	CSQZ();
	~CSQZ();

	static BinaryData Decompress(uint8_t* input, int length);
};
