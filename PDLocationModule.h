#pragma once

#include "LocationModule.h"

class CPDLocationModule : public CLocationModule
{
public:
    CPDLocationModule(int locationId, int startupPosition);

    void Render() override;

protected:
    void Inventory() override;
    void Travel() override;

    bool _abductorMode{false};
};
