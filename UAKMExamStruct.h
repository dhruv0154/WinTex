#pragma once

typedef struct UAKMExamStruct
{
	uint8_t Id;
	uint8_t AddItemId;
	uint8_t ParameterAIndex;
	uint8_t ParameterAValue;
	uint8_t AskAbout1;
	uint8_t AskAbout2;
	uint8_t Travel1;
	uint8_t Travel2;
	uint8_t ExamFileNumber;
	uint8_t ExamEntryNumber;
	int DescriptionOffset;
	uint8_t Flags;
	uint16_t Rate;
	uint16_t HintState;
};
