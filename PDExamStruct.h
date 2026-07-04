#pragma once

#include <cstdint>

#pragma pack(1)
struct PDExamStruct
{
	uint16_t Id;
	uint16_t AddItemId;
	uint16_t ParameterAIndex;
	uint8_t ParameterAValue;
	uint8_t AskAbout1;
	uint8_t AskAbout2;
	uint8_t Travel1;
	uint8_t Travel2;
	uint8_t Unknown1;
	uint8_t ExamFileNumber;
	uint8_t ExamEntryNumber;
	int DescriptionOffset;
	uint8_t Flags;
	uint8_t Rate;
	uint8_t Unknown2;
	uint16_t HintState;
};
#pragma pack(8)
