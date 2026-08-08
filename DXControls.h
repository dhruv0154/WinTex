#pragma once

#include "DXBitmap.h"
#include "DXButton.h"
#include "DXCheckBox.h"
#include "DXControl.h"
#include "DXComboBox.h"
#include "DXLabel.h"
#include "DXListBox.h"
#include "DXScreen.h"
#include "DXFrame.h"
#include "DXImageButton.h"
#include "DXDialogueOption.h"
#include "SaveGameControl.h"
#include "DXTabItem.h"
#include "DXSlider.h"

class CDXControls
{
public:
    CDXControls() = delete;
    ~CDXControls() = delete;

    static void Init();
    static void Dispose();
};