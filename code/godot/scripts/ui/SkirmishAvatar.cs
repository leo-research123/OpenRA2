using Godot;

/// <summary>Round slot portrait: commander, computer or an open seat.</summary>
public partial class SkirmishAvatar : Control
{
    private SkirmishSlotKind _kind = SkirmishSlotKind.Open;

    public SkirmishSlotKind Kind
    {
        get => _kind;
        set { _kind = value; QueueRedraw(); }
    }

    public override void _Draw()
    {
        var center = Size / 2;
        var radius = Mathf.Min(Size.X, Size.Y) / 2 - 2;
        switch (_kind)
        {
            case SkirmishSlotKind.Human:
                DrawCircle(center, radius, SkirmishStyle.AccentDark);
                DrawArc(center, radius, 0, Mathf.Tau, 48, SkirmishStyle.Accent, 3, true);
                DrawPerson(center, radius, SkirmishStyle.Accent);
                break;
            case SkirmishSlotKind.Open:
                DrawArc(center, radius - 2, 0, Mathf.Tau, 48, SkirmishStyle.CardBorder, 1, true);
                DrawLine(center - new Vector2(8, 0), center + new Vector2(8, 0), SkirmishStyle.TextDim, 2);
                DrawLine(center - new Vector2(0, 8), center + new Vector2(0, 8), SkirmishStyle.TextDim, 2);
                break;
            default:
                DrawCircle(center, radius, SkirmishStyle.Panel);
                DrawArc(center, radius - 3, 0, Mathf.Tau, 48, SkirmishStyle.CardBorder, 2, true);
                DrawMonitor(center, radius, SkirmishStyle.TextDim);
                break;
        }
    }

    private void DrawPerson(Vector2 center, float radius, Color color)
    {
        DrawCircle(center + new Vector2(0, -radius * 0.2f), radius * 0.24f, color);
        // Shoulders: the upper half of a disc below the head.
        var body = new Vector2[17];
        var origin = center + new Vector2(0, radius * 0.5f);
        for (var i = 0; i <= 16; i++)
        {
            var angle = Mathf.Pi + Mathf.Pi * i / 16;
            body[i] = origin + new Vector2(Mathf.Cos(angle), Mathf.Sin(angle) * 0.85f) * radius * 0.46f;
        }
        DrawColoredPolygon(body, color);
    }

    private void DrawMonitor(Vector2 center, float radius, Color color)
    {
        var screen = new Rect2(center.X - radius * 0.48f, center.Y - radius * 0.38f, radius * 0.96f, radius * 0.6f);
        DrawRect(screen, color, false, 3);
        DrawLine(new Vector2(screen.Position.X + 6, screen.End.Y - 8), new Vector2(screen.End.X - 6, screen.End.Y - 8), color, 2);
        DrawLine(new Vector2(center.X, screen.End.Y), new Vector2(center.X, screen.End.Y + radius * 0.16f), color, 3);
        DrawLine(new Vector2(center.X - radius * 0.26f, screen.End.Y + radius * 0.18f),
            new Vector2(center.X + radius * 0.26f, screen.End.Y + radius * 0.18f), color, 3);
    }
}
