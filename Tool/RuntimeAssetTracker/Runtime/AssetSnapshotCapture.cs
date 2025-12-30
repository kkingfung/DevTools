#nullable enable
using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Profiling;
using UnityEngine.SceneManagement;

namespace RuntimeAssetTracker
{
    /// <summary>
    /// ランタイムでロードされているアセットのスナップショットをキャプチャするシステム
    /// </summary>
    public static class AssetSnapshotCapture
    {
        /// <summary>
        /// 現在ロードされているテクスチャとオーディオクリップのスナップショットを取得
        /// </summary>
        /// <param name="label">スナップショットの識別ラベル</param>
        /// <returns>アセットスナップショット</returns>
        public static AssetSnapshot CaptureSnapshot(string label = "")
        {
            var snapshot = new AssetSnapshot
            {
                Timestamp = DateTime.Now.ToString("o"),
                SceneName = SceneManager.GetActiveScene().name,
                Label = string.IsNullOrEmpty(label) ? $"Snapshot_{DateTime.Now:HHmmss}" : label
            };

            // テクスチャをキャプチャ
            CaptureTextures(snapshot);

            // オーディオクリップをキャプチャ
            CaptureAudioClips(snapshot);

            // 合計メモリを計算
            snapshot.TotalMemoryBytes = snapshot.TextureMemoryBytes + snapshot.AudioMemoryBytes;

            Debug.Log($"[RuntimeAssetTracker] スナップショット取得完了: {snapshot.Label}\n" +
                      $"  テクスチャ: {snapshot.Textures.Count}個 ({FormatBytes(snapshot.TextureMemoryBytes)})\n" +
                      $"  オーディオ: {snapshot.AudioClips.Count}個 ({FormatBytes(snapshot.AudioMemoryBytes)})\n" +
                      $"  合計: {FormatBytes(snapshot.TotalMemoryBytes)}");

            return snapshot;
        }

        /// <summary>
        /// ロードされているすべてのテクスチャをキャプチャ
        /// </summary>
        private static void CaptureTextures(AssetSnapshot snapshot)
        {
            var textures = Resources.FindObjectsOfTypeAll<Texture>();
            long totalMemory = 0;

            foreach (var texture in textures)
            {
                // エディタ専用アセットやシステムテクスチャをスキップ
                if (ShouldSkipAsset(texture))
                {
                    continue;
                }

                var memorySize = Profiler.GetRuntimeMemorySizeLong(texture);
                totalMemory += memorySize;

                var info = new TextureAssetInfo
                {
                    InstanceId = texture.GetInstanceID(),
                    Name = texture.name,
                    MemoryBytes = memorySize,
                    Width = texture.width,
                    Height = texture.height,
                    MipmapCount = GetMipmapCount(texture)
                };

                if (texture is Texture2D tex2D)
                {
                    info.Format = tex2D.format.ToString();
                }
                else if (texture is RenderTexture rt)
                {
                    info.Format = $"RenderTexture ({rt.format})";
                }
                else
                {
                    info.Format = texture.GetType().Name;
                }

                snapshot.Textures.Add(info);
            }

            snapshot.TextureMemoryBytes = totalMemory;

            // メモリ使用量の大きい順にソート
            snapshot.Textures.Sort((a, b) => b.MemoryBytes.CompareTo(a.MemoryBytes));
        }

        /// <summary>
        /// ロードされているすべてのオーディオクリップをキャプチャ
        /// </summary>
        private static void CaptureAudioClips(AssetSnapshot snapshot)
        {
            var audioClips = Resources.FindObjectsOfTypeAll<AudioClip>();
            long totalMemory = 0;

            foreach (var clip in audioClips)
            {
                // エディタ専用アセットをスキップ
                if (ShouldSkipAsset(clip))
                {
                    continue;
                }

                var memorySize = Profiler.GetRuntimeMemorySizeLong(clip);
                totalMemory += memorySize;

                var info = new AudioClipAssetInfo
                {
                    InstanceId = clip.GetInstanceID(),
                    Name = clip.name,
                    MemoryBytes = memorySize,
                    Length = clip.length,
                    Channels = clip.channels,
                    Frequency = clip.frequency,
                    LoadType = clip.loadType.ToString()
                };

                snapshot.AudioClips.Add(info);
            }

            snapshot.AudioMemoryBytes = totalMemory;

            // メモリ使用量の大きい順にソート
            snapshot.AudioClips.Sort((a, b) => b.MemoryBytes.CompareTo(a.MemoryBytes));
        }

        /// <summary>
        /// アセットをスキップすべきかどうかを判定
        /// </summary>
        private static bool ShouldSkipAsset(UnityEngine.Object asset)
        {
            if (asset == null)
            {
                return true;
            }

            var name = asset.name;

            // 空の名前やシステムアセットをスキップ
            if (string.IsNullOrEmpty(name))
            {
                return true;
            }

            // Unityの内部アセットをスキップ
            if (name.StartsWith("unity_builtin") ||
                name.StartsWith("Hidden/") ||
                name == "Default-Material" ||
                name == "Default-Skybox" ||
                name == "Default-Line" ||
                name == "Default-Particle")
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// テクスチャのミップマップ数を取得
        /// </summary>
        private static int GetMipmapCount(Texture texture)
        {
            if (texture is Texture2D tex2D)
            {
                return tex2D.mipmapCount;
            }
            return 1;
        }

        /// <summary>
        /// バイト数を読みやすい形式にフォーマット
        /// </summary>
        public static string FormatBytes(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB" };
            int order = 0;
            double size = bytes;

            while (size >= 1024 && order < sizes.Length - 1)
            {
                order++;
                size /= 1024;
            }

            return $"{size:0.##} {sizes[order]}";
        }
    }
}
