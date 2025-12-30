#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

namespace InputLatencyAnalyzer
{
    /// <summary>
    /// 入力遅延計測マネージャー
    /// </summary>
    public class InputLatencyMeasurer : MonoBehaviour
    {
        /// <summary>シングルトンインスタンス</summary>
        public static InputLatencyMeasurer? Instance { get; private set; }

        [Header("計測設定")]
        [SerializeField]
        [Tooltip("レスポンス待機タイムアウト（秒）")]
        private float _responseTimeout = 1.0f;

        [SerializeField]
        [Tooltip("計測結果をコンソールに出力")]
        private bool _logResults = true;

        [Header("デフォルト入力設定")]
        [SerializeField]
        [Tooltip("計測開始/停止キー")]
        private KeyCode _toggleMeasurementKey = KeyCode.F9;

        [SerializeField]
        [Tooltip("デフォルトの入力キー")]
        private KeyCode _defaultInputKey = KeyCode.Space;

        // 現在のセッション
        private LatencySession? _currentSession;
        private int _measurementIdCounter;

        // 計測状態
        private MeasurementState _state = MeasurementState.Idle;
        private double _inputTimestamp;
        private int _inputFrame;
        private LatencyMeasurement? _currentMeasurement;

        // 入力とレスポンス検出
        private IInputTrigger? _inputTrigger;
        private readonly List<IResponseDetector> _responseDetectors = new();

        // 履歴
        private readonly List<LatencySession> _sessionHistory = new();

        /// <summary>現在の計測状態</summary>
        public MeasurementState State => _state;

        /// <summary>現在のセッション</summary>
        public LatencySession? CurrentSession => _currentSession;

        /// <summary>セッション履歴</summary>
        public IReadOnlyList<LatencySession> SessionHistory => _sessionHistory;

        /// <summary>計測中かどうか</summary>
        public bool IsMeasuring => _currentSession != null;

        /// <summary>計測完了時イベント</summary>
        public event Action<LatencyMeasurement>? OnMeasurementCompleted;

        /// <summary>セッション終了時イベント</summary>
        public event Action<LatencySession>? OnSessionCompleted;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }

            Instance = this;
            DontDestroyOnLoad(gameObject);
        }

        private void OnDestroy()
        {
            if (Instance == this)
            {
                Instance = null;
            }

            // リソース解放
            foreach (var detector in _responseDetectors)
            {
                if (detector is VisualResponseDetector visual)
                {
                    visual.Dispose();
                }
            }
        }

        private void Update()
        {
            // 計測開始/停止トグル
            if (Input.GetKeyDown(_toggleMeasurementKey))
            {
                if (IsMeasuring)
                {
                    StopSession();
                }
                else
                {
                    StartDefaultSession();
                }
            }

            // 入力チェック
            _inputTrigger?.CheckInput();

            // レスポンス待機中の場合
            if (_state == MeasurementState.WaitingForResponse)
            {
                CheckForResponse();
                CheckTimeout();
            }
        }

        /// <summary>
        /// デフォルト設定でセッションを開始
        /// </summary>
        public void StartDefaultSession()
        {
            // デフォルトの入力トリガー
            var trigger = new KeyInputTrigger(_defaultInputKey);

            // デフォルトのレスポンス検出（すべて）
            var detectors = new List<IResponseDetector>();

            // Transform検出（メインカメラの子オブジェクトなど）
            var mainCamera = Camera.main;
            if (mainCamera != null)
            {
                detectors.Add(new VisualResponseDetector(mainCamera));
            }

            // Audio検出
            detectors.Add(AudioResponseDetector.CreateForAllSources());

            StartSession($"Session_{DateTime.Now:HHmmss}", trigger, detectors.ToArray());
        }

        /// <summary>
        /// カスタム設定でセッションを開始
        /// </summary>
        public void StartSession(string sessionName, IInputTrigger inputTrigger, params IResponseDetector[] responseDetectors)
        {
            if (IsMeasuring)
            {
                StopSession();
            }

            _inputTrigger = inputTrigger;
            _inputTrigger.OnInputDetected += OnInputDetected;

            _responseDetectors.Clear();
            _responseDetectors.AddRange(responseDetectors);

            ResponseType targetTypes = ResponseType.None;
            foreach (var detector in responseDetectors)
            {
                targetTypes |= detector.Type;
            }

            _currentSession = new LatencySession
            {
                SessionId = Guid.NewGuid().ToString(),
                SessionName = sessionName,
                StartTime = DateTime.Now.ToString("o"),
                TargetInputAction = inputTrigger.ActionName,
                TargetResponseTypes = targetTypes
            };

            _state = MeasurementState.Idle;

            Debug.Log($"[InputLatencyAnalyzer] セッション開始: {sessionName}\n" +
                      $"  入力: {inputTrigger.ActionName}\n" +
                      $"  レスポンス検出: {targetTypes}");
        }

        /// <summary>
        /// セッションを終了
        /// </summary>
        public void StopSession()
        {
            if (_currentSession == null) return;

            _currentSession.EndTime = DateTime.Now.ToString("o");
            CalculateStatistics(_currentSession);

            _sessionHistory.Add(_currentSession);
            OnSessionCompleted?.Invoke(_currentSession);

            if (_logResults)
            {
                LogSessionSummary(_currentSession);
            }

            // クリーンアップ
            if (_inputTrigger != null)
            {
                _inputTrigger.OnInputDetected -= OnInputDetected;
                _inputTrigger = null;
            }

            foreach (var detector in _responseDetectors)
            {
                detector.Reset();
            }

            _currentSession = null;
            _state = MeasurementState.Idle;

            Debug.Log("[InputLatencyAnalyzer] セッション終了");
        }

        /// <summary>
        /// 入力検出時のコールバック
        /// </summary>
        private void OnInputDetected(double timestamp, int frame)
        {
            if (_state != MeasurementState.Idle || _currentSession == null)
            {
                return;
            }

            _inputTimestamp = timestamp;
            _inputFrame = frame;

            _currentMeasurement = new LatencyMeasurement
            {
                MeasurementId = ++_measurementIdCounter,
                Timestamp = DateTime.Now.ToString("o"),
                InputTime = timestamp,
                InputFrame = frame,
                InputAction = _inputTrigger?.ActionName ?? "Unknown"
            };

            // すべてのレスポンス検出器を開始
            foreach (var detector in _responseDetectors)
            {
                detector.StartDetection();
            }

            _state = MeasurementState.WaitingForResponse;

            if (_logResults)
            {
                Debug.Log($"[InputLatencyAnalyzer] 入力検出: Frame={frame}");
            }
        }

        /// <summary>
        /// レスポンスをチェック
        /// </summary>
        private void CheckForResponse()
        {
            foreach (var detector in _responseDetectors)
            {
                var result = detector.CheckResponse();
                if (result.Detected)
                {
                    CompleteMeasurement(result);
                    return;
                }
            }
        }

        /// <summary>
        /// タイムアウトをチェック
        /// </summary>
        private void CheckTimeout()
        {
            double elapsed = Time.realtimeSinceStartupAsDouble - _inputTimestamp;
            if (elapsed > _responseTimeout)
            {
                TimeoutMeasurement();
            }
        }

        /// <summary>
        /// 計測を完了
        /// </summary>
        private void CompleteMeasurement(ResponseDetectionResult result)
        {
            if (_currentMeasurement == null || _currentSession == null) return;

            _currentMeasurement.ResponseTime = result.Timestamp;
            _currentMeasurement.ResponseFrame = result.Frame;
            _currentMeasurement.DetectedResponseType = result.Type;
            _currentMeasurement.ResponseDescription = result.Description;
            _currentMeasurement.LatencyMs = (result.Timestamp - _inputTimestamp) * 1000.0;
            _currentMeasurement.LatencyFrames = result.Frame - _inputFrame;
            _currentMeasurement.IsValid = true;
            _currentMeasurement.TimedOut = false;

            _currentSession.Measurements.Add(_currentMeasurement);
            OnMeasurementCompleted?.Invoke(_currentMeasurement);

            if (_logResults)
            {
                Debug.Log($"[InputLatencyAnalyzer] 計測完了: {_currentMeasurement.LatencyMs:F2}ms ({_currentMeasurement.LatencyFrames}フレーム)\n" +
                          $"  レスポンス: {result.Description}");
            }

            ResetMeasurement();
        }

        /// <summary>
        /// 計測タイムアウト
        /// </summary>
        private void TimeoutMeasurement()
        {
            if (_currentMeasurement == null || _currentSession == null) return;

            _currentMeasurement.IsValid = false;
            _currentMeasurement.TimedOut = true;
            _currentMeasurement.LatencyMs = _responseTimeout * 1000.0;

            _currentSession.Measurements.Add(_currentMeasurement);

            if (_logResults)
            {
                Debug.LogWarning($"[InputLatencyAnalyzer] 計測タイムアウト: {_responseTimeout}秒以内にレスポンスが検出されませんでした");
            }

            ResetMeasurement();
        }

        /// <summary>
        /// 計測状態をリセット
        /// </summary>
        private void ResetMeasurement()
        {
            foreach (var detector in _responseDetectors)
            {
                detector.Reset();
            }

            _currentMeasurement = null;
            _state = MeasurementState.Idle;
        }

        /// <summary>
        /// 統計情報を計算
        /// </summary>
        private void CalculateStatistics(LatencySession session)
        {
            var validMeasurements = session.Measurements.Where(m => m.IsValid).ToList();

            session.Statistics.SampleCount = session.Measurements.Count;
            session.Statistics.ValidSampleCount = validMeasurements.Count;

            if (validMeasurements.Count == 0) return;

            var latencies = validMeasurements.Select(m => m.LatencyMs).OrderBy(x => x).ToList();
            var frames = validMeasurements.Select(m => m.LatencyFrames).ToList();

            session.Statistics.AverageMs = latencies.Average();
            session.Statistics.MinMs = latencies.Min();
            session.Statistics.MaxMs = latencies.Max();
            session.Statistics.MedianMs = GetPercentile(latencies, 50);
            session.Statistics.Percentile95Ms = GetPercentile(latencies, 95);
            session.Statistics.Percentile99Ms = GetPercentile(latencies, 99);

            // 標準偏差
            double avg = session.Statistics.AverageMs;
            double sumSquares = latencies.Sum(x => (x - avg) * (x - avg));
            session.Statistics.StandardDeviationMs = Math.Sqrt(sumSquares / latencies.Count);

            session.Statistics.AverageFrames = frames.Average();
            session.Statistics.MinFrames = frames.Min();
            session.Statistics.MaxFrames = frames.Max();
        }

        /// <summary>
        /// パーセンタイル値を取得
        /// </summary>
        private double GetPercentile(List<double> sortedList, int percentile)
        {
            if (sortedList.Count == 0) return 0;
            int index = (int)Math.Ceiling(percentile / 100.0 * sortedList.Count) - 1;
            return sortedList[Math.Max(0, Math.Min(index, sortedList.Count - 1))];
        }

        /// <summary>
        /// セッションサマリーをログ出力
        /// </summary>
        private void LogSessionSummary(LatencySession session)
        {
            var stats = session.Statistics;
            Debug.Log($"[InputLatencyAnalyzer] セッションサマリー: {session.SessionName}\n" +
                      $"  計測回数: {stats.ValidSampleCount}/{stats.SampleCount}\n" +
                      $"  平均: {stats.AverageMs:F2}ms ({stats.AverageFrames:F1}フレーム)\n" +
                      $"  最小: {stats.MinMs:F2}ms | 最大: {stats.MaxMs:F2}ms\n" +
                      $"  中央値: {stats.MedianMs:F2}ms\n" +
                      $"  標準偏差: {stats.StandardDeviationMs:F2}ms\n" +
                      $"  95%ile: {stats.Percentile95Ms:F2}ms | 99%ile: {stats.Percentile99Ms:F2}ms");
        }

        /// <summary>
        /// すべての履歴をクリア
        /// </summary>
        public void ClearHistory()
        {
            _sessionHistory.Clear();
            Debug.Log("[InputLatencyAnalyzer] 履歴をクリアしました");
        }

        /// <summary>
        /// セッションをJSON形式でエクスポート
        /// </summary>
        public string ExportSessionToJson(LatencySession session)
        {
            return JsonUtility.ToJson(session, true);
        }
    }
}
