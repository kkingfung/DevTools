#nullable enable
using UnityEditor;
using UnityEngine;

namespace BuildSizeAnalyzer
{
    /// <summary>
    /// BuildSizeAnalyzerの設定を管理するクラス
    /// EditorPrefsを使用して設定を永続化
    /// </summary>
    public static class BuildSizeAnalyzerSettings
    {
        private const string EnableAutoCaptureKey = "BuildSizeAnalyzer_EnableAutoCapture";
        private const string OutputDirectoryKey = "BuildSizeAnalyzer_OutputDirectory";
        private const string TopAssetsCountKey = "BuildSizeAnalyzer_TopAssetsCount";

        /// <summary>
        /// ビルド時に自動でレポートをキャプチャするかどうか
        /// </summary>
        public static bool EnableAutoCapture
        {
            get => EditorPrefs.GetBool(EnableAutoCaptureKey, true);
            set => EditorPrefs.SetBool(EnableAutoCaptureKey, value);
        }

        /// <summary>
        /// レポート出力ディレクトリ（空の場合はプロジェクトルート/BuildSizeReports）
        /// </summary>
        public static string OutputDirectory
        {
            get => EditorPrefs.GetString(OutputDirectoryKey, string.Empty);
            set => EditorPrefs.SetString(OutputDirectoryKey, value);
        }

        /// <summary>
        /// 表示するトップアセットの数
        /// </summary>
        public static int TopAssetsCount
        {
            get => EditorPrefs.GetInt(TopAssetsCountKey, 50);
            set => EditorPrefs.SetInt(TopAssetsCountKey, Mathf.Clamp(value, 10, 500));
        }

        /// <summary>
        /// 設定をデフォルトにリセット
        /// </summary>
        public static void ResetToDefaults()
        {
            EditorPrefs.DeleteKey(EnableAutoCaptureKey);
            EditorPrefs.DeleteKey(OutputDirectoryKey);
            EditorPrefs.DeleteKey(TopAssetsCountKey);
        }
    }

    /// <summary>
    /// BuildSizeAnalyzer設定用のSettingsProvider
    /// </summary>
    public class BuildSizeAnalyzerSettingsProvider : SettingsProvider
    {
        public BuildSizeAnalyzerSettingsProvider(string path, SettingsScope scopes)
            : base(path, scopes)
        {
        }

        public override void OnGUI(string searchContext)
        {
            EditorGUILayout.Space(10);

            EditorGUILayout.LabelField("自動キャプチャ設定", EditorStyles.boldLabel);
            EditorGUI.indentLevel++;

            BuildSizeAnalyzerSettings.EnableAutoCapture = EditorGUILayout.Toggle(
                new GUIContent("ビルド時に自動保存", "ビルド完了時に自動的にサイズレポートを保存します"),
                BuildSizeAnalyzerSettings.EnableAutoCapture
            );

            EditorGUI.indentLevel--;
            EditorGUILayout.Space(10);

            EditorGUILayout.LabelField("出力設定", EditorStyles.boldLabel);
            EditorGUI.indentLevel++;

            EditorGUILayout.BeginHorizontal();
            BuildSizeAnalyzerSettings.OutputDirectory = EditorGUILayout.TextField(
                new GUIContent("出力ディレクトリ", "空の場合は ProjectRoot/BuildSizeReports に保存されます"),
                BuildSizeAnalyzerSettings.OutputDirectory
            );
            if (GUILayout.Button("選択", GUILayout.Width(50)))
            {
                var path = EditorUtility.OpenFolderPanel("レポート出力先を選択", BuildSizeAnalyzerSettings.OutputDirectory, "");
                if (!string.IsNullOrEmpty(path))
                {
                    BuildSizeAnalyzerSettings.OutputDirectory = path;
                }
            }
            EditorGUILayout.EndHorizontal();

            BuildSizeAnalyzerSettings.TopAssetsCount = EditorGUILayout.IntSlider(
                new GUIContent("表示アセット数", "比較ビューで表示するトップアセットの数"),
                BuildSizeAnalyzerSettings.TopAssetsCount,
                10,
                500
            );

            EditorGUI.indentLevel--;
            EditorGUILayout.Space(20);

            if (GUILayout.Button("設定をリセット", GUILayout.Width(120)))
            {
                if (EditorUtility.DisplayDialog("確認", "設定をデフォルトに戻しますか？", "はい", "いいえ"))
                {
                    BuildSizeAnalyzerSettings.ResetToDefaults();
                }
            }
        }

        [SettingsProvider]
        public static SettingsProvider CreateSettingsProvider()
        {
            return new BuildSizeAnalyzerSettingsProvider("Project/Build Size Analyzer", SettingsScope.Project)
            {
                keywords = new[] { "Build", "Size", "Analyzer", "Report" }
            };
        }
    }
}
