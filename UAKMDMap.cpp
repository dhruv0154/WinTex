#include "UAKMDMap.h"
#include "BinaryData.h"
#include "LZ.h"
#include "Utilities.h"
#include <cstdint>

bool CUAKMDMap::Init()
{
    BinaryData dmap = CLZ::Decompress("DMAP.LZ");

    uint8_t* data = dmap.Data;
    if (data != nullptr && dmap.Length > 0)
    {
        for (int i = 0; i < 64; i++)
        {
            CMapData* pMD = new CMapData();

            int ptr = GetInt(data, i * 4, 4);
            if (ptr > 0 && ptr < dmap.Length)
            {
                for (int j = 0; j < 10; j++)
                {
                    if (ptr + 2 > dmap.Length) break;
                    int w = GetInt(data, ptr, 2);
                    ptr += 2;
                    if (w == 0xffff) break;
                }

                for (int j = 0; j < 99; j++)
                {
                    if (ptr + 2 > dmap.Length) break;
                    int w = GetInt(data, ptr, 2);
                    ptr += 2;
                    if (w == 0xffff) break;
                }

                ptr += 2;

                if (ptr + 4 <= dmap.Length)
                {
                    int w1 = GetInt(data, ptr, 2);
                    ptr += 2;
                    int w2 = GetInt(data, ptr, 2);
                    ptr += 2;
                    if (w1 != 0xffff)
                    {
                        ptr += 4;
                    }
                    else
                    {
                        ptr += 4;
                    }
                }

                while (ptr < dmap.Length)
                {
                    int a = data[ptr++];
                    if (a == 0xff) break;

                    if (ptr + 4 > dmap.Length) break;
                    int a1 = GetInt(data, ptr, 2);
                    ptr += 2;
                    int a2 = GetInt(data, ptr, 2);
                    ptr += 2;
                }

                if (ptr + 4 <= dmap.Length)
                {
                    int f1 = GetInt(data, ptr, 2);
                    ptr += 2;
                    int f2 = GetInt(data, ptr, 2);
                    ptr += 2;
                    if (f1 != 0xffff)
                    {
                    }
                }

                if (ptr + 4 <= dmap.Length)
                {
                    pMD->ScriptFileIndex = GetInt(data, ptr, 2);
                    ptr += 2;
                    pMD->ScriptFileEntry = GetInt(data, ptr, 2);
                    ptr += 2;
                }

                while (ptr + 2 <= dmap.Length)
                {
                    int fi1 = GetInt(data, ptr, 2);
                    ptr += 2;
                    if (fi1 == 0xffff) break;

                    if (ptr + 2 > dmap.Length) break;
                    int fe1 = GetInt(data, ptr, 2);
                    ptr += 2;

                    FileMap fm;
                    fm.File = fi1;
                    fm.Entry = fe1;
                    pMD->VideoMap.push_back(fm);

                    if (ptr + 4 > dmap.Length) break;
                    int fi2 = GetInt(data, ptr, 2);
                    ptr += 2;
                    int fe2 = GetInt(data, ptr, 2);
                    ptr += 2;

                    FileMap afm;
                    afm.File = fi2;
                    afm.Entry = fe2;
                    pMD->AudioMap.push_back(afm);
                }

                while (ptr + 2 <= dmap.Length)
                {
                    FileMap fm;
                    fm.File = GetInt(data, ptr, 2);
                    ptr += 2;
                    if (fm.File == 0xffff) break;

                    if (ptr + 2 > dmap.Length) break;
                    fm.Entry = GetInt(data, ptr, 2);
                    ptr += 2;
                    pMD->ImageMap.push_back(fm);
                }
            }

            _entries.push_back(pMD);
        }

        delete[] dmap.Data;

        return true;
    }

    return false;
}