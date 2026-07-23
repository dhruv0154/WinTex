#include "SQZ.h"

CSQZ::CSQZ()
{
}

CSQZ::~CSQZ()
{
}

BinaryData CSQZ::Decompress(uint8_t* input, int length)
{
	// Must start with '.SQZ'
	// Looks like it is always followed by '.bmp'
	// Compressed size
	// Decompressed size

	BinaryData bd;
	bd.Data = NULL;
	bd.Length = 0;

	if (length >= 16 && 
        input[0] == '.' && input[1] == 'S' && input[2] == 'Q' && input[3] == 'Z' &&
        input[4] == '.' && input[5] == 'b' && input[6] == 'm' && input[7] == 'p')
	{
		int compressedSize = GetInt(input, 8, 4);
		int decompressedSize = GetInt(input, 12, 4);

		uint8_t* output = new uint8_t[decompressedSize];
		if (output != NULL)
		{
			memset(output, 0, decompressedSize);

			bd.Data = output;
			bd.Length = decompressedSize;

			int src = 16;
			int dst = 0;

			while (src < length)
			{
				int chunkSize = GetInt(input, src, 2);
				src += 2;
				if ((chunkSize & 0x8000) != 0)
				{
					// Direct copy
					chunkSize = (-chunkSize) & 0xffff;

					for (int cp = 0; cp < chunkSize; cp++)
					{
						output[dst++] = input[src++];
					}
				}
				else
				{
					// chunkSize bytes are compressed, decompress them...
					int chunkEnd = src + chunkSize;
					while (src < chunkEnd)
					{
						int dataCount = input[src++];
						if ((dataCount & 0x80) != 0)
						{
							// Signed
							int count = 0, offset = 0;
							int lowCount = dataCount & 0x7f;
							int additional = input[src++];
							if ((additional & 0x80) != 0)
							{
								// Signed
								count = (lowCount >> 4) + 3;
								offset = ((additional & 0x7f) << 4) | (lowCount & 0xf);
							}
							else
							{
								// Unsigned
								count = lowCount + 3;
								offset = (additional << 8) | input[src++];
							}

							for (int cp = 0; cp < count; cp++)
							{
								output[dst] = output[dst - offset];
								dst++;
							}
						}
						else
						{
							// Unsigned, copy uncompressed chunk data
							dataCount++;
							for (int cp = 0; cp < dataCount; cp++)
							{
								output[dst++] = input[src++];
							}
						}
					}
				}
			}
		}
	}

	return bd;
}
