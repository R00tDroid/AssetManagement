#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FAssetManagementStyle
{
public:
    static void Init();
    static void Release();

    static const ISlateStyle& Get();

private:
    static TSharedPtr<FSlateStyleSet> Instance;
};
