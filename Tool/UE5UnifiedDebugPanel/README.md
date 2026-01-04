# UE5 Unified Debug & Insight Panel

🧠 **UE5の内部を"人間の言葉"に翻訳する統合デバッグパネル**

![UE5](https://img.shields.io/badge/UE5-5.0%2B-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## 概要

UE5の標準デバッグツールは点在・分断・専門家向けで、BP / C++ / Animation / AI / Ability が分離しています。

このプラグインは**1画面に全てを統合表示**し、「今このキャラが何をしているか」を即座に把握できるようにします。

### 表示内容

- 🎮 **Gameplay State**: アクターの基本状態（位置、速度、ティック情報）
- ⚔️ **Ability System (GAS)**: 実行中アビリティ、付与済みアビリティ、クールダウン
- ✨ **Gameplay Effects**: アクティブなエフェクト、スタック数、残り時間
- 🎬 **Animation**: 再生中のMontage、セクション、再生位置
- 🤖 **AI State**: Behavior Tree実行状況、Blackboard値
- ⏱️ **Tick/Task**: ティック有効状態、TickGroup
- 🏷️ **GameplayTags**: 保持しているタグ一覧

## なぜ必要か

| 従来の問題 | このプラグインの解決策 |
|-----------|---------------------|
| デバッグ情報が散在 | 1画面に統合表示 |
| 専門知識が必要 | 人間が読める形式で表示 |
| 毎プロジェクトで再実装 | 汎用プラグインとして再利用可能 |
| リアルタイム確認が困難 | 自動更新で常に最新状態 |

## インストール

1. `UE5UnifiedDebugPanel` フォルダを `YourProject/Plugins/` にコピー
2. プロジェクトを再起動
3. **Window** → **Unified Debug Panel** でパネルを開く

## 使い方

### エディタでの使用

1. **Window** メニューから **Unified Debug Panel** を開く
2. **Watch Player** ボタンでプレイヤーPawnを監視対象に追加
3. PIE（Play In Editor）を開始
4. リアルタイムでアクターの状態を確認

### Blueprintでの使用

```
// アクターを監視対象に追加
Watch Actor (Actor)

// プレイヤーPawnを監視
Watch Player Pawn (PlayerIndex)

// タグを持つアクターを全て監視
Watch Actors With Tag (Tag)

// Insightデータを取得
Get Actor Insight (Actor) → FActorInsightData

// 画面にサマリーを表示
Display Actor Insight On Screen (Actor, Duration, Color)

// アクターの状態を確認
Is Actor Executing Ability (Actor) → bool
Is Actor Playing Montage (Actor) → bool
Get Active Effect Count (Actor) → int
```

### C++での使用

```cpp
#include "DebugDataCollectorSubsystem.h"

// サブシステムを取得
UDebugDataCollectorSubsystem* DebugSubsystem = GetWorld()->GetSubsystem<UDebugDataCollectorSubsystem>();

// アクターを監視
DebugSubsystem->WatchActor(MyActor);

// Insightデータを取得
FActorInsightData InsightData;
if (DebugSubsystem->GetActorInsight(MyActor, InsightData))
{
    UE_LOG(LogTemp, Log, TEXT("Summary: %s"), *InsightData.HumanReadableSummary);
}

// イベントにバインド
DebugSubsystem->OnActorInsightUpdated.AddDynamic(this, &AMyActor::OnInsightUpdated);
```

## UI パネル構成

```
┌─────────────────────────────────────────────────────────┐
│ [Watch Player] [Clear All]          [✓ 自動更新] [Refresh] │
├──────────────────┬──────────────────────────────────────┤
│ 監視対象アクター    │ Insight 詳細                          │
│                  │                                      │
│ ┌──────────────┐ │ ┌────────────────────────────────────┐ │
│ │ BP_Player    │ │ │ ▼ 基本情報                          │ │
│ │ [Active]     │ │ │   Class: BP_PlayerCharacter        │ │
│ │ Abilities: 2 │ │ │   Location: (100, 200, 0)          │ │
│ │ Effects: 3   │ │ │   Velocity: 450 cm/s               │ │
│ │ Montages: 1  │ │ │                                    │ │
│ └──────────────┘ │ │ ▼ アビリティ                         │ │
│                  │ │   実行中: GA_Attack, GA_Dash        │ │
│ ┌──────────────┐ │ │   付与済み: 5個                      │ │
│ │ BP_Enemy_01  │ │ │                                    │ │
│ │ [Active]     │ │ │ ▼ エフェクト                         │ │
│ │ AI: BT_Patrol│ │ │   GE_Buff_Strength (x2) 5.2s       │ │
│ └──────────────┘ │ │   GE_DOT_Poison 3.1s               │ │
│                  │ │                                    │ │
└──────────────────┴──────────────────────────────────────┘
```

## データ構造

### FActorInsightData

監視対象アクターの全情報を含む構造体:

| プロパティ | 型 | 説明 |
|-----------|-----|------|
| Actor | TWeakObjectPtr<AActor> | 監視対象アクター |
| BasicState | FActorDebugState | 基本状態（位置、速度等） |
| ActiveAbilities | TArray<FAbilityDebugInfo> | 実行中アビリティ |
| GrantedAbilities | TArray<FAbilityDebugInfo> | 付与済みアビリティ |
| ActiveEffects | TArray<FEffectDebugInfo> | アクティブなエフェクト |
| ActiveMontages | TArray<FMontageDebugInfo> | 再生中モンタージュ |
| BehaviorTree | FBehaviorTreeDebugInfo | BT実行状況 |
| Blackboard | FBlackboardDebugInfo | Blackboard値 |
| TickInfo | TArray<FTickDebugInfo> | ティック情報 |
| OwnedGameplayTags | FGameplayTagContainer | 保持タグ |
| HumanReadableSummary | FString | 人間向けサマリー |

## 設定

### 更新間隔の変更

```cpp
// デフォルト: 0.1秒（10Hz）
DebugSubsystem->SetUpdateInterval(0.05f);  // 20Hz に変更
```

### 有効/無効の切り替え

```cpp
// 一時的に無効化（パフォーマンス向上）
DebugSubsystem->SetEnabled(false);

// 再度有効化
DebugSubsystem->SetEnabled(true);
```

## 依存関係

このプラグインは以下のモジュールに依存しています:

- **GameplayAbilities** - GAS機能のため
- **GameplayTags** - タグ表示のため
- **GameplayTasks** - タスク監視のため
- **AIModule** - AI/BehaviorTree監視のため

## ファイル構成

```
UE5UnifiedDebugPanel/
├── UnifiedDebugPanel.uplugin
├── README.md
├── Resources/
│   └── Icon128.png
└── Source/
    ├── UnifiedDebugPanel/          # ランタイムモジュール
    │   ├── UnifiedDebugPanel.Build.cs
    │   ├── Public/
    │   │   ├── UnifiedDebugPanelModule.h
    │   │   ├── DebugDataTypes.h
    │   │   ├── DebugDataCollectorSubsystem.h
    │   │   └── UnifiedDebugPanelBPLibrary.h
    │   └── Private/
    │       ├── UnifiedDebugPanelModule.cpp
    │       ├── DebugDataCollectorSubsystem.cpp
    │       └── UnifiedDebugPanelBPLibrary.cpp
    └── UnifiedDebugPanelEditor/    # エディタモジュール
        ├── UnifiedDebugPanelEditor.Build.cs
        ├── Public/
        │   ├── UnifiedDebugPanelEditorModule.h
        │   └── SUnifiedDebugPanel.h
        └── Private/
            ├── UnifiedDebugPanelEditorModule.cpp
            └── SUnifiedDebugPanel.cpp
```

## 今後の機能拡張予定

- [ ] StateTree サポート
- [ ] Enhanced Input サポート
- [ ] Niagara エフェクト追跡
- [ ] ネットワーク状態表示
- [ ] 履歴/タイムライン表示
- [ ] カスタムデータプロバイダ機能
- [ ] プリセット（表示項目のカスタマイズ）

## トラブルシューティング

### パネルが表示されない

1. プラグインが有効になっているか確認（Edit → Plugins）
2. GameplayAbilitiesプラグインが有効か確認
3. エディタを再起動

### データが更新されない

1. PIE（Play In Editor）が実行中か確認
2. `SetEnabled(true)` が呼ばれているか確認
3. アクターが監視対象に追加されているか確認

### GAS情報が表示されない

1. アクターにAbilitySystemComponentがアタッチされているか確認
2. AbilitySystemComponentが正しく初期化されているか確認

## ライセンス

MIT License

## 作者

DevTools Project

---

初心者から AAA まで、全てのUE5開発者のデバッグ体験を向上させます。
