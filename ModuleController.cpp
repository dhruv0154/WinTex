#include "ModuleController.h"
#include "Globals.h"
#include "AmbientAudio.h"
#include "Gamepad.h"
#include "GameController.h"
#include "InputMapping.h"
#include <vector>
#include <SDL2/SDL.h>

CModuleBase* CModuleController::CurrentModule = nullptr;
CModuleBase* CModuleController::NextModule = nullptr;
std::list<CModuleBase*> CModuleController::Modules;

CAnimatedCursor CModuleController::Cursors[13];

uint32_t CModuleController::MainThreadId = 0;
uint32_t CModuleController::D3DThreadId = 0;
uint32_t CModuleController::TimerThreadId = 0;

CMap* CModuleController::pMap = nullptr;
CMap* CModuleController::pDMap = nullptr;

namespace {
    struct MutexGuard {
        CMutex& m;
        explicit MutexGuard(CMutex& mut) : m(mut) { m.Lock(); }
        ~MutexGuard() { m.Release(); }
    };
}

bool CModuleController::Init(CMap* map, CMap* dmap)
{
    if (map != nullptr && dmap != nullptr)
    {
        pMap = map;
        pDMap = dmap;
        bool ret = (map->Init() && dmap->Init());

#ifdef DEBUGx
        int mapIndex = 0;
        CMapData* pMapData = nullptr;
        while ((pMapData = map->Get(mapIndex)) != nullptr)
        {
            Trace("DMap ");
            TraceLine(mapIndex++);
            Trace("Dialogue file: ");
            TraceLine(CGameController::GetFileName(pMapData->LocationFileIndex).c_str());
            Trace("Script file: ");
            TraceLine(CGameController::GetFileName(pMapData->ScriptFileIndex).c_str());
            Trace("Script entry: ");
            TraceLine(pMapData->ScriptFileEntry);
            TraceLine("Animations");
            int ix = 0;
            for (auto anim : pMapData->AnimationMap)
            {
                Trace("\t");
                Trace(ix++);
                Trace(" = ");
                TraceLine(anim);
            }
            TraceLine("Audio");
            ix = 0;
            for (const auto& audio : pMapData->AudioMap)
            {
                Trace("\t");
                Trace(ix++);
                Trace(" = ");
                Trace(CGameController::GetFileName(audio.File).c_str());
                Trace(" #");
                TraceLine(audio.Entry);
            }
            TraceLine("Environmental audio");
            ix = 0;
            for (const auto& audio : pMapData->EnvironmentAudioMap)
            {
                Trace("\t");
                Trace(ix++);
                Trace(" = ");
                Trace(CGameController::GetFileName(audio.File).c_str());
                Trace(" #");
                TraceLine(audio.Entry);
            }
            TraceLine("Video");
            ix = 0;
            for (const auto& video : pMapData->VideoMap)
            {
                Trace("\t");
                Trace(ix++);
                Trace(" = ");
                Trace(CGameController::GetFileName(video.File).c_str());
                Trace(" #");
                TraceLine(video.Entry);
            }
            TraceLine("Images");
            ix = 0;
            for (const auto& image : pMapData->ImageMap)
            {
                Trace("\t");
                Trace(ix++);
                Trace(" = ");
                Trace(CGameController::GetFileName(image.File).c_str());
                Trace(" #");
                TraceLine(image.Entry);
            }
            TraceLine("Objects");
            ix = 0;
            for (const auto& obj : pMapData->ObjectMap)
            {
                Trace("\t");
                Trace(ix++);
                Trace(" = ");
                TraceLine(obj, 16);
            }
            TraceLine("Startup positions");
            ix = 0;
            for (const auto& pos : pMapData->StartupPositions)
            {
                Trace("\t");
                Trace(ix++);
                Trace(" = (x:");
                Trace(pos.X);
                Trace(", y:");
                Trace(pos.Y);
                Trace(", z:");
                Trace(pos.Z);
                Trace("), angle = ");
                Trace(pos.Angle);

                Trace("IEL = ");
                Trace(pos.InitialEyeLevel);
                Trace("MinY = ");
                Trace(pos.MinYAdj);
                Trace("MaxY = ");
                Trace(pos.MaxYAdj);
                Trace("Elevation = ");
                TraceLine(pos.Elevation);
            }
            TraceLine("");
        }
#endif

        return ret;
    }

    return false;
}

void CModuleController::Push(CModuleBase* pModule, bool overrideCurrent)
{
    MutexGuard guard(_lock);

    if (pModule != nullptr)
    {
        NextModule = pModule;
        Modules.push_front(pModule);
        pModule->Init();
    }
}

void CModuleController::Pop(CModuleBase* pModule)
{
    MutexGuard guard(_lock);

    if (pModule != nullptr)
    {
        if (CurrentModule == pModule) 
        {
            CurrentModule->Pause();
        }
        
        Modules.remove(pModule);

        if (CurrentModule == pModule)
        {
            if (!Modules.empty())
            {
                CurrentModule = Modules.front();
                ResumeModule(CurrentModule);
            }
            else
            {
                CurrentModule = nullptr;
            }
        }

        delete pModule;
    }
}

void CModuleController::SendToBack(CModuleBase* pModule)
{
    MutexGuard guard(_lock);

    if (pModule != nullptr)
    {
        Modules.remove(pModule);
        if (!Modules.empty())
        {
            pModule->Pause();
            Modules.push_back(pModule);
            CurrentModule = Modules.front();
            CurrentModule->Resume();
        }
        else
        {
            SDL_Event event;
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
        }
    }
}

void CModuleController::SendToFront(CModuleBase* pModule)
{
    MutexGuard guard(_lock);

    if (pModule != nullptr)
    {
        PauseModule(CurrentModule);
        Modules.remove(pModule);
        Modules.push_front(pModule);
        CurrentModule = pModule;
        CurrentModule->Resume();
    }
}

void CModuleController::Render()
{
    MutexGuard guard(_lock);

    if (NextModule != nullptr)
    {
        PauseModule(CurrentModule);

        std::vector<CModuleBase*> modulesToDelete;
        for (auto* it : Modules)
        {
            if (it != nullptr && it->Type == NextModule->Type && it != NextModule)
            {
                modulesToDelete.push_back(it);
            }
        }

        for (auto* it : modulesToDelete)
        {
            Modules.remove(it);
            delete it;
        }

        CurrentModule = NextModule;
        NextModule = nullptr;
    }

    if (CurrentModule != nullptr)
    {
        dx.Clear();
        CurrentModule->Render();
        CurrentModule->CheckInput();
        dx.Present(1, 0);
    }
}

void CModuleController::MouseMove(Point pt)
{
    CInputMapping::Input(InputSource::Mouse, 0, ((pt.ix() & 0xffff) << 16) | (pt.iy() & 0xffff));
    if (CurrentModule != nullptr)
    {
        CurrentModule->MouseMove(pt);
    }
}

void CModuleController::MouseDown(Point pt, int btn)
{
    CInputMapping::Input(InputSource::MouseButton, btn, 1);
    if (CurrentModule != nullptr)
    {
        CurrentModule->MouseDown(pt, btn);
    }
}

void CModuleController::MouseUp(Point pt, int btn)
{
    CInputMapping::Input(InputSource::MouseButton, btn, 0);
    if (CurrentModule != nullptr)
    {
        CurrentModule->MouseUp(pt, btn);
    }
}

void CModuleController::MouseWheel(int scroll)
{
    CInputMapping::Input(InputSource::MouseWheel, scroll > 0 ? 1 : -1, 1);
    if (CurrentModule != nullptr)
    {
        CurrentModule->MouseWheel(scroll);
    }
}

void CModuleController::KeyDown(uint32_t key, uint32_t lParam)
{
    CInputMapping::Input(InputSource::Key, lParam & 0xff0000, 1);
    if (CurrentModule != nullptr)
    {
        CurrentModule->KeyDown(key, lParam);
    }
}

void CModuleController::KeyUp(uint32_t key, uint32_t lParam)
{
    CInputMapping::Input(InputSource::Key, lParam & 0xff0000, 0);
    if (CurrentModule != nullptr)
    {
        CurrentModule->KeyUp(key, lParam);
    }
}

void CModuleController::GamepadInput(InputSource source, int offset, int data)
{
    CInputMapping::Input(source, offset, data);
    if (CurrentModule != nullptr)
    {
        CurrentModule->GamepadInput(source, offset, data);
    }
}

void CModuleController::PauseModule(CModuleBase* pModule)
{
    if (pModule != nullptr)
    {
        pModule->Pause();
    }
}

void CModuleController::ResumeModule(CModuleBase* pModule)
{
    if (pModule != nullptr)
    {
        pModule->Resume();
    }
}

CModuleBase* CModuleController::Get(ModuleType type)
{
    MutexGuard guard(_lock);

    for (auto* it : Modules)
    {
        if (it != nullptr && it->Type == type)
        {
            return it;
        }
    }

    return nullptr;
}

void CModuleController::ClearExcept(CModuleBase* pModule)
{
    MutexGuard guard(_lock);

    for (auto* it : Modules)
    {
        if (it != pModule && it != CurrentModule)
        {
            delete it;
        }
    }

    Modules.clear();
    
    if (pModule != nullptr)
    {
        Modules.push_back(pModule);
    }
    
    if (CurrentModule != nullptr && CurrentModule != pModule)
    {
        Modules.push_back(CurrentModule);
    }
    
    CurrentModule = pModule;
}

void CModuleController::Resize(int width, int height)
{
    MutexGuard guard(_lock);

    for (auto* it : Modules)
    {
        if (it != nullptr)
        {
            it->Resize(width, height);
        }
    }

    for (int c = 0; c < 13; c++)
    {
        Cursors[c].SetPosition(width / 2.0f, height / 2.0f);
    }

    for (int i = 0; i < 3; i++)
    {
        DialogueOptions[i].Resize(i, width, height);
    }
}

void CModuleController::GotFocus()
{
    if (CurrentModule != nullptr) 
    {
        CurrentModule->GotFocus();
    }
}

void CModuleController::LostFocus()
{
    if (CurrentModule != nullptr) 
    {
        CurrentModule->LostFocus();
    }
}

void CModuleController::Resume()
{
    MutexGuard guard(_lock);

    if (CurrentModule != nullptr)
    {
        CurrentModule->Resume();
    }
}