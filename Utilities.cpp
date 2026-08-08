#include "Utilities.h"
#include "Globals.h"
#include <stdio.h>
#include <stdlib.h>
#include "LZ.h"
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <SDL2/SDL.h>

const int MAX_PATH = 1024;

int GetInt(uint8_t* pData, int offset, int length)
{
	int ret = 0;
	for (int i = length - 1; i >= 0; i--)
	{
		ret <<= 8;
		ret |= pData[offset + i];
	}

	return ret;
}

void SetInt(uint8_t* pData, int offset, int value, int length)
{
	for (int i = 0; i < length; i++)
	{
		pData[offset + i] = (uint8_t)(value & 0xff);
		value >>= 8;
	}
}

char _fileName[MAX_PATH];
int _pathLen = 0;
BinaryData LoadEntry(const char* fileName, int itemIndex)
{
	BinaryData bd;
	memset(&bd, 0, sizeof(bd));

	auto nameLen = strlen(fileName);
	if ((_pathLen + nameLen) < MAX_PATH)
	{
		memcpy(_fileName + _pathLen, fileName, nameLen + 1);

		CFile file;
		if (file.Open(_fileName))
		{
			// TraceLine(L"LoadEntry: Successfully opened file");

			int len = 10 + itemIndex * 4;
			uint8_t* header = new uint8_t[len];

			if (header != nullptr)
			{
				if (file.Read(header, len) == len)
				{
					int count = GetInt(header, 0, 2);
					if (itemIndex >= 0 && itemIndex < (count - 1))
					{
						int offset1 = GetInt(header, 2 + itemIndex * 4, 4);
						int offset2 = GetInt(header, 6 + itemIndex * 4, 4);
						if (offset2 > offset1 && offset1 >= (2 + count * 4))
						{
							int size = offset2 - offset1;
							uint8_t* pBuffer = new uint8_t[size];
							if (pBuffer != nullptr)
							{
								if (file.Seek(offset1, CFile::SeekMethod::Begin) == offset1)
								{
									if (file.Read(pBuffer, size) == size)
									{
										if (CLZ::IsCompressed(pBuffer, size))
										{
											// Decompress
											BinaryData decompressed = CLZ::Decompress(pBuffer, size);
											bd.Data = decompressed.Data;
											bd.Length = decompressed.Length;
											delete[] pBuffer;
										}
										else
										{
											bd.Data = pBuffer;
											bd.Length = size;
										}
									}
									else
									{
										delete[] pBuffer;
									}
								}
							}
						}
					}
				}

				delete[] header;
			}
		}
		else
		{
			std::string err = "Failed to open file ";
			err += fileName;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load AP Entry", err.c_str(), nullptr);
		}
	}

	return bd;
}

DoubleData LoadDoubleEntry(const char* fileName, int itemIndex)
{
	DoubleData dd;
	memset(&dd, 0, sizeof(dd));

	// Read 2 sequential files (typically palette + image)

	auto nameLen = strlen(fileName);
	if ((_pathLen + nameLen) < MAX_PATH)
	{
		memcpy(_fileName + _pathLen, fileName, nameLen + 1);

		CFile file;
		if (file.Open(_fileName))
		{
			int len = 14 + itemIndex * 4;
			uint8_t* header = new uint8_t[len];

			if (header != nullptr)
			{
				if (file.Read(header, len) == len)
				{
					int count = GetInt(header, 0, 2);
					if (itemIndex >= 0 && itemIndex < (count - 1))
					{
						int offset1 = GetInt(header, 2 + itemIndex * 4, 4);
						int offset2 = GetInt(header, 6 + itemIndex * 4, 4);
						int offset3 = GetInt(header, 10 + itemIndex * 4, 4);
						if (offset2 > offset1 && offset3 > offset2 && offset1 >= (2 + count * 4))
						{
							int size1 = offset2 - offset1;
							uint8_t* pBuffer1 = new uint8_t[size1];
							if (pBuffer1 != nullptr)
							{
								int size2 = offset3 - offset2;
								uint8_t* pBuffer2 = new uint8_t[size2];
								if (pBuffer2 != nullptr)
								{
									if (file.Seek(offset1, CFile::SeekMethod::Begin) == offset1)
									{
										if (file.Read(pBuffer1, size1) == size1)
										{
											if (CLZ::IsCompressed(pBuffer1, size1))
											{
												// Decompress
												BinaryData decompressed = CLZ::Decompress(pBuffer1, size1);
												dd.File1.Data = decompressed.Data;
												dd.File1.Length = decompressed.Length;
												delete[] pBuffer1;
											}
											else
											{
												dd.File1.Data = pBuffer1;
												dd.File1.Length = size1;
											}

											if (file.Read(pBuffer2, size2) == size2)
											{
												if (CLZ::IsCompressed(pBuffer2, size2))
												{
													// Decompress
													BinaryData decompressed = CLZ::Decompress(pBuffer2, size2);
													dd.File2.Data = decompressed.Data;
													dd.File2.Length = decompressed.Length;
													delete[] pBuffer2;
												}
												else
												{
													dd.File2.Data = pBuffer2;
													dd.File2.Length = size2;
												}
											}
											else
											{
												delete[] pBuffer2;
											}
										}
										else
										{
											delete[] pBuffer1;
										}
									}
								}
							}
						}
					}
				}

				delete[] header;
			}
		}
		else
		{
			std::string err = "Failed to open file ";
			err += fileName;
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load AP Entry", err.c_str(), nullptr);
		}
	}

	return dd;
}

void SetGamePath(const char* path)
{
	_pathLen = static_cast<int>(strlen(path));
	gamePath = path;

	memcpy(_fileName, path, _pathLen);
}

CCaption* GetFrameCaption(int frame)
{
	std::list<CCaption*>::iterator it = pDisplayCaptions->begin();
	std::list<CCaption*>::iterator end = pDisplayCaptions->end();
	while (it != end && (*it)->Frame() != frame) it++;
	return (it != end) ? *it : nullptr;
}

void Trace(const char* text)
{
	std::cout << text;
}

void Trace(float val, int dc)
{
	if (val < 0 && ((int)val) == 0)
	{
		Trace("-");
	}
	Trace(std::to_string(static_cast<int>(val)).c_str());
	Trace(".");
	if (val < 0.0f) val = -val;
	while (dc-- > 0)
	{
		val -= (int)val;
		val *= 10;
		Trace(std::to_string(static_cast<int>(val)).c_str());
	}
}

void Trace(int val, int rad)
{
	std::stringstream buffer;
	buffer << std::setbase(rad) << val; // std::to_wstring(val);
	Trace(buffer.str().c_str());
}

void TraceLine(const char* text) { Trace(text); Trace("\r\n"); }
void TraceLine(float val, int dc) { Trace(val, dc); Trace("\r\n"); }
void TraceLine(int val, int rad) { Trace(val, rad); Trace("\r\n"); }

uint8_t* GetResource(int resource, const char* type, uint32_t* pSize)
{
    std::string filename;
    switch (resource) {
        case IDR_XML_UAKM: filename = "UAKM.xml"; break;
        case IDR_XML_PD: filename = "PD.xml"; break;
        
        case IDB_SLIDER: filename = "Images/Slider.png"; break;
        case IDB_LISTBOX: filename = "Images/ListBox.png"; break;
        case IDB_IMAGEBUTTON: filename = "Images/ImageButton.png"; break;
        case IDB_BUBBLE: filename = "Images/Bubble.png"; break;
        case IDB_FONT_PD: filename = "Images/PandoraFont.png"; break;
        case IDB_CHECKMARK: filename = "Images/CheckMark.png"; break;
        case IDB_BUTTON_MOUSEOVER: filename = "Images/Button_MouseOver.png"; break;
        case IDB_SAVEGAMEBOX: filename = "Images/SaveGameBox.png"; break;
        case IDB_FRAME: filename = "Images/Frame.png"; break;
        case IDB_IMAGEBUTTON_MOUSEOVER: filename = "Images/ImageButton_MouseOver.png"; break;
        case IDB_FONT_UAKM: filename = "Images/UAKMFont.png"; break;
        case IDB_TABHEADER: filename = "Images/TabHeader.png"; break;
        case IDB_BUTTON: filename = "Images/Button.png"; break;
        
        case IDB_JPG_UAKM_TITLE: filename = "Images/UAKM-Title.jpg"; break;
        case IDB_JPG_PD_TITLE: filename = "Images/PD-Title.jpg"; break;
        
        case IDR_RAWFONT_UAKM: filename = "Images/UAKMFont.bin"; break;
        case IDR_RAWFONT_PD: filename = "Images/PDFont.bin"; break;
        
        case 123: filename = "Sounds/ButtonMouseOver.wav"; break; // IDR_WAVE_BUTTON_MOUSEOVER
        case 124: filename = "Sounds/ButtonClick.wav"; break; // IDR_WAVE_BUTTON_CLICK

        default:
            return nullptr;
    }
    
    // Try to find the file if it doesn't exist
    if (!std::filesystem::exists(filename)) {
        
        // Try to find it recursively in current directory
        try {
            std::string targetName = std::filesystem::path(filename).filename().string();
            for(auto& p: std::filesystem::recursive_directory_iterator(".")) {
                if (p.path().filename() == targetName) {
                    filename = p.path().string();
                    break;
                }
            }
        } catch (...) {}
    }

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return nullptr;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (size <= 0) {
        return nullptr;
    }
    
    // Note: This leaks memory if not freed, but Windows behavior is similar (no free needed)
    // We should probably track it if we want to be clean, but for now we emulate Windows "static" memory
    uint8_t* buffer = new uint8_t[size];
    if (file.read((char*)buffer, size)) {
        *pSize = (uint32_t)size;
        return buffer;
    } else {
        delete[] buffer;
        return nullptr;
    }
}

void ClearCaptions(std::list<CCaption*>* pCap)
{
	std::list<CCaption*>::iterator it = pCap->begin();
	std::list<CCaption*>::iterator end = pCap->end();
	while (it != end)
	{
		delete* (it++);
	}
	pCap->clear();
}

int GetRegistryInt(HKEY key, const char* valueName, int defaultValue)
{
	return defaultValue;
}

void SetRegistryInt(HKEY key, const char* valueName, int value)
{
}

float GetRegistryFloat(HKEY key, const char* valueName, float defaultValue)
{
	return defaultValue;
}

void SetRegistryFloat(HKEY key, const char* valueName, float value)
{
}

void DebugTrace(CScriptState* pState, const char* text)
{
	if (pState->DebugMode)
	{
		Trace(static_cast<int>(reinterpret_cast<uintptr_t>(pState)), 16);
		Trace(" - ");
		Trace(pState->ExecutionPointer - 1, 16);
		Trace(" - ");
		TraceLine(text);
	}
}

void SwapCaptions()
{
	std::list<CCaption*>* pOld = pDisplayCaptions;
	pDisplayCaptions = pAddCaptions;
	pAddCaptions = pOld;
	ClearCaptions(pOld);
}

float From12_4(int v)
{
	return ((float)v) / 16.0f;
}

float From16_16(int v)
{
	return ((float)v) / 65536.0f;
}

std::string IntToString(int value, int size)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%0*i", size, value);
	return std::string(buffer);
}

ActionType& operator|=(ActionType& left, ActionType right)
{
	return left = static_cast<ActionType>(static_cast<int>(left) | static_cast<int>(right));
}

ActionType& operator<<=(ActionType& left, int amount)
{
	return left = static_cast<ActionType>(static_cast<int>(left) << amount);
}

ActionType operator&(ActionType left, ActionType right)
{
	return static_cast<ActionType>(static_cast<int>(left) & static_cast<int>(right));
}

ActionType operator>>(ActionType left, int amount)
{
	return static_cast<ActionType>(static_cast<int>(left) >> amount);
}

int ReadBits(uint8_t* data, int bitsToRead, int& bitOffset)
{
	int byteOffset = bitOffset / 8;
	int bitShift = 8 - bitsToRead - (bitOffset & 7);
	int mask = 0xff >> (8 - bitsToRead);
	int bits = data[byteOffset] | data[byteOffset + 1] << 8;

	bits >>= bitShift;
	bitOffset += bitsToRead;
	bits = bits & mask;

	return bits;
}
