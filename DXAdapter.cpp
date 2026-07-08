#include "DXAdapter.h"

CDXAdapter::CDXAdapter()
{
    _numModes = 1;
    _displayModeList = new DummyDisplayMode[1];
    _displayModeList[0].Width = 640;
    _displayModeList[0].Height = 480;
}

CDXAdapter::~CDXAdapter()
{
    if (_displayModeList != nullptr)
    {
        delete[] _displayModeList;
        _displayModeList = nullptr;
    }
}