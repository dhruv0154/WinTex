#pragma once
#include "DXButton.h"
#include <unordered_map>
#include "InputMapping.h"
#include <string>

class CControllerData;

class CDXControlButton : public CDXButton
{
public:
	CDXControlButton(const char* function, std::unordered_map<InputAction, InputMap>* pMapping, bool isJoystick, float w, float h, float textX, void(*onClick)(InputAction) = nullptr, InputAction action = InputAction::Cursor);
	~CDXControlButton();
	virtual void Click() override;
	virtual void Render() override;
	virtual CDXControl* HitTest(float x, float y) override;
	virtual void SetMouseOver(bool mouseOver) override;

	std::string GetMapText(CControllerData* pControllerData);

	void UpdateControlText(CControllerData* pControllerData);
	void UpdateControlText(std::unordered_map<InputAction, InputMap>* pMapping, InputAction action);

	void SetIsBeingConfigured(bool configuring) { _isBeingConfigured = configuring; }

	bool IsJoystickConfigControl() const { return _isJoystick; }

	virtual void SetColours(int colour1, int colour2, int colour3, int colour4) override;

protected:
	bool _isJoystick;
	CDXText _binding;
	bool _isBeingConfigured;
	void(*_controlClicked)(InputAction action);
	InputAction _action;
};
