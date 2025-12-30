#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;

namespace UnusedAssetDetector
{
    /// <summary>
    /// アセットの参照状態
    /// </summary>
    public enum ReferenceStatus
    {
        /// <summary>未スキャン</summary>
        Unknown,
        /// <summary>参照されている</summary>
        Referenced,
        /// <summary>未参照（未使用）</summary>
        Unreferenced,
        /// <summary>ルートアセット（ビルドシーン、Resources等）</summary>
        Root
    }

    /// <summary>
    /// アセット情報
    /// </summary>
    public class AssetInfo
    {
        public string Guid { get; }
        public string Path { get; }
        public string AssetType { get; set; } = string.Empty;
        public ReferenceStatus Status { get; set; } = ReferenceStatus.Unknown;
        public long FileSize { get; set; }

        /// <summary>このアセットを参照しているGUID</summary>
        public HashSet<string> ReferencedBy { get; } = new();

        /// <summary>このアセットが参照しているGUID</summary>
        public HashSet<string> References { get; } = new();

        public AssetInfo(string guid, string path)
        {
            Guid = guid;
            Path = path;
        }
    }

    /// <summary>
    /// アセット参照マップ - GUIDと参照状態を管理
    /// </summary>
    public class AssetReferenceMap
    {
        private readonly Dictionary<string, AssetInfo> _byGuid = new();
        private readonly Dictionary<string, string> _guidByPath = new();

        public int Count => _byGuid.Count;
        public IEnumerable<AssetInfo> All => _byGuid.Values;
        public IEnumerable<AssetInfo> Unreferenced => _byGuid.Values.Where(a => a.Status == ReferenceStatus.Unreferenced);
        public IEnumerable<AssetInfo> Referenced => _byGuid.Values.Where(a => a.Status == ReferenceStatus.Referenced || a.Status == ReferenceStatus.Root);
        public IEnumerable<AssetInfo> Roots => _byGuid.Values.Where(a => a.Status == ReferenceStatus.Root);

        /// <summary>
        /// アセットを登録
        /// </summary>
        public AssetInfo Register(string guid, string path)
        {
            if (_byGuid.TryGetValue(guid, out var existing))
                return existing;

            var info = new AssetInfo(guid, path);
            _byGuid[guid] = info;
            _guidByPath[path] = guid;
            return info;
        }

        public AssetInfo? GetByGuid(string guid) => _byGuid.TryGetValue(guid, out var info) ? info : null;
        public AssetInfo? GetByPath(string path) => _guidByPath.TryGetValue(path, out var guid) ? GetByGuid(guid) : null;
        public bool Contains(string guid) => _byGuid.ContainsKey(guid);

        /// <summary>
        /// 参照関係を追加（sourceがtargetを参照）
        /// </summary>
        public void AddReference(string sourceGuid, string targetGuid)
        {
            if (string.IsNullOrEmpty(sourceGuid) || string.IsNullOrEmpty(targetGuid) || sourceGuid == targetGuid)
                return;

            var source = GetByGuid(sourceGuid);
            var target = GetByGuid(targetGuid);

            source?.References.Add(targetGuid);
            target?.ReferencedBy.Add(sourceGuid);
        }

        /// <summary>
        /// ルートアセットとしてマーク
        /// </summary>
        public void MarkAsRoot(string guid)
        {
            if (GetByGuid(guid) is { } asset)
                asset.Status = ReferenceStatus.Root;
        }

        /// <summary>
        /// ルートから参照を伝播して参照状態を確定
        /// </summary>
        public void PropagateReferences()
        {
            // ルート以外を未参照にリセット
            foreach (var asset in _byGuid.Values)
            {
                if (asset.Status != ReferenceStatus.Root)
                    asset.Status = ReferenceStatus.Unreferenced;
            }

            // BFSでルートから参照を伝播
            var visited = new HashSet<string>();
            var queue = new Queue<string>();

            foreach (var root in Roots)
            {
                queue.Enqueue(root.Guid);
                visited.Add(root.Guid);
            }

            while (queue.Count > 0)
            {
                var current = GetByGuid(queue.Dequeue());
                if (current == null) continue;

                foreach (var refGuid in current.References)
                {
                    if (visited.Contains(refGuid)) continue;
                    visited.Add(refGuid);

                    if (GetByGuid(refGuid) is { } refAsset)
                    {
                        if (refAsset.Status != ReferenceStatus.Root)
                            refAsset.Status = ReferenceStatus.Referenced;
                        queue.Enqueue(refGuid);
                    }
                }
            }
        }

        public void Clear()
        {
            _byGuid.Clear();
            _guidByPath.Clear();
        }

        /// <summary>
        /// 統計情報を取得
        /// </summary>
        public Statistics GetStatistics()
        {
            var unreferenced = _byGuid.Values.Where(a => a.Status == ReferenceStatus.Unreferenced).ToList();
            return new Statistics
            {
                TotalCount = _byGuid.Count,
                ReferencedCount = _byGuid.Values.Count(a => a.Status == ReferenceStatus.Referenced),
                UnreferencedCount = unreferenced.Count,
                RootCount = _byGuid.Values.Count(a => a.Status == ReferenceStatus.Root),
                UnreferencedSize = unreferenced.Sum(a => a.FileSize)
            };
        }

        public class Statistics
        {
            public int TotalCount;
            public int ReferencedCount;
            public int UnreferencedCount;
            public int RootCount;
            public long UnreferencedSize;

            public string FormattedSize
            {
                get
                {
                    string[] units = { "B", "KB", "MB", "GB" };
                    double size = UnreferencedSize;
                    int i = 0;
                    while (size >= 1024 && i < units.Length - 1) { size /= 1024; i++; }
                    return $"{size:0.##} {units[i]}";
                }
            }
        }
    }
}
