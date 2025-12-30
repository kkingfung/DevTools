#nullable enable
using System;
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace LogicPresentationSync.Editor
{
    /// <summary>
    /// ロジック/プレゼンテーション同期分析のEditorウィンドウ
    /// </summary>
    public class LogicPresentationSyncWindow : EditorWindow
    {
        private Vector2 _scrollPosition;
        private Vector2 _issueScrollPosition;

        private enum Tab { Overview, SyncIssues, EventTimeline, Settings }
        private Tab _currentTab = Tab.Overview;

        // フィルタ
        private bool _showOnlyIssues = true;
        private string _tagFilter = string.Empty;
        private int _selectedPairIndex = -1;

        [MenuItem("Tools/Logic Presentation Sync")]
        public static void ShowWindow()
        {
            var window = GetWindow<LogicPresentationSyncWindow>();
            window.titleContent = new GUIContent("Logic/Presentation Sync");
            window.minSize = new Vector2(600, 450);
        }

        private void OnGUI()
        {
            if (!Application.isPlaying)
            {
                DrawSetupUI();
                return;
            }

            if (LogicPresentationSyncTracker.Instance == null)
            {
                EditorGUILayout.HelpBox("LogicPresentationSyncTracker がシーンに存在しません。", MessageType.Warning);

                if (GUILayout.Button("トラッカーを作成", GUILayout.Height(30)))
                {
                    CreateTracker();
                }

                return;
            }

            // タブ選択
            _currentTab = (Tab)GUILayout.Toolbar((int)_currentTab, new[] { "概要", "同期問題", "イベントタイムライン", "設定" });

            EditorGUILayout.Space(5);

            switch (_currentTab)
            {
                case Tab.Overview:
                    DrawOverviewTab();
                    break;
                case Tab.SyncIssues:
                    DrawSyncIssuesTab();
                    break;
                case Tab.EventTimeline:
                    DrawEventTimelineTab();
                    break;
                case Tab.Settings:
                    DrawSettingsTab();
                    break;
            }
        }

        /// <summary>
        /// セットアップUI
        /// </summary>
        private void DrawSetupUI()
        {
            EditorGUILayout.HelpBox("ロジック/プレゼンテーション同期分析はプレイモード中のみ有効です。", MessageType.Info);

            EditorGUILayout.Space(10);
            EditorGUILayout.LabelField("使用方法", EditorStyles.boldLabel);
            EditorGUILayout.HelpBox(
                "1. シーンに LogicPresentationSyncTracker を配置\n" +
                "2. ゲームコードにイベントマーカーを追加:\n\n" +
                "   // ロジックイベント\n" +
                "   SyncEventMarker.MarkHitConfirm(\"attack_01\", this);\n\n" +
                "   // プレゼンテーションイベント\n" +
                "   SyncEventMarker.MarkVFX(\"attack_01\", \"HitEffect\", this);\n" +
                "   SyncEventMarker.MarkAudio(\"attack_01\", \"HitSound\", this);\n\n" +
                "3. プレイモードで同期状態を確認",
                MessageType.None);

            EditorGUILayout.Space(10);

            if (GUILayout.Button("トラッカーを作成", GUILayout.Height(30)))
            {
                CreateTracker();
            }
        }

        /// <summary>
        /// 概要タブ
        /// </summary>
        private void DrawOverviewTab()
        {
            var tracker = LogicPresentationSyncTracker.Instance!;
            var analysis = tracker.GetCurrentAnalysis();

            // ステータス
            EditorGUILayout.BeginHorizontal(EditorStyles.helpBox);
            var statusColor = tracker.IsTracking ? Color.green : Color.gray;
            var prevColor = GUI.color;
            GUI.color = statusColor;
            EditorGUILayout.LabelField("●", GUILayout.Width(20));
            GUI.color = prevColor;
            EditorGUILayout.LabelField(tracker.IsTracking ? "トラッキング中" : "停止中", EditorStyles.boldLabel);
            EditorGUILayout.LabelField($"保留中: {tracker.Correlator.PendingPairCount}件");
            EditorGUILayout.EndHorizontal();

            // コントロール
            EditorGUILayout.BeginHorizontal();
            if (tracker.IsTracking)
            {
                if (GUILayout.Button("停止", GUILayout.Height(25)))
                {
                    tracker.StopTracking();
                }
            }
            else
            {
                if (GUILayout.Button("開始", GUILayout.Height(25)))
                {
                    tracker.StartTracking();
                }
            }
            if (GUILayout.Button("クリア", GUILayout.Width(60), GUILayout.Height(25)))
            {
                tracker.ClearAll();
            }
            if (GUILayout.Button("サマリー出力", GUILayout.Width(100), GUILayout.Height(25)))
            {
                tracker.LogSummary();
            }
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space(10);

            // 統計情報
            EditorGUILayout.LabelField("統計情報", EditorStyles.boldLabel);

            EditorGUILayout.BeginVertical(EditorStyles.helpBox);

            // イベント数
            EditorGUILayout.BeginHorizontal();
            DrawStatBox("総イベント", analysis.TotalEventCount.ToString(), "");
            DrawStatBox("ロジック", analysis.LogicEventCount.ToString(), "");
            DrawStatBox("プレゼンテーション", analysis.PresentationEventCount.ToString(), "");
            EditorGUILayout.EndHorizontal();

            // ペアリング状況
            EditorGUILayout.BeginHorizontal();
            DrawStatBox("ペアリング済み", analysis.PairedEventCount.ToString(), "");
            DrawStatBox("未ペア(Logic)", analysis.UnpairedLogicCount.ToString(), "", analysis.UnpairedLogicCount > 0 ? Color.yellow : Color.white);
            DrawStatBox("未ペア(Present)", analysis.UnpairedPresentationCount.ToString(), "", analysis.UnpairedPresentationCount > 0 ? Color.yellow : Color.white);
            EditorGUILayout.EndHorizontal();

            // 同期状況
            EditorGUILayout.BeginHorizontal();
            var issueColor = analysis.SyncIssueCount > 0 ? new Color(1f, 0.6f, 0.6f) : Color.white;
            DrawStatBox("同期問題", analysis.SyncIssueCount.ToString(), "", issueColor);
            DrawStatBox("平均差分", $"{analysis.AverageFrameDifference:F1}F", $"({analysis.AverageTimeDifferenceMs:F1}ms)");
            DrawStatBox("最大差分", $"{analysis.MaxFrameDifference}F", "");
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.EndVertical();

            // 最近の同期問題
            EditorGUILayout.Space(10);
            EditorGUILayout.LabelField("最近の同期問題", EditorStyles.boldLabel);

            var issues = tracker.Correlator.GetSyncIssues().Take(5).ToList();
            if (issues.Count == 0)
            {
                EditorGUILayout.HelpBox("同期問題は検出されていません。", MessageType.Info);
            }
            else
            {
                foreach (var issue in issues)
                {
                    DrawEventPairCompact(issue);
                }

                if (tracker.Correlator.GetSyncIssues().Count() > 5)
                {
                    EditorGUILayout.LabelField($"... 他 {tracker.Correlator.GetSyncIssues().Count() - 5}件");
                }
            }
        }

        /// <summary>
        /// 同期問題タブ
        /// </summary>
        private void DrawSyncIssuesTab()
        {
            var tracker = LogicPresentationSyncTracker.Instance!;

            // フィルタ
            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField("タグフィルタ:", GUILayout.Width(80));
            _tagFilter = EditorGUILayout.TextField(_tagFilter);
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space(5);

            var issues = tracker.Correlator.GetSyncIssues()
                .Where(p => string.IsNullOrEmpty(_tagFilter) || p.Tag.Contains(_tagFilter))
                .ToList();

            EditorGUILayout.LabelField($"同期問題: {issues.Count}件", EditorStyles.boldLabel);

            if (issues.Count == 0)
            {
                EditorGUILayout.HelpBox("条件に一致する同期問題はありません。", MessageType.Info);
                return;
            }

            _issueScrollPosition = EditorGUILayout.BeginScrollView(_issueScrollPosition);

            for (int i = 0; i < issues.Count; i++)
            {
                var issue = issues[i];
                var isSelected = i == _selectedPairIndex;

                EditorGUILayout.BeginVertical(isSelected ? EditorStyles.helpBox : EditorStyles.textField);

                EditorGUILayout.BeginHorizontal();
                if (GUILayout.Button(isSelected ? "▼" : "▶", GUILayout.Width(25)))
                {
                    _selectedPairIndex = isSelected ? -1 : i;
                }

                // 問題タイプアイコン
                var issueIcon = GetIssueIcon(issue);
                EditorGUILayout.LabelField(issueIcon, GUILayout.Width(25));

                EditorGUILayout.LabelField(issue.Tag, EditorStyles.boldLabel);

                // 差分表示
                if (!issue.IsMissingLogic && !issue.IsMissingPresentation)
                {
                    var diffColor = GetDifferenceColor(issue.FrameDifference);
                    var prevContentColor = GUI.contentColor;
                    GUI.contentColor = diffColor;
                    EditorGUILayout.LabelField($"{issue.FrameDifference:+0;-0;0}F", GUILayout.Width(50));
                    EditorGUILayout.LabelField($"({issue.TimeDifferenceMs:+0.0;-0.0;0.0}ms)", GUILayout.Width(80));
                    GUI.contentColor = prevContentColor;
                }
                else
                {
                    EditorGUILayout.LabelField(issue.IsMissingPresentation ? "Presentationなし" : "Logicなし", GUILayout.Width(130));
                }

                EditorGUILayout.EndHorizontal();

                // 詳細
                if (isSelected)
                {
                    DrawEventPairDetails(issue);
                }

                EditorGUILayout.EndVertical();
                EditorGUILayout.Space(2);
            }

            EditorGUILayout.EndScrollView();
        }

        /// <summary>
        /// イベントタイムラインタブ
        /// </summary>
        private void DrawEventTimelineTab()
        {
            var tracker = LogicPresentationSyncTracker.Instance!;
            var analysis = tracker.GetCurrentAnalysis();

            _showOnlyIssues = EditorGUILayout.Toggle("問題のあるペアのみ表示", _showOnlyIssues);

            EditorGUILayout.Space(5);

            var pairs = _showOnlyIssues
                ? analysis.EventPairs.Where(p => p.HasSyncIssue).ToList()
                : analysis.EventPairs;

            EditorGUILayout.LabelField($"イベントペア: {pairs.Count()}件", EditorStyles.boldLabel);

            _scrollPosition = EditorGUILayout.BeginScrollView(_scrollPosition);

            foreach (var pair in pairs.OrderByDescending(p => p.LogicEvent?.Frame ?? p.PresentationEvents.FirstOrDefault()?.Frame ?? 0).Take(50))
            {
                DrawEventPairTimeline(pair);
            }

            if (pairs.Count() > 50)
            {
                EditorGUILayout.LabelField($"... 他 {pairs.Count() - 50}件");
            }

            EditorGUILayout.EndScrollView();
        }

        /// <summary>
        /// 設定タブ
        /// </summary>
        private void DrawSettingsTab()
        {
            var tracker = LogicPresentationSyncTracker.Instance!;
            var thresholds = tracker.Correlator.Thresholds;

            EditorGUILayout.LabelField("同期閾値設定", EditorStyles.boldLabel);

            EditorGUI.indentLevel++;

            thresholds.AcceptableFrameDifference = EditorGUILayout.IntField(
                new GUIContent("許容フレーム差分", "この値を超えると同期問題として報告"),
                thresholds.AcceptableFrameDifference);

            thresholds.AcceptableTimeDifferenceMs = EditorGUILayout.DoubleField(
                new GUIContent("許容時間差分 (ms)", "参考値として使用"),
                thresholds.AcceptableTimeDifferenceMs);

            thresholds.PairingTimeoutSeconds = EditorGUILayout.FloatField(
                new GUIContent("ペアリングタイムアウト (秒)", "この時間内にペアが完成しない場合は未ペアとして処理"),
                thresholds.PairingTimeoutSeconds);

            EditorGUI.indentLevel--;

            EditorGUILayout.Space(20);

            EditorGUILayout.LabelField("エクスポート", EditorStyles.boldLabel);

            if (GUILayout.Button("現在の分析結果を保存", GUILayout.Height(30)))
            {
                tracker.SaveCurrentAnalysis();
            }
        }

        /// <summary>
        /// 統計ボックスを描画
        /// </summary>
        private void DrawStatBox(string label, string value, string subValue, Color? color = null)
        {
            var prevBgColor = GUI.backgroundColor;
            if (color.HasValue)
            {
                GUI.backgroundColor = color.Value;
            }

            EditorGUILayout.BeginVertical(EditorStyles.helpBox, GUILayout.MinWidth(80));
            GUI.backgroundColor = prevBgColor;

            EditorGUILayout.LabelField(label, EditorStyles.miniLabel);
            EditorGUILayout.LabelField(value, EditorStyles.boldLabel);
            if (!string.IsNullOrEmpty(subValue))
            {
                EditorGUILayout.LabelField(subValue, EditorStyles.miniLabel);
            }
            EditorGUILayout.EndVertical();
        }

        /// <summary>
        /// イベントペアをコンパクトに表示
        /// </summary>
        private void DrawEventPairCompact(EventPair pair)
        {
            EditorGUILayout.BeginHorizontal(EditorStyles.helpBox);

            var issueIcon = GetIssueIcon(pair);
            EditorGUILayout.LabelField(issueIcon, GUILayout.Width(25));
            EditorGUILayout.LabelField(pair.Tag, GUILayout.Width(150));

            if (!pair.IsMissingLogic && !pair.IsMissingPresentation)
            {
                EditorGUILayout.LabelField($"差分: {pair.FrameDifference}F ({pair.TimeDifferenceMs:F1}ms)");
            }
            else
            {
                EditorGUILayout.LabelField(pair.IsMissingPresentation ? "Presentationなし" : "Logicなし");
            }

            EditorGUILayout.EndHorizontal();
        }

        /// <summary>
        /// イベントペアの詳細を表示
        /// </summary>
        private void DrawEventPairDetails(EventPair pair)
        {
            EditorGUI.indentLevel++;

            // ロジックイベント
            if (pair.LogicEvent != null)
            {
                EditorGUILayout.LabelField("ロジックイベント:", EditorStyles.boldLabel);
                EditorGUILayout.LabelField($"  名前: {pair.LogicEvent.EventName}");
                EditorGUILayout.LabelField($"  フレーム: {pair.LogicEvent.Frame}");
                EditorGUILayout.LabelField($"  オブジェクト: {pair.LogicEvent.SourceObject}");
                if (!string.IsNullOrEmpty(pair.LogicEvent.Details))
                {
                    EditorGUILayout.LabelField($"  詳細: {pair.LogicEvent.Details}");
                }
            }
            else
            {
                EditorGUILayout.LabelField("ロジックイベント: なし", EditorStyles.boldLabel);
            }

            EditorGUILayout.Space(5);

            // プレゼンテーションイベント
            EditorGUILayout.LabelField($"プレゼンテーションイベント: {pair.PresentationEvents.Count}個", EditorStyles.boldLabel);
            foreach (var pe in pair.PresentationEvents)
            {
                EditorGUILayout.LabelField($"  [{pe.Type}] {pe.EventName} (Frame {pe.Frame})");
            }

            EditorGUI.indentLevel--;
        }

        /// <summary>
        /// イベントペアをタイムライン形式で表示
        /// </summary>
        private void DrawEventPairTimeline(EventPair pair)
        {
            var bgColor = pair.HasSyncIssue ? new Color(1f, 0.9f, 0.8f) : Color.white;
            var prevBg = GUI.backgroundColor;
            GUI.backgroundColor = bgColor;

            EditorGUILayout.BeginVertical(EditorStyles.helpBox);
            GUI.backgroundColor = prevBg;

            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField(pair.Tag, EditorStyles.boldLabel, GUILayout.Width(150));

            // タイムライン表示
            if (pair.LogicEvent != null)
            {
                EditorGUILayout.LabelField($"L:{pair.LogicEvent.Frame}", GUILayout.Width(60));
            }
            else
            {
                EditorGUILayout.LabelField("L:---", GUILayout.Width(60));
            }

            EditorGUILayout.LabelField("→", GUILayout.Width(20));

            if (pair.PresentationEvents.Count > 0)
            {
                var frames = string.Join(",", pair.PresentationEvents.Select(p => p.Frame));
                EditorGUILayout.LabelField($"P:{frames}", GUILayout.Width(100));
            }
            else
            {
                EditorGUILayout.LabelField("P:---", GUILayout.Width(100));
            }

            // 差分
            if (!pair.IsMissingLogic && !pair.IsMissingPresentation)
            {
                var diffColor = GetDifferenceColor(pair.FrameDifference);
                var prevContentColor = GUI.contentColor;
                GUI.contentColor = diffColor;
                EditorGUILayout.LabelField($"Δ{pair.FrameDifference}F", GUILayout.Width(50));
                GUI.contentColor = prevContentColor;
            }

            EditorGUILayout.EndHorizontal();
            EditorGUILayout.EndVertical();
        }

        /// <summary>
        /// 問題タイプに応じたアイコンを取得
        /// </summary>
        private string GetIssueIcon(EventPair pair)
        {
            if (pair.IsMissingPresentation) return "⚠"; // プレゼンテーションなし
            if (pair.IsMissingLogic) return "❓"; // ロジックなし
            if (Math.Abs(pair.FrameDifference) > 3) return "⛔"; // 大きな差分
            if (Math.Abs(pair.FrameDifference) > 1) return "⚡"; // 中程度の差分
            return "●";
        }

        /// <summary>
        /// フレーム差分に応じた色を取得
        /// </summary>
        private Color GetDifferenceColor(int frameDiff)
        {
            var absDiff = Math.Abs(frameDiff);
            if (absDiff == 0) return Color.green;
            if (absDiff == 1) return Color.yellow;
            if (absDiff <= 3) return new Color(1f, 0.6f, 0f);
            return Color.red;
        }

        /// <summary>
        /// トラッカーを作成
        /// </summary>
        private void CreateTracker()
        {
            var go = new GameObject("LogicPresentationSyncTracker");
            go.AddComponent<LogicPresentationSyncTracker>();

            if (!Application.isPlaying)
            {
                Undo.RegisterCreatedObjectUndo(go, "Create LogicPresentationSyncTracker");
            }

            Selection.activeGameObject = go;
            Debug.Log("[LogicPresentationSync] トラッカーを作成しました");
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
