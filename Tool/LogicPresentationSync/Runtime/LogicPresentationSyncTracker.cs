#nullable enable
using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;

namespace LogicPresentationSync
{
    /// <summary>
    /// ロジックとプレゼンテーションの同期を追跡するマネージャーコンポーネント
    /// </summary>
    public class LogicPresentationSyncTracker : MonoBehaviour
    {
        /// <summary>シングルトンインスタンス</summary>
        public static LogicPresentationSyncTracker? Instance { get; private set; }

        [Header("トラッキング設定")]
        [SerializeField]
        [Tooltip("トラッキングを有効にする")]
        private bool _enableTracking = true;

        [SerializeField]
        [Tooltip("同期問題をコンソールに出力")]
        private bool _logSyncIssues = true;

        [SerializeField]
        [Tooltip("許容フレーム差分")]
        private int _acceptableFrameDifference = 1;

        [SerializeField]
        [Tooltip("ペアリングタイムアウト（秒）")]
        private float _pairingTimeout = 1.0f;

        [Header("自動保存設定")]
        [SerializeField]
        [Tooltip("終了時に自動でレポートを保存")]
        private bool _autoSaveOnQuit;

        [SerializeField]
        [Tooltip("保存先ディレクトリ")]
        private string _outputDirectory = string.Empty;

        // Correlator
        private SyncEventCorrelator _correlator = new();
        private float _lastTimeoutCheck;
        private const float TimeoutCheckInterval = 0.5f;

        // 履歴
        private readonly List<SyncAnalysisResult> _analysisHistory = new();

        /// <summary>トラッキングが有効かどうか</summary>
        public bool IsTracking => _enableTracking;

        /// <summary>Correlator</summary>
        public SyncEventCorrelator Correlator => _correlator;

        /// <summary>分析履歴</summary>
        public IReadOnlyList<SyncAnalysisResult> AnalysisHistory => _analysisHistory;

        /// <summary>同期問題検出時イベント</summary>
        public event Action<EventPair>? OnSyncIssue;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }

            Instance = this;
            DontDestroyOnLoad(gameObject);

            // 閾値設定
            _correlator.Thresholds = new SyncThresholds
            {
                AcceptableFrameDifference = _acceptableFrameDifference,
                PairingTimeoutSeconds = _pairingTimeout
            };

            // イベント購読
            SyncEventMarker.OnLogicEvent += HandleLogicEvent;
            SyncEventMarker.OnPresentationEvent += HandlePresentationEvent;
            _correlator.OnSyncIssueDetected += HandleSyncIssue;
        }

        private void OnDestroy()
        {
            if (Instance == this)
            {
                SyncEventMarker.OnLogicEvent -= HandleLogicEvent;
                SyncEventMarker.OnPresentationEvent -= HandlePresentationEvent;
                _correlator.OnSyncIssueDetected -= HandleSyncIssue;
                Instance = null;
            }
        }

        private void OnApplicationQuit()
        {
            if (_autoSaveOnQuit)
            {
                SaveCurrentAnalysis();
            }
        }

        private void Update()
        {
            if (!_enableTracking) return;

            // 定期的にタイムアウトをチェック
            if (Time.realtimeSinceStartup - _lastTimeoutCheck > TimeoutCheckInterval)
            {
                _correlator.ProcessTimeouts(Time.realtimeSinceStartupAsDouble);
                _lastTimeoutCheck = Time.realtimeSinceStartup;
            }
        }

        /// <summary>
        /// ロジックイベントを処理
        /// </summary>
        private void HandleLogicEvent(LogicEvent logicEvent)
        {
            if (!_enableTracking) return;
            _correlator.AddLogicEvent(logicEvent);
        }

        /// <summary>
        /// プレゼンテーションイベントを処理
        /// </summary>
        private void HandlePresentationEvent(PresentationEvent presentationEvent)
        {
            if (!_enableTracking) return;
            _correlator.AddPresentationEvent(presentationEvent);
        }

        /// <summary>
        /// 同期問題を処理
        /// </summary>
        private void HandleSyncIssue(EventPair pair)
        {
            if (_logSyncIssues)
            {
                string issueType;
                if (pair.IsMissingPresentation)
                {
                    issueType = "プレゼンテーションイベントなし";
                }
                else if (pair.IsMissingLogic)
                {
                    issueType = "ロジックイベントなし";
                }
                else
                {
                    issueType = $"フレーム差分: {pair.FrameDifference}F ({pair.TimeDifferenceMs:F1}ms)";
                }

                Debug.LogWarning($"[LogicPresentationSync] 同期問題検出\n" +
                                 $"  タグ: {pair.Tag}\n" +
                                 $"  問題: {issueType}\n" +
                                 $"  ロジック: {(pair.LogicEvent != null ? $"{pair.LogicEvent.EventName} (Frame {pair.LogicEvent.Frame})" : "なし")}\n" +
                                 $"  プレゼンテーション: {pair.PresentationEvents.Count}個");
            }

            OnSyncIssue?.Invoke(pair);
        }

        /// <summary>
        /// トラッキングを開始
        /// </summary>
        public void StartTracking()
        {
            _enableTracking = true;
            SyncEventMarker.IsEnabled = true;
            Debug.Log("[LogicPresentationSync] トラッキングを開始しました");
        }

        /// <summary>
        /// トラッキングを停止
        /// </summary>
        public void StopTracking()
        {
            _enableTracking = false;
            SyncEventMarker.IsEnabled = false;
            _correlator.CompleteAllPending();
            Debug.Log("[LogicPresentationSync] トラッキングを停止しました");
        }

        /// <summary>
        /// 現在の分析結果を取得
        /// </summary>
        public SyncAnalysisResult GetCurrentAnalysis()
        {
            return _correlator.GenerateAnalysis();
        }

        /// <summary>
        /// 分析結果をスナップショットとして保存
        /// </summary>
        public void SnapshotAnalysis(string? label = null)
        {
            var analysis = GetCurrentAnalysis();
            analysis.StartTime = label ?? $"Snapshot_{DateTime.Now:HHmmss}";
            _analysisHistory.Add(analysis);
            Debug.Log($"[LogicPresentationSync] 分析スナップショットを保存: {analysis.StartTime}");
        }

        /// <summary>
        /// 現在の分析結果をJSONファイルに保存
        /// </summary>
        public void SaveCurrentAnalysis()
        {
            var analysis = GetCurrentAnalysis();
            SaveAnalysisToFile(analysis);
        }

        /// <summary>
        /// 分析結果をファイルに保存
        /// </summary>
        private void SaveAnalysisToFile(SyncAnalysisResult analysis)
        {
            var outputDir = string.IsNullOrEmpty(_outputDirectory)
                ? Path.Combine(Application.persistentDataPath, "SyncAnalysis")
                : _outputDirectory;

            if (!Directory.Exists(outputDir))
            {
                Directory.CreateDirectory(outputDir);
            }

            var fileName = $"SyncAnalysis_{DateTime.Now:yyyyMMdd_HHmmss}.json";
            var filePath = Path.Combine(outputDir, fileName);

            var json = JsonUtility.ToJson(analysis, true);
            File.WriteAllText(filePath, json);

            Debug.Log($"[LogicPresentationSync] 分析結果を保存: {filePath}");
        }

        /// <summary>
        /// すべてのデータをクリア
        /// </summary>
        public void ClearAll()
        {
            _correlator.Clear();
            SyncEventMarker.ResetCounter();
            Debug.Log("[LogicPresentationSync] すべてのデータをクリアしました");
        }

        /// <summary>
        /// 履歴をクリア
        /// </summary>
        public void ClearHistory()
        {
            _analysisHistory.Clear();
        }

        /// <summary>
        /// サマリーをログ出力
        /// </summary>
        public void LogSummary()
        {
            var analysis = GetCurrentAnalysis();
            Debug.Log($"[LogicPresentationSync] === 同期分析サマリー ===\n" +
                      $"  総イベント数: {analysis.TotalEventCount}\n" +
                      $"    ロジック: {analysis.LogicEventCount}\n" +
                      $"    プレゼンテーション: {analysis.PresentationEventCount}\n" +
                      $"  ペアリング済み: {analysis.PairedEventCount}\n" +
                      $"  未ペア (ロジックのみ): {analysis.UnpairedLogicCount}\n" +
                      $"  未ペア (プレゼンテーションのみ): {analysis.UnpairedPresentationCount}\n" +
                      $"  同期問題: {analysis.SyncIssueCount}\n" +
                      $"  平均フレーム差分: {analysis.AverageFrameDifference:F2}F\n" +
                      $"  最大フレーム差分: {analysis.MaxFrameDifference}F\n" +
                      $"  平均時間差分: {analysis.AverageTimeDifferenceMs:F2}ms");
        }
    }
}
