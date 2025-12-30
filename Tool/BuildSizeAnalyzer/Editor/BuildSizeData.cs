#nullable enable
using System;
using System.Collections.Generic;

namespace BuildSizeAnalyzer
{
    /// <summary>
    /// ビルドサイズレポート全体のデータ構造
    /// </summary>
    [Serializable]
    public class BuildSizeReport
    {
        /// <summary>レポートのバージョン（将来の互換性のため）</summary>
        public int Version = 1;

        /// <summary>ビルド日時（ISO 8601形式）</summary>
        public string BuildDateTime = string.Empty;

        /// <summary>Unityバージョン</summary>
        public string UnityVersion = string.Empty;

        /// <summary>ビルドターゲットプラットフォーム</summary>
        public string BuildTarget = string.Empty;

        /// <summary>ビルド出力パス</summary>
        public string OutputPath = string.Empty;

        /// <summary>合計ビルドサイズ（バイト）</summary>
        public long TotalSizeBytes;

        /// <summary>ビルド結果（Success, Failed, Cancelled）</summary>
        public string BuildResult = string.Empty;

        /// <summary>アセットカテゴリ別サイズ情報</summary>
        public List<CategorySizeInfo> Categories = new();

        /// <summary>個別アセットサイズ情報</summary>
        public List<AssetSizeInfo> Assets = new();

        /// <summary>シーン別サイズ情報</summary>
        public List<SceneSizeInfo> Scenes = new();
    }

    /// <summary>
    /// カテゴリ別サイズ情報（Textures, Meshes, Audio等）
    /// </summary>
    [Serializable]
    public class CategorySizeInfo
    {
        /// <summary>カテゴリ名</summary>
        public string Category = string.Empty;

        /// <summary>サイズ（バイト）</summary>
        public long SizeBytes;

        /// <summary>全体に占める割合（0-100）</summary>
        public float Percentage;
    }

    /// <summary>
    /// 個別アセットのサイズ情報
    /// </summary>
    [Serializable]
    public class AssetSizeInfo
    {
        /// <summary>アセットパス</summary>
        public string Path = string.Empty;

        /// <summary>アセットタイプ</summary>
        public string Type = string.Empty;

        /// <summary>サイズ（バイト）</summary>
        public long SizeBytes;

        /// <summary>圧縮後サイズ（バイト、取得可能な場合）</summary>
        public long CompressedSizeBytes;
    }

    /// <summary>
    /// シーン別サイズ情報
    /// </summary>
    [Serializable]
    public class SceneSizeInfo
    {
        /// <summary>シーンパス</summary>
        public string Path = string.Empty;

        /// <summary>サイズ（バイト）</summary>
        public long SizeBytes;
    }
}
