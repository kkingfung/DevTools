#nullable enable
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace RuntimeAssetTracker.Editor
{
    /// <summary>
    /// ランタイムアセットトラッカーのEditorウィンドウ
    /// </summary>
    public class RuntimeAssetTrackerWindow : EditorWindow
    {
        private Vector2 _scrollPosition;
        private int _selectedSnapshotIndex = -1;
        private int _compareSnapshotIndex = -1;
        private SnapshotComparisonResult? _lastComparisonResult;

        private enum Tab { Snapshots, Comparison }
        private Tab _currentTab = Tab.Snapshots;

        private Vector2 _snapshotDetailScroll;
        private Vector2 _comparisonScroll;

        [MenuItem("Tools/Runtime Asset Tracker")]
        public static void ShowWindow()
        {
            var window = GetWindow<RuntimeAssetTrackerWindow>();
            window.titleContent = new GUIContent("Runtime Asset Tracker");
            window.minSize = new Vector2(500, 400);
        }

        private void OnGUI()
        {
            // プレイモードチェック
            if (!Application.isPlaying)
            {
                EditorGUILayout.HelpBox("ランタイムアセットトラッキングはプレイモード中のみ有効です。", MessageType.Info);

                EditorGUILayout.Space(10);
                EditorGUILayout.LabelField("セットアップ手順", EditorStyles.boldLabel);
                EditorGUILayout.HelpBox(
                    "1. シーンに空のGameObjectを作成\n" +
                    "2. RuntimeAssetTrackerManager コンポーネントを追加\n" +
                    "3. プレイモードを開始\n" +
                    "4. シーン遷移時に自動でスナップショットが取得されます",
                    MessageType.None);

                EditorGUILayout.Space(10);
                if (GUILayout.Button("マネージャーを作成", GUILayout.Height(30)))
                {
                    CreateManager();
                }

                return;
            }

            // マネージャーが存在しない場合
            if (RuntimeAssetTrackerManager.Instance == null)
            {
                EditorGUILayout.HelpBox("RuntimeAssetTrackerManager がシーンに存在しません。", MessageType.Warning);

                if (GUILayout.Button("マネージャーを作成", GUILayout.Height(30)))
                {
                    CreateManager();
                }

                return;
            }

            // タブ選択
            _currentTab = (Tab)GUILayout.Toolbar((int)_currentTab, new[] { "スナップショット", "比較" });

            EditorGUILayout.Space(5);

            switch (_currentTab)
            {
                case Tab.Snapshots:
                    DrawSnapshotsTab();
                    break;
                case Tab.Comparison:
                    DrawComparisonTab();
                    break;
            }
        }

        /// <summary>
        /// スナップショットタブを描画
        /// </summary>
        private void DrawSnapshotsTab()
        {
            var manager = RuntimeAssetTrackerManager.Instance!;

            // コントロールボタン
            EditorGUILayout.BeginHorizontal();
            if (GUILayout.Button("今すぐスナップショット取得", GUILayout.Height(30)))
            {
                manager.TakeSnapshot($"Manual_{System.DateTime.Now:HHmmss}");
                Repaint();
            }
            if (GUILayout.Button("クリア", GUILayout.Width(60), GUILayout.Height(30)))
            {
                manager.ClearSnapshots();
                _selectedSnapshotIndex = -1;
                Repaint();
            }
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space(10);

            if (manager.SnapshotCount == 0)
            {
                EditorGUILayout.HelpBox("スナップショットがありません。シーンを遷移するか、手動でスナップショットを取得してください。", MessageType.Info);
                return;
            }

            EditorGUILayout.LabelField($"スナップショット数: {manager.SnapshotCount}", EditorStyles.boldLabel);

            _scrollPosition = EditorGUILayout.BeginScrollView(_scrollPosition);

            // スナップショット一覧
            for (int i = 0; i < manager.SnapshotCount; i++)
            {
                var snapshot = manager.GetSnapshot(i);
                if (snapshot == null) continue;

                var isSelected = i == _selectedSnapshotIndex;
                var style = isSelected ? EditorStyles.helpBox : EditorStyles.textField;

                EditorGUILayout.BeginVertical(style);

                EditorGUILayout.BeginHorizontal();
                if (GUILayout.Button(isSelected ? "▼" : "▶", GUILayout.Width(25)))
                {
                    _selectedSnapshotIndex = isSelected ? -1 : i;
                }
                EditorGUILayout.LabelField($"#{i + 1}: {snapshot.Label}", EditorStyles.boldLabel);
                EditorGUILayout.LabelField(AssetSnapshotCapture.FormatBytes(snapshot.TotalMemoryBytes), GUILayout.Width(80));
                EditorGUILayout.EndHorizontal();

                EditorGUILayout.LabelField($"シーン: {snapshot.SceneName}  |  時刻: {snapshot.Timestamp}");

                // 詳細表示
                if (isSelected)
                {
                    EditorGUI.indentLevel++;
                    _snapshotDetailScroll = EditorGUILayout.BeginScrollView(_snapshotDetailScroll, GUILayout.MaxHeight(200));

                    EditorGUILayout.LabelField($"テクスチャ: {snapshot.Textures.Count}個 ({AssetSnapshotCapture.FormatBytes(snapshot.TextureMemoryBytes)})");
                    foreach (var tex in snapshot.Textures.Take(20))
                    {
                        EditorGUILayout.LabelField($"  {tex.Name} - {tex.Width}x{tex.Height} ({AssetSnapshotCapture.FormatBytes(tex.MemoryBytes)})");
                    }
                    if (snapshot.Textures.Count > 20)
                    {
                        EditorGUILayout.LabelField($"  ... 他 {snapshot.Textures.Count - 20}個");
                    }

                    EditorGUILayout.Space(5);

                    EditorGUILayout.LabelField($"オーディオ: {snapshot.AudioClips.Count}個 ({AssetSnapshotCapture.FormatBytes(snapshot.AudioMemoryBytes)})");
                    foreach (var audio in snapshot.AudioClips.Take(20))
                    {
                        EditorGUILayout.LabelField($"  {audio.Name} - {audio.Length:F1}秒 ({AssetSnapshotCapture.FormatBytes(audio.MemoryBytes)})");
                    }
                    if (snapshot.AudioClips.Count > 20)
                    {
                        EditorGUILayout.LabelField($"  ... 他 {snapshot.AudioClips.Count - 20}個");
                    }

                    EditorGUILayout.EndScrollView();
                    EditorGUI.indentLevel--;
                }

                EditorGUILayout.EndVertical();
                EditorGUILayout.Space(2);
            }

            EditorGUILayout.EndScrollView();
        }

        /// <summary>
        /// 比較タブを描画
        /// </summary>
        private void DrawComparisonTab()
        {
            var manager = RuntimeAssetTrackerManager.Instance!;

            if (manager.SnapshotCount < 2)
            {
                EditorGUILayout.HelpBox("比較には2つ以上のスナップショットが必要です。", MessageType.Info);
                return;
            }

            // スナップショット選択
            EditorGUILayout.BeginHorizontal();
            EditorGUILayout.LabelField("比較元:", GUILayout.Width(60));
            _selectedSnapshotIndex = EditorGUILayout.Popup(_selectedSnapshotIndex, GetSnapshotLabels(manager));
            EditorGUILayout.LabelField("→", GUILayout.Width(20));
            EditorGUILayout.LabelField("比較先:", GUILayout.Width(60));
            _compareSnapshotIndex = EditorGUILayout.Popup(_compareSnapshotIndex, GetSnapshotLabels(manager));
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space(5);

            EditorGUILayout.BeginHorizontal();
            if (GUILayout.Button("比較実行", GUILayout.Height(30)))
            {
                if (_selectedSnapshotIndex >= 0 && _compareSnapshotIndex >= 0)
                {
                    _lastComparisonResult = manager.CompareSnapshots(_selectedSnapshotIndex, _compareSnapshotIndex);
                }
            }
            if (GUILayout.Button("直前2つを比較", GUILayout.Height(30)))
            {
                _lastComparisonResult = manager.CompareLastTwoSnapshots();
                _selectedSnapshotIndex = manager.SnapshotCount - 2;
                _compareSnapshotIndex = manager.SnapshotCount - 1;
            }
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space(10);

            // 比較結果表示
            if (_lastComparisonResult != null)
            {
                DrawComparisonResult(_lastComparisonResult);
            }
        }

        /// <summary>
        /// 比較結果を描画
        /// </summary>
        private void DrawComparisonResult(SnapshotComparisonResult result)
        {
            EditorGUILayout.LabelField($"比較結果: {result.BaseSnapshotLabel} → {result.CompareSnapshotLabel}", EditorStyles.boldLabel);

            _comparisonScroll = EditorGUILayout.BeginScrollView(_comparisonScroll);

            // 残存アセット
            DrawAssetSection("残存アセット（遷移後も残っている）", result.ResidualTextures, result.ResidualAudioClips, new Color(1f, 0.8f, 0.5f));

            EditorGUILayout.Space(10);

            // 新規アセット
            DrawAssetSection("新規ロードアセット", result.NewTextures, result.NewAudioClips, new Color(0.5f, 1f, 0.5f));

            EditorGUILayout.Space(10);

            // 解放されたアセット
            DrawAssetSection("解放されたアセット", result.ReleasedTextures, result.ReleasedAudioClips, new Color(0.7f, 0.7f, 0.7f));

            EditorGUILayout.Space(10);

            EditorGUILayout.LabelField($"残存メモリ合計: {AssetSnapshotCapture.FormatBytes(result.ResidualMemoryBytes)}", EditorStyles.boldLabel);

            EditorGUILayout.EndScrollView();
        }

        /// <summary>
        /// アセットセクションを描画
        /// </summary>
        private void DrawAssetSection(string title, System.Collections.Generic.List<ResidualAssetInfo> textures, System.Collections.Generic.List<ResidualAssetInfo> audioClips, Color bgColor)
        {
            var prevColor = GUI.backgroundColor;
            GUI.backgroundColor = bgColor;

            EditorGUILayout.BeginVertical(EditorStyles.helpBox);
            GUI.backgroundColor = prevColor;

            EditorGUILayout.LabelField(title, EditorStyles.boldLabel);

            var textureMemory = textures.Sum(t => t.MemoryBytes);
            var audioMemory = audioClips.Sum(a => a.MemoryBytes);

            EditorGUILayout.LabelField($"テクスチャ: {textures.Count}個 ({AssetSnapshotCapture.FormatBytes(textureMemory)})");
            EditorGUI.indentLevel++;
            foreach (var tex in textures.Take(10))
            {
                EditorGUILayout.LabelField($"{tex.Name} ({AssetSnapshotCapture.FormatBytes(tex.MemoryBytes)})");
            }
            if (textures.Count > 10)
            {
                EditorGUILayout.LabelField($"... 他 {textures.Count - 10}個");
            }
            EditorGUI.indentLevel--;

            EditorGUILayout.Space(5);

            EditorGUILayout.LabelField($"オーディオ: {audioClips.Count}個 ({AssetSnapshotCapture.FormatBytes(audioMemory)})");
            EditorGUI.indentLevel++;
            foreach (var audio in audioClips.Take(10))
            {
                EditorGUILayout.LabelField($"{audio.Name} ({AssetSnapshotCapture.FormatBytes(audio.MemoryBytes)})");
            }
            if (audioClips.Count > 10)
            {
                EditorGUILayout.LabelField($"... 他 {audioClips.Count - 10}個");
            }
            EditorGUI.indentLevel--;

            EditorGUILayout.EndVertical();
        }

        /// <summary>
        /// スナップショットラベルの配列を取得
        /// </summary>
        private string[] GetSnapshotLabels(RuntimeAssetTrackerManager manager)
        {
            var labels = new string[manager.SnapshotCount];
            for (int i = 0; i < manager.SnapshotCount; i++)
            {
                var snapshot = manager.GetSnapshot(i);
                labels[i] = snapshot != null ? $"#{i + 1}: {snapshot.Label}" : $"#{i + 1}";
            }
            return labels;
        }

        /// <summary>
        /// マネージャーを作成
        /// </summary>
        private void CreateManager()
        {
            var go = new GameObject("RuntimeAssetTrackerManager");
            go.AddComponent<RuntimeAssetTrackerManager>();

            if (Application.isPlaying)
            {
                // プレイモード中はDontDestroyOnLoadに移動
            }
            else
            {
                Undo.RegisterCreatedObjectUndo(go, "Create RuntimeAssetTrackerManager");
            }

            Selection.activeGameObject = go;
            Debug.Log("[RuntimeAssetTracker] マネージャーを作成しました");
        }

        private void OnInspectorUpdate()
        {
            // プレイモード中は定期的に再描画
            if (Application.isPlaying)
            {
                Repaint();
            }
        }
    }
}
