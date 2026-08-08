#pragma once

#include "ModuleBase.h"
#include "Location.h"
#include "ScriptBase.h"
#include "DXText.h"
#include "ScriptState.h"
#include "Point.h"
#include <cstdint>
#include <string>

#define MOVEMENT_WALK_SPEED 0.2f
#define MOVEMENT_RUN_SPEED 0.5f

class CLocationModule : public CModuleBase
{
public:
    CLocationModule(int locationId, int startupPosition);
    virtual ~CLocationModule();

    void Resize(int width, int height) override;

    void Pause() override;
    void Resume() override;
    void Render() override;

    static float _movement_left;
    static float _movement_right;
    static float _movement_forward;
    static float _movement_backward;
    static float _movement_x;
    static float _movement_y;
    static float _movement_z;
    static float _smooth_movement_x;
    static float _smooth_movement_z;
    static float _speed;

    static ActionType CurrentAction;

    void KeyDown(uint32_t key, uint32_t lParam) override;
#ifdef DEBUG
    void MouseWheel(int scroll) override;
#endif

protected:
    void Initialize() override;

    int _actionColour1{0};
    int _actionColour2{0};
    int _actionColour3{0};
    int _actionColour4{0};
    int _currentActionColour1{0};
    int _currentActionColour2{0};
    int _currentActionColour3{0};
    int _currentActionColour4{0};
    int _unavailableActionColour1{0};
    int _unavailableActionColour2{0};
    int _unavailableActionColour3{0};
    int _unavailableActionColour4{0};

    int _locationId{0};
    int _startupPosition{0};

    CLocation _location;

    int CurrentObjectIndex{-1};
    ActionType CurrentActions{ActionType::None};
    int CurrentActionMousePointerIndex{0};
    CDXText _actionText[7];

    virtual void SelectMouseAction();
    void CycleActions(bool allowUse);

    CScriptState* _environmentScriptState{nullptr};
    CScriptState* _actionScriptState{nullptr};
    CScriptState* _queryActionScriptState{nullptr};
    CScriptBase* _scriptEngine{nullptr};
    CScriptState* _initScriptState{nullptr};

    Point _oldPoint;

    virtual void LoadLocation(int locationFileIndex, BinaryData script, const std::string& file, int entry);
    virtual void SetLocationPosition(StartupPosition pos) { _location.SetPosition(pos); _location.UpdateSprites(); }

    void Cursor(float x, float y, bool relative) override;
    void BeginAction() override;
    void Back() override;
    void Cycle() override;
    void MoveForward(float v) override;
    void MoveBack(float v) override;
    void MoveLeft(float v) override;
    void MoveRight(float v) override;
    void MoveUp(float y) override;
    void MoveDown(float y) override;
    void Run(bool run) override;
    void Next() override;
    void Prev() override;
    void Inventory() override;
    void Travel() override;
    void Hints() override;

    void CycleItems(int direction);
};