#pragma once

#ifndef __UTILITIES__
#define __UTILITIES__

#include "AnimBase.h"
#include "BIC.h"
#include "PTF.h"
#include "Wave.h"
#include "H2O.h"
#include "LZ.h"
#include "File.h"
#include "DoubleData.h"

#include <cstdint>
#include <string>
#include <list>

enum EngineResourceID 
{
    IDR_SHADER = 101,
    IDB_JPG_UAKM_TITLE = 102,
    IDR_XML_UAKM = 103,
    IDI_ICON_UAKM = 104,
    IDB_FONT_UAKM = 105,
    IDR_RAWFONT_UAKM = 106,
    IDB_JPG_PD_TITLE = 107,
    IDR_XML_PD = 108,
    IDI_ICON_PD = 109,
    IDB_FONT_PD = 110,
    IDR_RAWFONT_PD = 111,
    IDB_BUTTON = 112,
    IDB_BUTTON_MOUSEOVER = 113,
    IDB_FRAME = 114,
    IDB_IMAGEBUTTON = 115,
    IDB_IMAGEBUTTON_MOUSEOVER = 116,
    IDB_CHECKMARK = 117,
    IDB_BUBBLE = 118,
    IDB_LISTBOX = 119,
    IDB_SAVEGAMEBOX = 120,
    IDB_TABHEADER = 121,
    IDB_SLIDER = 122,
    IDR_WAVE_BUTTON_MOUSEOVER = 123,
    IDR_WAVE_BUTTON_CLICK = 124
};

typedef void* HKEY;

#define TIMER_SCALE	16.66666

int GetInt(uint8_t* pData, int offset, int length);
void SetInt(uint8_t* pData, int offset, int value, int length);

BinaryData LoadEntry(const char* fileName, int itemIndex);
DoubleData LoadDoubleEntry(const char* fileName, int itemIndex);
CCaption* GetFrameCaption(int frame);

void SetGamePath(const char* path);
void Trace(const char* text);
void Trace(float val, int dc = 2);
void Trace(int val, int rad = 10);
void TraceLine(const char* text);
void TraceLine(const char* text);
void TraceLine(float val, int dc = 2);
void TraceLine(int val, int rad = 10);

uint8_t* GetResource(int resource, const char* type, uint32_t* pSize);

void ClearCaptions(std::list<CCaption*>* pCap);

int GetRegistryInt(HKEY key, const char* valueName, int defaultValue);
void SetRegistryInt(HKEY key, const char* valueName, int value);
float GetRegistryFloat(HKEY key, const char* valueName, float defaultValue);
void SetRegistryFloat(HKEY key, const char* valueName, float value);

void DebugTrace(CScriptState* pState, const char* text);

void SwapCaptions();

float From12_4(int v);
float From16_16(int v);

std::string IntToString(int value, int size);

ActionType& operator|=(ActionType& left, ActionType right);
ActionType& operator<<=(ActionType& left, int amount);
ActionType operator&(ActionType left, ActionType right);
ActionType operator>>(ActionType left, int amount);

int ReadBits(uint8_t* data, int bitsToRead, int& bitOffset);

#endif // __UTILITIES__