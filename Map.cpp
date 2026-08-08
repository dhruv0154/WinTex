#include "Map.h"
#include "LZ.h"
#include "Utilities.h"
#include <cstring>

CMap::~CMap()
{
    for (auto* entry : _entries)
    {
        delete entry;
    }
    _entries.clear();
}

CMapData* CMap::Get(int entry)
{
    if (entry >= 0 && static_cast<size_t>(entry) < _entries.size())
    {
        return _entries[entry];
    }

    return nullptr;
}

StartupPosition CMap::GetStartupPosition(int index, int entry)
{
    StartupPosition sp{};

    if (index >= 0 && static_cast<size_t>(index) < _entries.size())
    {
        CMapData* pMap = _entries[index];
        if (pMap != nullptr && entry >= 0 && static_cast<size_t>(entry) < pMap->StartupPositions.size())
        {
            sp = pMap->StartupPositions[entry];
        }
    }

    return sp;
}

int CMap::ReadStartupPositions(CMapData* pMapdata, uint8_t* data, int offset, int numberOfStartupPositions, int positionDataStructSize)
{
    if (pMapdata == nullptr || data == nullptr)
    {
        return offset;
    }

    for (int p = 0; p < numberOfStartupPositions; p++)
    {
        int16_t ix = static_cast<int16_t>(GetInt(data, offset, 2));
        int16_t iz = static_cast<int16_t>(GetInt(data, offset + 2, 2));
        int16_t iy = static_cast<int16_t>(GetInt(data, offset + 4, 2));
        int16_t iym = static_cast<int16_t>(GetInt(data, offset + 6, 2));
        int16_t a = static_cast<int16_t>(GetInt(data, offset + 8, 2));

        float x = static_cast<float>(ix) / 16.0f;
        float z = static_cast<float>(iz) / 16.0f;
        float y = static_cast<float>(iy) / 16.0f;
        float ym = static_cast<float>(iym) / 16.0f;
        float dy = (y - ym) / 6.0f;
        float fa = static_cast<float>(a) / 10.0f;

        StartupPosition pos;
        pos.X = -x;
        pos.Y = -y;
        pos.Z = -z;
        pos.InitialEyeLevel = -dy * 6.0f;
        pos.MinYAdj = -dy * 8.0f;
        pos.MaxYAdj = -dy * 1.5f;
        pos.Elevation = -ym;
        pos.Angle = -fa * 3.141592654f / 180.0f;

        pMapdata->StartupPositions.push_back(pos);

        offset += positionDataStructSize;
    }

    return offset;
}