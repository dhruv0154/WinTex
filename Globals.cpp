#include "DirectX.h"
#include "AnimBase.h"
#include <list>
#include "Caption.h"
#include "GameBase.h"
#include "ModuleBase.h"
#include "Globals.h"

CDirectX dx;

VideoMode videoMode = VideoMode::FullScreen;	// Play title video in fullscreen
ConversationOption conversationOption = ConversationOption::None;
std::list<CCaption*> captions1;
std::list<CCaption*> captions2;
std::list<CCaption*>* pDisplayCaptions = &captions1;
std::list<CCaption*>* pAddCaptions = &captions2;

std::string gamePath = "";

float twopi = 6.283185307f;
void* _hWnd = nullptr;

CConfiguration* pConfig = nullptr;

CMutex _lock;

CDXDialogueOption DialogueOptions[3];
int DialogueOptionsCount = 0;
CDXFont TexFont;

CMIDIPlayer* pMIDI = nullptr;

bool isUAKM = true;

COverlay* pOverlay = nullptr;
COverlay* pClimbLadderOverlay = nullptr;
COverlay* pConvertPointsOverlay = nullptr;
COverlay* pElevationModOverlay = nullptr;

int DefaultCaptionColour1 = 0;
int DefaultCaptionColour2 = -1;
int DefaultCaptionColour3 = -1;
int DefaultCaptionColour4 = 0;
