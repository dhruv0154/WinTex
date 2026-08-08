#pragma once

#include "AnimBase.h"
#include "Mutex.h"
#include "DXText.h"
#include "Image.h"
#include <cstdint>
#include <string>

class CAnimationController
{
public:
    static void Init();
    static void Clear();
    
    static bool Load(const char* fileName, int itemIndex);
    static bool Skip();
    static bool IsWave();
    static bool HasAnim();
    static bool NoAnimOrWave();
    static bool UpdateAndRender(bool render = true);
    static bool UpdateAndRender(CAnimBase* pAnim, bool render = true);
    static bool IsDone();
    static bool AnimNotDoneOrCondition(bool condition);
    static bool NoVideoAnim();
    static int Frame();
    static int Frame(CAnimBase* pAnim);
    static int Exists() { return (_anim != nullptr); }
    static void Resize(int width, int height);
    
    static void SetOutputBuffer(uint8_t* pBuffer, int width, int height, int offsetX, int offsetY, int* pPalette, int minColAllowChange, int maxColAllowChange);

    static CAnimBase* Load(BinaryData bd, int factor = 1);
    static CImage* LoadImage(DoubleData bd, int width, int height, int factor = 1);
    static CImage* LoadImage(uint8_t* palette, BinaryData image, int width, int height, int factor = 1);

    static int Width() { return (_anim != nullptr) ? _anim->Width() : 0; }
    static int Height() { return (_anim != nullptr) ? _anim->Height() : 0; }

    static void SetCaptionColours(int texColour1, int texColour2, int texColour3, int texColour4, int otherColour1, int otherColour2, int otherColour3, int otherColour4);

    static void RenderCaptions(float z);

protected:
    static CAnimBase* _anim;
    static CDXText* _pCaption;

    static int _texCaptionColour1;
    static int _texCaptionColour2;
    static int _texCaptionColour3;
    static int _texCaptionColour4;
    static int _otherCaptionColour1;
    static int _otherCaptionColour2;
    static int _otherCaptionColour3;
    static int _otherCaptionColour4;
};
