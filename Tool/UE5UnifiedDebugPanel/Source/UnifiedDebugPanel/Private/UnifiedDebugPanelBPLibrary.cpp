// Copyright DevTools. All Rights Reserved.

#include "UnifiedDebugPanelBPLibrary.h"
#include "DebugDataCollectorSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

UDebugDataCollectorSubsystem* UUnifiedDebugPanelBPLibrary::GetSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	return World->GetSubsystem<UDebugDataCollectorSubsystem>();
}

void UUnifiedDebugPanelBPLibrary::WatchActor(const UObject* WorldContextObject, AActor* Actor)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->WatchActor(Actor);
	}
}

void UUnifiedDebugPanelBPLibrary::UnwatchActor(const UObject* WorldContextObject, AActor* Actor)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->UnwatchActor(Actor);
	}
}

void UUnifiedDebugPanelBPLibrary::ClearAllWatches(const UObject* WorldContextObject)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->ClearAllWatches();
	}
}

void UUnifiedDebugPanelBPLibrary::WatchPlayerPawn(const UObject* WorldContextObject, int32 PlayerIndex)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->WatchPlayerPawn(PlayerIndex);
	}
}

void UUnifiedDebugPanelBPLibrary::WatchActorsWithTag(const UObject* WorldContextObject, FName Tag)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->WatchActorsWithTag(Tag);
	}
}

bool UUnifiedDebugPanelBPLibrary::GetActorInsight(const UObject* WorldContextObject, AActor* Actor, FActorInsightData& OutData)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->GetActorInsight(Actor, OutData);
	}
	return false;
}

TArray<FActorInsightData> UUnifiedDebugPanelBPLibrary::GetAllInsightData(const UObject* WorldContextObject)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->GetAllInsightData();
	}
	return TArray<FActorInsightData>();
}

TArray<AActor*> UUnifiedDebugPanelBPLibrary::GetWatchedActors(const UObject* WorldContextObject)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->GetWatchedActors();
	}
	return TArray<AActor*>();
}

int32 UUnifiedDebugPanelBPLibrary::GetWatchedActorCount(const UObject* WorldContextObject)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->GetWatchedActorCount();
	}
	return 0;
}

void UUnifiedDebugPanelBPLibrary::SetUpdateInterval(const UObject* WorldContextObject, float Interval)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->SetUpdateInterval(Interval);
	}
}

void UUnifiedDebugPanelBPLibrary::SetEnabled(const UObject* WorldContextObject, bool bEnable)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		Subsystem->SetEnabled(bEnable);
	}
}

bool UUnifiedDebugPanelBPLibrary::IsEnabled(const UObject* WorldContextObject)
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetSubsystem(WorldContextObject))
	{
		return Subsystem->IsEnabled();
	}
	return false;
}

FString UUnifiedDebugPanelBPLibrary::GetActorSummary(const UObject* WorldContextObject, AActor* Actor)
{
	if (!Actor)
	{
		return TEXT("");
	}

	FActorInsightData Data;
	if (GetActorInsight(WorldContextObject, Actor, Data))
	{
		return Data.HumanReadableSummary;
	}

	return FString::Printf(TEXT("[%s] - No insight data available"), *Actor->GetName());
}

bool UUnifiedDebugPanelBPLibrary::IsActorExecutingAbility(const UObject* WorldContextObject, AActor* Actor)
{
	FActorInsightData Data;
	if (GetActorInsight(WorldContextObject, Actor, Data))
	{
		return Data.ActiveAbilities.Num() > 0;
	}
	return false;
}

bool UUnifiedDebugPanelBPLibrary::IsActorPlayingMontage(const UObject* WorldContextObject, AActor* Actor)
{
	FActorInsightData Data;
	if (GetActorInsight(WorldContextObject, Actor, Data))
	{
		return Data.ActiveMontages.Num() > 0;
	}
	return false;
}

int32 UUnifiedDebugPanelBPLibrary::GetActiveEffectCount(const UObject* WorldContextObject, AActor* Actor)
{
	FActorInsightData Data;
	if (GetActorInsight(WorldContextObject, Actor, Data))
	{
		return Data.ActiveEffects.Num();
	}
	return 0;
}

void UUnifiedDebugPanelBPLibrary::DisplayActorInsightOnScreen(const UObject* WorldContextObject, AActor* Actor, float Duration, FColor Color)
{
	if (!Actor)
	{
		return;
	}

	FActorInsightData Data;
	if (GetActorInsight(WorldContextObject, Actor, Data))
	{
		// GEngineを使用して画面にデバッグメッセージを表示
		if (GEngine)
		{
			// キーにアクターのアドレスを使用して重複を防ぐ
			int32 Key = GetTypeHash(Actor);
			GEngine->AddOnScreenDebugMessage(Key, Duration > 0.0f ? Duration : -1.0f, Color, Data.HumanReadableSummary);

			// 追加情報
			if (Data.ActiveAbilities.Num() > 0)
			{
				FString AbilityList = TEXT("  Abilities: ");
				for (const FAbilityDebugInfo& Ability : Data.ActiveAbilities)
				{
					AbilityList += Ability.AbilityName + TEXT(" ");
				}
				GEngine->AddOnScreenDebugMessage(Key + 1, Duration > 0.0f ? Duration : -1.0f, FColor::Cyan, AbilityList);
			}

			if (Data.ActiveMontages.Num() > 0)
			{
				FString MontageList = TEXT("  Montages: ");
				for (const FMontageDebugInfo& Montage : Data.ActiveMontages)
				{
					MontageList += FString::Printf(TEXT("%s (%.1fs) "), *Montage.MontageName, Montage.RemainingTime);
				}
				GEngine->AddOnScreenDebugMessage(Key + 2, Duration > 0.0f ? Duration : -1.0f, FColor::Orange, MontageList);
			}

			if (Data.ActiveEffects.Num() > 0)
			{
				FString EffectList = FString::Printf(TEXT("  Effects: %d active"), Data.ActiveEffects.Num());
				GEngine->AddOnScreenDebugMessage(Key + 3, Duration > 0.0f ? Duration : -1.0f, FColor::Magenta, EffectList);
			}
		}
	}
}
