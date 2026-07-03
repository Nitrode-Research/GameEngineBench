#include "LobbyCharacterTradeWidget.h"

FText ULobbyCharacterTradeWidget::GetTradeTime() { return FText::GetEmpty(); }

FText ULobbyCharacterTradeWidget::GetTargetCharacterName() { return FText::GetEmpty(); }

FSlateBrush ULobbyCharacterTradeWidget::GetTargetCharacterImage() { return FSlateBrush{}; }

FText ULobbyCharacterTradeWidget::GetOwnCharacterName() { return FText::GetEmpty(); }

FSlateBrush ULobbyCharacterTradeWidget::GetOwnCharacterImage() { return FSlateBrush{}; }

ESlateVisibility ULobbyCharacterTradeWidget::IsInCharacterTrade() { return ESlateVisibility::Collapsed; }
