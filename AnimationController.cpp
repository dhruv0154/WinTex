#include "AnimationController.h"
#include "Globals.h"
#include "PTF.h"
#include "BIC.h"
#include "SilentBIC.h"
#include "Wave.h"
#include "Utilities.h"
#include "Image.h"
#include "MediaIdentifiers.h"
#include "StaticImage.h"
#include <iostream>
#include <SDL2/SDL.h>

CAnimBase* CAnimationController::_anim = nullptr;
CDXText* CAnimationController::_pCaption = nullptr;

int CAnimationController::_texCaptionColour1 = 0xff000000;
int CAnimationController::_texCaptionColour2 = 0xff00c300;
int CAnimationController::_texCaptionColour3 = 0xff24ff00;
int CAnimationController::_texCaptionColour4 = 0xff000000;
int CAnimationController::_otherCaptionColour1 = 0xff000000;
int CAnimationController::_otherCaptionColour2 = 0xff0096ff;
int CAnimationController::_otherCaptionColour3 = 0xff00cfff;
int CAnimationController::_otherCaptionColour4 = 0xff000000;

void CAnimationController::Init()
{
    _pCaption = new CDXText();
}

void CAnimationController::Clear()
{
    if (_anim != nullptr)
    {
        delete _anim;
        _anim = nullptr;
    }

    if (_pCaption != nullptr)
    {
        _pCaption->SetText("");
    }
}

bool CAnimationController::Load(const char* fileName, int itemIndex)
{
    Clear();

    BinaryData bd = LoadEntry(fileName, itemIndex);
    if (bd.Data != nullptr && bd.Length > 0)
    {
        _anim = Load(bd);
        return (_anim != nullptr);
    }
    else
    {
        std::string err = "Failed to open animation file: ";
        err += (fileName ? fileName : "nullptr");
        std::cerr << "[AnimationController Error] " << err << std::endl;
        SDL_Log("%s", err.c_str());
    }

    return false;
}

bool CAnimationController::Skip()
{
    if (_anim != nullptr && !_anim->IsDone())
    {
        _anim->Skip();
        return true;
    }

    return false;
}

bool CAnimationController::IsWave()
{
    return (_anim != nullptr && _anim->IsWave());
}

bool CAnimationController::HasAnim()
{
    return (_anim != nullptr && !_anim->IsDone());
}

bool CAnimationController::NoAnimOrWave()
{
    return (_anim == nullptr || _anim->IsDone() || _anim->IsWave());
}

bool CAnimationController::UpdateAndRender(bool render)
{
    return UpdateAndRender(_anim, render);
}

bool CAnimationController::UpdateAndRender(CAnimBase* pAnim, bool render)
{
    bool updated = false;

    if (pAnim != nullptr)
    {
        int frame = Frame(pAnim);
        CCaption* pC = GetFrameCaption(frame);
        if (pC != nullptr && !pC->Processed())
        {
            _pCaption->SetText(pC->Text(), CDXText::Alignment::Justify);
            if (pC->Tex())
            {
                _pCaption->SetColours(_texCaptionColour1, _texCaptionColour2, _texCaptionColour3, _texCaptionColour4);
            }
            else
            {
                _pCaption->SetColours(_otherCaptionColour1, _otherCaptionColour2, _otherCaptionColour3, _otherCaptionColour4);
            }

            pC->SetProcessed(true);
        }

        updated = pAnim->Update();
        if (render)
        {
            pAnim->Render();
        }

        if (pConfig && pConfig->Captions && !pAnim->IsDone())
        {
            _pCaption->Render(0.0f, dx.GetHeight() - _pCaption->Height() - 10.0f, -1.0f);
        }
    }

    return updated;
}

bool CAnimationController::IsDone()
{
    return (_anim != nullptr && _anim->IsDone());
}

bool CAnimationController::AnimNotDoneOrCondition(bool condition)
{
    return (_anim != nullptr && (!_anim->IsDone() || condition));
}

bool CAnimationController::NoVideoAnim()
{
    return (_anim == nullptr || !_anim->HasVideo() || _anim->IsDone());
}

int CAnimationController::Frame()
{
    return Frame(_anim);
}

int CAnimationController::Frame(CAnimBase* pAnim)
{
    if (pAnim == nullptr) return 0;
    return pAnim->Frame();
}

CAnimBase* CAnimationController::Load(BinaryData bd, int factor)
{
    CAnimBase* pAnim = nullptr;

    // Safety check
    if (bd.Data == nullptr || bd.Length < 8)
    {
        std::cerr << "[AnimationController Error] Invalid or corrupt binary data passed to loader." << std::endl;
        return nullptr;
    }

    uint32_t sig0 = GetInt(bd.Data, 0, 4);
    uint32_t sig4 = GetInt(bd.Data, 4, 4);

    if (sig4 == PTF)
    {
        pAnim = new CPTF(factor);
    }
    else if (sig0 == BIC)
    {
        pAnim = new CBIC(factor);
    }
    else if (sig0 == RIFF)
    {
        pAnim = new CWave();
    }
    else if ((sig0 + 0x30c) == bd.Length)
    {
        // Assuming silent BIC
        pAnim = new CSilentBIC(factor);
    }
    else if (sig0 == H2O)
    {
        pAnim = new CH2O(factor);
    }

    if (pAnim != nullptr) 
    {
        pAnim->Init(bd);
    }

    return pAnim;
}

CImage* CAnimationController::LoadImage(DoubleData dd, int width, int height, int factor)
{
    return new CImage(dd, width, height, factor);
}

CImage* CAnimationController::LoadImage(uint8_t* palette, BinaryData image, int width, int height, int factor)
{
    return new CImage(palette, image, width, height, factor);
}

void CAnimationController::Resize(int width, int height)
{
    if (_anim != nullptr)
    {
        _anim->Resize(width, height);
    }
}

void CAnimationController::SetCaptionColours(int texColour1, int texColour2, int texColour3, int texColour4, int otherColour1, int otherColour2, int otherColour3, int otherColour4)
{
    _texCaptionColour1 = texColour1;
    _texCaptionColour2 = texColour2;
    _texCaptionColour3 = texColour3;
    _texCaptionColour4 = texColour4;
    _otherCaptionColour1 = otherColour1;
    _otherCaptionColour2 = otherColour2;
    _otherCaptionColour3 = otherColour3;
    _otherCaptionColour4 = otherColour4;
}

void CAnimationController::SetOutputBuffer(uint8_t* pBuffer, int width, int height, int offsetX, int offsetY, int* pPalette, int minColAllowChange, int maxColAllowChange)
{
    if (_anim != nullptr)
    {
        static_cast<CH2O*>(_anim)->SetOutputBuffer(pBuffer, width, height, offsetX, offsetY, pPalette, minColAllowChange, maxColAllowChange);
    }
}

void CAnimationController::RenderCaptions(float z)
{
    if (pConfig && pConfig->Captions && _anim != nullptr && !_anim->IsDone())
    {
        _pCaption->Render(0.0f, dx.GetHeight() - _pCaption->Height() - 10.0f, z);
    }
}
