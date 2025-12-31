#include "DXControls.h"
#include "DXScreen.h"
#include "DXFrame.h"
#include "DXImageButton.h"
#include "DXCheckBox.h"
#include "DXDialogueOption.h"
#include "SaveGameControl.h"
#include "DXTabItem.h"
#include "DXSlider.h"

#include <iostream>
void CDXControls::Init()
{
    std::cout << "CDXControls::Init Start" << std::endl;
	// Initialize control classes
    std::cout << "CDXScreen::Init" << std::endl;
	CDXScreen::Init();
    std::cout << "CDXButton::Init" << std::endl;
	CDXButton::Init();
    std::cout << "CDXFrame::Init" << std::endl;
	CDXFrame::Init();
    std::cout << "CDXImageButton::Init" << std::endl;
	CDXImageButton::Init();
    std::cout << "CDXCheckBox::Init" << std::endl;
	CDXCheckBox::Init();
    std::cout << "CDXDialogueOption::Init" << std::endl;
	CDXDialogueOption::Init();
    std::cout << "CDXSound::Init" << std::endl;
	CDXSound::Init();
    std::cout << "CDXListBox::Init" << std::endl;
	CDXListBox::Init();
    std::cout << "CDXTabItem::Init" << std::endl;
	CDXTabItem::Init();
    std::cout << "CDXSlider::Init" << std::endl;
	CDXSlider::Init();

    std::cout << "CSaveGameControl::Init" << std::endl;
	CSaveGameControl::Init();
    std::cout << "CDXControls::Init End" << std::endl;
}

void CDXControls::Dispose()
{
	// Initialize control classes
	CDXScreen::Dispose();
	CDXButton::Dispose();
	CDXFrame::Dispose();
	CDXImageButton::Dispose();
	CDXCheckBox::Dispose();
	CDXDialogueOption::Dispose();
	CDXSound::Dispose();
	CDXListBox::Dispose();
	CDXTabItem::Dispose();
	CDXSlider::Dispose();

	CSaveGameControl::Dispose();
}
