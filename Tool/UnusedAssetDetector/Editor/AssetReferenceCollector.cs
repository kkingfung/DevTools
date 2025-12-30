#nullable enable
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using UnityEditor;
using UnityEngine;

namespace UnusedAssetDetector
{
    /// <summary>
    /// アセット参照を収集するクラス
    /// Build Settingsのシーン、Prefab、ScriptableObjectをスキャンし、
    /// 参照マップを構築する
    /// </summary>
    public class AssetReferenceCollector
    {
        // GUIDを抽出する正規表現（.metaファイルや.asset、.prefab、.unity等から）
        private static readonly Regex GuidRegex = new(@"guid:\s*([a-fA-F0-9]{32})", RegexOptions.Compiled);

        private readonly AssetReferenceMap _map = new();
        private readonly HashSet<string> _excludedFolders = new(StringComparer.OrdinalIgnoreCase);
        private readonly HashSet<string> _excludedExtensions = new(StringComparer.OrdinalIgnoreCase);

        /// <summary>
        /// 参照マップを取得
        /// </summary>
        public AssetReferenceMap Map => _map;

        /// <summary>
        /// 進捗コールバック（現在のステップ、総ステップ、メッセージ）
        /// </summary>
        public Action<int, int, string>? OnProgress;

        public AssetReferenceCollector()
        {
            // デフォルトで除外するフォルダ
            _excludedFolders.Add("Packages");
            _excludedFolders.Add("Library");
            _excludedFolders.Add("Temp");
            _excludedFolders.Add("Logs");
            _excludedFolders.Add("obj");
            _excludedFolders.Add(".git");

            // デフォルトで除外する拡張子
            _excludedExtensions.Add(".cs");
            _excludedExtensions.Add(".js");
            _excludedExtensions.Add(".dll");
            _excludedExtensions.Add(".asmdef");
            _excludedExtensions.Add(".asmref");
        }

        /// <summary>
        /// 除外フォルダを追加
        /// </summary>
        public void AddExcludedFolder(string folder) => _excludedFolders.Add(folder);

        /// <summary>
        /// 除外拡張子を追加
        /// </summary>
        public void AddExcludedExtension(string ext) => _excludedExtensions.Add(ext);

        /// <summary>
        /// 全アセットの参照を収集
        /// </summary>
        public void Collect()
        {
            _map.Clear();

            // Step 1: プロジェクト内の全アセットを登録
            ReportProgress(1, 5, "プロジェクトアセットを登録中...");
            RegisterAllAssets();

            // Step 2: Build Settingsのシーンをルートとしてマーク
            ReportProgress(2, 5, "ビルドシーンをスキャン中...");
            ScanBuildScenes();

            // Step 3: Resourcesフォルダのアセットをルートとしてマーク
            ReportProgress(3, 5, "Resourcesフォルダをスキャン中...");
            ScanResourcesFolder();

            // Step 4: 全アセットの依存関係を収集
            ReportProgress(4, 5, "依存関係を収集中...");
            CollectAllDependencies();

            // Step 5: 参照を伝播して状態を確定
            ReportProgress(5, 5, "参照状態を計算中...");
            _map.PropagateReferences();
        }

        /// <summary>
        /// プロジェクト内の全アセットを登録
        /// </summary>
        private void RegisterAllAssets()
        {
            var allAssets = AssetDatabase.GetAllAssetPaths();
            var count = 0;

            foreach (var path in allAssets)
            {
                // Assetsフォルダ外は除外
                if (!path.StartsWith("Assets/"))
                    continue;

                // 除外フォルダチェック
                if (IsExcludedPath(path))
                    continue;

                // フォルダは除外
                if (AssetDatabase.IsValidFolder(path))
                    continue;

                // 除外拡張子チェック
                var ext = Path.GetExtension(path);
                if (_excludedExtensions.Contains(ext))
                    continue;

                var guid = AssetDatabase.AssetPathToGUID(path);
                if (string.IsNullOrEmpty(guid))
                    continue;

                var info = _map.Register(guid, path);
                info.AssetType = GetAssetType(path);

                // ファイルサイズを取得
                try
                {
                    var fullPath = Path.GetFullPath(path);
                    if (File.Exists(fullPath))
                        info.FileSize = new FileInfo(fullPath).Length;
                }
                catch
                {
                    // ファイルアクセスエラーは無視
                }

                count++;
            }

            Debug.Log($"[UnusedAssetDetector] {count} アセットを登録しました");
        }

        /// <summary>
        /// Build Settingsのシーンをスキャン
        /// </summary>
        private void ScanBuildScenes()
        {
            var scenes = EditorBuildSettings.scenes;
            var count = 0;

            foreach (var scene in scenes)
            {
                if (!scene.enabled || string.IsNullOrEmpty(scene.path))
                    continue;

                var guid = AssetDatabase.AssetPathToGUID(scene.path);
                if (string.IsNullOrEmpty(guid))
                    continue;

                // シーンを登録（まだ登録されていない場合）
                if (!_map.Contains(guid))
                {
                    var info = _map.Register(guid, scene.path);
                    info.AssetType = "Scene";
                }

                // ルートとしてマーク
                _map.MarkAsRoot(guid);
                count++;
            }

            Debug.Log($"[UnusedAssetDetector] {count} ビルドシーンをルートとしてマークしました");
        }

        /// <summary>
        /// Resourcesフォルダのアセットをスキャン
        /// </summary>
        private void ScanResourcesFolder()
        {
            var resourceGuids = AssetDatabase.FindAssets("", new[] { "Assets" });
            var count = 0;

            foreach (var guid in resourceGuids)
            {
                var path = AssetDatabase.GUIDToAssetPath(guid);

                // Resourcesフォルダ内かチェック
                if (!path.Contains("/Resources/"))
                    continue;

                // フォルダは除外
                if (AssetDatabase.IsValidFolder(path))
                    continue;

                // 登録されていなければ登録
                if (!_map.Contains(guid))
                {
                    var info = _map.Register(guid, path);
                    info.AssetType = GetAssetType(path);
                }

                // ルートとしてマーク
                _map.MarkAsRoot(guid);
                count++;
            }

            Debug.Log($"[UnusedAssetDetector] {count} Resourcesアセットをルートとしてマークしました");
        }

        /// <summary>
        /// 全アセットの依存関係を収集
        /// </summary>
        private void CollectAllDependencies()
        {
            var totalCount = 0;

            // コレクション変更エラーを避けるため、先にリストにコピー
            var assetsToProcess = new List<AssetInfo>(_map.All);

            foreach (var asset in assetsToProcess)
            {
                var dependencies = AssetDatabase.GetDependencies(asset.Path, false);

                foreach (var depPath in dependencies)
                {
                    if (depPath == asset.Path)
                        continue;

                    var depGuid = AssetDatabase.AssetPathToGUID(depPath);
                    if (string.IsNullOrEmpty(depGuid))
                        continue;

                    // 依存先が登録されていなければ登録
                    if (!_map.Contains(depGuid))
                    {
                        var depInfo = _map.Register(depGuid, depPath);
                        depInfo.AssetType = GetAssetType(depPath);
                    }

                    // 参照関係を追加
                    _map.AddReference(asset.Guid, depGuid);
                    totalCount++;
                }
            }

            Debug.Log($"[UnusedAssetDetector] {totalCount} 参照関係を収集しました");
        }

        /// <summary>
        /// アセットの種類を判定
        /// </summary>
        private string GetAssetType(string path)
        {
            var ext = Path.GetExtension(path).ToLowerInvariant();
            return ext switch
            {
                ".unity" => "Scene",
                ".prefab" => "Prefab",
                ".asset" => "ScriptableObject",
                ".mat" => "Material",
                ".png" or ".jpg" or ".jpeg" or ".tga" or ".psd" or ".gif" or ".bmp" => "Texture",
                ".fbx" or ".obj" or ".blend" or ".dae" or ".3ds" or ".max" => "Model",
                ".anim" => "Animation",
                ".controller" => "AnimatorController",
                ".wav" or ".mp3" or ".ogg" or ".aif" or ".aiff" => "Audio",
                ".shader" or ".shadergraph" or ".shadersubgraph" => "Shader",
                ".ttf" or ".otf" => "Font",
                ".uxml" => "UXML",
                ".uss" => "USS",
                ".json" => "JSON",
                ".xml" => "XML",
                ".txt" => "Text",
                ".playable" => "Timeline",
                ".signal" => "Signal",
                ".mixer" => "AudioMixer",
                ".lighting" or ".giparams" => "Lighting",
                ".terrainlayer" => "TerrainLayer",
                ".brush" => "Brush",
                ".flare" => "Flare",
                ".compute" => "ComputeShader",
                ".renderTexture" => "RenderTexture",
                ".cubemap" => "Cubemap",
                ".physicMaterial" or ".physicsmaterial" => "PhysicsMaterial",
                ".guiskin" => "GUISkin",
                ".fontsettings" => "FontSettings",
                ".spriteatlas" => "SpriteAtlas",
                ".mask" => "AvatarMask",
                _ => "Asset"
            };
        }

        /// <summary>
        /// 除外パスかチェック
        /// </summary>
        private bool IsExcludedPath(string path)
        {
            foreach (var folder in _excludedFolders)
            {
                if (path.Contains($"/{folder}/") || path.StartsWith($"{folder}/"))
                    return true;
            }
            return false;
        }

        /// <summary>
        /// 進捗を報告
        /// </summary>
        private void ReportProgress(int current, int total, string message)
        {
            OnProgress?.Invoke(current, total, message);
        }
    }
}
