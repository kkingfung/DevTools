#nullable enable
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace BuildSizeAnalyzer
{
    /// <summary>
    /// ビルド完了後にBuildReportからサイズ情報を抽出し、JSONファイルに保存するポストプロセッサ
    /// </summary>
    public class BuildSizePostprocessor : IPostprocessBuildWithReport
    {
        /// <summary>コールバックの実行順序（低いほど先に実行）</summary>
        public int callbackOrder => 999;

        /// <summary>レポート保存先のデフォルトディレクトリ名</summary>
        private const string DefaultOutputDirectory = "BuildSizeReports";

        /// <summary>
        /// ビルド完了後に呼び出されるコールバック
        /// </summary>
        /// <param name="report">Unityが生成したビルドレポート</param>
        public void OnPostprocessBuild(BuildReport report)
        {
            if (!BuildSizeAnalyzerSettings.EnableAutoCapture)
            {
                return;
            }

            try
            {
                var sizeReport = ExtractBuildSizeReport(report);
                var outputPath = GetOutputPath(report);
                SaveReportToJson(sizeReport, outputPath);

                Debug.Log($"[BuildSizeAnalyzer] ビルドサイズレポートを保存しました: {outputPath}");
            }
            catch (Exception ex)
            {
                Debug.LogError($"[BuildSizeAnalyzer] レポート生成中にエラーが発生しました: {ex.Message}");
            }
        }

        /// <summary>
        /// BuildReportからBuildSizeReportを生成する
        /// </summary>
        private BuildSizeReport ExtractBuildSizeReport(BuildReport report)
        {
            var sizeReport = new BuildSizeReport
            {
                BuildDateTime = DateTime.Now.ToString("o"),
                UnityVersion = Application.unityVersion,
                BuildTarget = report.summary.platform.ToString(),
                OutputPath = report.summary.outputPath,
                TotalSizeBytes = (long)report.summary.totalSize,
                BuildResult = report.summary.result.ToString()
            };

            // カテゴリ別サイズを抽出
            ExtractCategorySizes(report, sizeReport);

            // 個別アセットサイズを抽出
            ExtractAssetSizes(report, sizeReport);

            // シーンサイズを抽出
            ExtractSceneSizes(report, sizeReport);

            return sizeReport;
        }

        /// <summary>
        /// カテゴリ別のサイズ情報を抽出
        /// </summary>
        private void ExtractCategorySizes(BuildReport report, BuildSizeReport sizeReport)
        {
            var categoryTotals = new Dictionary<string, long>();

            foreach (var packedAsset in report.packedAssets)
            {
                foreach (var content in packedAsset.contents)
                {
                    var category = GetAssetCategory(content.type.ToString());
                    var size = (long)content.packedSize;

                    if (categoryTotals.ContainsKey(category))
                    {
                        categoryTotals[category] += size;
                    }
                    else
                    {
                        categoryTotals[category] = size;
                    }
                }
            }

            var totalSize = categoryTotals.Values.Sum();
            foreach (var kvp in categoryTotals.OrderByDescending(x => x.Value))
            {
                sizeReport.Categories.Add(new CategorySizeInfo
                {
                    Category = kvp.Key,
                    SizeBytes = kvp.Value,
                    Percentage = totalSize > 0 ? (float)kvp.Value / totalSize * 100f : 0f
                });
            }
        }

        /// <summary>
        /// 個別アセットのサイズ情報を抽出
        /// </summary>
        private void ExtractAssetSizes(BuildReport report, BuildSizeReport sizeReport)
        {
            var assetSizes = new Dictionary<string, AssetSizeInfo>();

            foreach (var packedAsset in report.packedAssets)
            {
                foreach (var content in packedAsset.contents)
                {
                    var path = content.sourceAssetPath;
                    if (string.IsNullOrEmpty(path))
                    {
                        continue;
                    }

                    var size = (long)content.packedSize;

                    if (assetSizes.TryGetValue(path, out var existing))
                    {
                        // 同じアセットが複数回参照されている場合は合算
                        existing.SizeBytes += size;
                    }
                    else
                    {
                        assetSizes[path] = new AssetSizeInfo
                        {
                            Path = path,
                            Type = content.type.ToString(),
                            SizeBytes = size,
                            CompressedSizeBytes = size
                        };
                    }
                }
            }

            // サイズの大きい順にソートして追加
            sizeReport.Assets = assetSizes.Values
                .OrderByDescending(x => x.SizeBytes)
                .ToList();
        }

        /// <summary>
        /// シーン別サイズ情報を抽出
        /// </summary>
        private void ExtractSceneSizes(BuildReport report, BuildSizeReport sizeReport)
        {
            foreach (var packedAsset in report.packedAssets)
            {
                // シーンファイルを特定
                var sceneContents = packedAsset.contents
                    .Where(c => c.sourceAssetPath.EndsWith(".unity"))
                    .GroupBy(c => c.sourceAssetPath);

                foreach (var sceneGroup in sceneContents)
                {
                    var totalSceneSize = sceneGroup.Sum(c => (long)c.packedSize);
                    sizeReport.Scenes.Add(new SceneSizeInfo
                    {
                        Path = sceneGroup.Key,
                        SizeBytes = totalSceneSize
                    });
                }
            }

            // 重複を除去してサイズ順にソート
            sizeReport.Scenes = sizeReport.Scenes
                .GroupBy(s => s.Path)
                .Select(g => new SceneSizeInfo
                {
                    Path = g.Key,
                    SizeBytes = g.Sum(s => s.SizeBytes)
                })
                .OrderByDescending(s => s.SizeBytes)
                .ToList();
        }

        /// <summary>
        /// アセットタイプからカテゴリ名を取得
        /// </summary>
        private string GetAssetCategory(string assetType)
        {
            // 主要なアセットタイプをカテゴリに分類
            if (assetType.Contains("Texture") || assetType.Contains("Sprite"))
                return "Textures";
            if (assetType.Contains("Mesh"))
                return "Meshes";
            if (assetType.Contains("Audio") || assetType.Contains("Sound"))
                return "Audio";
            if (assetType.Contains("Shader"))
                return "Shaders";
            if (assetType.Contains("Material"))
                return "Materials";
            if (assetType.Contains("Animation") || assetType.Contains("Animator"))
                return "Animations";
            if (assetType.Contains("Font"))
                return "Fonts";
            if (assetType.Contains("Script") || assetType.Contains("MonoScript"))
                return "Scripts";
            if (assetType.Contains("Prefab"))
                return "Prefabs";
            if (assetType.Contains("Scene"))
                return "Scenes";
            if (assetType.Contains("Asset"))
                return "ScriptableObjects";

            return "Other";
        }

        /// <summary>
        /// 出力ファイルパスを生成
        /// </summary>
        private string GetOutputPath(BuildReport report)
        {
            var outputDir = BuildSizeAnalyzerSettings.OutputDirectory;
            if (string.IsNullOrEmpty(outputDir))
            {
                outputDir = Path.Combine(Directory.GetParent(Application.dataPath)!.FullName, DefaultOutputDirectory);
            }

            if (!Directory.Exists(outputDir))
            {
                Directory.CreateDirectory(outputDir);
            }

            var timestamp = DateTime.Now.ToString("yyyyMMdd_HHmmss");
            var platform = report.summary.platform.ToString();
            var fileName = $"BuildSize_{platform}_{timestamp}.json";

            return Path.Combine(outputDir, fileName);
        }

        /// <summary>
        /// レポートをJSONファイルに保存
        /// </summary>
        private void SaveReportToJson(BuildSizeReport report, string path)
        {
            var json = JsonUtility.ToJson(report, true);
            File.WriteAllText(path, json);
        }
    }
}
