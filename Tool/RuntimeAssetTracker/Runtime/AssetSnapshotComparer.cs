#nullable enable
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

namespace RuntimeAssetTracker
{
    /// <summary>
    /// 2つのアセットスナップショットを比較し、残存アセットを検出するシステム
    /// </summary>
    public static class AssetSnapshotComparer
    {
        /// <summary>
        /// 2つのスナップショットを比較し、残存・新規・解放されたアセットを検出
        /// </summary>
        /// <param name="baseSnapshot">比較元（遷移前）のスナップショット</param>
        /// <param name="compareSnapshot">比較先（遷移後）のスナップショット</param>
        /// <returns>比較結果</returns>
        public static SnapshotComparisonResult Compare(AssetSnapshot baseSnapshot, AssetSnapshot compareSnapshot)
        {
            var result = new SnapshotComparisonResult
            {
                BaseSnapshotLabel = baseSnapshot.Label,
                CompareSnapshotLabel = compareSnapshot.Label
            };

            // テクスチャの比較
            CompareAssets(
                baseSnapshot.Textures.Select(t => (t.InstanceId, t.Name, t.MemoryBytes)).ToList(),
                compareSnapshot.Textures.Select(t => (t.InstanceId, t.Name, t.MemoryBytes)).ToList(),
                "Texture",
                result.ResidualTextures,
                result.NewTextures,
                result.ReleasedTextures
            );

            // オーディオクリップの比較
            CompareAssets(
                baseSnapshot.AudioClips.Select(a => (a.InstanceId, a.Name, a.MemoryBytes)).ToList(),
                compareSnapshot.AudioClips.Select(a => (a.InstanceId, a.Name, a.MemoryBytes)).ToList(),
                "AudioClip",
                result.ResidualAudioClips,
                result.NewAudioClips,
                result.ReleasedAudioClips
            );

            // 残存メモリの計算
            result.ResidualMemoryBytes =
                result.ResidualTextures.Sum(t => t.MemoryBytes) +
                result.ResidualAudioClips.Sum(a => a.MemoryBytes);

            return result;
        }

        /// <summary>
        /// アセットリストを比較
        /// </summary>
        private static void CompareAssets(
            List<(int InstanceId, string Name, long MemoryBytes)> baseAssets,
            List<(int InstanceId, string Name, long MemoryBytes)> compareAssets,
            string assetType,
            List<ResidualAssetInfo> residualList,
            List<ResidualAssetInfo> newList,
            List<ResidualAssetInfo> releasedList)
        {
            var baseIds = new HashSet<int>(baseAssets.Select(a => a.InstanceId));
            var compareIds = new HashSet<int>(compareAssets.Select(a => a.InstanceId));

            // 残存アセット（両方に存在）
            foreach (var asset in compareAssets)
            {
                if (baseIds.Contains(asset.InstanceId))
                {
                    residualList.Add(new ResidualAssetInfo
                    {
                        InstanceId = asset.InstanceId,
                        Name = asset.Name,
                        MemoryBytes = asset.MemoryBytes,
                        AssetType = assetType
                    });
                }
            }

            // 新規アセット（比較先のみに存在）
            foreach (var asset in compareAssets)
            {
                if (!baseIds.Contains(asset.InstanceId))
                {
                    newList.Add(new ResidualAssetInfo
                    {
                        InstanceId = asset.InstanceId,
                        Name = asset.Name,
                        MemoryBytes = asset.MemoryBytes,
                        AssetType = assetType
                    });
                }
            }

            // 解放されたアセット（比較元のみに存在）
            foreach (var asset in baseAssets)
            {
                if (!compareIds.Contains(asset.InstanceId))
                {
                    releasedList.Add(new ResidualAssetInfo
                    {
                        InstanceId = asset.InstanceId,
                        Name = asset.Name,
                        MemoryBytes = asset.MemoryBytes,
                        AssetType = assetType
                    });
                }
            }

            // メモリ使用量でソート
            residualList.Sort((a, b) => b.MemoryBytes.CompareTo(a.MemoryBytes));
            newList.Sort((a, b) => b.MemoryBytes.CompareTo(a.MemoryBytes));
            releasedList.Sort((a, b) => b.MemoryBytes.CompareTo(a.MemoryBytes));
        }

        /// <summary>
        /// 比較結果をログに出力
        /// </summary>
        public static void LogComparisonResult(SnapshotComparisonResult result)
        {
            var sb = new System.Text.StringBuilder();
            sb.AppendLine($"[RuntimeAssetTracker] スナップショット比較結果");
            sb.AppendLine($"  比較: {result.BaseSnapshotLabel} → {result.CompareSnapshotLabel}");
            sb.AppendLine();

            // 残存アセット
            sb.AppendLine($"=== 残存アセット（シーン遷移後も残っている） ===");
            sb.AppendLine($"  テクスチャ: {result.ResidualTextures.Count}個 ({AssetSnapshotCapture.FormatBytes(result.ResidualTextures.Sum(t => t.MemoryBytes))})");
            foreach (var texture in result.ResidualTextures.Take(10))
            {
                sb.AppendLine($"    - {texture.Name} ({AssetSnapshotCapture.FormatBytes(texture.MemoryBytes)})");
            }
            if (result.ResidualTextures.Count > 10)
            {
                sb.AppendLine($"    ... 他 {result.ResidualTextures.Count - 10}個");
            }

            sb.AppendLine($"  オーディオ: {result.ResidualAudioClips.Count}個 ({AssetSnapshotCapture.FormatBytes(result.ResidualAudioClips.Sum(a => a.MemoryBytes))})");
            foreach (var audio in result.ResidualAudioClips.Take(10))
            {
                sb.AppendLine($"    - {audio.Name} ({AssetSnapshotCapture.FormatBytes(audio.MemoryBytes)})");
            }
            if (result.ResidualAudioClips.Count > 10)
            {
                sb.AppendLine($"    ... 他 {result.ResidualAudioClips.Count - 10}個");
            }

            sb.AppendLine();
            sb.AppendLine($"=== 新規ロードアセット ===");
            sb.AppendLine($"  テクスチャ: {result.NewTextures.Count}個");
            sb.AppendLine($"  オーディオ: {result.NewAudioClips.Count}個");

            sb.AppendLine();
            sb.AppendLine($"=== 解放されたアセット ===");
            sb.AppendLine($"  テクスチャ: {result.ReleasedTextures.Count}個");
            sb.AppendLine($"  オーディオ: {result.ReleasedAudioClips.Count}個");

            sb.AppendLine();
            sb.AppendLine($"残存メモリ合計: {AssetSnapshotCapture.FormatBytes(result.ResidualMemoryBytes)}");

            Debug.Log(sb.ToString());
        }
    }
}
