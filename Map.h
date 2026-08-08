#pragma once

#include <cstdint>
#include <vector>
#include "StartupPosition.h"
#include "MapData.h"

class CMap
{
public:
    virtual ~CMap();
    virtual bool Init() = 0;

    CMapData* Get(int entry);
    StartupPosition GetStartupPosition(int index, int entry);

protected:
    std::vector<CMapData*> _entries;

    int ReadStartupPositions(CMapData* pMapdata, uint8_t* data, int offset, int numberOfStartupPositions, int positionDataStructSize);
};