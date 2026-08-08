#pragma once

#include "MIDIPlayer.h"
#include <cstdint>

class CPDMIDIPlayer : public CMIDIPlayer
{
public:
    virtual void Init(BinaryData data) override;

protected:
    virtual uint32_t Player() override;
};