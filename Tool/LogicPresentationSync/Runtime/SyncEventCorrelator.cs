#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;

namespace LogicPresentationSync
{
    /// <summary>
    /// ロジックイベントとプレゼンテーションイベントを相関付け、
    /// 同期ミスマッチを検出するシステム
    /// </summary>
    public class SyncEventCorrelator
    {
        private readonly Dictionary<string, EventPair> _pendingPairs = new();
        private readonly List<EventPair> _completedPairs = new();
        private readonly List<LogicEvent> _allLogicEvents = new();
        private readonly List<PresentationEvent> _allPresentationEvents = new();

        private int _pairIdCounter;
        private SyncThresholds _thresholds = new();

        /// <summary>同期閾値設定</summary>
        public SyncThresholds Thresholds
        {
            get => _thresholds;
            set => _thresholds = value;
        }

        /// <summary>完了したペアのリスト</summary>
        public IReadOnlyList<EventPair> CompletedPairs => _completedPairs;

        /// <summary>保留中のペア数</summary>
        public int PendingPairCount => _pendingPairs.Count;

        /// <summary>同期問題が検出されたときに発火</summary>
        public event Action<EventPair>? OnSyncIssueDetected;

        /// <summary>ペアが完成したときに発火</summary>
        public event Action<EventPair>? OnPairCompleted;

        /// <summary>
        /// ロジックイベントを追加
        /// </summary>
        public void AddLogicEvent(LogicEvent logicEvent)
        {
            _allLogicEvents.Add(logicEvent);

            if (_pendingPairs.TryGetValue(logicEvent.Tag, out var existingPair))
            {
                // 既にプレゼンテーションイベントがある場合
                existingPair.LogicEvent = logicEvent;
                CalculateDifference(existingPair);
            }
            else
            {
                // 新しいペアを作成
                var pair = new EventPair
                {
                    PairId = ++_pairIdCounter,
                    Tag = logicEvent.Tag,
                    LogicEvent = logicEvent
                };
                _pendingPairs[logicEvent.Tag] = pair;
            }
        }

        /// <summary>
        /// プレゼンテーションイベントを追加
        /// </summary>
        public void AddPresentationEvent(PresentationEvent presentationEvent)
        {
            _allPresentationEvents.Add(presentationEvent);

            if (_pendingPairs.TryGetValue(presentationEvent.Tag, out var existingPair))
            {
                // 既存のペアにプレゼンテーションイベントを追加
                existingPair.PresentationEvents.Add(presentationEvent);

                // ロジックイベントがあれば差分を計算
                if (existingPair.LogicEvent != null)
                {
                    CalculateDifference(existingPair);
                }
            }
            else
            {
                // 新しいペアを作成（ロジックイベントなし）
                var pair = new EventPair
                {
                    PairId = ++_pairIdCounter,
                    Tag = presentationEvent.Tag,
                    PresentationEvents = new List<PresentationEvent> { presentationEvent }
                };
                _pendingPairs[presentationEvent.Tag] = pair;
            }
        }

        /// <summary>
        /// ペアの差分を計算
        /// </summary>
        private void CalculateDifference(EventPair pair)
        {
            if (pair.LogicEvent == null || pair.PresentationEvents.Count == 0)
            {
                return;
            }

            // 最初のプレゼンテーションイベントとの差分を計算
            var firstPresentation = pair.PresentationEvents
                .OrderBy(p => p.Timestamp)
                .First();

            pair.FrameDifference = firstPresentation.Frame - pair.LogicEvent.Frame;
            pair.TimeDifferenceMs = (firstPresentation.Timestamp - pair.LogicEvent.Timestamp) * 1000.0;

            // 同期問題があるかチェック
            if (pair.HasSyncIssue)
            {
                OnSyncIssueDetected?.Invoke(pair);
            }
        }

        /// <summary>
        /// タイムアウトしたペアを処理
        /// </summary>
        /// <param name="currentTime">現在時刻</param>
        public void ProcessTimeouts(double currentTime)
        {
            var timedOutTags = new List<string>();

            foreach (var kvp in _pendingPairs)
            {
                var pair = kvp.Value;
                double lastEventTime;

                if (pair.LogicEvent != null)
                {
                    lastEventTime = pair.LogicEvent.Timestamp;
                }
                else if (pair.PresentationEvents.Count > 0)
                {
                    lastEventTime = pair.PresentationEvents.Max(p => p.Timestamp);
                }
                else
                {
                    continue;
                }

                if (currentTime - lastEventTime > _thresholds.PairingTimeoutSeconds)
                {
                    timedOutTags.Add(kvp.Key);
                }
            }

            foreach (var tag in timedOutTags)
            {
                CompletePair(tag);
            }
        }

        /// <summary>
        /// ペアを完了としてマーク
        /// </summary>
        public void CompletePair(string tag)
        {
            if (_pendingPairs.TryGetValue(tag, out var pair))
            {
                _pendingPairs.Remove(tag);
                _completedPairs.Add(pair);
                OnPairCompleted?.Invoke(pair);

                // 同期問題があれば通知
                if (pair.HasSyncIssue)
                {
                    OnSyncIssueDetected?.Invoke(pair);
                }
            }
        }

        /// <summary>
        /// すべての保留中ペアを完了
        /// </summary>
        public void CompleteAllPending()
        {
            var tags = _pendingPairs.Keys.ToList();
            foreach (var tag in tags)
            {
                CompletePair(tag);
            }
        }

        /// <summary>
        /// 分析結果を生成
        /// </summary>
        public SyncAnalysisResult GenerateAnalysis()
        {
            // 保留中のペアも含めてすべてのペアを取得
            var allPairs = _completedPairs.Concat(_pendingPairs.Values).ToList();

            var validPairs = allPairs
                .Where(p => p.LogicEvent != null && p.PresentationEvents.Count > 0)
                .ToList();

            var result = new SyncAnalysisResult
            {
                StartTime = _allLogicEvents.Count > 0 || _allPresentationEvents.Count > 0
                    ? DateTime.Now.ToString("o")
                    : string.Empty,
                EndTime = DateTime.Now.ToString("o"),
                TotalEventCount = _allLogicEvents.Count + _allPresentationEvents.Count,
                LogicEventCount = _allLogicEvents.Count,
                PresentationEventCount = _allPresentationEvents.Count,
                PairedEventCount = validPairs.Count,
                UnpairedLogicCount = allPairs.Count(p => p.IsMissingPresentation),
                UnpairedPresentationCount = allPairs.Count(p => p.IsMissingLogic),
                SyncIssueCount = allPairs.Count(p => p.HasSyncIssue),
                EventPairs = allPairs
            };

            if (validPairs.Count > 0)
            {
                result.AverageFrameDifference = validPairs.Average(p => Math.Abs(p.FrameDifference));
                result.MaxFrameDifference = validPairs.Max(p => Math.Abs(p.FrameDifference));
                result.AverageTimeDifferenceMs = validPairs.Average(p => Math.Abs(p.TimeDifferenceMs));
            }

            return result;
        }

        /// <summary>
        /// 同期問題のあるペアを取得
        /// </summary>
        public IEnumerable<EventPair> GetSyncIssues()
        {
            return _completedPairs.Concat(_pendingPairs.Values)
                .Where(p => p.HasSyncIssue)
                .OrderByDescending(p => Math.Abs(p.FrameDifference));
        }

        /// <summary>
        /// 特定のタグのペアを取得
        /// </summary>
        public EventPair? GetPair(string tag)
        {
            if (_pendingPairs.TryGetValue(tag, out var pending))
            {
                return pending;
            }
            return _completedPairs.FirstOrDefault(p => p.Tag == tag);
        }

        /// <summary>
        /// すべてのデータをクリア
        /// </summary>
        public void Clear()
        {
            _pendingPairs.Clear();
            _completedPairs.Clear();
            _allLogicEvents.Clear();
            _allPresentationEvents.Clear();
            _pairIdCounter = 0;
        }

        /// <summary>
        /// 統計サマリーを取得
        /// </summary>
        public string GetSummary()
        {
            var analysis = GenerateAnalysis();
            return $"Total: {analysis.TotalEventCount} events | " +
                   $"Paired: {analysis.PairedEventCount} | " +
                   $"Issues: {analysis.SyncIssueCount} | " +
                   $"Avg Diff: {analysis.AverageFrameDifference:F1}F ({analysis.AverageTimeDifferenceMs:F1}ms)";
        }
    }
}
