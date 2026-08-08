#include "LocationModule.h"
#include "AnimationController.h"
#include "AnimatedCursor.h"
#include "Globals.h"
#include "GameBase.h"
#include "GameController.h"
#include "Utilities.h"
#include "MainMenuModule.h"
#include "UAKMNewsPaperModule.h"
#include "InventoryModule.h"
#include "UAKMTravelModule.h"
#include "Items.h"
#include "AmbientAudio.h"
#include "HintModule.h"
#include <cmath>
#include <SDL2/SDL.h>

float CLocationModule::_movement_forward = 0.0f;
float CLocationModule::_movement_backward = 0.0f;
float CLocationModule::_movement_left = 0.0f;
float CLocationModule::_movement_right = 0.0f;
float CLocationModule::_movement_x = 0.0f;
float CLocationModule::_movement_y = 0.0f;
float CLocationModule::_movement_z = 0.0f;
float CLocationModule::_smooth_movement_x = 0.0f;
float CLocationModule::_smooth_movement_z = 0.0f;
float CLocationModule::_speed = 0.0f;

ActionType CLocationModule::CurrentAction = ActionType::None;

CLocationModule::CLocationModule(int locationId, int startupPosition) : CModuleBase(ModuleType::Location)
{
    _locationId = locationId;
    _startupPosition = startupPosition;

    CurrentObjectIndex = -1;
    CurrentActions = ActionType::None;
    CurrentAction = ActionType::None;
    CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::Crosshair);

    _movement_forward = 0.0f;
    _movement_backward = 0.0f;
    _movement_left = 0.0f;
    _movement_right = 0.0f;
    _movement_x = 0.0f;
    _movement_y = 0.0f;
    _movement_z = 0.0f;
    _smooth_movement_x = 0.0f;
    _smooth_movement_z = 0.0f;
    _speed = MOVEMENT_WALK_SPEED;

    _oldPoint = Point(0, 0);

    _scriptEngine = CGameController::GetScriptEngine();
    _scriptEngine->_pLoc = &_location;

    Resize(0, 0);

    _actionScriptState = CGameController::GetScriptState();
    _environmentScriptState = CGameController::GetScriptState();
    _queryActionScriptState = CGameController::GetScriptState();
    _initScriptState = CGameController::GetScriptState();

    _actionColour1 = _actionColour4 = 0;
    _actionColour2 = _actionColour3 = -1;
    _currentActionColour1 = _currentActionColour4 = 0;
    _currentActionColour2 = _currentActionColour3 = 0xffffff00;
    _unavailableActionColour1 = _unavailableActionColour4 = 0;
    _unavailableActionColour2 = _unavailableActionColour3 = 0xff808080;
}

CLocationModule::~CLocationModule()
{
    if (_actionScriptState != nullptr && _actionScriptState->Script != nullptr)
    {
        delete[] _actionScriptState->Script;
        _actionScriptState->Script = nullptr;
    }

    if (_environmentScriptState != nullptr)
    {
        _environmentScriptState->Script = nullptr;
    }

    if (_queryActionScriptState != nullptr)
    {
        _queryActionScriptState->Script = nullptr;
    }
}

void CLocationModule::Initialize()
{
    CenterMouse();
    SetCursorClipping();

    CGameController::ResetTimers();

    _scriptEngine->_mapEntry = _location._mapEntry = _scriptEngine->_mapEntry = CModuleController::pMap->Get(_locationId);

    std::string scriptFile = CGameController::GetFileName(_scriptEngine->_mapEntry->ScriptFileIndex);
    BinaryData bd = LoadEntry(scriptFile.c_str(), _scriptEngine->_mapEntry->ScriptFileEntry);

    CAnimationController::Clear();

    LoadLocation(_scriptEngine->_mapEntry->LocationFileIndex, bd, scriptFile, _scriptEngine->_mapEntry->ScriptFileEntry);

    SetLocationPosition(CModuleController::pMap->GetStartupPosition(_locationId, _startupPosition));

    dx.Clear();
    dx.Present();

    _initScriptState->Init(bd.Data, bd.Length, scriptFile, _scriptEngine->_mapEntry->ScriptFileEntry);
    _scriptEngine->Execute(_initScriptState, CGameController::GetLocationInitializationScriptId());
}

void CLocationModule::Render()
{
    if (pOverlay != nullptr)
    {
        int decision = pOverlay->GetDecision();
        if (decision < 0)
        {
            pOverlay->ClearDecision();
            pOverlay = nullptr;
            _movement_backward = _movement_forward = _movement_left = _movement_right = _movement_x = _movement_y = _movement_z = _smooth_movement_x = _smooth_movement_z = 0.0f;
        }
        else if (decision > 0)
        {
            pOverlay->ClearDecision();
            pOverlay = nullptr;
            _environmentScriptState->ExecutionPointer = _environmentScriptState->Parameter;
        }
    }

    if (CAnimationController::NoVideoAnim())
    {
        bool waitingForInput = (_initScriptState->WaitingForInput || _actionScriptState->WaitingForInput || _environmentScriptState->WaitingForInput);
        if (_initScriptState->WaitingForInput && pOverlay == nullptr)
        {
            _scriptEngine->Resume(_initScriptState, true);
        }
        else if (_actionScriptState->WaitingForInput && pOverlay == nullptr)
        {
            _scriptEngine->Resume(_actionScriptState, true);
        }
        else if (_environmentScriptState->WaitingForInput && pOverlay == nullptr)
        {
            if (!_environmentScriptState->WaitingForExternalModule)
            {
                _scriptEngine->Resume(_environmentScriptState, true);
                _location.Render();
            }
        }
        else if (_initScriptState->ExecutionPointer < 0)
        {
            if (!CAnimationController::HasAnim())
            {
                _location.Animate();
            }
            _location.Render();
        }

        if (pOverlay == nullptr)
        {
            if (!waitingForInput && !_environmentScriptState->WaitingForMediaToFinish && !_actionScriptState->WaitingForMediaToFinish && !_initScriptState->WaitingForMediaToFinish)
            {
                _scriptEngine->Execute(_environmentScriptState, CGameController::GetLocationEnvironmentScriptId());
            }
            else if (!CAnimationController::HasAnim() || CAnimationController::IsDone())
            {
                if (_initScriptState->WaitingForMediaToFinish)
                {
                    _scriptEngine->Resume(_initScriptState, true);
                }
                else if (_actionScriptState->WaitingForMediaToFinish)
                {
                    _scriptEngine->Resume(_actionScriptState, true);
                }
                else if (_environmentScriptState->WaitingForMediaToFinish)
                {
                    _scriptEngine->Resume(_environmentScriptState, true);
                }
            }
        }
    }

    if (CModuleController::CurrentModule != this)
    {
        return;
    }

    CAnimationController::UpdateAndRender();
    if (CAnimationController::IsDone())
    {
        if (_actionScriptState->WaitingForMediaToFinish)
        {
            _scriptEngine->Resume(_actionScriptState, true);
        }
        else
        {
            CAnimationController::Clear();
        }
    }

    if (pOverlay == nullptr && !CAnimationController::HasAnim())
    {
        float mx{ _movement_x };
        float mz{ _movement_z };
        float tmx = mx;
        float tmz = mz;

        if (mx != 0.0f || mz != 0.0f)
        {
            mx = _speed * mx;
            mz = _speed * mz;

            float totalmovement = std::sqrt(mx * mx + mz * mz);
            if (totalmovement > _speed)
            {
                mx /= (totalmovement / _speed);
                mz /= (totalmovement / _speed);
            }
        }

        _smooth_movement_x = _smooth_movement_x * 0.85f + mx * 0.15f;
        _smooth_movement_z = _smooth_movement_z * 0.85f + mz * 0.15f;

        if (std::abs(_smooth_movement_x) < 0.01f && _movement_x == 0.0f) { _smooth_movement_x = 0.0f; }
        if (std::abs(_smooth_movement_z) < 0.01f && _movement_z == 0.0f) { _smooth_movement_z = 0.0f; }

        int currentItemId = CGameController::GetCurrentItemId();
        if (CurrentAction == ActionType::Use && currentItemId == -1)
        {
            CurrentAction = ActionType::None;
        }

        _location.Move(_smooth_movement_x, _movement_y, _smooth_movement_z, tmz);

        if (_location.PointingChanged)
        {
            int objectId = -1, subObjectId = -1;
            int hitObject = _location.GetPickObject(objectId, subObjectId);
            if (hitObject == -1)
            {
                CurrentObjectIndex = -1;
                CurrentActions = ActionType::None;
                if (CurrentAction != ActionType::Use)
                {
                    CurrentAction = ActionType::None;
                }
                CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::Crosshair);
            }
            else
            {
                bool resetAction = (hitObject != CurrentObjectIndex);
                CurrentObjectIndex = hitObject;
                CurrentActions = _scriptEngine->GetCurrentActions(_queryActionScriptState, CurrentObjectIndex);

                if (CurrentAction != ActionType::Use && resetAction)
                {
                    if (CurrentActions != ActionType::None)
                    {
                        CurrentAction = ActionType::Look;
                        while ((CurrentActions & CurrentAction) == ActionType::None)
                        {
                            CurrentAction <<= 1;
                        }

                        SelectMouseAction();
                    }
                    else
                    {
                        CurrentAction = ActionType::None;
                        CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::Crosshair);
                    }
                }
            }
        }

        ActionType actions = CurrentActions;
        bool useYellow = true;
        if (currentItemId >= 0)
        {
            actions |= ActionType::Use;
            useYellow = (CurrentObjectIndex >= 0);
        }
        ActionType action = CurrentAction;

        ActionType mask = ActionType::Look;

        float space = (dx.GetWidth() - _actionText[6].Width() - 20.0f) / 6.0f;
        for (int i = 0; i < 7; i++)
        {
            if (mask == action && CAnimationController::NoAnimOrWave() && useYellow)
            {
                _actionText[i].SetColours(_currentActionColour1, _currentActionColour2, _currentActionColour3, _currentActionColour4);
            }
            else if ((actions & mask) != ActionType::None)
            {
                _actionText[i].SetColours(_actionColour1, _actionColour2, _actionColour3, _actionColour4);
            }
            else
            {
                _actionText[i].SetColours(_unavailableActionColour1, _unavailableActionColour2, _unavailableActionColour3, _unavailableActionColour4);
            }
            mask <<= 1;

            _actionText[i].Render(space * i, 10.0f);
        }

        if (CAnimationController::NoAnimOrWave())
        {
            CConstantBuffers::Setup2D(dx);
            CShaders::SelectOrthoShader();
            dx.DisableZBuffer();

            if (currentItemId >= 0)
            {
                CItems::RenderItemImage(currentItemId, static_cast<float>(dx.GetWidth() - 98), static_cast<float>(dx.GetHeight() - 78), false);
            }

            if (action == ActionType::Use && currentItemId >= 0)
            {
                CModuleController::Cursors[static_cast<int>(CAnimatedCursor::CursorType::Crosshair)].Render();
                CItems::RenderItemImage(currentItemId, static_cast<float>(dx.GetWidth() / 2), static_cast<float>(dx.GetHeight() / 2), false);
            }
            else
            {
                CModuleController::Cursors[CurrentActionMousePointerIndex].Render();
            }
        }
    }

    if (pOverlay != nullptr)
    {
        pOverlay->Render();
    }
}

void CLocationModule::SelectMouseAction()
{
    switch (CurrentAction)
    {
        case ActionType::Look:
            CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::Look);
            break;
        case ActionType::Move:
            CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::Move);
            break;
        case ActionType::Get:
            CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::Grab);
            break;
        case ActionType::OnOff:
            CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::OnOff);
            break;
        case ActionType::Talk:
            CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::Talk);
            break;
        case ActionType::Open:
            CurrentActionMousePointerIndex = static_cast<int>(CAnimatedCursor::CursorType::Open);
            break;
        default:
            break;
    }
}

void CLocationModule::CycleActions(bool allowUse)
{
    ActionType actions = CurrentActions;
    if (allowUse && CGameController::GetCurrentItemId() >= 0)
    {
        actions |= ActionType::Use;
        if (CurrentAction == ActionType::None)
        {
            CurrentAction = ActionType::Use >> 1;
        }
    }

    if (actions != ActionType::None)
    {
        CurrentAction <<= 1;
        while (CurrentAction != ActionType::None && (actions & CurrentAction) == ActionType::None)
        {
            if (CurrentAction == ActionType::Use && allowUse && CGameController::GetCurrentItemId() >= 0)
            {
                break;
            }

            CurrentAction <<= 1;
            if (CurrentAction == ActionType::Terminate)
            {
                CurrentAction = ActionType::Look;
            }
        }

        SelectMouseAction();
    }
}

void CLocationModule::LoadLocation(int locationFileIndex, BinaryData script, const std::string& file, int entry)
{
    _movement_x = _movement_y = _movement_z = 0.0f;

    if (_location.Load(locationFileIndex))
    {
        _queryActionScriptState->Clear();
        _queryActionScriptState->Init(script.Data, script.Length, file, entry);
        _actionScriptState->Clear();
        _actionScriptState->Init(script.Data, script.Length, file, entry);
        _environmentScriptState->Clear();
        _environmentScriptState->Init(script.Data, script.Length, file, entry);
        _queryActionScriptState->DebugMode = false;
        _environmentScriptState->DebugMode = false;

        _oldPoint = Point(dx.GetWidth() / 2, dx.GetHeight() / 2);

        if (CGameController::GetCurrentItemId() >= 0)
        {
            CurrentActions |= ActionType::Use;
        }
    }
}

void CLocationModule::Pause()
{
    UnsetCursorClipping();
}

void CLocationModule::Resume()
{
    CenterMouse();
    SetCursorClipping();

    if (_actionScriptState->WaitingForInput || _actionScriptState->WaitingForExternalModule)
    {
        _scriptEngine->Resume(_actionScriptState, true);
    }
}

void CLocationModule::Resize(int width, int height)
{
    _actionText[0].SetText("Look");
    _actionText[1].SetText("Move");
    _actionText[2].SetText("Get");
    _actionText[3].SetText("On/off");
    _actionText[4].SetText("Talk");
    _actionText[5].SetText("Open");
    _actionText[6].SetText("Use");
}

void CLocationModule::Cursor(float x, float y, bool relative)
{
    if (pOverlay == nullptr)
    {
        if (_hasFocus && CAnimationController::NoAnimOrWave() && !CInputMapping::IgnoreNextMouseInput)
        {
            float delta_x = relative ? x : x - static_cast<float>(dx.GetWidth()) / 2.0f;
            float delta_y = relative ? y : y - static_cast<float>(dx.GetHeight()) / 2.0f;

            if (pConfig->InvertY) delta_y = -delta_y;
            auto scale = pConfig->MouselookScaling;

            _location.DeltaAngles((delta_y / 2000.0f) * scale, (delta_x / 2000.0f) * scale);
        }

        CenterMouse();
    }
    else
    {
        pOverlay->Cursor(x, y, relative);
    }
}

void CLocationModule::BeginAction()
{
    if (pOverlay == nullptr)
    {
        if (!CAnimationController::HasAnim())
        {
            if (CurrentAction != ActionType::None)
            {
                ClearCaptions(pDisplayCaptions);

                if (CurrentObjectIndex >= 0)
                {
                    int currentItemId = CGameController::GetCurrentItemId();
                    _scriptEngine->PermformAction(_actionScriptState, CurrentObjectIndex, CurrentAction, CurrentAction == ActionType::Use ? currentItemId : -1);

                    if (CurrentAction != ActionType::Use || CGameController::GetCurrentItemId() != currentItemId)
                    {
                        CycleActions(false);
                    }

                    _location.PointingChanged = true;
                }
            }
        }
        else
        {
            CAnimationController::Skip();
            ClearCaptions(pDisplayCaptions);

            if (_actionScriptState->WaitingForMediaToFinish)
            {
                _scriptEngine->Resume(_actionScriptState, true);
            }
        }
    }
    else
    {
        pOverlay->BeginAction();
    }
}

void CLocationModule::Back()
{
    if (pOverlay == nullptr)
    {
        CModuleController::SendToFront(CMainMenuModule::MainMenuModule);
    }
}

void CLocationModule::Cycle()
{
    if (pOverlay == nullptr)
    {
        if (CAnimationController::HasAnim())
        {
            CAnimationController::Skip();
            ClearCaptions(pDisplayCaptions);

            if (_actionScriptState->WaitingForMediaToFinish)
            {
                _scriptEngine->Resume(_actionScriptState, true);
            }
        }
        else if (CurrentActions != ActionType::None || CGameController::GetCurrentItemId() >= 0)
        {
            CycleActions(true);
        }
    }
}

void CLocationModule::MoveForward(float v)
{
    if (pOverlay == nullptr)
    {
        _movement_forward = v;
        _movement_z = _movement_backward - _movement_forward;
    }
}

void CLocationModule::MoveBack(float v)
{
    if (pOverlay == nullptr)
    {
        _movement_backward = v;
        _movement_z = _movement_backward - _movement_forward;
    }
}

void CLocationModule::MoveLeft(float v)
{
    if (pOverlay == nullptr)
    {
        _movement_left = v;
        _movement_x = _movement_left - _movement_right;
    }
}

void CLocationModule::MoveRight(float v)
{
    if (pOverlay == nullptr)
    {
        _movement_right = v;
        _movement_x = _movement_left - _movement_right;
    }
}

void CLocationModule::MoveUp(float y)
{
    if (pOverlay == nullptr)
    {
        _movement_y = -y / 10.0f;
    }
}

void CLocationModule::MoveDown(float y)
{
    if (pOverlay == nullptr)
    {
        _movement_y = y / 10.0f;
    }
}

void CLocationModule::Run(bool run)
{
    if (pOverlay == nullptr)
    {
        _speed = run ? MOVEMENT_RUN_SPEED : MOVEMENT_WALK_SPEED;
    }
}

void CLocationModule::Next()
{
    if (pOverlay == nullptr)
    {
        CycleItems(1);
    }
}

void CLocationModule::Prev()
{
    if (pOverlay == nullptr)
    {
        CycleItems(-1);
    }
}

void CLocationModule::Inventory()
{
    if (pOverlay == nullptr)
    {
        CModuleController::Push(new CInventoryModule());
    }
}

void CLocationModule::Travel()
{
    if (pOverlay == nullptr)
    {
        CModuleController::Push(new CUAKMTravelModule());
    }
}

void CLocationModule::CycleItems(int direction)
{
    if (direction > 0)
    {
        CGameController::SelectNextItem();
    }
    else
    {
        CGameController::SelectPreviousItem();
    }

    if (CGameController::GetCurrentItemId() == -1 && CurrentAction == ActionType::Use)
    {
        CycleActions(false);
    }
}

void CLocationModule::Hints()
{
    if (pOverlay == nullptr)
    {
        CModuleController::Push(CGameController::GetHintModule());
    }
}

void CLocationModule::KeyDown(uint32_t key, uint32_t lParam)
{
    if (pOverlay == nullptr)
    {
#ifdef DEBUG
        constexpr uint32_t KEY_F1 = 0x70;
        constexpr uint32_t KEY_F2 = 0x71;
        constexpr uint32_t KEY_F3 = 0x72;
        constexpr uint32_t KEY_F4 = 0x73;
        constexpr uint32_t KEY_F5 = 0x74;

        if (key == KEY_F1)
        {
            _location._renderPoints = !_location._renderPoints;
        }
        else if (key == KEY_F2)
        {
            _location._renderLines = !_location._renderLines;
        }
        else if (key == KEY_F3)
        {
            _location._renderPaths = !_location._renderPaths;
        }
        else if (key == KEY_F4)
        {
            _location._renderTextured = !_location._renderTextured;
        }
        else if (key == KEY_F5)
        {
            _location._disableClipping = !_location._disableClipping;
        }
#endif
    }
    else
    {
        pOverlay->KeyDown(key, lParam);
    }
}

#ifdef DEBUG
void CLocationModule::MouseWheel(int scroll)
{
    SDL_Keymod modState = SDL_GetModState();
    bool ctrl  = (modState & KMOD_CTRL) != 0;
    bool alt   = (modState & KMOD_ALT) != 0;
    bool shift = (modState & KMOD_SHIFT) != 0;

    if (ctrl || alt || shift)
    {
        _location.MoveObject(scroll > 0 ? 0.1f : -0.1f, ctrl, alt, shift);
    }
}
#endif