using Godot;

/// <summary>Lobby surfaces use the main menu's charcoal and warm orange palette.</summary>
public static class SkirmishStyle
{
    public static readonly Color Background = new(0.045f, 0.06f, 0.08f);
    public static readonly Color Card = new(0.12f, 0.15f, 0.19f);
    public static readonly Color CardEmpty = new(0.065f, 0.08f, 0.10f);
    public static readonly Color CardBorder = new(0.28f, 0.32f, 0.37f);
    public static readonly Color Accent = new(0.95f, 0.58f, 0.31f);
    public static readonly Color AccentDark = new(0.38f, 0.23f, 0.15f);
    public static readonly Color Panel = new(0.08f, 0.10f, 0.13f);
    public static readonly Color PanelDark = new(0.055f, 0.07f, 0.09f);
    public static readonly Color Text = new(0.93f, 0.92f, 0.88f);
    public static readonly Color TextDim = new(0.64f, 0.67f, 0.70f);
    public static readonly Color PlayerName = Accent;
    public static readonly Color Marker = new(0.62f, 0.29f, 0.12f);

    public static StyleBoxFlat Box(Color background, int radius = 0, Color? border = null, int borderWidth = 0)
    {
        var box = new StyleBoxFlat { BgColor = background };
        box.SetCornerRadiusAll(radius);
        if (border is { } color)
        {
            box.BorderColor = color;
            box.SetBorderWidthAll(borderWidth);
        }
        return box;
    }

    // Drop-downs inside cards read as plain text with an arrow, as in the original lobby.
    public static void Flat(Button button, int fontSize, Color? color = null)
    {
        var hover = Box(new Color(1f, 1f, 1f, 0.07f), 4);
        var empty = new StyleBoxEmpty();
        button.AddThemeStyleboxOverride("normal", empty);
        button.AddThemeStyleboxOverride("hover", hover);
        button.AddThemeStyleboxOverride("pressed", hover);
        button.AddThemeStyleboxOverride("focus", Box(Colors.Transparent, 4, Accent, 1));
        button.AddThemeStyleboxOverride("disabled", empty);
        button.AddThemeFontSizeOverride("font_size", fontSize);
        var text = color ?? Text;
        button.AddThemeColorOverride("font_color", text);
        button.AddThemeColorOverride("font_hover_color", Colors.White);
        button.AddThemeColorOverride("font_pressed_color", Colors.White);
        button.AddThemeColorOverride("font_focus_color", text);
        button.AddThemeColorOverride("font_disabled_color", TextDim);
        if (button is OptionButton option)
        {
            var popup = option.GetPopup();
            popup.AddThemeStyleboxOverride("panel", Box(Panel, 6, CardBorder, 1));
            popup.AddThemeStyleboxOverride("hover", Box(AccentDark, 4));
            popup.AddThemeColorOverride("font_color", Text);
            popup.AddThemeColorOverride("font_hover_color", Accent);
        }
    }

    public static ImageTexture Swatch(Color color, int width = 30, int height = 16)
    {
        using var image = Image.CreateEmpty(width, height, false, Image.Format.Rgba8);
        image.Fill(color);
        return ImageTexture.CreateFromImage(image);
    }
}
