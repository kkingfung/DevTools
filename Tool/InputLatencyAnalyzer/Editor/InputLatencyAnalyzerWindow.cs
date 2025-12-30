#nullable enable
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace InputLatencyAnalyzer.Editor
{
    /// <summary>
    /// 入力遅延分析のEditorウィンドウ
    /// </summary>
    public class InputLatencyAnalyzerWindow : EditorWindow
    {
        private Vector2 _scrollPosition;
        private int _selectedSessionIndex = -1;

        private enum Tab { LiveMeasurement, History, Comparison }
        private Tab _currentTab = Tab.LiveMeasurement;

        // ライブ計測設定
        private KeyCode _inputKey = KeyCode.Space;
        private bool _detectVisual = true;
        private bool _detectAnimation = true;
        private bool _detectAudio = true;

        // 比較用
        private int _compareSession1Index = -1;
        private int _compareSession2Index = -1;

        [MenuItem("Tools/Input Latency Analyzer")]
        public static void ShowWindow()
        {
            var window = GetWindow<InputLatencyAnalyzerWindow>();
            window.titleContent = new GUIContent("Input Latency Analyzer");
            window.minSize = new Vector2(550, 450);
        }

        private void OnGUI()
        {
            // プレイモードチェック
            if (!Application.isPlaying)
            {
                DrawSetupUI();
                return;
            }

            // マネージャー存在チェック
            if (InputLatencyMeasurer.Instance == null)
            {
                EditorGUILayout.HelpBox("InputLatencyMeasurer がシーンに存在しません。", MessageType.Warning);

                if (GUILayout.Button("マネージャーを作成", GUILayout.Height(30)))
                {
                    CreateManager();
                }

                return;
            }

            // タブ選択
            _currentTab = (Tab)GUILayout.Toolbar((int)_currentTab, new[] { "ライブ計測", "履歴", "比較" });

            EditorGUILayout.Space(5);

            switch (_currentTab)
            {
                case Tab.LiveMeasurement:
                    DrawLiveMeasurementTab();
                    break;
                case Tab.History:
                    DrawHistoryTab();
                    break;
                case Tab.Comparison:
                    DrawComparisonTab();
                    break;
            }
        }

        /// <summary>
        /// セットアップUI（非プレイモード時）
        /// </summary>
        private void DrawSetupUI()
        {
            EditorGUILayout.HelpBox("入力遅延計測はプレイモード中のみ有効です。", MessageType.Info);

            EditorGUILayout.Space(10);
            EditorGUILayout.LabelField("セットアップ手順", EditorStyles.boldLabel);
            EditorGUILayout.HelpBox(
                "1. シーンに空のGameObjectを作成\n" +
                "2. InputLatencyMeasurer コンポーネントを追加\n" +
                "3. プレイモードを開始\n" +
                "4. F9キーで計測開始/停止\n" +
                "5. 設定したキー（デフォルト: Space）を押して計測",
                MessageType.None);

            EditorGUILayout.Space(10);

            EditorGUILayout.LabelField("計測設定（プレビュー）", EditorStyles.boldLabel);
            EditorGUI.indentLevel++;
            _inputKey = (KeyCode)EditorGUILayout.EnumPopup("入力キー", _inputKey);
            _detectVisual = EditorGUILayout.Toggle("画面変化を検出", _detectVisual);
            _detectAnimation = EditorGUILayout.Toggle("アニメーションを検出", _detectAnimation);
            _detectAudio = EditorGUILayout.Toggle("オーディオを検出", _detectAudio);
            EditorGUI.indentLevel--;

            EditorGUILayout.Space(10);

            if (GUILayout.Button("マネージャーを作成", GUILayout.Height(30)))
            {
                CreateManager();
            }
        }

        /// <summary>
        /// ライブ計測タブ
        /// </summary>
        private void DrawLiveMeasurementTab()
        {
            var measurer = InputLatencyMeasurer.Instance!;

            // ステータス表示
            EditorGUILayout.BeginHorizontal(EditorStyles.helpBox);
            var statusColor = measurer.IsMeasuring ? Color.green : Color.gray;
            var prevColor = GUI.color;
            GUI.color = statusColor;
            EditorGUILayout.LabelField("●", GUILayout.Width(20));
            GUI.color = prevColor;
            EditorGUILayout.LabelField(measurer.IsMeasuring ? "計測中" : "停止中", EditorStyles.boldLabel);
            EditorGUILayout.LabelField($"状態: {measurer.State}");
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space(5);

            // コントロールボタン
            EditorGUILayout.BeginHorizontal();
            if (!measurer.IsMeasuring)
            {
                if (GUILayout.Button("計測開始 (F9)", GUILayout.Height(30)))
                {
                    measurer.StartDefaultSession();
                }
            }
            else
            {
                if (GUILayout.Button("計測停止 (F9)", GUILayout.Height(30)))
                {
                    measurer.StopSession();
                }
            }
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space(10);

            // 現在のセッション情報
            if (measurer.CurrentSession != null)
            {
                DrawSessionDetails(measurer.CurrentSession, true);
            }
            else
            {
                EditorGUILayout.HelpBox("計測を開始すると、ここにリアルタイムの結果が表示されます。", MessageType.Info);
            }
        }

        /// <summary>
        /// 履歴タブ
        /// </summary>
        private void DrawHistoryTab()
        {
            var measurer = InputLatencyMeasurer.Instance!;

            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField($"セッション履歴: {measurer.SessionHistory.Count}件", EditorStyles.boldLabel);
            if (GUILayout.Button("クリア", GUILayout.Width(60)))
            {
                measurer.ClearHistory();
                _selectedSessionIndex = -1;
            }
            EditorGUILayout.EndHorizontal();

            if (measurer.SessionHistory.Count == 0)
            {
                EditorGUILayout.HelpBox("履歴がありません。計測を完了するとここに表示されます。", MessageType.Info);
                return;
            }

            _scrollPosition = EditorGUILayout.BeginScrollView(_scrollPosition);

            // セッション一覧
            for (int i = 0; i < measurer.SessionHistory.Count; i++)
            {
                var session = measurer.SessionHistory[i];
                var isSelected = i == _selectedSessionIndex;

                EditorGUILayout.BeginVertical(isSelected ? EditorStyles.helpBox : EditorStyles.textField);

                EditorGUILayout.BeginHorizontal();
                if (GUILayout.Button(isSelected ? "▼" : "▶", GUILayout.Width(25)))
                {
                    _selectedSessionIndex = isSelected ? -1 : i;
                }
                EditorGUILayout.LabelField($"#{i + 1}: {session.SessionName}", EditorStyles.boldLabel);
                EditorGUILayout.LabelField($"Avg: {session.Statistics.AverageMs:F1}ms", GUILayout.Width(100));
                EditorGUILayout.EndHorizontal();

                if (isSelected)
                {
                    DrawSessionDetails(session, false);
                }

                EditorGUILayout.EndVertical();
                EditorGUILayout.Space(2);
            }

            EditorGUILayout.EndScrollView();
        }

        /// <summary>
        /// 比較タブ
        /// </summary>
        private void DrawComparisonTab()
        {
            var measurer = InputLatencyMeasurer.Instance!;

            if (measurer.SessionHistory.Count < 2)
            {
                EditorGUILayout.HelpBox("比較には2つ以上のセッション履歴が必要です。", MessageType.Info);
                return;
            }

            var sessionNames = measurer.SessionHistory
                .Select((s, i) => $"#{i + 1}: {s.SessionName}")
                .ToArray();

            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField("セッション1:", GUILayout.Width(80));
            _compareSession1Index = EditorGUILayout.Popup(_compareSession1Index, sessionNames);
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField("セッション2:", GUILayout.Width(80));
            _compareSession2Index = EditorGUILayout.Popup(_compareSession2Index, sessionNames);
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space(10);

            if (_compareSession1Index >= 0 && _compareSession2Index >= 0 &&
                _compareSession1Index < measurer.SessionHistory.Count &&
                _compareSession2Index < measurer.SessionHistory.Count)
            {
                var session1 = measurer.SessionHistory[_compareSession1Index];
                var session2 = measurer.SessionHistory[_compareSession2Index];

                DrawComparisonTable(session1, session2);
            }
        }

        /// <summary>
        /// セッション詳細を描画
        /// </summary>
        private void DrawSessionDetails(LatencySession session, bool isLive)
        {
            var stats = session.Statistics;

            EditorGUI.indentLevel++;

            // 統計情報
            EditorGUILayout.LabelField("統計情報", EditorStyles.boldLabel);

            EditorGUILayout.BeginVertical(EditorStyles.helpBox);

            EditorGUILayout.BeginHorizontal();
            DrawStatBox("平均", $"{stats.AverageMs:F2}ms", $"({stats.AverageFrames:F1}F)");
            DrawStatBox("最小", $"{stats.MinMs:F2}ms", $"({stats.MinFrames}F)");
            DrawStatBox("最大", $"{stats.MaxMs:F2}ms", $"({stats.MaxFrames}F)");
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.BeginHorizontal();
            DrawStatBox("中央値", $"{stats.MedianMs:F2}ms", "");
            DrawStatBox("95%ile", $"{stats.Percentile95Ms:F2}ms", "");
            DrawStatBox("99%ile", $"{stats.Percentile99Ms:F2}ms", "");
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.BeginHorizontal();
            DrawStatBox("標準偏差", $"{stats.StandardDeviationMs:F2}ms", "");
            DrawStatBox("サンプル数", $"{stats.ValidSampleCount}/{stats.SampleCount}", "");
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.EndVertical();

            // 最近の計測結果
            EditorGUILayout.Space(5);
            EditorGUILayout.LabelField($"計測結果（{session.Measurements.Count}件）", EditorStyles.boldLabel);

            var recentMeasurements = isLive
                ? session.Measurements.TakeLast(10).Reverse()
                : session.Measurements.Take(20);

            foreach (var measurement in recentMeasurements)
            {
                var color = measurement.IsValid ? Color.white : Color.red;
                var prevColor = GUI.contentColor;
                GUI.contentColor = color;

                EditorGUILayout.BeginHorizontal();
                EditorGUILayout.LabelField($"#{measurement.MeasurementId}", GUILayout.Width(40));
                EditorGUILayout.LabelField($"{measurement.LatencyMs:F2}ms", GUILayout.Width(80));
                EditorGUILayout.LabelField($"{measurement.LatencyFrames}F", GUILayout.Width(40));
                EditorGUILayout.LabelField(measurement.IsValid ? measurement.DetectedResponseType.ToString() : "Timeout");
                EditorGUILayout.EndHorizontal();

                GUI.contentColor = prevColor;
            }

            if (session.Measurements.Count > 20 && !isLive)
            {
                EditorGUILayout.LabelField($"... 他 {session.Measurements.Count - 20}件");
            }

            EditorGUI.indentLevel--;
        }

        /// <summary>
        /// 統計ボックスを描画
        /// </summary>
        private void DrawStatBox(string label, string value, string subValue)
        {
            EditorGUILayout.BeginVertical(EditorStyles.helpBox, GUILayout.MinWidth(100));
            EditorGUILayout.LabelField(label, EditorStyles.miniLabel);
            EditorGUILayout.LabelField(value, EditorStyles.boldLabel);
            if (!string.IsNullOrEmpty(subValue))
            {
                EditorGUILayout.LabelField(subValue, EditorStyles.miniLabel);
            }
            EditorGUILayout.EndVertical();
        }

        /// <summary>
        /// 比較テーブルを描画
        /// </summary>
        private void DrawComparisonTable(LatencySession session1, LatencySession session2)
        {
            EditorGUILayout.LabelField("比較結果", EditorStyles.boldLabel);

            EditorGUILayout.BeginVertical(EditorStyles.helpBox);

            // ヘッダー
            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField("項目", EditorStyles.boldLabel, GUILayout.Width(100));
            EditorGUILayout.LabelField(session1.SessionName, EditorStyles.boldLabel, GUILayout.Width(120));
            EditorGUILayout.LabelField(session2.SessionName, EditorStyles.boldLabel, GUILayout.Width(120));
            EditorGUILayout.LabelField("差分", EditorStyles.boldLabel, GUILayout.Width(100));
            EditorGUILayout.EndHorizontal();

            DrawComparisonRow("平均", session1.Statistics.AverageMs, session2.Statistics.AverageMs);
            DrawComparisonRow("最小", session1.Statistics.MinMs, session2.Statistics.MinMs);
            DrawComparisonRow("最大", session1.Statistics.MaxMs, session2.Statistics.MaxMs);
            DrawComparisonRow("中央値", session1.Statistics.MedianMs, session2.Statistics.MedianMs);
            DrawComparisonRow("95%ile", session1.Statistics.Percentile95Ms, session2.Statistics.Percentile95Ms);
            DrawComparisonRow("標準偏差", session1.Statistics.StandardDeviationMs, session2.Statistics.StandardDeviationMs);

            EditorGUILayout.EndVertical();
        }

        /// <summary>
        /// 比較行を描画
        /// </summary>
        private void DrawComparisonRow(string label, double value1, double value2)
        {
            double diff = value2 - value1;
            var diffColor = diff < 0 ? Color.green : (diff > 0 ? Color.red : Color.white);

            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField(label, GUILayout.Width(100));
            EditorGUILayout.LabelField($"{value1:F2}ms", GUILayout.Width(120));
            EditorGUILayout.LabelField($"{value2:F2}ms", GUILayout.Width(120));

            var prevColor = GUI.contentColor;
            GUI.contentColor = diffColor;
            EditorGUILayout.LabelField($"{diff:+0.00;-0.00;0.00}ms", GUILayout.Width(100));
            GUI.contentColor = prevColor;

            EditorGUILayout.EndHorizontal();
        }

        /// <summary>
        /// マネージャーを作成
        /// </summary>
        private void CreateManager()
        {
            var go = new GameObject("InputLatencyMeasurer");
            go.AddComponent<InputLatencyMeasurer>();

            if (!Application.isPlaying)
            {
                Undo.RegisterCreatedObjectUndo(go, "Create InputLatencyMeasurer");
            }

            Selection.activeGameObject = go;
            Debug.Log("[InputLatencyAnalyzer] マネージャーを作成しました");
        }

        private void OnInspectorUpdate()
        {
            if (Application.isPlaying)
            {
                Repaint();
            }
        }
    }
}
