#pragma once

#include "DXBase.h"

// dummy struct to replace DXGI_MODE_DESC
struct DummyDisplayMode {
    unsigned int Width;
    unsigned int Height;
};

class CDXAdapter : public CDXBase
{
public:
    CDXAdapter();
    ~CDXAdapter();

    unsigned int _numModes;
    DummyDisplayMode* _displayModeList;
};