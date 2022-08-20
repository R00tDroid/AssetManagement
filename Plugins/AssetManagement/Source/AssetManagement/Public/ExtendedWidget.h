#pragma once
#include "Widgets/SUserWidget.h"

class SExtendedWidget : public SCompoundWidget 
{
public:
    void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
    virtual void Start() {};

private:
    bool started = false;
};