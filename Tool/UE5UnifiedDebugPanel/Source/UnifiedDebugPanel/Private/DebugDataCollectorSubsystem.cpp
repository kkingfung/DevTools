// Copyright DevTools. All Rights Reserved.

#include "DebugDataCollectorSubsystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "ActiveGameplayEffectHandle.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTService.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

void UDebugDataCollectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// ワールドのティックにバインド
	if (UWorld* World = GetWorld())
	{
		TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([this](float DeltaTime) -> bool
			{
				if (bIsEnabled)
				{
					Tick(DeltaTime);
				}
				return true;
			}),
			UpdateInterval
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[DebugDataCollector] Subsystem initialized"));
}

void UDebugDataCollectorSubsystem::Deinitialize()
{
	// ティックデリゲートを解除
	if (TickDelegateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
		TickDelegateHandle.Reset();
	}

	ClearAllWatches();

	UE_LOG(LogTemp, Log, TEXT("[DebugDataCollector] Subsystem deinitialized"));

	Super::Deinitialize();
}

bool UDebugDataCollectorSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	// ゲームとPIE（Play In Editor）で有効
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UDebugDataCollectorSubsystem::Tick(float DeltaTime)
{
	TimeSinceLastUpdate += DeltaTime;

	if (TimeSinceLastUpdate < UpdateInterval)
	{
		return;
	}

	TimeSinceLastUpdate = 0.0f;

	// 無効なアクターをクリーンアップ
	CleanupInvalidActors();

	// 全監視対象のデータを更新
	for (const TWeakObjectPtr<AActor>& WeakActor : WatchedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			FActorInsightData InsightData = CollectActorInsight(Actor);
			CachedInsightData.Add(WeakActor, InsightData);

			// イベント発火
			OnActorInsightUpdated.Broadcast(InsightData);
		}
	}
}

void UDebugDataCollectorSubsystem::WatchActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// 既に監視中かチェック
	for (const TWeakObjectPtr<AActor>& WeakActor : WatchedActors)
	{
		if (WeakActor.Get() == Actor)
		{
			return;
		}
	}

	WatchedActors.Add(Actor);
	OnWatchedActorAdded.Broadcast(Actor);

	UE_LOG(LogTemp, Log, TEXT("[DebugDataCollector] Now watching: %s"), *Actor->GetName());
}

void UDebugDataCollectorSubsystem::UnwatchActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	for (int32 i = WatchedActors.Num() - 1; i >= 0; --i)
	{
		if (WatchedActors[i].Get() == Actor)
		{
			WatchedActors.RemoveAt(i);
			CachedInsightData.Remove(Actor);
			OnWatchedActorRemoved.Broadcast(Actor);

			UE_LOG(LogTemp, Log, TEXT("[DebugDataCollector] Stopped watching: %s"), *Actor->GetName());
			return;
		}
	}
}

void UDebugDataCollectorSubsystem::ClearAllWatches()
{
	for (const TWeakObjectPtr<AActor>& WeakActor : WatchedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			OnWatchedActorRemoved.Broadcast(Actor);
		}
	}

	WatchedActors.Empty();
	CachedInsightData.Empty();

	UE_LOG(LogTemp, Log, TEXT("[DebugDataCollector] All watches cleared"));
}

void UDebugDataCollectorSubsystem::WatchPlayerPawn(int32 PlayerIndex)
{
	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, PlayerIndex);
		if (PC && PC->GetPawn())
		{
			WatchActor(PC->GetPawn());
		}
	}
}

void UDebugDataCollectorSubsystem::WatchActorsWithTag(FName Tag)
{
	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> TaggedActors;
		UGameplayStatics::GetAllActorsWithTag(World, Tag, TaggedActors);

		for (AActor* Actor : TaggedActors)
		{
			WatchActor(Actor);
		}
	}
}

bool UDebugDataCollectorSubsystem::GetActorInsight(AActor* Actor, FActorInsightData& OutData) const
{
	if (!Actor)
	{
		return false;
	}

	const FActorInsightData* CachedData = CachedInsightData.Find(Actor);
	if (CachedData)
	{
		OutData = *CachedData;
		return true;
	}

	return false;
}

TArray<FActorInsightData> UDebugDataCollectorSubsystem::GetAllInsightData() const
{
	TArray<FActorInsightData> Result;
	CachedInsightData.GenerateValueArray(Result);
	return Result;
}

TArray<AActor*> UDebugDataCollectorSubsystem::GetWatchedActors() const
{
	TArray<AActor*> Result;
	for (const TWeakObjectPtr<AActor>& WeakActor : WatchedActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			Result.Add(Actor);
		}
	}
	return Result;
}

FActorInsightData UDebugDataCollectorSubsystem::CollectActorInsight(AActor* Actor)
{
	FActorInsightData Data;
	if (!Actor)
	{
		return Data;
	}

	Data.Actor = Actor;
	Data.LastUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// 基本状態
	Data.BasicState = CollectBasicState(Actor);

	// Ability System
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
	{
		CollectAbilitySystemData(ASC, Data);
	}

	// Animation (Character の場合)
	if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (USkeletalMeshComponent* SkelMesh = Character->GetMesh())
		{
			CollectAnimationData(SkelMesh, Data);
		}
	}

	// AI (Pawn with AI Controller)
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (AAIController* AIC = Cast<AAIController>(Pawn->GetController()))
		{
			if (UBehaviorTreeComponent* BTC = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent()))
			{
				CollectBehaviorTreeData(BTC, Data);
			}

			if (UBlackboardComponent* BBC = AIC->GetBlackboardComponent())
			{
				CollectBlackboardData(BBC, Data);
			}
		}
	}

	// Tick情報
	CollectTickData(Actor, Data);

	// 人間向けサマリー生成
	Data.HumanReadableSummary = GenerateHumanReadableSummary(Data);

	return Data;
}

FActorDebugState UDebugDataCollectorSubsystem::CollectBasicState(AActor* Actor)
{
	FActorDebugState State;

	if (!Actor)
	{
		return State;
	}

	State.ActorName = Actor->GetName();
	State.ClassName = Actor->GetClass()->GetName();
	State.Location = Actor->GetActorLocation();
	State.Rotation = Actor->GetActorRotation();
	State.bIsActive = !Actor->IsHidden();
	State.bIsTickEnabled = Actor->PrimaryActorTick.bCanEverTick && Actor->PrimaryActorTick.IsTickFunctionEnabled();

	// 速度取得（MovementComponent がある場合）
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		State.Velocity = Pawn->GetVelocity();
	}

	return State;
}

void UDebugDataCollectorSubsystem::CollectAbilitySystemData(UAbilitySystemComponent* ASC, FActorInsightData& OutData)
{
	if (!ASC)
	{
		return;
	}

	// 所有しているGameplayTags
	ASC->GetOwnedGameplayTags(OutData.OwnedGameplayTags);

	// 付与されているアビリティ
	const TArray<FGameplayAbilitySpec>& ActivatableAbilities = ASC->GetActivatableAbilities();
	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities)
	{
		if (!Spec.Ability)
		{
			continue;
		}

		FAbilityDebugInfo AbilityInfo;
		AbilityInfo.AbilityName = Spec.Ability->GetName();
		AbilityInfo.ClassName = Spec.Ability->GetClass()->GetName();
		AbilityInfo.Level = Spec.Level;
		AbilityInfo.bIsActive = Spec.IsActive();
		AbilityInfo.bInputBound = Spec.InputID != INDEX_NONE;

		// アビリティタグ取得
		if (const UGameplayAbility* AbilityCDO = Spec.Ability->GetClass()->GetDefaultObject<UGameplayAbility>())
		{
			AbilityInfo.AbilityTags = AbilityCDO->AbilityTags;
		}

		// クールダウン確認
		const UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
		if (AbilityInstance)
		{
			float RemainingCooldown = 0.0f;
			float CooldownDuration = 0.0f;
			AbilityInstance->GetCooldownTimeRemainingAndDuration(Spec.Handle, ASC->AbilityActorInfo.Get(), RemainingCooldown, CooldownDuration);
			AbilityInfo.bIsOnCooldown = RemainingCooldown > 0.0f;
			AbilityInfo.CooldownRemaining = RemainingCooldown;
		}

		if (AbilityInfo.bIsActive)
		{
			OutData.ActiveAbilities.Add(AbilityInfo);
		}
		OutData.GrantedAbilities.Add(AbilityInfo);
	}

	// アクティブなGameplayEffects
	FGameplayEffectQuery Query;
	TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffects(Query);

	for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
	{
		const FActiveGameplayEffect* ActiveEffect = ASC->GetActiveGameplayEffect(Handle);
		if (!ActiveEffect || !ActiveEffect->Spec.Def)
		{
			continue;
		}

		FEffectDebugInfo EffectInfo;
		EffectInfo.EffectName = ActiveEffect->Spec.Def->GetName();
		EffectInfo.StackCount = ActiveEffect->Spec.GetStackCount();

		// エフェクトタグ
		ActiveEffect->Spec.Def->GetAssetTags(EffectInfo.EffectTags);

		// 残り時間
		float Duration = ActiveEffect->GetDuration();
		float StartTime = ActiveEffect->StartWorldTime;
		if (Duration > 0.0f && GetWorld())
		{
			float ElapsedTime = GetWorld()->GetTimeSeconds() - StartTime;
			EffectInfo.RemainingTime = FMath::Max(0.0f, Duration - ElapsedTime);
		}

		// ソース情報
		if (ActiveEffect->Spec.GetContext().GetInstigator())
		{
			EffectInfo.InstigatorName = ActiveEffect->Spec.GetContext().GetInstigator()->GetName();
		}

		OutData.ActiveEffects.Add(EffectInfo);
	}
}

void UDebugDataCollectorSubsystem::CollectAnimationData(USkeletalMeshComponent* SkelMesh, FActorInsightData& OutData)
{
	if (!SkelMesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = SkelMesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	// アクティブなモンタージュ
	if (UAnimMontage* CurrentMontage = AnimInstance->GetCurrentActiveMontage())
	{
		FMontageDebugInfo MontageInfo;
		MontageInfo.MontageName = CurrentMontage->GetName();
		MontageInfo.Position = AnimInstance->Montage_GetPosition(CurrentMontage);
		MontageInfo.PlayRate = AnimInstance->Montage_GetPlayRate(CurrentMontage);
		MontageInfo.bIsBlendingOut = AnimInstance->Montage_GetIsStopped(CurrentMontage);

		// 現在のセクション
		FName CurrentSection = AnimInstance->Montage_GetCurrentSection(CurrentMontage);
		MontageInfo.CurrentSectionName = CurrentSection.ToString();

		// 残り時間計算
		float MontageLength = CurrentMontage->GetPlayLength();
		MontageInfo.RemainingTime = FMath::Max(0.0f, MontageLength - MontageInfo.Position);

		OutData.ActiveMontages.Add(MontageInfo);
	}

	// ステートマシン情報
	// Note: UE5ではステートマシン情報の取得が複雑なため、簡略化実装
	// より詳細な情報が必要な場合はAnimInstance内部のステートマシンを直接参照
}

void UDebugDataCollectorSubsystem::CollectBehaviorTreeData(UBehaviorTreeComponent* BTC, FActorInsightData& OutData)
{
	if (!BTC)
	{
		return;
	}

	OutData.BehaviorTree.bIsRunning = BTC->IsRunning();

	// ビヘイビアツリー名
	if (UBehaviorTree* Tree = BTC->GetCurrentTree())
	{
		OutData.BehaviorTree.TreeName = Tree->GetName();
	}

	// 現在実行中のノード（デバッグ情報から取得）
	// Note: ビヘイビアツリーの内部状態へのアクセスは制限されているため
	// エディタビルドでのみ詳細情報を取得
#if WITH_EDITOR
	// エディタでの詳細なデバッグ情報取得
	TArray<FString> ExecutionDesc;
	BTC->DescribeRuntimeValues(ExecutionDesc);

	if (ExecutionDesc.Num() > 0)
	{
		OutData.BehaviorTree.CurrentNodeName = ExecutionDesc[0];
	}
#endif
}

void UDebugDataCollectorSubsystem::CollectBlackboardData(UBlackboardComponent* BBC, FActorInsightData& OutData)
{
	if (!BBC)
	{
		return;
	}

	// Blackboardの全キーを取得
	if (UBlackboardData* BBData = BBC->GetBlackboardAsset())
	{
		for (const FBlackboardEntry& Key : BBData->Keys)
		{
			FString KeyName = Key.EntryName.ToString();
			FString ValueStr = BBC->DescribeKeyValue(BBC->GetKeyID(Key.EntryName), EBlackboardDescription::Detailed);
			OutData.Blackboard.KeyValues.Add(KeyName, ValueStr);
		}
	}
}

void UDebugDataCollectorSubsystem::CollectTickData(AActor* Actor, FActorInsightData& OutData)
{
	if (!Actor)
	{
		return;
	}

	// アクター自身のティック情報
	if (Actor->PrimaryActorTick.bCanEverTick)
	{
		FTickDebugInfo ActorTickInfo;
		ActorTickInfo.Name = Actor->GetName();
		ActorTickInfo.bIsEnabled = Actor->PrimaryActorTick.IsTickFunctionEnabled();

		// TickGroup を文字列に変換
		switch (Actor->PrimaryActorTick.TickGroup)
		{
		case TG_PrePhysics:
			ActorTickInfo.TickGroup = TEXT("PrePhysics");
			break;
		case TG_DuringPhysics:
			ActorTickInfo.TickGroup = TEXT("DuringPhysics");
			break;
		case TG_PostPhysics:
			ActorTickInfo.TickGroup = TEXT("PostPhysics");
			break;
		case TG_PostUpdateWork:
			ActorTickInfo.TickGroup = TEXT("PostUpdateWork");
			break;
		default:
			ActorTickInfo.TickGroup = TEXT("Unknown");
			break;
		}

		OutData.TickInfo.Add(ActorTickInfo);
	}

	// コンポーネントのティック情報
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		if (Component && Component->PrimaryComponentTick.bCanEverTick)
		{
			FTickDebugInfo CompTickInfo;
			CompTickInfo.Name = Component->GetName();
			CompTickInfo.bIsEnabled = Component->PrimaryComponentTick.IsTickFunctionEnabled();

			switch (Component->PrimaryComponentTick.TickGroup)
			{
			case TG_PrePhysics:
				CompTickInfo.TickGroup = TEXT("PrePhysics");
				break;
			case TG_DuringPhysics:
				CompTickInfo.TickGroup = TEXT("DuringPhysics");
				break;
			case TG_PostPhysics:
				CompTickInfo.TickGroup = TEXT("PostPhysics");
				break;
			case TG_PostUpdateWork:
				CompTickInfo.TickGroup = TEXT("PostUpdateWork");
				break;
			default:
				CompTickInfo.TickGroup = TEXT("Unknown");
				break;
			}

			OutData.TickInfo.Add(CompTickInfo);
		}
	}
}

FString UDebugDataCollectorSubsystem::GenerateHumanReadableSummary(const FActorInsightData& Data)
{
	TArray<FString> SummaryParts;

	// 基本情報
	SummaryParts.Add(FString::Printf(TEXT("[%s]"), *Data.BasicState.ActorName));

	// 移動状態
	float Speed = Data.BasicState.Velocity.Size();
	if (Speed > 10.0f)
	{
		SummaryParts.Add(FString::Printf(TEXT("移動中 (%.0f cm/s)"), Speed));
	}
	else
	{
		SummaryParts.Add(TEXT("静止中"));
	}

	// アビリティ状態
	if (Data.ActiveAbilities.Num() > 0)
	{
		TArray<FString> AbilityNames;
		for (const FAbilityDebugInfo& Ability : Data.ActiveAbilities)
		{
			AbilityNames.Add(Ability.AbilityName);
		}
		SummaryParts.Add(FString::Printf(TEXT("実行中アビリティ: %s"), *FString::Join(AbilityNames, TEXT(", "))));
	}

	// モンタージュ状態
	if (Data.ActiveMontages.Num() > 0)
	{
		const FMontageDebugInfo& Montage = Data.ActiveMontages[0];
		SummaryParts.Add(FString::Printf(TEXT("再生中: %s (%.1fs)"), *Montage.MontageName, Montage.RemainingTime));
	}

	// エフェクト状態
	if (Data.ActiveEffects.Num() > 0)
	{
		SummaryParts.Add(FString::Printf(TEXT("エフェクト: %d個適用中"), Data.ActiveEffects.Num()));
	}

	// AI状態
	if (Data.BehaviorTree.bIsRunning)
	{
		FString AIStatus = FString::Printf(TEXT("AI: %s"), *Data.BehaviorTree.TreeName);
		if (!Data.BehaviorTree.CurrentNodeName.IsEmpty())
		{
			AIStatus += FString::Printf(TEXT(" → %s"), *Data.BehaviorTree.CurrentNodeName);
		}
		SummaryParts.Add(AIStatus);
	}

	// GameplayTags
	if (!Data.OwnedGameplayTags.IsEmpty())
	{
		TArray<FString> TagStrings;
		for (const FGameplayTag& Tag : Data.OwnedGameplayTags)
		{
			TagStrings.Add(Tag.ToString());
		}
		SummaryParts.Add(FString::Printf(TEXT("Tags: %s"), *FString::Join(TagStrings, TEXT(", "))));
	}

	return FString::Join(SummaryParts, TEXT(" | "));
}

void UDebugDataCollectorSubsystem::CleanupInvalidActors()
{
	for (int32 i = WatchedActors.Num() - 1; i >= 0; --i)
	{
		if (!WatchedActors[i].IsValid())
		{
			WatchedActors.RemoveAt(i);
		}
	}

	// キャッシュからも削除
	TArray<TWeakObjectPtr<AActor>> KeysToRemove;
	for (auto& Pair : CachedInsightData)
	{
		if (!Pair.Key.IsValid())
		{
			KeysToRemove.Add(Pair.Key);
		}
	}
	for (const TWeakObjectPtr<AActor>& Key : KeysToRemove)
	{
		CachedInsightData.Remove(Key);
	}
}
