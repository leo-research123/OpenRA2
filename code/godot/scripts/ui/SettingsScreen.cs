using Godot;
using System.Collections.Generic;

/// <summary>Edits a draft; applying persists it, returning discards unapplied changes.</summary>
public partial class SettingsScreen : Control
{
    [Signal] public delegate void BackRequestedEventHandler();

    private DisplayService _display = null!;
    private List<Vector2I> _resolutions = new();
    public OptionButton ResolutionOption { get; private set; } = null!;
    public OptionButton ModeOption { get; private set; } = null!;
    public Label Status { get; private set; } = null!;

    public override void _Ready()
    {
        ResolutionOption = GetNode<OptionButton>("%ResolutionOption");
        ModeOption = GetNode<OptionButton>("%ModeOption");
        Status = GetNode<Label>("%Status");
    }

    public void Configure(DisplayService display)
    {
        _display = display;
        ModeOption.Clear();
        ModeOption.AddItem("窗口化", 0);
        ModeOption.AddItem("全屏窗口化", 1);
        ModeOption.AddItem("全屏", 2);
    }

    public void Present()
    {
        _resolutions = _display.Resolutions();
        ResolutionOption.Clear();
        foreach (var resolution in _resolutions)
            ResolutionOption.AddItem($"{resolution.X} × {resolution.Y}");
        ResolutionOption.Select(_resolutions.IndexOf(_display.Config.Resolution));
        ModeOption.Select((int)_display.Config.WindowMode);
        Status.Text = "选择后点击应用；按 Esc 返回主菜单。";
        ResolutionOption.GrabFocus();
    }

    public void OnApplyPressed()
    {
        var error = _display.ApplySelection(
            _resolutions[ResolutionOption.Selected], ModeOption.GetSelectedId());
        Status.Text = error != Error.Ok
            ? $"设置已应用，但保存失败（{error}）。"
            : $"已应用并保存：{_display.Config.Resolution.X} × {_display.Config.Resolution.Y} · " +
              ModeOption.GetItemText(ModeOption.Selected);
    }

    public void OnBackPressed() => EmitSignal(SignalName.BackRequested);
}
