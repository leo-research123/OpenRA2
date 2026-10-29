using Godot;
using System;
using System.Collections.Generic;

/// <summary>Map thumbnail with the numbered multiplayer start markers (waypoints 0-7).</summary>
public partial class SkirmishMapPreview : Control
{
    public event Action<int>? StartClicked;

    private Texture2D? _texture;
    private readonly List<Vector2> _starts = new();
    private readonly Dictionary<int, (string Text, Color? Ring)> _labels = new();
    private string _placeholder = "";
    private const float MarkerRadius = 13;

    public void SetMap(Texture2D? texture, IEnumerable<Vector2> starts, string placeholder)
    {
        _texture = texture;
        _starts.Clear();
        _starts.AddRange(starts);
        _placeholder = placeholder;
        QueueRedraw();
    }

    public void SetLabels(IDictionary<int, (string Text, Color? Ring)> labels)
    {
        _labels.Clear();
        foreach (var pair in labels) _labels[pair.Key] = pair.Value;
        QueueRedraw();
    }

    private Rect2 ImageRect()
    {
        if (_texture == null) return new Rect2(Vector2.Zero, Size);
        var source = _texture.GetSize();
        var scale = Mathf.Min(Size.X / source.X, Size.Y / source.Y);
        var size = source * scale;
        return new Rect2((Size - size) / 2, size);
    }

    private Vector2 MarkerPosition(Rect2 area, Vector2 fraction) =>
        area.Position + new Vector2(Mathf.Clamp(fraction.X, 0.03f, 0.97f), Mathf.Clamp(fraction.Y, 0.04f, 0.96f)) * area.Size;

    public override void _Draw()
    {
        DrawRect(new Rect2(Vector2.Zero, Size), SkirmishStyle.PanelDark);
        var area = ImageRect();
        var font = GetThemeDefaultFont();
        if (_texture != null) DrawTextureRect(_texture, area, false);
        else if (_placeholder.Length > 0)
            DrawString(font, new Vector2(0, Size.Y / 2), _placeholder, HorizontalAlignment.Center, Size.X, 20,
                SkirmishStyle.TextDim);
        for (var i = 0; i < _starts.Count; i++)
        {
            var center = MarkerPosition(area, _starts[i]);
            var labeled = _labels.TryGetValue(i, out var label);
            DrawCircle(center + new Vector2(1, 2), MarkerRadius, new Color(0, 0, 0, 0.45f));
            DrawCircle(center, MarkerRadius, SkirmishStyle.Marker);
            DrawArc(center, MarkerRadius, 0, Mathf.Tau, 32, labeled && label.Ring is { } ring ? ring : new Color(0.25f, 0.25f, 0.25f),
                labeled ? 3 : 2, true);
            var number = (i + 1).ToString();
            DrawString(font, center + new Vector2(-MarkerRadius, 7), number, HorizontalAlignment.Center, MarkerRadius * 2, 19,
                Colors.White);
            if (!labeled || label.Text.Length == 0) continue;
            var textSize = font.GetStringSize(label.Text, HorizontalAlignment.Left, -1, 16);
            var box = new Rect2(center + new Vector2(MarkerRadius + 4, -14), new Vector2(textSize.X + 16, 28));
            if (box.End.X > Size.X) box.Position = new Vector2(center.X - MarkerRadius - 4 - box.Size.X, box.Position.Y);
            DrawRect(box, new Color(0.08f, 0.09f, 0.12f, 0.88f));
            DrawRect(new Rect2(box.Position, new Vector2(3, box.Size.Y)), label.Ring ?? Colors.White);
            DrawString(font, box.Position + new Vector2(9, 20), label.Text, HorizontalAlignment.Left, -1, 16, Colors.White);
        }
    }

    public override void _GuiInput(InputEvent @event)
    {
        if (@event is not InputEventMouseButton { Pressed: true, ButtonIndex: MouseButton.Left } click) return;
        var area = ImageRect();
        for (var i = 0; i < _starts.Count; i++)
        {
            if (click.Position.DistanceTo(MarkerPosition(area, _starts[i])) > MarkerRadius + 3) continue;
            StartClicked?.Invoke(i);
            AcceptEvent();
            return;
        }
    }

    public override void _Notification(int what)
    {
        if (what == NotificationResized) QueueRedraw();
    }
}
