#nullable enable
using System;
using System.Collections.Generic;

namespace InputLatencyAnalyzer
{
    /// <summary>
    /// レスポンス検出タイプ
    /// </summary>
    [Flags]
    public enum ResponseType
    {
        None = 0,
        /// <summary>画面の変化（レンダリング）</summary>
        Visual = 1 << 0,
        /// <summary>アニメーション開始</summary>
        Animation = 1 << 1,
        /// <summary>オーディオ再生開始</summary>
        Audio = 1 << 2,
        /// <summary>すべて</summary>
        All = Visual | Animation | Audio
    }

    /// <summary>
    /// 単一の遅延計測結果
    /// </summary>
    [Serializable]
    public class LatencyMeasurement
    {
        /// <summary>計測ID</summary>
        public int MeasurementId;

        /// <summary>計測時刻</summary>
        public string Timestamp = string.Empty;

        /// <summary>入力タイムスタンプ（秒、Time.realtimeSinceStartup）</summary>
        public double InputTime;

        /// <summary>入力時のフレーム番号</summary>
        public int InputFrame;

        /// <summary>レスポンスタイムスタンプ（秒）</summary>
        public double ResponseTime;

        /// <summary>レスポンス時のフレーム番号</summary>
        public int ResponseFrame;

        /// <summary>検出されたレスポンスタイプ</summary>
        public ResponseType DetectedResponseType;

        /// <summary>遅延時間（ミリ秒）</summary>
        public double LatencyMs;

        /// <summary>遅延フレーム数</summary>
        public int LatencyFrames;

        /// <summary>入力アクション名</summary>
        public string InputAction = string.Empty;

        /// <summary>レスポンスの詳細説明</summary>
        public string ResponseDescription = string.Empty;

        /// <summary>計測成功フラグ</summary>
        public bool IsValid;

        /// <summary>タイムアウトしたかどうか</summary>
        public bool TimedOut;
    }

    /// <summary>
    /// 計測セッション（複数の計測をグループ化）
    /// </summary>
    [Serializable]
    public class LatencySession
    {
        /// <summary>セッションID</summary>
        public string SessionId = string.Empty;

        /// <summary>セッション名</summary>
        public string SessionName = string.Empty;

        /// <summary>開始時刻</summary>
        public string StartTime = string.Empty;

        /// <summary>終了時刻</summary>
        public string EndTime = string.Empty;

        /// <summary>計測対象の入力アクション</summary>
        public string TargetInputAction = string.Empty;

        /// <summary>検出対象のレスポンスタイプ</summary>
        public ResponseType TargetResponseTypes;

        /// <summary>計測結果リスト</summary>
        public List<LatencyMeasurement> Measurements = new();

        /// <summary>計測の統計情報</summary>
        public LatencyStatistics Statistics = new();
    }

    /// <summary>
    /// 遅延統計情報
    /// </summary>
    [Serializable]
    public class LatencyStatistics
    {
        /// <summary>計測回数</summary>
        public int SampleCount;

        /// <summary>有効な計測数</summary>
        public int ValidSampleCount;

        /// <summary>平均遅延（ミリ秒）</summary>
        public double AverageMs;

        /// <summary>最小遅延（ミリ秒）</summary>
        public double MinMs;

        /// <summary>最大遅延（ミリ秒）</summary>
        public double MaxMs;

        /// <summary>標準偏差（ミリ秒）</summary>
        public double StandardDeviationMs;

        /// <summary>中央値（ミリ秒）</summary>
        public double MedianMs;

        /// <summary>95パーセンタイル（ミリ秒）</summary>
        public double Percentile95Ms;

        /// <summary>99パーセンタイル（ミリ秒）</summary>
        public double Percentile99Ms;

        /// <summary>平均遅延（フレーム）</summary>
        public double AverageFrames;

        /// <summary>最小遅延（フレーム）</summary>
        public int MinFrames;

        /// <summary>最大遅延（フレーム）</summary>
        public int MaxFrames;
    }

    /// <summary>
    /// 計測状態
    /// </summary>
    public enum MeasurementState
    {
        /// <summary>待機中（入力待ち）</summary>
        Idle,
        /// <summary>入力検出済み、レスポンス待ち</summary>
        WaitingForResponse,
        /// <summary>計測完了</summary>
        Completed,
        /// <summary>タイムアウト</summary>
        TimedOut
    }
}
