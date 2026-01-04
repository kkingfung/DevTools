// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TuningTypes.generated.h"

/**
 * パラメータレイヤー
 */
UENUM(BlueprintType)
enum class ETuningLayer : uint8
{
	Character	UMETA(DisplayName = "キャラクター"),
	Weapon		UMETA(DisplayName = "武器"),
	Skill		UMETA(DisplayName = "スキル"),
	Stage		UMETA(DisplayName = "ステージ"),
	AI			UMETA(DisplayName = "AI"),
	Economy		UMETA(DisplayName = "経済"),
	Custom		UMETA(DisplayName = "カスタム")
};

/**
 * パラメータの値タイプ
 */
UENUM(BlueprintType)
enum class ETuningValueType : uint8
{
	Float		UMETA(DisplayName = "Float"),
	Integer		UMETA(DisplayName = "Integer"),
	Boolean		UMETA(DisplayName = "Boolean"),
	Vector		UMETA(DisplayName = "Vector"),
	Curve		UMETA(DisplayName = "Curve")
};

/**
 * 警告レベル
 */
UENUM(BlueprintType)
enum class ETuningWarningLevel : uint8
{
	None		UMETA(DisplayName = "なし"),
	Info		UMETA(DisplayName = "情報"),
	Warning		UMETA(DisplayName = "警告"),
	Critical	UMETA(DisplayName = "危険")
};

/**
 * パラメータ値（多様な型に対応）
 */
USTRUCT(BlueprintType)
struct FTuningValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETuningValueType ValueType = ETuningValueType::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloatValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IntValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool BoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector VectorValue = FVector::ZeroVector;

	/** 値を文字列で取得 */
	FString ToString() const
	{
		switch (ValueType)
		{
		case ETuningValueType::Float:
			return FString::Printf(TEXT("%.3f"), FloatValue);
		case ETuningValueType::Integer:
			return FString::Printf(TEXT("%d"), IntValue);
		case ETuningValueType::Boolean:
			return BoolValue ? TEXT("true") : TEXT("false");
		case ETuningValueType::Vector:
			return FString::Printf(TEXT("(%.1f, %.1f, %.1f)"), VectorValue.X, VectorValue.Y, VectorValue.Z);
		default:
			return TEXT("N/A");
		}
	}

	/** Float値として取得（正規化） */
	float GetAsFloat() const
	{
		switch (ValueType)
		{
		case ETuningValueType::Float:
			return FloatValue;
		case ETuningValueType::Integer:
			return static_cast<float>(IntValue);
		case ETuningValueType::Boolean:
			return BoolValue ? 1.0f : 0.0f;
		default:
			return 0.0f;
		}
	}

	/** 差分を計算 */
	float GetDifference(const FTuningValue& Other) const
	{
		return GetAsFloat() - Other.GetAsFloat();
	}

	/** パーセント変化を計算 */
	float GetPercentChange(const FTuningValue& Original) const
	{
		float OriginalFloat = Original.GetAsFloat();
		if (FMath::IsNearlyZero(OriginalFloat))
		{
			return 0.0f;
		}
		return ((GetAsFloat() - OriginalFloat) / FMath::Abs(OriginalFloat)) * 100.0f;
	}
};

/**
 * パラメータの安全閾値
 */
USTRUCT(BlueprintType)
struct FTuningThreshold
{
	GENERATED_BODY()

	/** 最小値（これ以下で警告） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinValue = 0.0f;

	/** 最大値（これ以上で警告） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxValue = 100.0f;

	/** 最小危険値（これ以下で危険） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalMinValue = -100.0f;

	/** 最大危険値（これ以上で危険） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CriticalMaxValue = 200.0f;

	/** 最大変化率（1回の変更でこれ以上変わると警告） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxChangePercent = 50.0f;

	/** 閾値有効フラグ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;

	/** 値が閾値内かチェック */
	ETuningWarningLevel CheckValue(float Value) const
	{
		if (!bEnabled) return ETuningWarningLevel::None;

		if (Value <= CriticalMinValue || Value >= CriticalMaxValue)
		{
			return ETuningWarningLevel::Critical;
		}
		if (Value <= MinValue || Value >= MaxValue)
		{
			return ETuningWarningLevel::Warning;
		}
		return ETuningWarningLevel::None;
	}

	/** 変化率が閾値内かチェック */
	ETuningWarningLevel CheckChange(float PercentChange) const
	{
		if (!bEnabled) return ETuningWarningLevel::None;

		if (FMath::Abs(PercentChange) >= MaxChangePercent * 2.0f)
		{
			return ETuningWarningLevel::Critical;
		}
		if (FMath::Abs(PercentChange) >= MaxChangePercent)
		{
			return ETuningWarningLevel::Warning;
		}
		return ETuningWarningLevel::None;
	}
};

/**
 * チューニングパラメータ定義
 */
USTRUCT(BlueprintType)
struct FTuningParameter
{
	GENERATED_BODY()

	/** パラメータID（一意識別子） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParameterId;

	/** 表示名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	/** 説明 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;

	/** レイヤー */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETuningLayer Layer = ETuningLayer::Character;

	/** カテゴリ（レイヤー内のサブグループ） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Category;

	/** 現在値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTuningValue CurrentValue;

	/** デフォルト値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTuningValue DefaultValue;

	/** 安全閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTuningThreshold Threshold;

	/** 対象オブジェクトパス（適用先） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TargetObjectPath;

	/** 対象プロパティ名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TargetPropertyName;

	/** タグ（検索用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> Tags;

	/** 最終更新時刻 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime LastModified;

	/** 変更者 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ModifiedBy;

	FTuningParameter()
		: LastModified(FDateTime::Now())
	{
	}
};

/**
 * パラメータ変更履歴エントリ
 */
USTRUCT(BlueprintType)
struct FTuningHistoryEntry
{
	GENERATED_BODY()

	/** パラメータID */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName ParameterId;

	/** 変更前の値 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTuningValue OldValue;

	/** 変更後の値 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTuningValue NewValue;

	/** 変更時刻 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime Timestamp;

	/** 変更者 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString ModifiedBy;

	/** セッションID（関連する変更をグループ化） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid SessionId;

	/** コメント */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Comment;

	FTuningHistoryEntry()
		: Timestamp(FDateTime::Now())
		, SessionId(FGuid::NewGuid())
	{
	}

	/** 変化率を取得 */
	float GetPercentChange() const
	{
		return NewValue.GetPercentChange(OldValue);
	}
};

/**
 * チューニングセッション（一連の変更をグループ化）
 */
USTRUCT(BlueprintType)
struct FTuningSession
{
	GENERATED_BODY()

	/** セッションID */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid SessionId;

	/** セッション名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SessionName;

	/** 開始時刻 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime StartTime;

	/** 終了時刻 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime EndTime;

	/** このセッションでの変更履歴 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FTuningHistoryEntry> Changes;

	/** セッションメモ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Notes;

	/** セッションがアクティブか */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsActive = true;

	FTuningSession()
		: SessionId(FGuid::NewGuid())
		, StartTime(FDateTime::Now())
		, bIsActive(true)
	{
	}

	/** セッション内の変更数 */
	int32 GetChangeCount() const { return Changes.Num(); }

	/** セッションを閉じる */
	void Close()
	{
		EndTime = FDateTime::Now();
		bIsActive = false;
	}
};

/**
 * パラメータ比較結果
 */
USTRUCT(BlueprintType)
struct FTuningComparison
{
	GENERATED_BODY()

	/** パラメータ */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTuningParameter Parameter;

	/** 比較元の値（Before） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTuningValue BeforeValue;

	/** 比較先の値（After） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTuningValue AfterValue;

	/** 差分 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Difference = 0.0f;

	/** 変化率（%） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float PercentChange = 0.0f;

	/** 警告レベル */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETuningWarningLevel WarningLevel = ETuningWarningLevel::None;
};

/**
 * レイヤーサマリー
 */
USTRUCT(BlueprintType)
struct FTuningLayerSummary
{
	GENERATED_BODY()

	/** レイヤー */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ETuningLayer Layer = ETuningLayer::Character;

	/** パラメータ数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ParameterCount = 0;

	/** 変更済みパラメータ数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ModifiedCount = 0;

	/** 警告数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 WarningCount = 0;

	/** 危険警告数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CriticalCount = 0;
};

/**
 * プリセット（パラメータセットの保存用）
 */
USTRUCT(BlueprintType)
struct FTuningPreset
{
	GENERATED_BODY()

	/** プリセットID */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid PresetId;

	/** プリセット名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PresetName;

	/** 説明 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;

	/** 作成日時 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime CreatedAt;

	/** パラメータ値のマップ（ParameterId -> Value） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FTuningValue> ParameterValues;

	/** 適用対象レイヤー */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<ETuningLayer> TargetLayers;

	FTuningPreset()
		: PresetId(FGuid::NewGuid())
		, CreatedAt(FDateTime::Now())
	{
	}
};

/**
 * ベンチマーク結果
 */
USTRUCT(BlueprintType)
struct FTuningBenchmarkResult
{
	GENERATED_BODY()

	/** ベンチマーク名 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString BenchmarkName;

	/** 実行時刻 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FDateTime Timestamp;

	/** 全警告リスト */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FTuningComparison> Warnings;

	/** 致命的警告リスト */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FTuningComparison> CriticalWarnings;

	/** チェック済みパラメータ数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CheckedParameterCount = 0;

	/** 合格判定 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bPassed = true;

	FTuningBenchmarkResult()
		: Timestamp(FDateTime::Now())
	{
	}
};
