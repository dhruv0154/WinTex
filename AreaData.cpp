#include "AreaData.h"

CAreaData::CAreaData()
{
    CategoryOptionOffsets = nullptr;
    CategoryOptionCounts = nullptr;
    CategoryAreas = nullptr;
    Table2 = nullptr;
    Table3 = nullptr;
    Table4 = nullptr;
}

CAreaData::~CAreaData()
{
}

void CAreaData::Init(uint8_t* data)
{
    if (data == nullptr) return;

    CategoryOptionOffsets = data;
    CategoryOptionCounts = data + 6;
    CategoryAreas = reinterpret_cast<AreaData_Table1b*>(data + 12);
    data += 61;

    Table2 = data + 2;
    data += (2 + data[0] + (data[1] << 8));

    Table3 = reinterpret_cast<AreaData_Table3*>(data + 2);
    data += (2 + data[0] + (data[1] << 8));

    Table4 = reinterpret_cast<AreaData_Table4*>(data + 2);
}
