#pragma once

#include <cstdint>

class CColourTranslationTable
{
public:
	CColourTranslationTable()
	{
		for (int i = 0; i < 64; i++)
		{
			_colourTranslationTable[i] = (uint8_t)(4.04762 * i);
		}
	}

	uint8_t operator[] (int index) const
	{
		return _colourTranslationTable[index];
	}

protected:
	uint8_t _colourTranslationTable[64];
};
