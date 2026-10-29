using Godot;
using System;
using System.Collections.Generic;

/// <summary>One player seat of the skirmish lobby. The lobby owns the rules between seats.</summary>
public partial class SkirmishSlotCard : PanelContainer
{
    public event Action? Changed;

    public SkirmishSlot Slot { get; }
    public string PlayerName { get; set; } = "";

    // OptionButton treats id -1 as "use the item index", so slot values are offset.
    private const int IdOffset = 10;
    private static int Id(int value) => value + IdOffset;
    private static int Value(OptionButton option, long index) => option.GetItemId((int)index) - IdOffset;

    private readonly SkirmishAvatar _avatar = new();
    private readonly OptionButton _start = new();
    private readonly Label _name = new();
    private readonly OptionButton _kind = new();
    private readonly OptionButton _color = new();
    private readonly OptionButton _country = new();
    private readonly OptionButton _team = new();
    private readonly Control _details = new VBoxContainer();
    private bool _updating;

    public SkirmishSlotCard() : this(new SkirmishSlot()) { }

    public SkirmishSlotCard(SkirmishSlot slot)
    {
        Slot = slot;
        SizeFlagsHorizontal = SizeFlags.ExpandFill;
        SizeFlagsVertical = SizeFlags.ExpandFill;
        var content = new VBoxContainer();
        content.AddThemeConstantOverride("separation", 0);
        AddChild(content);

        var top = new HBoxContainer();
        content.AddChild(top);
        SkirmishStyle.Flat(_start, 14);
        _start.AddThemeStyleboxOverride("normal", SkirmishStyle.Box(SkirmishStyle.PanelDark, 4));
        _start.CustomMinimumSize = new Vector2(64, 26);
        _start.TooltipText = "出生位置";
        _start.ItemSelected += index => Commit(() => Slot.Start = Value(_start, index));
        top.AddChild(_start);

        var center = new VBoxContainer { Alignment = BoxContainer.AlignmentMode.Center, SizeFlagsVertical = SizeFlags.ExpandFill };
        center.AddThemeConstantOverride("separation", 4);
        content.AddChild(center);
        _avatar.CustomMinimumSize = new Vector2(46, 46);
        _avatar.SizeFlagsHorizontal = SizeFlags.ShrinkCenter;
        _avatar.MouseFilter = MouseFilterEnum.Ignore;
        center.AddChild(_avatar);

        _name.HorizontalAlignment = HorizontalAlignment.Center;
        _name.AddThemeFontSizeOverride("font_size", 17);
        _name.AddThemeColorOverride("font_color", SkirmishStyle.PlayerName);
        _name.ClipText = true;
        center.AddChild(_name);

        SkirmishStyle.Flat(_kind, 17);
        _kind.Alignment = HorizontalAlignment.Center;
        _kind.SizeFlagsHorizontal = SizeFlags.ShrinkCenter;
        foreach (var kind in new[] { SkirmishSlotKind.Open, SkirmishSlotKind.EasyAI, SkirmishSlotKind.NormalAI, SkirmishSlotKind.HardAI })
            _kind.AddItem(SkirmishSetup.KindLabel(kind), Id((int)kind));
        _kind.ItemSelected += index => Commit(() => Slot.Kind = (SkirmishSlotKind)Value(_kind, index));
        center.AddChild(_kind);

        center.AddChild(_details);
        _details.AddThemeConstantOverride("separation", 2);
        SkirmishStyle.Flat(_color, 15);
        _color.Alignment = HorizontalAlignment.Center;
        _color.SizeFlagsHorizontal = SizeFlags.ShrinkCenter;
        _color.AddItem("随机色", Id(SkirmishSlot.Random));
        for (var i = 0; i < SkirmishSetup.Colors.Length; i++)
            _color.AddIconItem(SkirmishStyle.Swatch(SkirmishSetup.Colors[i].Value), SkirmishSetup.Colors[i].Name, Id(i));
        _color.ItemSelected += index => Commit(() => Slot.Color = Value(_color, index));
        _details.AddChild(_color);

        var row = new HBoxContainer { Alignment = BoxContainer.AlignmentMode.Center };
        row.AddThemeConstantOverride("separation", 0);
        _details.AddChild(row);
        SkirmishStyle.Flat(_country, 15);
        _country.FitToLongestItem = false;
        _country.ClipText = true;
        _country.CustomMinimumSize = new Vector2(94, 0);
        _country.ItemSelected += index => Commit(() => Slot.Country = Value(_country, index));
        row.AddChild(_country);
        SkirmishStyle.Flat(_team, 15);
        _team.AddItem("单人", Id(SkirmishSlot.NoTeam));
        for (var i = 0; i < SkirmishSetup.Teams.Length; i++) _team.AddItem(SkirmishSetup.Teams[i], Id(i));
        _team.ItemSelected += index => Commit(() => Slot.Team = Value(_team, index));
        row.AddChild(_team);
    }

    public void SetCountries(IReadOnlyList<(int Index, string Name)> countries)
    {
        _country.Clear();
        _country.AddItem("随机国家", Id(SkirmishSlot.Random));
        foreach (var (index, name) in countries) _country.AddItem(name, Id(index));
        if (_country.GetItemIndex(Id(Slot.Country)) < 0) Slot.Country = SkirmishSlot.Random;
    }

    /// <summary>Rebuild the visible state from the slot and the seats other cards hold.</summary>
    public void Refresh(int startCount, ISet<int> takenStarts, ISet<int> takenColors)
    {
        _updating = true;
        var occupied = Slot.IsOccupied;
        var human = Slot.Kind == SkirmishSlotKind.Human;
        _avatar.Kind = Slot.Kind;
        _name.Visible = human;
        _name.Text = PlayerName;
        _kind.Visible = !human;
        _kind.Select(Math.Max(0, _kind.GetItemIndex(Id((int)Slot.Kind))));
        var kindColor = occupied ? SkirmishStyle.Text : SkirmishStyle.TextDim;
        _kind.AddThemeColorOverride("font_color", kindColor);
        _kind.AddThemeColorOverride("font_focus_color", kindColor);
        _details.Visible = occupied;

        _start.Clear();
        _start.AddItem("随机", Id(SkirmishSlot.Random));
        for (var i = 0; i < startCount; i++)
        {
            _start.AddItem((i + 1).ToString(), Id(i));
            _start.SetItemDisabled(i + 1, takenStarts.Contains(i) && Slot.Start != i);
        }
        _start.Select(Math.Max(0, _start.GetItemIndex(Id(Slot.Start))));
        _start.Disabled = !occupied;

        for (var i = 0; i < SkirmishSetup.Colors.Length; i++)
            _color.SetItemDisabled(_color.GetItemIndex(Id(i)), takenColors.Contains(i) && Slot.Color != i);
        _color.Select(Math.Max(0, _color.GetItemIndex(Id(Slot.Color))));
        _country.Select(Math.Max(0, _country.GetItemIndex(Id(Slot.Country))));
        _team.Select(Math.Max(0, _team.GetItemIndex(Id(Slot.Team))));

        var style = SkirmishStyle.Box(occupied ? SkirmishStyle.Card : SkirmishStyle.CardEmpty, 8,
            human ? SkirmishStyle.Accent : SkirmishStyle.CardBorder, 1);
        style.SetContentMarginAll(8);
        if (human) style.BorderWidthTop = 2;
        AddThemeStyleboxOverride("panel", style);
        _updating = false;
    }

    private void Commit(Action change)
    {
        if (_updating) return;
        change();
        Changed?.Invoke();
    }
}
