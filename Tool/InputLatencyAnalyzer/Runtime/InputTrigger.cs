#nullable enable
using System;
using UnityEngine;

namespace InputLatencyAnalyzer
{
    /// <summary>
    /// 入力トリガーのインターフェース
    /// </summary>
    public interface IInputTrigger
    {
        /// <summary>入力アクション名</summary>
        string ActionName { get; }

        /// <summary>入力が検出されたときに発火</summary>
        event Action<double, int>? OnInputDetected;

        /// <summary>入力検出の有効/無効</summary>
        bool IsEnabled { get; set; }

        /// <summary>入力をチェック（毎フレーム呼び出し）</summary>
        void CheckInput();
    }

    /// <summary>
    /// キーボードキー入力トリガー
    /// </summary>
    public class KeyInputTrigger : IInputTrigger
    {
        private readonly KeyCode _keyCode;

        public string ActionName { get; }
        public event Action<double, int>? OnInputDetected;
        public bool IsEnabled { get; set; } = true;

        public KeyInputTrigger(KeyCode keyCode, string? actionName = null)
        {
            _keyCode = keyCode;
            ActionName = actionName ?? $"Key_{keyCode}";
        }

        public void CheckInput()
        {
            if (!IsEnabled) return;

            if (Input.GetKeyDown(_keyCode))
            {
                OnInputDetected?.Invoke(Time.realtimeSinceStartupAsDouble, Time.frameCount);
            }
        }
    }

    /// <summary>
    /// マウスボタン入力トリガー
    /// </summary>
    public class MouseButtonTrigger : IInputTrigger
    {
        private readonly int _button;

        public string ActionName { get; }
        public event Action<double, int>? OnInputDetected;
        public bool IsEnabled { get; set; } = true;

        public MouseButtonTrigger(int button, string? actionName = null)
        {
            _button = button;
            ActionName = actionName ?? $"Mouse_{button}";
        }

        public void CheckInput()
        {
            if (!IsEnabled) return;

            if (Input.GetMouseButtonDown(_button))
            {
                OnInputDetected?.Invoke(Time.realtimeSinceStartupAsDouble, Time.frameCount);
            }
        }
    }

    /// <summary>
    /// Inputシステムのボタン/軸入力トリガー
    /// </summary>
    public class ButtonInputTrigger : IInputTrigger
    {
        private readonly string _buttonName;
        private bool _wasPressed;

        public string ActionName { get; }
        public event Action<double, int>? OnInputDetected;
        public bool IsEnabled { get; set; } = true;

        public ButtonInputTrigger(string buttonName, string? actionName = null)
        {
            _buttonName = buttonName;
            ActionName = actionName ?? buttonName;
        }

        public void CheckInput()
        {
            if (!IsEnabled) return;

            try
            {
                bool isPressed = Input.GetButtonDown(_buttonName);
                if (isPressed && !_wasPressed)
                {
                    OnInputDetected?.Invoke(Time.realtimeSinceStartupAsDouble, Time.frameCount);
                }
                _wasPressed = isPressed;
            }
            catch
            {
                // ボタンが存在しない場合は無視
            }
        }
    }

    /// <summary>
    /// 任意の条件による入力トリガー
    /// </summary>
    public class CustomInputTrigger : IInputTrigger
    {
        private readonly Func<bool> _condition;

        public string ActionName { get; }
        public event Action<double, int>? OnInputDetected;
        public bool IsEnabled { get; set; } = true;

        public CustomInputTrigger(Func<bool> condition, string actionName)
        {
            _condition = condition;
            ActionName = actionName;
        }

        public void CheckInput()
        {
            if (!IsEnabled) return;

            if (_condition())
            {
                OnInputDetected?.Invoke(Time.realtimeSinceStartupAsDouble, Time.frameCount);
            }
        }
    }

    /// <summary>
    /// 手動トリガー（テスト用）
    /// </summary>
    public class ManualInputTrigger : IInputTrigger
    {
        public string ActionName { get; }
        public event Action<double, int>? OnInputDetected;
        public bool IsEnabled { get; set; } = true;

        public ManualInputTrigger(string actionName = "ManualTrigger")
        {
            ActionName = actionName;
        }

        public void CheckInput() { }

        /// <summary>
        /// 手動で入力をトリガー
        /// </summary>
        public void Trigger()
        {
            if (!IsEnabled) return;
            OnInputDetected?.Invoke(Time.realtimeSinceStartupAsDouble, Time.frameCount);
        }
    }
}
