#nullable enable
using System;
using System.Collections.Generic;

namespace LogicPresentationSync
{
    /// <summary>
    /// イベントのカテゴリ
    /// </summary>
    public enum EventCategory
    {
        /// <summary>ゲームロジックイベント</summary>
        Logic,
        /// <summary>プレゼンテーション（表現）イベント</summary>
        Presentation
    }

    /// <summary>
    /// プレゼンテーションイベントのタイプ
    /// </summary>
    public enum PresentationType
    {
        /// <summary>アニメーションイベント</summary>
        Animation,
        /// <summary>VFX/パーティクル</summary>
        VFX,
        /// <summary>オーディオ</summary>
        Audio,
        /// <summary>UI更新</summary>
        UI,
        /// <summary>カメラエフェクト</summary>
        Camera,
        /// <summary>その他</summary>
        Other
    }

    /// <summary>
    /// 同期イベントの基本データ
    /// </summary>
    [Serializable]
    public class SyncEvent
    {
        /// <summary>イベントID</summary>
        public int EventId;

        /// <summary>イベントカテゴリ</summary>
        public EventCategory Category;

        /// <summary>イベントタグ（ペアリング用識別子）</summary>
        public string Tag = string.Empty;

        /// <summary>イベント名</summary>
        public string EventName = string.Empty;

        /// <summary>タイムスタンプ（秒）</summary>
        public double Timestamp;

        /// <summary>フレーム番号</summary>
        public int Frame;

        /// <summary>関連オブジェクト名</summary>
        public string SourceObject = string.Empty;

        /// <summary>追加情報</summary>
        public string Details = string.Empty;
    }

    /// <summary>
    /// ロジックイベント
    /// </summary>
    [Serializable]
    public class LogicEvent : SyncEvent
    {
        public LogicEvent()
        {
            Category = EventCategory.Logic;
        }
    }

    /// <summary>
    /// プレゼンテーションイベント
    /// </summary>
    [Serializable]
    public class PresentationEvent : SyncEvent
    {
        /// <summary>プレゼンテーションタイプ</summary>
        public PresentationType Type;

        public PresentationEvent()
        {
            Category = EventCategory.Presentation;
        }
    }

    /// <summary>
    /// イベントペア（ロジックとプレゼンテーションの対応）
    /// </summary>
    [Serializable]
    public class EventPair
    {
        /// <summary>ペアID</summary>
        public int PairId;

        /// <summary>タグ</summary>
        public string Tag = string.Empty;

        /// <summary>ロジックイベント</summary>
        public LogicEvent? LogicEvent;

        /// <summary>プレゼンテーションイベントのリスト</summary>
        public List<PresentationEvent> PresentationEvents = new();

        /// <summary>フレーム差分（最初のプレゼンテーション）</summary>
        public int FrameDifference;

        /// <summary>時間差分（ミリ秒）</summary>
        public double TimeDifferenceMs;

        /// <summary>ロジックイベントのみ（プレゼンテーションなし）</summary>
        public bool IsMissingPresentation => LogicEvent != null && PresentationEvents.Count == 0;

        /// <summary>プレゼンテーションイベントのみ（ロジックなし）</summary>
        public bool IsMissingLogic => LogicEvent == null && PresentationEvents.Count > 0;

        /// <summary>同期問題あり</summary>
        public bool HasSyncIssue => IsMissingPresentation || IsMissingLogic || Math.Abs(FrameDifference) > 0;
    }

    /// <summary>
    /// 同期分析結果
    /// </summary>
    [Serializable]
    public class SyncAnalysisResult
    {
        /// <summary>分析期間の開始時刻</summary>
        public string StartTime = string.Empty;

        /// <summary>分析期間の終了時刻</summary>
        public string EndTime = string.Empty;

        /// <summary>総イベント数</summary>
        public int TotalEventCount;

        /// <summary>ロジックイベント数</summary>
        public int LogicEventCount;

        /// <summary>プレゼンテーションイベント数</summary>
        public int PresentationEventCount;

        /// <summary>ペアリングされたイベント数</summary>
        public int PairedEventCount;

        /// <summary>未ペアのロジックイベント数</summary>
        public int UnpairedLogicCount;

        /// <summary>未ペアのプレゼンテーションイベント数</summary>
        public int UnpairedPresentationCount;

        /// <summary>同期問題のあるペア数</summary>
        public int SyncIssueCount;

        /// <summary>平均フレーム差分</summary>
        public double AverageFrameDifference;

        /// <summary>最大フレーム差分</summary>
        public int MaxFrameDifference;

        /// <summary>平均時間差分（ミリ秒）</summary>
        public double AverageTimeDifferenceMs;

        /// <summary>イベントペアのリスト</summary>
        public List<EventPair> EventPairs = new();
    }

    /// <summary>
    /// 同期閾値設定
    /// </summary>
    [Serializable]
    public class SyncThresholds
    {
        /// <summary>許容フレーム差分（これを超えると警告）</summary>
        public int AcceptableFrameDifference = 1;

        /// <summary>許容時間差分（ミリ秒）</summary>
        public double AcceptableTimeDifferenceMs = 33.33; // 約2フレーム @60fps

        /// <summary>ペアリングタイムアウト（秒）</summary>
        public float PairingTimeoutSeconds = 1.0f;
    }
}
