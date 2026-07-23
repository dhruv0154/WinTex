#include "DXAdapter.h"
#include <SDL2/SDL.h>

CDXAdapter::CDXAdapter()
{
    _numModes = 0;
    _displayModeList = nullptr;

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    int rawModes = SDL_GetNumDisplayModes(0); 
    
    if (rawModes > 0)
    {
        _displayModeList = new DummyDisplayMode[rawModes];
        for (int i = 0; i < rawModes; i++)
        {
            SDL_DisplayMode mode;
            SDL_GetDisplayMode(0, i, &mode);
            _displayModeList[_numModes].Width = mode.w;
            _displayModeList[_numModes].Height = mode.h;
            _numModes++;
        }


        for (unsigned int i = 0; i < _numModes; i++)
        {
            for (unsigned int j = i + 1; j < _numModes; j++)
            {
                if (_displayModeList[i].Width == _displayModeList[j].Width && 
                    _displayModeList[i].Height == _displayModeList[j].Height)
                {
                    for (unsigned int k = j + 1; k < _numModes; k++)
                    {
                        _displayModeList[k - 1] = _displayModeList[k];
                    }
                    j--;
                    _numModes--;
                }
            }
        }
    }
    else
    {
        _numModes = 1;
        _displayModeList = new DummyDisplayMode[1];
        _displayModeList[0].Width = 640;
        _displayModeList[0].Height = 480;
    }
}

CDXAdapter::~CDXAdapter()
{
    if (_displayModeList != nullptr)
    {
        delete[] _displayModeList;
        _displayModeList = nullptr;
    }
}