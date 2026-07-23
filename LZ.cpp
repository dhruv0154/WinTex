#include "LZ.h"
#include "Utilities.h"
#include "File.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <string>

BinaryData CLZ::Decompress(uint8_t* pInput, int length)
{
	return Decompress(pInput, 0, length);
}

BinaryData CLZ::Decompress(uint8_t* pInput, int offset, int length)
{
	BinaryData ret;
	ret.Data = NULL;
	ret.Length = 0;

	if (pInput != NULL)
	{
		if (IsCompressed(pInput, offset, length))
		{
			int decompressedLength = GetInt(pInput, offset + 4, 4);
			if (decompressedLength > 0)
			{
				uint8_t* pOutput = new uint8_t[decompressedLength];
				if (pOutput != NULL)
				{
					ret.Length = decompressedLength;
					ret.Data = pOutput;

					int outputPtr = 0;
					int bitsRead = 128;		// Skip headers
					int bitsToRead = 9;
					int mask = 0x1ff;
					int nextCode = 0x102;
					int lastCode = -1;

					int codeOffset[8192];	// Points to where to copy from
					int codeLength[8192];	// Length of code, including "next"
					memset(codeOffset, 0, sizeof(codeOffset));
                    memset(codeLength, 0, sizeof(codeLength));

					while (outputPtr < decompressedLength)
					{
						int code = -1;

						int bitOffset = bitsRead / 8;
						int bitShift = bitsRead & 7;
						int mask = 0x1fff >> (13 - bitsToRead);

						int bytesToRead = std::min(length - bitOffset, 4);
						if (bytesToRead > 0)
						{
							code = GetInt(pInput, offset + bitOffset, bytesToRead);
							code >>= bitShift;
							bitsRead += bitsToRead;

							code &= mask;
						}

						if (code == 0x100)
						{
							// Reset
							bitsToRead = 9;
							mask = 0x1ff;
							nextCode = 0x102;
							lastCode = -1;
						}
						else if (code == 0x101 || code < 0)
						{
							// End of data
							break;
						}
						else if (code >= 0)
						{
							// Decode
							int tmp = outputPtr;

							if (outputPtr < decompressedLength)
							{
								if (code >= 0x102 && code < nextCode)
								{
									int codePtr = codeOffset[code];
									for (int c = 0; c < codeLength[code]; c++)
									{
										if ((outputPtr + c) < decompressedLength)
										{
											pOutput[outputPtr + c] = pOutput[codePtr + c];
										}
									}

									outputPtr += codeLength[code];
								}
								else if (code >= 0 && code <= 0xff)
								{
									pOutput[outputPtr++] = (uint8_t)code;
								}
							}

							// Add code to dictionary
							if (nextCode < 0x2000)
							{
								codeOffset[nextCode] = tmp;
								codeLength[nextCode++] = outputPtr - tmp + 1;
							}

							if ((nextCode - 1) > mask && bitsToRead < 13)
							{
								bitsToRead++;
								mask <<= 1;
								mask |= 1;
							}
						}
					}

					if (outputPtr != decompressedLength)
					{
						int debug = 0;
					}
				}
			}
		}
	}

	return ret;
}

BinaryData CLZ::Decompress(const char* pFileName)
{
	std::string path = gamePath;
	path += pFileName;

	BinaryData ret;
	memset(&ret, 0, sizeof(ret));

	CFile file;
	if (file.Open(path.c_str(), CFile::Mode::Read))
	{
		int fileSize = file.Seek(0, CFile::SeekMethod::End);
		file.Seek(0, CFile::SeekMethod::Begin);

		if (fileSize > 0)
		{
			uint8_t* pInput = new uint8_t[fileSize];
			if (pInput != NULL)
			{
				if (file.Read(pInput, fileSize) == fileSize)
				{
					ret = Decompress(pInput, fileSize);
				}
				delete[] pInput;
			}
		}
		file.Close();
	}
	else
	{
		std::cerr << "Decompress LZ: Could not open file " << pFileName << std::endl;
	}

	return ret;
}

bool CLZ::IsCompressed(uint8_t* pInput, int length)
{
	return IsCompressed(pInput, 0, length);
}

bool CLZ::IsCompressed(uint8_t* pInput, int offset, int length)
{
	int type = GetInt(pInput, offset, 4);
	return (type == 0x01454244 || type == 0x01434341);
}
