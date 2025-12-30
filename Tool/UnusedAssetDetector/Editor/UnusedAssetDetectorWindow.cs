#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEditor.IMGUI.Controls;
using UnityEngine;

namespace UnusedAssetDetector
{
    /// <summary>
    /// 未使用アセット検出ツールのEditorWindow
    /// </summary>
    public class UnusedAssetDetectorWindow : EditorWindow
    {
        // スキャン状態
        private AssetReferenceCollector? _collector;
        private bool _isScanning;
        private string _progressMessage = string.Empty;
        private float _progress;

        // 結果表示
        private Vector2 _scrollPosition;
        private List<AssetInfo> _displayedAssets = new();
        private HashSet<string> _selectedGuids = new();

        // フィルタリング
        private string _searchFilter = string.Empty;
        private string _typeFilter = "All";
        private string[] _availableTypes = { "All" };

        // ソート
        private SortColumn _sortColumn = SortColumn.Path;
        private bool _sortAscending = true;

        // 統計
        private AssetReferenceMap.Statistics? _statistics;

        // UI状態
        private bool _showReferencedBy;
        private AssetInfo? _selectedAssetForDetails;

        private enum SortColumn { Path, Type, Size }

        [MenuItem("Tools/Unused Asset Detector")]
        public static void ShowWindow()
        {
            var window = GetWindow<UnusedAssetDetectorWindow>();
            window.titleContent = new GUIContent("Unused Asset Detector");
            window.minSize = new Vector2(600, 400);
            window.Show();
        }

        private void OnEnable()
        {
            _collector = new AssetReferenceCollector();
            _collector.OnProgress = OnScanProgress;
        }

        private void OnGUI()
        {
            DrawToolbar();

            if (_isScanning)
            {
                DrawProgressBar();
            }
            else if (_statistics != null)
            {
                DrawStatistics();
                DrawFilters();
                DrawAssetList();
            }
            else
            {
                DrawWelcomeMessage();
            }
        }

        /// <summary>
        /// ツールバーを描画
        /// </summary>
        private void DrawToolbar()
        {
            EditorGUILayout.BeginHorizontal(EditorStyles.toolbar);

            GUI.enabled = !_isScanning;

            if (GUILayout.Button("Scan Project", EditorStyles.toolbarButton, GUILayout.Width(100)))
            {
                StartScan();
            }

            GUILayout.FlexibleSpace();

            if (_statistics != null && _displayedAssets.Count > 0)
            {
                if (GUILayout.Button("Select All", EditorStyles.toolbarButton, GUILayout.Width(80)))
                {
                    SelectAll();
                }

                if (GUILayout.Button("Deselect All", EditorStyles.toolbarButton, GUILayout.Width(80)))
                {
                    DeselectAll();
                }

                GUI.enabled = !_isScanning && _selectedGuids.Count > 0;
                GUI.backgroundColor = new Color(1f, 0.5f, 0.5f);
                if (GUILayout.Button($"Delete Selected ({_selectedGuids.Count})", EditorStyles.toolbarButton, GUILayout.Width(140)))
                {
                    DeleteSelected();
                }
                GUI.backgroundColor = Color.white;
            }

            GUI.enabled = true;

            EditorGUILayout.EndHorizontal();
        }

        /// <summary>
        /// プログレスバーを描画
        /// </summary>
        private void DrawProgressBar()
        {
            EditorGUILayout.Space(20);
            EditorGUILayout.BeginVertical();
            GUILayout.FlexibleSpace();

            EditorGUILayout.BeginHorizontal();
            GUILayout.FlexibleSpace();
            EditorGUILayout.BeginVertical(GUILayout.Width(400));

            EditorGUILayout.LabelField("スキャン中...", EditorStyles.boldLabel);
            EditorGUILayout.Space(10);

            var rect = EditorGUILayout.GetControlRect(false, 20);
            EditorGUI.ProgressBar(rect, _progress, _progressMessage);

            EditorGUILayout.EndVertical();
            GUILayout.FlexibleSpace();
            EditorGUILayout.EndHorizontal();

            GUILayout.FlexibleSpace();
            EditorGUILayout.EndVertical();
        }

        /// <summary>
        /// 統計情報を描画
        /// </summary>
        private void DrawStatistics()
        {
            if (_statistics == null) return;

            EditorGUILayout.BeginVertical(EditorStyles.helpBox);
            EditorGUILayout.BeginHorizontal();

            DrawStatBox("Total", _statistics.TotalCount.ToString(), Color.white);
            DrawStatBox("Root", _statistics.RootCount.ToString(), new Color(0.5f, 0.8f, 1f));
            DrawStatBox("Referenced", _statistics.ReferencedCount.ToString(), new Color(0.5f, 1f, 0.5f));
            DrawStatBox("Unreferenced", _statistics.UnreferencedCount.ToString(), new Color(1f, 0.7f, 0.5f));
            DrawStatBox("Size", _statistics.FormattedSize, new Color(1f, 0.85f, 0.5f));

            EditorGUILayout.EndHorizontal();
            EditorGUILayout.EndVertical();
        }

        private void DrawStatBox(string label, string value, Color color)
        {
            EditorGUILayout.BeginVertical(GUILayout.Width(100));
            EditorGUILayout.LabelField(label, EditorStyles.centeredGreyMiniLabel);
            var originalColor = GUI.color;
            GUI.color = color;
            EditorGUILayout.LabelField(value, new GUIStyle(EditorStyles.boldLabel) { alignment = TextAnchor.MiddleCenter });
            GUI.color = originalColor;
            EditorGUILayout.EndVertical();
        }

        /// <summary>
        /// フィルターを描画
        /// </summary>
        private void DrawFilters()
        {
            EditorGUILayout.BeginHorizontal(EditorStyles.toolbar);

            // 検索フィルタ
            EditorGUILayout.LabelField("Search:", GUILayout.Width(50));
            var newSearch = EditorGUILayout.TextField(_searchFilter, EditorStyles.toolbarSearchField, GUILayout.Width(200));
            if (newSearch != _searchFilter)
            {
                _searchFilter = newSearch;
                ApplyFilters();
            }

            // タイプフィルタ
            EditorGUILayout.LabelField("Type:", GUILayout.Width(40));
            var typeIndex = Array.IndexOf(_availableTypes, _typeFilter);
            var newTypeIndex = EditorGUILayout.Popup(typeIndex, _availableTypes, EditorStyles.toolbarPopup, GUILayout.Width(120));
            if (newTypeIndex != typeIndex && newTypeIndex >= 0)
            {
                _typeFilter = _availableTypes[newTypeIndex];
                ApplyFilters();
            }

            GUILayout.FlexibleSpace();

            EditorGUILayout.LabelField($"Showing: {_displayedAssets.Count}", GUILayout.Width(100));

            EditorGUILayout.EndHorizontal();
        }

        /// <summary>
        /// アセットリストを描画
        /// </summary>
        private void DrawAssetList()
        {
            // ヘッダー
            EditorGUILayout.BeginHorizontal(EditorStyles.toolbar);

            // チェックボックス列
            GUILayout.Space(25);

            // ソート可能なヘッダー
            if (GUILayout.Button(GetSortLabel("Path", SortColumn.Path), EditorStyles.toolbarButton, GUILayout.ExpandWidth(true)))
            {
                ToggleSort(SortColumn.Path);
            }
            if (GUILayout.Button(GetSortLabel("Type", SortColumn.Type), EditorStyles.toolbarButton, GUILayout.Width(100)))
            {
                ToggleSort(SortColumn.Type);
            }
            if (GUILayout.Button(GetSortLabel("Size", SortColumn.Size), EditorStyles.toolbarButton, GUILayout.Width(80)))
            {
                ToggleSort(SortColumn.Size);
            }

            // アクション列
            GUILayout.Space(60);

            EditorGUILayout.EndHorizontal();

            // リスト本体
            _scrollPosition = EditorGUILayout.BeginScrollView(_scrollPosition);

            foreach (var asset in _displayedAssets)
            {
                DrawAssetRow(asset);
            }

            EditorGUILayout.EndScrollView();
        }

        /// <summary>
        /// アセット行を描画
        /// </summary>
        private void DrawAssetRow(AssetInfo asset)
        {
            var isSelected = _selectedGuids.Contains(asset.Guid);
            var bgColor = isSelected ? new Color(0.3f, 0.5f, 0.8f, 0.3f) : Color.clear;

            var rect = EditorGUILayout.BeginHorizontal();
            EditorGUI.DrawRect(rect, bgColor);

            // チェックボックス
            var newSelected = EditorGUILayout.Toggle(isSelected, GUILayout.Width(20));
            if (newSelected != isSelected)
            {
                if (newSelected)
                    _selectedGuids.Add(asset.Guid);
                else
                    _selectedGuids.Remove(asset.Guid);
            }

            // アイコン
            var icon = AssetDatabase.GetCachedIcon(asset.Path);
            if (icon != null)
            {
                GUILayout.Label(new GUIContent(icon), GUILayout.Width(18), GUILayout.Height(18));
            }
            else
            {
                GUILayout.Space(20);
            }

            // パス（クリックで選択）
            if (GUILayout.Button(asset.Path, EditorStyles.label, GUILayout.ExpandWidth(true)))
            {
                var obj = AssetDatabase.LoadAssetAtPath<UnityEngine.Object>(asset.Path);
                if (obj != null)
                {
                    EditorGUIUtility.PingObject(obj);
                    Selection.activeObject = obj;
                }
            }

            // タイプ
            EditorGUILayout.LabelField(asset.AssetType, GUILayout.Width(100));

            // サイズ
            EditorGUILayout.LabelField(FormatSize(asset.FileSize), GUILayout.Width(80));

            // アクションボタン
            if (GUILayout.Button("Show", EditorStyles.miniButton, GUILayout.Width(50)))
            {
                var obj = AssetDatabase.LoadAssetAtPath<UnityEngine.Object>(asset.Path);
                if (obj != null)
                {
                    EditorGUIUtility.PingObject(obj);
                    Selection.activeObject = obj;
                }
            }

            EditorGUILayout.EndHorizontal();
        }

        /// <summary>
        /// ウェルカムメッセージを描画
        /// </summary>
        private void DrawWelcomeMessage()
        {
            EditorGUILayout.Space(50);
            EditorGUILayout.BeginVertical();
            GUILayout.FlexibleSpace();

            EditorGUILayout.BeginHorizontal();
            GUILayout.FlexibleSpace();
            EditorGUILayout.BeginVertical(GUILayout.Width(400));

            var style = new GUIStyle(EditorStyles.label)
            {
                alignment = TextAnchor.MiddleCenter,
                fontSize = 14,
                wordWrap = true
            };

            EditorGUILayout.LabelField("Unused Asset Detector", new GUIStyle(EditorStyles.boldLabel)
            {
                alignment = TextAnchor.MiddleCenter,
                fontSize = 18
            });

            EditorGUILayout.Space(20);
            EditorGUILayout.LabelField("プロジェクト内の未使用アセットを検出します。", style);
            EditorGUILayout.Space(10);
            EditorGUILayout.LabelField("「Scan Project」ボタンをクリックして開始してください。", style);

            EditorGUILayout.Space(30);

            EditorGUILayout.LabelField("スキャン対象:", EditorStyles.boldLabel);
            EditorGUILayout.LabelField("• Build Settings のシーン", style);
            EditorGUILayout.LabelField("• Resources フォルダ", style);
            EditorGUILayout.LabelField("• Prefab と ScriptableObject の依存関係", style);

            EditorGUILayout.EndVertical();
            GUILayout.FlexibleSpace();
            EditorGUILayout.EndHorizontal();

            GUILayout.FlexibleSpace();
            EditorGUILayout.EndVertical();
        }

        /// <summary>
        /// スキャンを開始
        /// </summary>
        private void StartScan()
        {
            _isScanning = true;
            _progress = 0;
            _progressMessage = "準備中...";
            _selectedGuids.Clear();

            EditorApplication.delayCall += () =>
            {
                try
                {
                    _collector?.Collect();
                    OnScanComplete();
                }
                catch (Exception e)
                {
                    Debug.LogError($"[UnusedAssetDetector] スキャンエラー: {e}");
                    _isScanning = false;
                }

                Repaint();
            };
        }

        /// <summary>
        /// スキャン進捗コールバック
        /// </summary>
        private void OnScanProgress(int current, int total, string message)
        {
            _progress = (float)current / total;
            _progressMessage = message;
            Repaint();
        }

        /// <summary>
        /// スキャン完了時の処理
        /// </summary>
        private void OnScanComplete()
        {
            _isScanning = false;

            if (_collector == null) return;

            _statistics = _collector.Map.GetStatistics();

            // タイプリストを更新
            var types = _collector.Map.Unreferenced
                .Select(a => a.AssetType)
                .Distinct()
                .OrderBy(t => t)
                .ToList();
            types.Insert(0, "All");
            _availableTypes = types.ToArray();
            _typeFilter = "All";

            ApplyFilters();

            Debug.Log($"[UnusedAssetDetector] スキャン完了: {_statistics.UnreferencedCount} 未使用アセット ({_statistics.FormattedSize})");
        }

        /// <summary>
        /// フィルターを適用
        /// </summary>
        private void ApplyFilters()
        {
            if (_collector == null) return;

            var assets = _collector.Map.Unreferenced.AsEnumerable();

            // 検索フィルタ
            if (!string.IsNullOrEmpty(_searchFilter))
            {
                var search = _searchFilter.ToLowerInvariant();
                assets = assets.Where(a => a.Path.ToLowerInvariant().Contains(search));
            }

            // タイプフィルタ
            if (_typeFilter != "All")
            {
                assets = assets.Where(a => a.AssetType == _typeFilter);
            }

            // ソート
            assets = _sortColumn switch
            {
                SortColumn.Path => _sortAscending ? assets.OrderBy(a => a.Path) : assets.OrderByDescending(a => a.Path),
                SortColumn.Type => _sortAscending ? assets.OrderBy(a => a.AssetType) : assets.OrderByDescending(a => a.AssetType),
                SortColumn.Size => _sortAscending ? assets.OrderBy(a => a.FileSize) : assets.OrderByDescending(a => a.FileSize),
                _ => assets
            };

            _displayedAssets = assets.ToList();
        }

        /// <summary>
        /// ソートを切り替え
        /// </summary>
        private void ToggleSort(SortColumn column)
        {
            if (_sortColumn == column)
            {
                _sortAscending = !_sortAscending;
            }
            else
            {
                _sortColumn = column;
                _sortAscending = true;
            }
            ApplyFilters();
        }

        /// <summary>
        /// ソートラベルを取得
        /// </summary>
        private string GetSortLabel(string label, SortColumn column)
        {
            if (_sortColumn == column)
            {
                return label + (_sortAscending ? " ▲" : " ▼");
            }
            return label;
        }

        /// <summary>
        /// 全選択
        /// </summary>
        private void SelectAll()
        {
            foreach (var asset in _displayedAssets)
            {
                _selectedGuids.Add(asset.Guid);
            }
        }

        /// <summary>
        /// 全選択解除
        /// </summary>
        private void DeselectAll()
        {
            _selectedGuids.Clear();
        }

        /// <summary>
        /// 選択したアセットを削除
        /// </summary>
        private void DeleteSelected()
        {
            if (_selectedGuids.Count == 0) return;

            var paths = _selectedGuids
                .Select(g => _collector?.Map.GetByGuid(g)?.Path)
                .Where(p => p != null)
                .ToList();

            var totalSize = _selectedGuids
                .Select(g => _collector?.Map.GetByGuid(g)?.FileSize ?? 0)
                .Sum();

            var message = $"{paths.Count} 個のアセットを削除しますか？\n\n合計サイズ: {FormatSize(totalSize)}\n\nこの操作は取り消せません。";

            if (EditorUtility.DisplayDialog("アセットの削除", message, "削除", "キャンセル"))
            {
                var deleted = 0;
                var failed = new List<string>();

                foreach (var path in paths)
                {
                    if (path == null) continue;

                    if (AssetDatabase.DeleteAsset(path))
                    {
                        deleted++;
                    }
                    else
                    {
                        failed.Add(path);
                    }
                }

                AssetDatabase.Refresh();

                if (failed.Count > 0)
                {
                    Debug.LogWarning($"[UnusedAssetDetector] {failed.Count} 個のアセットの削除に失敗しました:\n" + string.Join("\n", failed));
                }

                Debug.Log($"[UnusedAssetDetector] {deleted} 個のアセットを削除しました");

                // 再スキャン
                StartScan();
            }
        }

        /// <summary>
        /// サイズをフォーマット
        /// </summary>
        private static string FormatSize(long bytes)
        {
            string[] units = { "B", "KB", "MB", "GB" };
            double size = bytes;
            int i = 0;
            while (size >= 1024 && i < units.Length - 1)
            {
                size /= 1024;
                i++;
            }
            return $"{size:0.#} {units[i]}";
        }
    }
}
