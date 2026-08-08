#pragma once

#include <cstdint>

struct AreaData_Table1b
{
    int16_t Y1;
    int16_t Y2;
    int16_t X1;
    int16_t X2;
};

struct AreaData_Table3
{
    int16_t X;
    int16_t Y;
    uint8_t unk1;
    uint8_t unk2;
    uint8_t unk3;
    uint8_t unk4;
};

struct AreaData_Table4
{
    uint8_t unk1;
    uint8_t unk2;
    int16_t Y1;
    int16_t Y2;
    int16_t X1;
    int16_t X2;
    uint8_t WidthIndex;
    uint8_t unk4;
    uint8_t unk5;
    uint8_t unk6;
};

class CAreaData
{
public:
    CAreaData();
    ~CAreaData();

    void Init(uint8_t* data);

    uint8_t* CategoryOptionOffsets{nullptr};
    uint8_t* CategoryOptionCounts{nullptr};
    AreaData_Table1b* CategoryAreas{nullptr};

    uint8_t* Table2{nullptr};
    AreaData_Table3* Table3{nullptr};
    AreaData_Table4* Table4{nullptr};
};
