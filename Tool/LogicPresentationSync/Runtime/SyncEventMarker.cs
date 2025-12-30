#nullable enable
using System;
using System.Diagnostics;
using UnityEngine;
using Debug = UnityEngine.Debug;
using Object = UnityEngine.Object;

namespace LogicPresentationSync
{
    /// <summary>
    /// 同期イベントをマークするための静的API
    /// ゲームコード内から簡単にイベントを記録できる
    /// </summary>
    public static class SyncEventMarker
    {
        /// <summary>イベント記録の有効/無効</summary>
        public static bool IsEnabled { get; set; } = true;

        /// <summary>ロジックイベント発生時</summary>
        public static event Action<LogicEvent>? OnLogicEvent;

        /// <summary>プレゼンテーションイベント発生時</summary>
        public static event Action<PresentationEvent>? OnPresentationEvent;

        private static int _eventIdCounter;

        #region ロジックイベント

        /// <summary>
        /// ロジックイベントをマーク
        /// </summary>
        /// <param name="tag">ペアリング用タグ（対応するプレゼンテーションイベントと同じタグを使用）</param>
        /// <param name="eventName">イベント名</param>
        /// <param name="source">発生元オブジェクト（オプション）</param>
        /// <param name="details">追加情報（オプション）</param>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkLogic(string tag, string eventName, Object? source = null, string? details = null)
        {
            if (!IsEnabled) return;

            var logicEvent = new LogicEvent
            {
                EventId = ++_eventIdCounter,
                Tag = tag,
                EventName = eventName,
                Timestamp = Time.realtimeSinceStartupAsDouble,
                Frame = Time.frameCount,
                SourceObject = source != null ? source.name : string.Empty,
                Details = details ?? string.Empty
            };

            OnLogicEvent?.Invoke(logicEvent);
        }

        /// <summary>
        /// ヒット確認イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkHitConfirm(string tag, Object? source = null, string? details = null)
        {
            MarkLogic(tag, "HitConfirm", source, details);
        }

        /// <summary>
        /// ダメージ適用イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkDamageApplied(string tag, float damage, Object? source = null)
        {
            MarkLogic(tag, "DamageApplied", source, $"Damage: {damage}");
        }

        /// <summary>
        /// ステート変更イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkStateChange(string tag, string fromState, string toState, Object? source = null)
        {
            MarkLogic(tag, "StateChange", source, $"{fromState} -> {toState}");
        }

        /// <summary>
        /// アクション実行イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkActionExecute(string tag, string actionName, Object? source = null)
        {
            MarkLogic(tag, "ActionExecute", source, actionName);
        }

        /// <summary>
        /// アイテム使用イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkItemUsed(string tag, string itemName, Object? source = null)
        {
            MarkLogic(tag, "ItemUsed", source, itemName);
        }

        /// <summary>
        /// トリガー発動イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkTrigger(string tag, string triggerName, Object? source = null)
        {
            MarkLogic(tag, "Trigger", source, triggerName);
        }

        #endregion

        #region プレゼンテーションイベント

        /// <summary>
        /// プレゼンテーションイベントをマーク
        /// </summary>
        /// <param name="tag">ペアリング用タグ</param>
        /// <param name="type">プレゼンテーションタイプ</param>
        /// <param name="eventName">イベント名</param>
        /// <param name="source">発生元オブジェクト（オプション）</param>
        /// <param name="details">追加情報（オプション）</param>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkPresentation(string tag, PresentationType type, string eventName, Object? source = null, string? details = null)
        {
            if (!IsEnabled) return;

            var presentationEvent = new PresentationEvent
            {
                EventId = ++_eventIdCounter,
                Tag = tag,
                Type = type,
                EventName = eventName,
                Timestamp = Time.realtimeSinceStartupAsDouble,
                Frame = Time.frameCount,
                SourceObject = source != null ? source.name : string.Empty,
                Details = details ?? string.Empty
            };

            OnPresentationEvent?.Invoke(presentationEvent);
        }

        /// <summary>
        /// アニメーションイベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkAnimation(string tag, string animationName, Object? source = null, string? details = null)
        {
            MarkPresentation(tag, PresentationType.Animation, animationName, source, details);
        }

        /// <summary>
        /// アニメーションイベント（Animatorから）
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkAnimationEvent(string tag, AnimationEvent animEvent, Object? source = null)
        {
            MarkPresentation(tag, PresentationType.Animation, animEvent.functionName, source, $"Time: {animEvent.time}");
        }

        /// <summary>
        /// VFX/パーティクル生成イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkVFX(string tag, string vfxName, Object? source = null, string? details = null)
        {
            MarkPresentation(tag, PresentationType.VFX, vfxName, source, details);
        }

        /// <summary>
        /// VFX/パーティクル生成イベント（ParticleSystemから）
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkVFXSpawn(string tag, ParticleSystem? particleSystem)
        {
            if (particleSystem == null) return;
            MarkPresentation(tag, PresentationType.VFX, particleSystem.name, particleSystem, $"Particles: {particleSystem.main.maxParticles}");
        }

        /// <summary>
        /// オーディオ再生イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkAudio(string tag, string audioName, Object? source = null, string? details = null)
        {
            MarkPresentation(tag, PresentationType.Audio, audioName, source, details);
        }

        /// <summary>
        /// オーディオ再生イベント（AudioSourceから）
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkAudioPlay(string tag, AudioSource? audioSource)
        {
            if (audioSource == null || audioSource.clip == null) return;
            MarkPresentation(tag, PresentationType.Audio, audioSource.clip.name, audioSource, $"Volume: {audioSource.volume}");
        }

        /// <summary>
        /// UI更新イベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkUI(string tag, string uiElementName, Object? source = null, string? details = null)
        {
            MarkPresentation(tag, PresentationType.UI, uiElementName, source, details);
        }

        /// <summary>
        /// カメラエフェクトイベント
        /// </summary>
        [Conditional("UNITY_EDITOR"), Conditional("DEVELOPMENT_BUILD")]
        public static void MarkCameraEffect(string tag, string effectName, Object? source = null, string? details = null)
        {
            MarkPresentation(tag, PresentationType.Camera, effectName, source, details);
        }

        #endregion

        #region ユーティリティ

        /// <summary>
        /// 一意のタグを生成
        /// </summary>
        public static string GenerateTag(string prefix = "Event")
        {
            return $"{prefix}_{Time.frameCount}_{_eventIdCounter}";
        }

        /// <summary>
        /// オブジェクトベースのタグを生成
        /// </summary>
        public static string GenerateTag(Object source, string eventType)
        {
            return $"{source.GetInstanceID()}_{eventType}_{Time.frameCount}";
        }

        /// <summary>
        /// イベントIDカウンターをリセット
        /// </summary>
        public static void ResetCounter()
        {
            _eventIdCounter = 0;
        }

        #endregion
    }
}
