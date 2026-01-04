// Copyright DevTools. All Rights Reserved.

#include "TuningSubsystem.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "UObject/PropertyAccessUtil.h"

UTuningSubsystem* UTuningSubsystem::EditorInstance = nullptr;

void UTuningSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// デフォルトセッション開始
	CurrentSession = FTuningSession();
	CurrentSession.SessionName = TEXT("Default Session");

	// エディタインスタンスを設定
	if (!EditorInstance)
	{
		EditorInstance = this;
	}

	UE_LOG(LogTemp, Log, TEXT("[TuningSubsystem] Initialized"));
}

void UTuningSubsystem::Deinitialize()
{
	// 現在のセッションを終了
	if (CurrentSession.bIsActive)
	{
		EndCurrentSession();
	}

	if (EditorInstance == this)
	{
		EditorInstance = nullptr;
	}

	Super::Deinitialize();
}

UTuningSubsystem* UTuningSubsystem::Get()
{
	// エディタの場合は静的インスタンスを使用
	if (EditorInstance)
	{
		return EditorInstance;
	}

	// ゲームインスタンスから取得を試行
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* World = Context.World())
			{
				if (UGameInstance* GameInstance = World->GetGameInstance())
				{
					return GameInstance->GetSubsystem<UTuningSubsystem>();
				}
			}
		}
	}

	return nullptr;
}

// ========== パラメータ管理 ==========

void UTuningSubsystem::RegisterParameter(const FTuningParameter& Parameter)
{
	Parameters.Add(Parameter.ParameterId, Parameter);
	UE_LOG(LogTemp, Log, TEXT("[TuningSubsystem] Registered parameter: %s"), *Parameter.ParameterId.ToString());
}

void UTuningSubsystem::RegisterParameters(const TArray<FTuningParameter>& InParameters)
{
	for (const FTuningParameter& Param : InParameters)
	{
		RegisterParameter(Param);
	}
}

bool UTuningSubsystem::GetParameter(FName ParameterId, FTuningParameter& OutParameter) const
{
	const FTuningParameter* Found = Parameters.Find(ParameterId);
	if (Found)
	{
		OutParameter = *Found;
		return true;
	}
	return false;
}

TArray<FTuningParameter> UTuningSubsystem::GetParametersByLayer(ETuningLayer Layer) const
{
	TArray<FTuningParameter> Result;
	for (const auto& Pair : Parameters)
	{
		if (Pair.Value.Layer == Layer)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FTuningParameter> UTuningSubsystem::GetParametersByCategory(const FString& Category) const
{
	TArray<FTuningParameter> Result;
	for (const auto& Pair : Parameters)
	{
		if (Pair.Value.Category == Category)
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

TArray<FTuningParameter> UTuningSubsystem::GetAllParameters() const
{
	TArray<FTuningParameter> Result;
	Parameters.GenerateValueArray(Result);
	return Result;
}

TArray<FTuningParameter> UTuningSubsystem::SearchParametersByTag(const FString& Tag) const
{
	TArray<FTuningParameter> Result;
	for (const auto& Pair : Parameters)
	{
		if (Pair.Value.Tags.Contains(Tag))
		{
			Result.Add(Pair.Value);
		}
	}
	return Result;
}

// ========== 値の変更 ==========

bool UTuningSubsystem::SetParameterValue(FName ParameterId, const FTuningValue& NewValue, const FString& Comment)
{
	FTuningParameter* Param = Parameters.Find(ParameterId);
	if (!Param)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TuningSubsystem] Parameter not found: %s"), *ParameterId.ToString());
		return false;
	}

	FTuningValue OldValue = Param->CurrentValue;

	// 警告チェック
	CheckWarnings(*Param, OldValue, NewValue);

	// 履歴に追加
	FTuningHistoryEntry Entry;
	Entry.ParameterId = ParameterId;
	Entry.OldValue = OldValue;
	Entry.NewValue = NewValue;
	Entry.SessionId = CurrentSession.SessionId;
	Entry.Comment = Comment;
	Entry.ModifiedBy = TEXT("User"); // TODO: 実際のユーザー名

	AddHistoryEntry(Entry);

	// 値を更新
	Param->CurrentValue = NewValue;
	Param->LastModified = FDateTime::Now();
	Param->ModifiedBy = Entry.ModifiedBy;

	// Redoスタッククリア
	RedoStack.Empty();

	// 実際のオブジェクトに適用
	ApplyValueToTarget(ParameterId);

	// イベント発火
	OnParameterChanged.Broadcast(ParameterId, NewValue);

	UE_LOG(LogTemp, Log, TEXT("[TuningSubsystem] Parameter changed: %s = %s"),
		*ParameterId.ToString(), *NewValue.ToString());

	return true;
}

bool UTuningSubsystem::SetFloatValue(FName ParameterId, float Value, const FString& Comment)
{
	FTuningValue NewValue;
	NewValue.ValueType = ETuningValueType::Float;
	NewValue.FloatValue = Value;
	return SetParameterValue(ParameterId, NewValue, Comment);
}

bool UTuningSubsystem::SetIntValue(FName ParameterId, int32 Value, const FString& Comment)
{
	FTuningValue NewValue;
	NewValue.ValueType = ETuningValueType::Integer;
	NewValue.IntValue = Value;
	return SetParameterValue(ParameterId, NewValue, Comment);
}

bool UTuningSubsystem::SetBoolValue(FName ParameterId, bool Value, const FString& Comment)
{
	FTuningValue NewValue;
	NewValue.ValueType = ETuningValueType::Boolean;
	NewValue.BoolValue = Value;
	return SetParameterValue(ParameterId, NewValue, Comment);
}

bool UTuningSubsystem::ResetToDefault(FName ParameterId)
{
	FTuningParameter* Param = Parameters.Find(ParameterId);
	if (!Param)
	{
		return false;
	}

	return SetParameterValue(ParameterId, Param->DefaultValue, TEXT("Reset to default"));
}

void UTuningSubsystem::ResetAllToDefault()
{
	for (auto& Pair : Parameters)
	{
		SetParameterValue(Pair.Key, Pair.Value.DefaultValue, TEXT("Reset all to default"));
	}
}

bool UTuningSubsystem::ApplyValueToTarget(FName ParameterId)
{
	FTuningParameter* Param = Parameters.Find(ParameterId);
	if (!Param || Param->TargetObjectPath.IsEmpty())
	{
		return false;
	}

	// オブジェクトを検索
	UObject* TargetObject = FindObject<UObject>(nullptr, *Param->TargetObjectPath);
	if (!TargetObject)
	{
		// ロードを試行
		TargetObject = LoadObject<UObject>(nullptr, *Param->TargetObjectPath);
	}

	if (!TargetObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TuningSubsystem] Target object not found: %s"), *Param->TargetObjectPath);
		return false;
	}

	return ApplyValueToProperty(TargetObject, Param->TargetPropertyName, Param->CurrentValue);
}

bool UTuningSubsystem::ApplyValueToProperty(UObject* Object, const FString& PropertyName, const FTuningValue& Value)
{
	if (!Object)
	{
		return false;
	}

	FProperty* Property = Object->GetClass()->FindPropertyByName(*PropertyName);
	if (!Property)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TuningSubsystem] Property not found: %s"), *PropertyName);
		return false;
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);

	switch (Value.ValueType)
	{
	case ETuningValueType::Float:
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
		{
			FloatProp->SetPropertyValue(ValuePtr, Value.FloatValue);
			return true;
		}
		else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
		{
			DoubleProp->SetPropertyValue(ValuePtr, static_cast<double>(Value.FloatValue));
			return true;
		}
		break;

	case ETuningValueType::Integer:
		if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
		{
			IntProp->SetPropertyValue(ValuePtr, Value.IntValue);
			return true;
		}
		break;

	case ETuningValueType::Boolean:
		if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			BoolProp->SetPropertyValue(ValuePtr, Value.BoolValue);
			return true;
		}
		break;

	case ETuningValueType::Vector:
		if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			if (StructProp->Struct == TBaseStructure<FVector>::Get())
			{
				*static_cast<FVector*>(ValuePtr) = Value.VectorValue;
				return true;
			}
		}
		break;
	}

	return false;
}

// ========== 履歴管理 ==========

TArray<FTuningHistoryEntry> UTuningSubsystem::GetHistory(int32 MaxEntries) const
{
	int32 Count = FMath::Min(MaxEntries, History.Num());
	TArray<FTuningHistoryEntry> Result;

	for (int32 i = History.Num() - 1; i >= 0 && Result.Num() < Count; --i)
	{
		Result.Add(History[i]);
	}

	return Result;
}

TArray<FTuningHistoryEntry> UTuningSubsystem::GetParameterHistory(FName ParameterId, int32 MaxEntries) const
{
	TArray<FTuningHistoryEntry> Result;

	for (int32 i = History.Num() - 1; i >= 0 && Result.Num() < MaxEntries; --i)
	{
		if (History[i].ParameterId == ParameterId)
		{
			Result.Add(History[i]);
		}
	}

	return Result;
}

bool UTuningSubsystem::UndoLastChange()
{
	if (History.Num() == 0)
	{
		return false;
	}

	FTuningHistoryEntry LastEntry = History.Pop();
	RedoStack.Add(LastEntry);

	// 値を戻す
	FTuningParameter* Param = Parameters.Find(LastEntry.ParameterId);
	if (Param)
	{
		Param->CurrentValue = LastEntry.OldValue;
		ApplyValueToTarget(LastEntry.ParameterId);
		OnParameterChanged.Broadcast(LastEntry.ParameterId, LastEntry.OldValue);
	}

	return true;
}

bool UTuningSubsystem::RedoChange()
{
	if (RedoStack.Num() == 0)
	{
		return false;
	}

	FTuningHistoryEntry Entry = RedoStack.Pop();

	// 値を再適用
	FTuningParameter* Param = Parameters.Find(Entry.ParameterId);
	if (Param)
	{
		Param->CurrentValue = Entry.NewValue;
		History.Add(Entry);
		ApplyValueToTarget(Entry.ParameterId);
		OnParameterChanged.Broadcast(Entry.ParameterId, Entry.NewValue);
	}

	return true;
}

void UTuningSubsystem::ClearHistory()
{
	History.Empty();
	RedoStack.Empty();
}

void UTuningSubsystem::AddHistoryEntry(const FTuningHistoryEntry& Entry)
{
	History.Add(Entry);

	// セッションにも追加
	if (CurrentSession.bIsActive)
	{
		CurrentSession.Changes.Add(Entry);
	}

	// 最大エントリ数を超えたら古いものを削除
	while (History.Num() > MaxHistoryEntries)
	{
		History.RemoveAt(0);
	}
}

// ========== セッション管理 ==========

FTuningSession UTuningSubsystem::StartSession(const FString& SessionName)
{
	// 現在のセッションを終了
	if (CurrentSession.bIsActive)
	{
		EndCurrentSession();
	}

	// 新しいセッションを開始
	CurrentSession = FTuningSession();
	CurrentSession.SessionName = SessionName;

	OnSessionChanged.Broadcast(CurrentSession);

	UE_LOG(LogTemp, Log, TEXT("[TuningSubsystem] Session started: %s"), *SessionName);

	return CurrentSession;
}

void UTuningSubsystem::EndCurrentSession()
{
	if (!CurrentSession.bIsActive)
	{
		return;
	}

	CurrentSession.Close();
	SessionHistory.Add(CurrentSession);

	OnSessionChanged.Broadcast(CurrentSession);

	UE_LOG(LogTemp, Log, TEXT("[TuningSubsystem] Session ended: %s (%d changes)"),
		*CurrentSession.SessionName, CurrentSession.GetChangeCount());
}

bool UTuningSubsystem::GetCurrentSession(FTuningSession& OutSession) const
{
	if (CurrentSession.bIsActive)
	{
		OutSession = CurrentSession;
		return true;
	}
	return false;
}

TArray<FTuningSession> UTuningSubsystem::GetSessionHistory() const
{
	return SessionHistory;
}

// ========== 比較機能 ==========

TArray<FTuningComparison> UTuningSubsystem::CompareWithDefault() const
{
	TArray<FTuningComparison> Result;

	for (const auto& Pair : Parameters)
	{
		const FTuningParameter& Param = Pair.Value;

		float Diff = Param.CurrentValue.GetDifference(Param.DefaultValue);
		if (!FMath::IsNearlyZero(Diff))
		{
			FTuningComparison Comp;
			Comp.Parameter = Param;
			Comp.BeforeValue = Param.DefaultValue;
			Comp.AfterValue = Param.CurrentValue;
			Comp.Difference = Diff;
			Comp.PercentChange = Param.CurrentValue.GetPercentChange(Param.DefaultValue);
			Comp.WarningLevel = Param.Threshold.CheckChange(Comp.PercentChange);
			Result.Add(Comp);
		}
	}

	return Result;
}

TArray<FTuningComparison> UTuningSubsystem::CompareSessions(const FGuid& SessionA, const FGuid& SessionB) const
{
	TArray<FTuningComparison> Result;

	// セッションを検索
	const FTuningSession* SessionAPtr = nullptr;
	const FTuningSession* SessionBPtr = nullptr;

	for (const FTuningSession& Session : SessionHistory)
	{
		if (Session.SessionId == SessionA) SessionAPtr = &Session;
		if (Session.SessionId == SessionB) SessionBPtr = &Session;
	}

	if (!SessionAPtr || !SessionBPtr)
	{
		return Result;
	}

	// 両セッションの最終値を比較
	TMap<FName, FTuningValue> ValuesA;
	TMap<FName, FTuningValue> ValuesB;

	for (const FTuningHistoryEntry& Entry : SessionAPtr->Changes)
	{
		ValuesA.Add(Entry.ParameterId, Entry.NewValue);
	}

	for (const FTuningHistoryEntry& Entry : SessionBPtr->Changes)
	{
		ValuesB.Add(Entry.ParameterId, Entry.NewValue);
	}

	// 比較
	TSet<FName> AllParams;
	for (const auto& Pair : ValuesA) AllParams.Add(Pair.Key);
	for (const auto& Pair : ValuesB) AllParams.Add(Pair.Key);

	for (const FName& ParamId : AllParams)
	{
		FTuningParameter Param;
		if (!GetParameter(ParamId, Param)) continue;

		FTuningValue ValueA = ValuesA.Contains(ParamId) ? ValuesA[ParamId] : Param.DefaultValue;
		FTuningValue ValueB = ValuesB.Contains(ParamId) ? ValuesB[ParamId] : Param.DefaultValue;

		float Diff = ValueB.GetDifference(ValueA);
		if (!FMath::IsNearlyZero(Diff))
		{
			FTuningComparison Comp;
			Comp.Parameter = Param;
			Comp.BeforeValue = ValueA;
			Comp.AfterValue = ValueB;
			Comp.Difference = Diff;
			Comp.PercentChange = ValueB.GetPercentChange(ValueA);
			Comp.WarningLevel = Param.Threshold.CheckChange(Comp.PercentChange);
			Result.Add(Comp);
		}
	}

	return Result;
}

TArray<FTuningComparison> UTuningSubsystem::CompareWithPreset(const FTuningPreset& Preset) const
{
	TArray<FTuningComparison> Result;

	for (const auto& Pair : Preset.ParameterValues)
	{
		FTuningParameter Param;
		if (!GetParameter(Pair.Key, Param)) continue;

		float Diff = Param.CurrentValue.GetDifference(Pair.Value);
		if (!FMath::IsNearlyZero(Diff))
		{
			FTuningComparison Comp;
			Comp.Parameter = Param;
			Comp.BeforeValue = Pair.Value;
			Comp.AfterValue = Param.CurrentValue;
			Comp.Difference = Diff;
			Comp.PercentChange = Param.CurrentValue.GetPercentChange(Pair.Value);
			Comp.WarningLevel = Param.Threshold.CheckChange(Comp.PercentChange);
			Result.Add(Comp);
		}
	}

	return Result;
}

// ========== プリセット ==========

FTuningPreset UTuningSubsystem::SaveAsPreset(const FString& PresetName, const FString& Description)
{
	FTuningPreset Preset;
	Preset.PresetName = PresetName;
	Preset.Description = Description;

	for (const auto& Pair : Parameters)
	{
		Preset.ParameterValues.Add(Pair.Key, Pair.Value.CurrentValue);

		// レイヤーを記録
		if (!Preset.TargetLayers.Contains(Pair.Value.Layer))
		{
			Preset.TargetLayers.Add(Pair.Value.Layer);
		}
	}

	Presets.Add(Preset);

	UE_LOG(LogTemp, Log, TEXT("[TuningSubsystem] Preset saved: %s"), *PresetName);

	return Preset;
}

bool UTuningSubsystem::ApplyPreset(const FTuningPreset& Preset)
{
	for (const auto& Pair : Preset.ParameterValues)
	{
		SetParameterValue(Pair.Key, Pair.Value, FString::Printf(TEXT("Applied preset: %s"), *Preset.PresetName));
	}

	UE_LOG(LogTemp, Log, TEXT("[TuningSubsystem] Preset applied: %s"), *Preset.PresetName);

	return true;
}

TArray<FTuningPreset> UTuningSubsystem::GetPresets() const
{
	return Presets;
}

bool UTuningSubsystem::DeletePreset(const FGuid& PresetId)
{
	for (int32 i = 0; i < Presets.Num(); ++i)
	{
		if (Presets[i].PresetId == PresetId)
		{
			Presets.RemoveAt(i);
			return true;
		}
	}
	return false;
}

// ========== ベンチマーク ==========

FTuningBenchmarkResult UTuningSubsystem::RunSafetyBenchmark()
{
	FTuningBenchmarkResult Result;
	Result.BenchmarkName = TEXT("Safety Benchmark");
	Result.bPassed = true;

	for (const auto& Pair : Parameters)
	{
		const FTuningParameter& Param = Pair.Value;
		Result.CheckedParameterCount++;

		// 値の閾値チェック
		ETuningWarningLevel ValueWarning = Param.Threshold.CheckValue(Param.CurrentValue.GetAsFloat());

		// デフォルトからの変化率チェック
		float PercentChange = Param.CurrentValue.GetPercentChange(Param.DefaultValue);
		ETuningWarningLevel ChangeWarning = Param.Threshold.CheckChange(PercentChange);

		// より厳しい警告レベルを採用
		ETuningWarningLevel FinalWarning = FMath::Max(ValueWarning, ChangeWarning);

		if (FinalWarning != ETuningWarningLevel::None)
		{
			FTuningComparison Comp;
			Comp.Parameter = Param;
			Comp.BeforeValue = Param.DefaultValue;
			Comp.AfterValue = Param.CurrentValue;
			Comp.Difference = Param.CurrentValue.GetDifference(Param.DefaultValue);
			Comp.PercentChange = PercentChange;
			Comp.WarningLevel = FinalWarning;

			if (FinalWarning == ETuningWarningLevel::Critical)
			{
				Result.CriticalWarnings.Add(Comp);
				Result.bPassed = false;
			}
			else
			{
				Result.Warnings.Add(Comp);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TuningSubsystem] Benchmark completed: %s (%d warnings, %d critical)"),
		Result.bPassed ? TEXT("PASSED") : TEXT("FAILED"),
		Result.Warnings.Num(),
		Result.CriticalWarnings.Num());

	return Result;
}

TArray<FTuningLayerSummary> UTuningSubsystem::GetLayerSummaries() const
{
	TMap<ETuningLayer, FTuningLayerSummary> SummaryMap;

	// 全レイヤーを初期化
	for (int32 i = 0; i < static_cast<int32>(ETuningLayer::Custom) + 1; ++i)
	{
		ETuningLayer Layer = static_cast<ETuningLayer>(i);
		FTuningLayerSummary Summary;
		Summary.Layer = Layer;
		SummaryMap.Add(Layer, Summary);
	}

	// パラメータを集計
	for (const auto& Pair : Parameters)
	{
		const FTuningParameter& Param = Pair.Value;
		FTuningLayerSummary& Summary = SummaryMap[Param.Layer];

		Summary.ParameterCount++;

		// デフォルトから変更されているか
		if (!FMath::IsNearlyZero(Param.CurrentValue.GetDifference(Param.DefaultValue)))
		{
			Summary.ModifiedCount++;
		}

		// 警告チェック
		ETuningWarningLevel Warning = Param.Threshold.CheckValue(Param.CurrentValue.GetAsFloat());
		if (Warning == ETuningWarningLevel::Critical)
		{
			Summary.CriticalCount++;
		}
		else if (Warning == ETuningWarningLevel::Warning)
		{
			Summary.WarningCount++;
		}
	}

	TArray<FTuningLayerSummary> Result;
	SummaryMap.GenerateValueArray(Result);
	return Result;
}

// ========== 警告チェック ==========

void UTuningSubsystem::CheckWarnings(const FTuningParameter& Parameter, const FTuningValue& OldValue, const FTuningValue& NewValue)
{
	float PercentChange = NewValue.GetPercentChange(OldValue);
	ETuningWarningLevel ChangeWarning = Parameter.Threshold.CheckChange(PercentChange);
	ETuningWarningLevel ValueWarning = Parameter.Threshold.CheckValue(NewValue.GetAsFloat());

	ETuningWarningLevel FinalWarning = FMath::Max(ChangeWarning, ValueWarning);

	if (FinalWarning != ETuningWarningLevel::None)
	{
		FTuningComparison Warning;
		Warning.Parameter = Parameter;
		Warning.BeforeValue = OldValue;
		Warning.AfterValue = NewValue;
		Warning.Difference = NewValue.GetDifference(OldValue);
		Warning.PercentChange = PercentChange;
		Warning.WarningLevel = FinalWarning;

		OnWarningTriggered.Broadcast(Warning);

		UE_LOG(LogTemp, Warning, TEXT("[TuningSubsystem] Warning: %s changed by %.1f%% (Level: %d)"),
			*Parameter.ParameterId.ToString(),
			PercentChange,
			static_cast<int32>(FinalWarning));
	}
}

// ========== インポート/エクスポート ==========

FString UTuningSubsystem::ExportToJson() const
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();

	// パラメータをエクスポート
	TArray<TSharedPtr<FJsonValue>> ParamArray;
	for (const auto& Pair : Parameters)
	{
		TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
		ParamObj->SetStringField(TEXT("Id"), Pair.Key.ToString());
		ParamObj->SetStringField(TEXT("DisplayName"), Pair.Value.DisplayName);
		ParamObj->SetNumberField(TEXT("Layer"), static_cast<int32>(Pair.Value.Layer));
		ParamObj->SetStringField(TEXT("Category"), Pair.Value.Category);
		ParamObj->SetNumberField(TEXT("ValueType"), static_cast<int32>(Pair.Value.CurrentValue.ValueType));
		ParamObj->SetNumberField(TEXT("FloatValue"), Pair.Value.CurrentValue.FloatValue);
		ParamObj->SetNumberField(TEXT("IntValue"), Pair.Value.CurrentValue.IntValue);
		ParamObj->SetBoolField(TEXT("BoolValue"), Pair.Value.CurrentValue.BoolValue);

		ParamArray.Add(MakeShared<FJsonValueObject>(ParamObj));
	}
	RootObject->SetArrayField(TEXT("Parameters"), ParamArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	return OutputString;
}

bool UTuningSubsystem::ImportFromJson(const FString& JsonString)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* ParamArray;
	if (!RootObject->TryGetArrayField(TEXT("Parameters"), ParamArray))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Value : *ParamArray)
	{
		TSharedPtr<FJsonObject> ParamObj = Value->AsObject();
		if (!ParamObj.IsValid()) continue;

		FTuningParameter Param;
		Param.ParameterId = FName(*ParamObj->GetStringField(TEXT("Id")));
		Param.DisplayName = ParamObj->GetStringField(TEXT("DisplayName"));
		Param.Layer = static_cast<ETuningLayer>(ParamObj->GetIntegerField(TEXT("Layer")));
		Param.Category = ParamObj->GetStringField(TEXT("Category"));
		Param.CurrentValue.ValueType = static_cast<ETuningValueType>(ParamObj->GetIntegerField(TEXT("ValueType")));
		Param.CurrentValue.FloatValue = ParamObj->GetNumberField(TEXT("FloatValue"));
		Param.CurrentValue.IntValue = ParamObj->GetIntegerField(TEXT("IntValue"));
		Param.CurrentValue.BoolValue = ParamObj->GetBoolField(TEXT("BoolValue"));

		Parameters.Add(Param.ParameterId, Param);
	}

	return true;
}

bool UTuningSubsystem::SaveToFile(const FString& FilePath)
{
	FString JsonContent = ExportToJson();
	return FFileHelper::SaveStringToFile(JsonContent, *FilePath);
}

bool UTuningSubsystem::LoadFromFile(const FString& FilePath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
	{
		return false;
	}
	return ImportFromJson(JsonContent);
}
