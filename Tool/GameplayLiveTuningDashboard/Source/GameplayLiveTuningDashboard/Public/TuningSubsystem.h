// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TuningTypes.h"
#include "TuningSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnParameterChanged, FName, ParameterId, const FTuningValue&, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionChanged, const FTuningSession&, Session);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWarningTriggered, const FTuningComparison&, Warning);

/**
 * ゲームプレイチューニングサブシステム
 * パラメータの管理、履歴追跡、ライブ変更を担当
 */
UCLASS()
class GAMEPLAYELIVETUNINGDASHBOARD_API UTuningSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** シングルトンアクセス（エディタ用） */
	static UTuningSubsystem* Get();

	// ========== パラメータ管理 ==========

	/**
	 * パラメータを登録
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	void RegisterParameter(const FTuningParameter& Parameter);

	/**
	 * パラメータを一括登録
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	void RegisterParameters(const TArray<FTuningParameter>& Parameters);

	/**
	 * パラメータを取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool GetParameter(FName ParameterId, FTuningParameter& OutParameter) const;

	/**
	 * レイヤーのパラメータを取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningParameter> GetParametersByLayer(ETuningLayer Layer) const;

	/**
	 * カテゴリのパラメータを取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningParameter> GetParametersByCategory(const FString& Category) const;

	/**
	 * 全パラメータを取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningParameter> GetAllParameters() const;

	/**
	 * タグでパラメータを検索
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningParameter> SearchParametersByTag(const FString& Tag) const;

	// ========== 値の変更 ==========

	/**
	 * パラメータ値を変更（即時反映）
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool SetParameterValue(FName ParameterId, const FTuningValue& NewValue, const FString& Comment = TEXT(""));

	/**
	 * Float値を設定
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool SetFloatValue(FName ParameterId, float Value, const FString& Comment = TEXT(""));

	/**
	 * Int値を設定
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool SetIntValue(FName ParameterId, int32 Value, const FString& Comment = TEXT(""));

	/**
	 * Bool値を設定
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool SetBoolValue(FName ParameterId, bool Value, const FString& Comment = TEXT(""));

	/**
	 * デフォルト値にリセット
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool ResetToDefault(FName ParameterId);

	/**
	 * 全パラメータをデフォルトにリセット
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	void ResetAllToDefault();

	/**
	 * 実際のオブジェクトに値を適用
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool ApplyValueToTarget(FName ParameterId);

	// ========== 履歴管理 ==========

	/**
	 * 変更履歴を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningHistoryEntry> GetHistory(int32 MaxEntries = 100) const;

	/**
	 * パラメータの履歴を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningHistoryEntry> GetParameterHistory(FName ParameterId, int32 MaxEntries = 50) const;

	/**
	 * 変更を取り消し（Undo）
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool UndoLastChange();

	/**
	 * 取り消しをやり直し（Redo）
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool RedoChange();

	/**
	 * 履歴をクリア
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	void ClearHistory();

	// ========== セッション管理 ==========

	/**
	 * 新しいセッションを開始
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	FTuningSession StartSession(const FString& SessionName);

	/**
	 * 現在のセッションを終了
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	void EndCurrentSession();

	/**
	 * 現在のセッションを取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool GetCurrentSession(FTuningSession& OutSession) const;

	/**
	 * 過去のセッション一覧を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningSession> GetSessionHistory() const;

	// ========== 比較機能 ==========

	/**
	 * 現在の値とデフォルトを比較
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningComparison> CompareWithDefault() const;

	/**
	 * 2つのセッションを比較
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningComparison> CompareSessions(const FGuid& SessionA, const FGuid& SessionB) const;

	/**
	 * 現在とプリセットを比較
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningComparison> CompareWithPreset(const FTuningPreset& Preset) const;

	// ========== プリセット ==========

	/**
	 * 現在の値をプリセットとして保存
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	FTuningPreset SaveAsPreset(const FString& PresetName, const FString& Description = TEXT(""));

	/**
	 * プリセットを適用
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool ApplyPreset(const FTuningPreset& Preset);

	/**
	 * プリセット一覧を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningPreset> GetPresets() const;

	/**
	 * プリセットを削除
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool DeletePreset(const FGuid& PresetId);

	// ========== ベンチマーク ==========

	/**
	 * 安全ベンチマークを実行
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	FTuningBenchmarkResult RunSafetyBenchmark();

	/**
	 * レイヤーのサマリーを取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	TArray<FTuningLayerSummary> GetLayerSummaries() const;

	// ========== インポート/エクスポート ==========

	/**
	 * JSONにエクスポート
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	FString ExportToJson() const;

	/**
	 * JSONからインポート
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool ImportFromJson(const FString& JsonString);

	/**
	 * ファイルに保存
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool SaveToFile(const FString& FilePath);

	/**
	 * ファイルから読み込み
	 */
	UFUNCTION(BlueprintCallable, Category = "Tuning")
	bool LoadFromFile(const FString& FilePath);

	// ========== イベント ==========

	/** パラメータ変更時 */
	UPROPERTY(BlueprintAssignable, Category = "Tuning")
	FOnParameterChanged OnParameterChanged;

	/** セッション変更時 */
	UPROPERTY(BlueprintAssignable, Category = "Tuning")
	FOnSessionChanged OnSessionChanged;

	/** 警告発生時 */
	UPROPERTY(BlueprintAssignable, Category = "Tuning")
	FOnWarningTriggered OnWarningTriggered;

protected:
	/** 履歴にエントリを追加 */
	void AddHistoryEntry(const FTuningHistoryEntry& Entry);

	/** 警告をチェック */
	void CheckWarnings(const FTuningParameter& Parameter, const FTuningValue& OldValue, const FTuningValue& NewValue);

	/** プロパティに値を適用 */
	bool ApplyValueToProperty(UObject* Object, const FString& PropertyName, const FTuningValue& Value);

private:
	/** パラメータマップ */
	UPROPERTY()
	TMap<FName, FTuningParameter> Parameters;

	/** 変更履歴 */
	UPROPERTY()
	TArray<FTuningHistoryEntry> History;

	/** Redo用スタック */
	UPROPERTY()
	TArray<FTuningHistoryEntry> RedoStack;

	/** 現在のセッション */
	UPROPERTY()
	FTuningSession CurrentSession;

	/** セッション履歴 */
	UPROPERTY()
	TArray<FTuningSession> SessionHistory;

	/** プリセット */
	UPROPERTY()
	TArray<FTuningPreset> Presets;

	/** 履歴の最大エントリ数 */
	static const int32 MaxHistoryEntries = 1000;

	/** シングルトンインスタンス（エディタ用） */
	static UTuningSubsystem* EditorInstance;
};
