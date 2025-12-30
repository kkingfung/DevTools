#nullable enable
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace RuntimeAssetTracker
{
    /// <summary>
    /// ランタイムアセットトラッカーのマネージャーコンポーネント
    /// シーン遷移時の自動スナップショットやスナップショット管理を行う
    /// </summary>
    public class RuntimeAssetTrackerManager : MonoBehaviour
    {
        /// <summary>シングルトンインスタンス</summary>
        public static RuntimeAssetTrackerManager? Instance { get; private set; }

        [Header("自動トラッキング設定")]
        [SerializeField]
        [Tooltip("シーン遷移時に自動でスナップショットを取得")]
        private bool _autoSnapshotOnSceneChange = true;

        [SerializeField]
        [Tooltip("スナップショットをJSONファイルに保存")]
        private bool _saveSnapshotsToFile;

        [SerializeField]
        [Tooltip("保存先ディレクトリ（空の場合はApplication.persistentDataPath）")]
        private string _outputDirectory = string.Empty;

        [Header("デバッグ設定")]
        [SerializeField]
        [Tooltip("比較結果をコンソールに出力")]
        private bool _logComparisonResults = true;

        /// <summary>保存されたスナップショットのリスト</summary>
        private readonly List<AssetSnapshot> _snapshots = new();

        /// <summary>最後に取得したスナップショット</summary>
        public AssetSnapshot? LastSnapshot => _snapshots.Count > 0 ? _snapshots[^1] : null;

        /// <summary>保存されたスナップショット数</summary>
        public int SnapshotCount => _snapshots.Count;

        /// <summary>保存されたすべてのスナップショット</summary>
        public IReadOnlyList<AssetSnapshot> Snapshots => _snapshots;

        private void Awake()
        {
            if (Instance != null && Instance != this)
            {
                Destroy(gameObject);
                return;
            }

            Instance = this;
            DontDestroyOnLoad(gameObject);

            if (_autoSnapshotOnSceneChange)
            {
                SceneManager.sceneLoaded += OnSceneLoaded;
                SceneManager.sceneUnloaded += OnSceneUnloaded;
            }
        }

        private void OnDestroy()
        {
            if (Instance == this)
            {
                SceneManager.sceneLoaded -= OnSceneLoaded;
                SceneManager.sceneUnloaded -= OnSceneUnloaded;
                Instance = null;
            }
        }

        /// <summary>
        /// シーンロード完了時のコールバック
        /// </summary>
        private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
        {
            // 1フレーム待ってからスナップショットを取得（シーン初期化完了後）
            StartCoroutine(CaptureSnapshotDelayed($"After_{scene.name}_Load"));
        }

        /// <summary>
        /// シーンアンロード時のコールバック
        /// </summary>
        private void OnSceneUnloaded(Scene scene)
        {
            TakeSnapshot($"After_{scene.name}_Unload");
        }

        /// <summary>
        /// 遅延してスナップショットを取得
        /// </summary>
        private System.Collections.IEnumerator CaptureSnapshotDelayed(string label)
        {
            yield return null; // 1フレーム待機
            TakeSnapshot(label);
        }

        /// <summary>
        /// スナップショットを取得
        /// </summary>
        /// <param name="label">スナップショットのラベル</param>
        /// <returns>取得したスナップショット</returns>
        public AssetSnapshot TakeSnapshot(string label = "")
        {
            var snapshot = AssetSnapshotCapture.CaptureSnapshot(label);
            _snapshots.Add(snapshot);

            if (_saveSnapshotsToFile)
            {
                SaveSnapshotToFile(snapshot);
            }

            // 2つ以上のスナップショットがある場合、直前と比較
            if (_logComparisonResults && _snapshots.Count >= 2)
            {
                var previousSnapshot = _snapshots[^2];
                var result = AssetSnapshotComparer.Compare(previousSnapshot, snapshot);
                AssetSnapshotComparer.LogComparisonResult(result);
            }

            return snapshot;
        }

        /// <summary>
        /// 2つのスナップショットを比較
        /// </summary>
        /// <param name="baseIndex">比較元のインデックス</param>
        /// <param name="compareIndex">比較先のインデックス</param>
        /// <returns>比較結果</returns>
        public SnapshotComparisonResult? CompareSnapshots(int baseIndex, int compareIndex)
        {
            if (baseIndex < 0 || baseIndex >= _snapshots.Count ||
                compareIndex < 0 || compareIndex >= _snapshots.Count)
            {
                Debug.LogError($"[RuntimeAssetTracker] 無効なスナップショットインデックス: base={baseIndex}, compare={compareIndex}");
                return null;
            }

            var result = AssetSnapshotComparer.Compare(_snapshots[baseIndex], _snapshots[compareIndex]);

            if (_logComparisonResults)
            {
                AssetSnapshotComparer.LogComparisonResult(result);
            }

            return result;
        }

        /// <summary>
        /// 最後の2つのスナップショットを比較
        /// </summary>
        /// <returns>比較結果</returns>
        public SnapshotComparisonResult? CompareLastTwoSnapshots()
        {
            if (_snapshots.Count < 2)
            {
                Debug.LogWarning("[RuntimeAssetTracker] 比較には2つ以上のスナップショットが必要です");
                return null;
            }

            return CompareSnapshots(_snapshots.Count - 2, _snapshots.Count - 1);
        }

        /// <summary>
        /// すべてのスナップショットをクリア
        /// </summary>
        public void ClearSnapshots()
        {
            _snapshots.Clear();
            Debug.Log("[RuntimeAssetTracker] スナップショットをクリアしました");
        }

        /// <summary>
        /// スナップショットをJSONファイルに保存
        /// </summary>
        private void SaveSnapshotToFile(AssetSnapshot snapshot)
        {
            var outputDir = string.IsNullOrEmpty(_outputDirectory)
                ? Path.Combine(Application.persistentDataPath, "AssetSnapshots")
                : _outputDirectory;

            if (!Directory.Exists(outputDir))
            {
                Directory.CreateDirectory(outputDir);
            }

            var fileName = $"Snapshot_{snapshot.Label}_{System.DateTime.Now:yyyyMMdd_HHmmss}.json";
            var filePath = Path.Combine(outputDir, fileName);

            var json = JsonUtility.ToJson(snapshot, true);
            File.WriteAllText(filePath, json);

            Debug.Log($"[RuntimeAssetTracker] スナップショットを保存: {filePath}");
        }

        /// <summary>
        /// 特定のインデックスのスナップショットを取得
        /// </summary>
        public AssetSnapshot? GetSnapshot(int index)
        {
            if (index < 0 || index >= _snapshots.Count)
            {
                return null;
            }
            return _snapshots[index];
        }
    }
}
