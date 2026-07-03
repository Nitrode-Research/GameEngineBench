#include "PlayerTraderWidget.h"

void UPlayerTraderWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

FText UPlayerTraderWidget::GetPlayerMoney() { return FText::GetEmpty(); }
