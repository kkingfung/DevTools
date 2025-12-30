#nullable enable
using System;
using System.Collections.Generic;

namespace RuntimeAssetTracker
{
    /// <summary>
    /// アセットスナップショット全体のデータ構造
    /// </summary>
    [Serializable]
    public class AssetSnapshot
    {
        /// <summary>スナップショット取得時刻</summary>
        public string Timestamp = string.Empty;

        /// <summary>スナップショット取得時のシーン名</summary>
        public string SceneName = string.Empty;

        /// <summary>スナップショットのラベル（識別用）</summary>
        public string Label = string.Empty;

        /// <summary>テクスチャ一覧</summary>
        public List<TextureAssetInfo> Textures = new();

        /// <summary>オーディオクリップ一覧</summary>
        public List<AudioClipAssetInfo> AudioClips = new();

        /// <summary>合計メモリ使用量（バイト）</summary>
        public long TotalMemoryBytes;

        /// <summary>テクスチャの合計メモリ（バイト）</summary>
        public long TextureMemoryBytes;

        /// <summary>オーディオの合計メモリ（バイト）</summary>
        public long AudioMemoryBytes;
    }

    /// <summary>
    /// テクスチャアセット情報
    /// </summary>
    [Serializable]
    public class TextureAssetInfo
    {
        /// <summary>インスタンスID</summary>
        public int InstanceId;

        /// <summary>アセット名</summary>
        public string Name = string.Empty;

        /// <summary>推定メモリ使用量（バイト）</summary>
        public long MemoryBytes;

        /// <summary>テクスチャ幅</summary>
        public int Width;

        /// <summary>テクスチャ高さ</summary>
        public int Height;

        /// <summary>テクスチャフォーマット</summary>
        public string Format = string.Empty;

        /// <summary>ミップマップ数</summary>
        public int MipmapCount;
    }

    /// <summary>
    /// オーディオクリップアセット情報
    /// </summary>
    [Serializable]
    public class AudioClipAssetInfo
    {
        /// <summary>インスタンスID</summary>
        public int InstanceId;

        /// <summary>アセット名</summary>
        public string Name = string.Empty;

        /// <summary>推定メモリ使用量（バイト）</summary>
        public long MemoryBytes;

        /// <summary>長さ（秒）</summary>
        public float Length;

        /// <summary>チャンネル数</summary>
        public int Channels;

        /// <summary>サンプルレート</summary>
        public int Frequency;

        /// <summary>ロードタイプ</summary>
        public string LoadType = string.Empty;
    }

    /// <summary>
    /// スナップショット比較結果
    /// </summary>
    [Serializable]
    public class SnapshotComparisonResult
    {
        /// <summary>比較元スナップショットのラベル</summary>
        public string BaseSnapshotLabel = string.Empty;

        /// <summary>比較先スナップショットのラベル</summary>
        public string CompareSnapshotLabel = string.Empty;

        /// <summary>残存テクスチャ（両方に存在）</summary>
        public List<ResidualAssetInfo> ResidualTextures = new();

        /// <summary>残存オーディオクリップ（両方に存在）</summary>
        public List<ResidualAssetInfo> ResidualAudioClips = new();

        /// <summary>新規テクスチャ（比較先のみ）</summary>
        public List<ResidualAssetInfo> NewTextures = new();

        /// <summary>新規オーディオクリップ（比較先のみ）</summary>
        public List<ResidualAssetInfo> NewAudioClips = new();

        /// <summary>解放されたテクスチャ（比較元のみ）</summary>
        public List<ResidualAssetInfo> ReleasedTextures = new();

        /// <summary>解放されたオーディオクリップ（比較元のみ）</summary>
        public List<ResidualAssetInfo> ReleasedAudioClips = new();

        /// <summary>残存アセットの合計メモリ（バイト）</summary>
        public long ResidualMemoryBytes;
    }

    /// <summary>
    /// 残存アセット情報
    /// </summary>
    [Serializable]
    public class ResidualAssetInfo
    {
        /// <summary>インスタンスID</summary>
        public int InstanceId;

        /// <summary>アセット名</summary>
        public string Name = string.Empty;

        /// <summary>推定メモリ使用量（バイト）</summary>
        public long MemoryBytes;

        /// <summary>アセットタイプ</summary>
        public string AssetType = string.Empty;
    }
}
