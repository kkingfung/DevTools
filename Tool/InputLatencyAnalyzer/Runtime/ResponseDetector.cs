#nullable enable
using System;
using System.Collections.Generic;
using UnityEngine;

namespace InputLatencyAnalyzer
{
    /// <summary>
    /// レスポンス検出結果
    /// </summary>
    public struct ResponseDetectionResult
    {
        public bool Detected;
        public double Timestamp;
        public int Frame;
        public ResponseType Type;
        public string Description;
    }

    /// <summary>
    /// レスポンス検出のインターフェース
    /// </summary>
    public interface IResponseDetector
    {
        /// <summary>レスポンスタイプ</summary>
        ResponseType Type { get; }

        /// <summary>検出の有効/無効</summary>
        bool IsEnabled { get; set; }

        /// <summary>検出を開始（基準状態をキャプチャ）</summary>
        void StartDetection();

        /// <summary>レスポンスをチェック</summary>
        ResponseDetectionResult CheckResponse();

        /// <summary>検出をリセット</summary>
        void Reset();
    }

    /// <summary>
    /// アニメーション開始検出
    /// </summary>
    public class AnimationResponseDetector : IResponseDetector
    {
        private readonly Animator _animator;
        private readonly string? _targetStateName;
        private readonly int _layerIndex;

        private int _baseStateHash;
        private float _baseNormalizedTime;
        private bool _isDetecting;

        public ResponseType Type => ResponseType.Animation;
        public bool IsEnabled { get; set; } = true;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="animator">監視対象のAnimator</param>
        /// <param name="targetStateName">検出対象のステート名（nullの場合は任意のステート変更）</param>
        /// <param name="layerIndex">監視するレイヤー</param>
        public AnimationResponseDetector(Animator animator, string? targetStateName = null, int layerIndex = 0)
        {
            _animator = animator;
            _targetStateName = targetStateName;
            _layerIndex = layerIndex;
        }

        public void StartDetection()
        {
            if (_animator == null || !IsEnabled) return;

            var stateInfo = _animator.GetCurrentAnimatorStateInfo(_layerIndex);
            _baseStateHash = stateInfo.fullPathHash;
            _baseNormalizedTime = stateInfo.normalizedTime;
            _isDetecting = true;
        }

        public ResponseDetectionResult CheckResponse()
        {
            var result = new ResponseDetectionResult { Type = Type };

            if (!_isDetecting || _animator == null || !IsEnabled)
            {
                return result;
            }

            var stateInfo = _animator.GetCurrentAnimatorStateInfo(_layerIndex);

            // ステートが変わったかチェック
            bool stateChanged = stateInfo.fullPathHash != _baseStateHash;

            // 特定のステートを監視する場合
            if (_targetStateName != null)
            {
                if (stateInfo.IsName(_targetStateName) && stateChanged)
                {
                    result.Detected = true;
                    result.Timestamp = Time.realtimeSinceStartupAsDouble;
                    result.Frame = Time.frameCount;
                    result.Description = $"Animation state changed to: {_targetStateName}";
                }
            }
            else
            {
                // 任意のステート変更を検出
                if (stateChanged)
                {
                    result.Detected = true;
                    result.Timestamp = Time.realtimeSinceStartupAsDouble;
                    result.Frame = Time.frameCount;
                    result.Description = "Animation state changed";
                }
            }

            return result;
        }

        public void Reset()
        {
            _isDetecting = false;
            _baseStateHash = 0;
            _baseNormalizedTime = 0;
        }
    }

    /// <summary>
    /// オーディオ再生開始検出
    /// </summary>
    public class AudioResponseDetector : IResponseDetector
    {
        private readonly AudioSource[] _audioSources;
        private readonly HashSet<int> _playingClipIds = new();
        private bool _isDetecting;

        public ResponseType Type => ResponseType.Audio;
        public bool IsEnabled { get; set; } = true;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="audioSources">監視対象のAudioSource配列</param>
        public AudioResponseDetector(params AudioSource[] audioSources)
        {
            _audioSources = audioSources;
        }

        /// <summary>
        /// シーン内のすべてのAudioSourceを監視対象にする
        /// </summary>
        public static AudioResponseDetector CreateForAllSources()
        {
            var sources = UnityEngine.Object.FindObjectsByType<AudioSource>(FindObjectsSortMode.None);
            return new AudioResponseDetector(sources);
        }

        public void StartDetection()
        {
            if (!IsEnabled) return;

            _playingClipIds.Clear();

            foreach (var source in _audioSources)
            {
                if (source != null && source.isPlaying && source.clip != null)
                {
                    _playingClipIds.Add(source.clip.GetInstanceID());
                }
            }

            _isDetecting = true;
        }

        public ResponseDetectionResult CheckResponse()
        {
            var result = new ResponseDetectionResult { Type = Type };

            if (!_isDetecting || !IsEnabled)
            {
                return result;
            }

            foreach (var source in _audioSources)
            {
                if (source == null) continue;

                if (source.isPlaying && source.clip != null)
                {
                    int clipId = source.clip.GetInstanceID();
                    if (!_playingClipIds.Contains(clipId))
                    {
                        result.Detected = true;
                        result.Timestamp = Time.realtimeSinceStartupAsDouble;
                        result.Frame = Time.frameCount;
                        result.Description = $"Audio started: {source.clip.name}";
                        return result;
                    }
                }
            }

            return result;
        }

        public void Reset()
        {
            _isDetecting = false;
            _playingClipIds.Clear();
        }
    }

    /// <summary>
    /// 画面変化検出（RenderTextureベース）
    /// </summary>
    public class VisualResponseDetector : IResponseDetector
    {
        private readonly Camera _camera;
        private readonly int _sampleWidth;
        private readonly int _sampleHeight;
        private readonly float _changeThreshold;

        private RenderTexture? _baseRenderTexture;
        private Texture2D? _baseTexture;
        private Texture2D? _currentTexture;
        private Color32[]? _basePixels;
        private bool _isDetecting;

        public ResponseType Type => ResponseType.Visual;
        public bool IsEnabled { get; set; } = true;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="camera">監視対象のカメラ（nullの場合はメインカメラ）</param>
        /// <param name="sampleWidth">サンプリング解像度（幅）</param>
        /// <param name="sampleHeight">サンプリング解像度（高さ）</param>
        /// <param name="changeThreshold">変化閾値（0-1）</param>
        public VisualResponseDetector(
            Camera? camera = null,
            int sampleWidth = 64,
            int sampleHeight = 64,
            float changeThreshold = 0.01f)
        {
            _camera = camera != null ? camera : Camera.main;
            _sampleWidth = sampleWidth;
            _sampleHeight = sampleHeight;
            _changeThreshold = changeThreshold;
        }

        public void StartDetection()
        {
            if (_camera == null || !IsEnabled) return;

            // RenderTextureを作成
            if (_baseRenderTexture == null)
            {
                _baseRenderTexture = new RenderTexture(_sampleWidth, _sampleHeight, 0);
            }

            if (_baseTexture == null)
            {
                _baseTexture = new Texture2D(_sampleWidth, _sampleHeight, TextureFormat.RGB24, false);
            }

            if (_currentTexture == null)
            {
                _currentTexture = new Texture2D(_sampleWidth, _sampleHeight, TextureFormat.RGB24, false);
            }

            // 現在のフレームをキャプチャ
            CaptureFrame(_baseTexture);
            _basePixels = _baseTexture.GetPixels32();
            _isDetecting = true;
        }

        public ResponseDetectionResult CheckResponse()
        {
            var result = new ResponseDetectionResult { Type = Type };

            if (!_isDetecting || _camera == null || !IsEnabled || _basePixels == null)
            {
                return result;
            }

            // 現在のフレームをキャプチャ
            CaptureFrame(_currentTexture!);
            var currentPixels = _currentTexture!.GetPixels32();

            // 差分を計算
            float difference = CalculateDifference(_basePixels, currentPixels);

            if (difference > _changeThreshold)
            {
                result.Detected = true;
                result.Timestamp = Time.realtimeSinceStartupAsDouble;
                result.Frame = Time.frameCount;
                result.Description = $"Visual change detected: {difference:P1}";
            }

            return result;
        }

        private void CaptureFrame(Texture2D targetTexture)
        {
            var prevTarget = _camera.targetTexture;
            _camera.targetTexture = _baseRenderTexture;
            _camera.Render();

            RenderTexture.active = _baseRenderTexture;
            targetTexture.ReadPixels(new Rect(0, 0, _sampleWidth, _sampleHeight), 0, 0);
            targetTexture.Apply();

            _camera.targetTexture = prevTarget;
            RenderTexture.active = null;
        }

        private float CalculateDifference(Color32[] basePixels, Color32[] currentPixels)
        {
            if (basePixels.Length != currentPixels.Length) return 1f;

            long totalDiff = 0;
            for (int i = 0; i < basePixels.Length; i++)
            {
                totalDiff += Math.Abs(basePixels[i].r - currentPixels[i].r);
                totalDiff += Math.Abs(basePixels[i].g - currentPixels[i].g);
                totalDiff += Math.Abs(basePixels[i].b - currentPixels[i].b);
            }

            return totalDiff / (float)(basePixels.Length * 255 * 3);
        }

        public void Reset()
        {
            _isDetecting = false;
            _basePixels = null;
        }

        /// <summary>
        /// リソースを解放
        /// </summary>
        public void Dispose()
        {
            if (_baseRenderTexture != null)
            {
                _baseRenderTexture.Release();
                UnityEngine.Object.Destroy(_baseRenderTexture);
            }
            if (_baseTexture != null) UnityEngine.Object.Destroy(_baseTexture);
            if (_currentTexture != null) UnityEngine.Object.Destroy(_currentTexture);
        }
    }

    /// <summary>
    /// Transform変化検出
    /// </summary>
    public class TransformResponseDetector : IResponseDetector
    {
        private readonly Transform _target;
        private readonly float _positionThreshold;
        private readonly float _rotationThreshold;

        private Vector3 _basePosition;
        private Quaternion _baseRotation;
        private Vector3 _baseScale;
        private bool _isDetecting;

        public ResponseType Type => ResponseType.Visual;
        public bool IsEnabled { get; set; } = true;

        public TransformResponseDetector(
            Transform target,
            float positionThreshold = 0.001f,
            float rotationThreshold = 0.1f)
        {
            _target = target;
            _positionThreshold = positionThreshold;
            _rotationThreshold = rotationThreshold;
        }

        public void StartDetection()
        {
            if (_target == null || !IsEnabled) return;

            _basePosition = _target.position;
            _baseRotation = _target.rotation;
            _baseScale = _target.localScale;
            _isDetecting = true;
        }

        public ResponseDetectionResult CheckResponse()
        {
            var result = new ResponseDetectionResult { Type = Type };

            if (!_isDetecting || _target == null || !IsEnabled)
            {
                return result;
            }

            float posDiff = Vector3.Distance(_target.position, _basePosition);
            float rotDiff = Quaternion.Angle(_target.rotation, _baseRotation);
            float scaleDiff = Vector3.Distance(_target.localScale, _baseScale);

            if (posDiff > _positionThreshold || rotDiff > _rotationThreshold || scaleDiff > _positionThreshold)
            {
                result.Detected = true;
                result.Timestamp = Time.realtimeSinceStartupAsDouble;
                result.Frame = Time.frameCount;
                result.Description = $"Transform changed: pos={posDiff:F3}, rot={rotDiff:F1}°";
            }

            return result;
        }

        public void Reset()
        {
            _isDetecting = false;
        }
    }
}
