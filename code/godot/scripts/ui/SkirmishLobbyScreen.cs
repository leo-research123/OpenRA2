using Godot;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

/// <summary>
/// Skirmish lobby: seats, map, and the options of the original skirmish dialog.
/// </summary>
public partial class SkirmishLobbyScreen : Control
{
    public event Action? BackRequested;
    public event Action<string, Godot.Collections.Dictionary>? StartRequested;
    public bool Scanning => _scan != null;
    public Button StartButton { get; private set; } = null!;
    public string SelectedMap => _map?.Filename ?? "";
    public int StartCount => _starts.Count;
    public IReadOnlyList<SkirmishSlot> Slots => _slots;
    public IReadOnlyList<MapCatalog.Entry> Maps => _maps;
    public IReadOnlyList<Vector2> Starts => _starts;
    public IReadOnlyList<(int Index, string Name)> Countries => _countries;
    public Vector2I PreviewSize { get; private set; }
    public OptionButton Mode => _mode;
    public Godot.Range Money => _money;
    public Godot.Range Units => _units;

    private const string PlayerName = "指挥官";
    private static readonly string[] ModeSections = { "Battle", "ManBattle", "Siege", "Unholy", "FreeForAll", "Cooperative" };

    private readonly SkirmishSlot[] _slots = new SkirmishSlot[SkirmishSetup.SlotCount];
    private readonly SkirmishSlotCard[] _cards = new SkirmishSlotCard[SkirmishSetup.SlotCount];
    private readonly List<MapCatalog.Entry> _maps = new();
    private readonly List<Vector2> _starts = new();
    private RefCounted? _core;
    private string _directory = "";
    private bool _catalogReady;
    private IEnumerator<MapCatalog.Entry?>? _scan;
    private MapCatalog.Entry? _map;
    private readonly List<(int Index, string Name)> _countries = new();

    private SkirmishMapPreview _preview = null!;
    private OptionButton _mode = null!;
    private Label _mapTitle = null!;
    private Label _status = null!;
    private Label _mapDetails = null!;
    private PopupPanel _mapPopup = null!;
    private ItemList _mapList = null!;
    private Label _mapPopupStatus = null!;
    private readonly Dictionary<string, CheckBox> _options = new();
    private HSlider _money = null!;
    private HSlider _units = null!;
    private HSlider _speed = null!;
    private Label _moneyValue = null!;
    private Label _unitsValue = null!;
    private Label _speedValue = null!;

    public override void _Ready()
    {
        for (var i = 0; i < _slots.Length; i++) _slots[i] = new SkirmishSlot();
        _slots[0].Kind = SkirmishSlotKind.Human;
        _slots[1].Kind = SkirmishSlotKind.NormalAI;
        BuildLayout();
        StartButton.Pressed += StartGame;
        _mode.ItemSelected += _ => RefreshStartButton();
        RefreshStartButton();
        SetProcess(false);
    }

    public void Present(RefCounted core, string directory)
    {
        if (_catalogReady && _directory == directory && ReferenceEquals(_core, core))
        {
            RefreshSlots();
            return;
        }
        CancelScan();
        _core = core;
        _directory = directory;
        _catalogReady = false;
        _maps.Clear();
        _mapList.Clear();
        _map = null;
        try
        {
            ReadRules(core);
            ReadModes(core);
        }
        catch (Exception exception)
        {
            _status.Text = "读取规则失败：" + exception.Message;
        }
        ShowMap(null);
        _status.Text = "正在查找遭遇战地图…";
        _scan = MapCatalog.Discover(core, directory).GetEnumerator();
        SetProcess(true);
    }

    public override void _Process(double delta)
    {
        if (_scan == null) return;
        try
        {
            // Spread metadata reads over frames, like the map picker.
            var deadline = Time.GetTicksUsec() + 4000;
            do
            {
                if (!_scan.MoveNext())
                {
                    CancelScan();
                    _catalogReady = true;
                    UpdateScanStatus();
                    return;
                }
                if (_scan.Current is not { IsCampaign: false, IsTest: false } entry) continue;
                _maps.Add(entry);
                _mapList.AddItem(entry.Name.Equals(entry.DisplayFilename, StringComparison.OrdinalIgnoreCase)
                    ? entry.DisplayFilename : $"{entry.Name}  ·  {entry.DisplayFilename}");
                if (_map == null) SelectMap(entry);
                UpdateScanStatus();
            } while (Time.GetTicksUsec() < deadline);
        }
        catch (Exception exception)
        {
            CancelScan();
            _status.Text = "读取地图列表失败：" + exception.Message;
        }
    }

    public void CancelScan()
    {
        _scan?.Dispose();
        _scan = null;
        SetProcess(false);
    }

    public void InvalidateCatalog()
    {
        CancelScan();
        _catalogReady = false;
        _directory = "";
        _core = null;
    }

    public override void _ExitTree() => CancelScan();

    public bool SelectMap(string filename)
    {
        var entry = _maps.Find(map => map.Filename.Equals(filename, StringComparison.OrdinalIgnoreCase));
        if (entry == null) return false;
        SelectMap(entry);
        return true;
    }

    private void SelectMap(MapCatalog.Entry entry)
    {
        _map = entry;
        ShowMap(entry);
    }

    // ---------------------------------------------------------------- data

    private void ReadRules(RefCounted core)
    {
        using var rules = MapCatalog.LoadIni(core, "RULESMD.INI") ?? MapCatalog.LoadIni(core, "RULES.INI");
        var countries = _countries;
        countries.Clear();
        if (rules != null)
        {
            using var names = rules.Call("get_key_names", "Countries");
            var index = 0;
            // HouseType indices follow the [Countries] list order (Read_Countries).
            foreach (var key in names.AsStringArray())
            {
                var id = MapCatalog.Read(rules, "Countries", key);
                if (id.Length == 0) continue;
                if (Bool(rules, id, "Multiplay", false) && !Bool(rules, id, "MultiplayPassive", false))
                {
                    var name = MapCatalog.Localized(core, MapCatalog.Read(rules, id, "UIName"));
                    countries.Add((index, name.Length > 0 ? name : id));
                }
                index++;
            }
        }
        foreach (var card in _cards) card.SetCountries(countries);

        const string section = "MultiplayerDialogSettings";
        var minMoney = Integer(rules, section, "MinMoney", 2500);
        var maxMoney = Math.Max(minMoney, Integer(rules, section, "MaxMoney", 10000));
        _money.MaxValue = maxMoney;
        _money.MinValue = minMoney;
        _money.Step = Math.Max(1, Integer(rules, section, "MoneyIncrement", 500));
        _money.Value = Integer(rules, section, "Money", maxMoney);
        var minUnits = Integer(rules, section, "MinUnitCount", 1);
        _units.MaxValue = Math.Max(minUnits, Integer(rules, section, "MaxUnitCount", 10));
        _units.MinValue = minUnits;
        _units.Value = Integer(rules, section, "UnitCount", minUnits);
        // The original slider shows 6 - GameSpeed, so the right end is fastest.
        _speed.Value = 6 - Math.Clamp(Integer(rules, section, "GameSpeed", 1), 0, 6);
        _options["ShortGame"].ButtonPressed = Bool(rules, section, "ShortGame", true);
        _options["BridgeDestruction"].ButtonPressed = Bool(rules, section, "BridgeDestruction", true);
        _options["BuildOffAlly"].ButtonPressed = Bool(rules, section, "BuildOffAlly", false);
        _options["MCVRedeploys"].ButtonPressed = Bool(rules, section, "MCVRedeploys", true);
        _options["SuperWeaponsAllowed"].ButtonPressed = Bool(rules, section, "SuperWeaponsAllowed", true);
        _options["Crates"].ButtonPressed = Bool(rules, section, "Crates", false);
        _moneyValue.Text = (Mathf.RoundToInt(_money.Value)).ToString(CultureInfo.InvariantCulture);
        _unitsValue.Text = (Mathf.RoundToInt(_units.Value)).ToString(CultureInfo.InvariantCulture);
        _speedValue.Text = (Mathf.RoundToInt(_speed.Value) + 1).ToString(CultureInfo.InvariantCulture);
    }

    private void ReadModes(RefCounted core)
    {
        _mode.Clear();
        using var modes = MapCatalog.LoadIni(core, "MPMODESMD.INI") ?? MapCatalog.LoadIni(core, "MPMODES.INI");
        var titles = new List<string>();
        if (modes != null)
            foreach (var section in ModeSections)
            {
                using var keys = modes.Call("get_key_names", section);
                foreach (var key in keys.AsStringArray())
                {
                    // Value fields: title label, tooltip label, rules file, map filter, AI allowed.
                    var fields = MapCatalog.Read(modes, section, key).Split(',').Select(field => field.Trim()).ToArray();
                    if (fields.Length == 0 || fields[0].Length == 0) continue;
                    if (fields.Length > 4 && fields[4].StartsWith("n", StringComparison.OrdinalIgnoreCase)) continue;
                    var title = MapCatalog.Localized(core, fields[0]);
                    if (title.Length == 0) title = fields[0];
                    if (titles.Contains(title)) continue;
                    titles.Add(title);
                    var supported = section == "Battle" && fields.Length > 2 &&
                        fields[2].Equals("MPBATTLE.INI", StringComparison.OrdinalIgnoreCase);
                    _mode.AddItem(supported ? title : title + "（暂未开放）");
                    _mode.SetItemDisabled(_mode.ItemCount - 1, !supported);
                }
            }
        var battle = Enumerable.Range(0, _mode.ItemCount).FirstOrDefault(index => !_mode.IsItemDisabled(index), -1);
        if (battle < 0) { battle = _mode.ItemCount; _mode.AddItem("作战"); }
        _mode.Select(battle);
        _mode.TooltipText = "当前开放普通作战模式。其他模式将在对应规则接入后开放。";
    }

    private void ShowMap(MapCatalog.Entry? entry)
    {
        _starts.Clear();
        PreviewSize = Vector2I.Zero;
        Texture2D? texture = null;
        if (entry != null && _core != null)
        {
            try
            {
                using var ini = MapCatalog.LoadIni(_core, entry.Filename);
                if (ini != null)
                {
                    ReadStarts(ini);
                    if (ini.HasMethod("get_preview_image"))
                    {
                        using var value = ini.Call("get_preview_image");
                        if (value.AsGodotObject() is Image image)
                        {
                            texture = ImageTexture.CreateFromImage(image);
                            PreviewSize = image.GetSize();
                        }
                    }
                }
            }
            catch (Exception exception)
            {
                _status.Text = "读取地图失败：" + exception.Message;
            }
        }
        _preview.SetMap(texture, _starts, entry == null ? "正在查找地图…" : "该地图没有预览图");
        _mapTitle.Text = entry == null ? "未选择地图" : $"[{_starts.Count}] {entry.Name}";
        _mapTitle.TooltipText = _mapTitle.Text;
        _mapDetails.Text = entry == null ? "" : $"{entry.DisplayFilename}  ·  {entry.Theater}  ·  {entry.Width} × {entry.Height}";
        FitSlotsToMap();
        RefreshSlots();
    }

    // Waypoints 0..7 are the multiplayer starts; 0x688380 stops at the first missing one.
    private void ReadStarts(RefCounted ini)
    {
        var size = Rect(MapCatalog.Read(ini, "Map", "Size"));
        var local = Rect(MapCatalog.Read(ini, "Map", "LocalSize"));
        if (size is not { } map) return;
        var visible = local ?? map;
        if (visible.Size.X <= 0 || visible.Size.Y <= 0) return;
        for (var i = 0; i < SkirmishSetup.MaxStarts; i++)
        {
            var key = i.ToString(CultureInfo.InvariantCulture);
            using var exists = ini.Call("has_key", "Waypoints", key);
            if (!exists.AsBool()) break;
            using var value = ini.Call("get_integer", "Waypoints", key, -1);
            var cell = value.AsInt32();
            if (cell < 0) break;
            int rx = cell % 1000, ry = cell / 1000;
            // Isometric display cell of (rx, ry), in half cells, relative to LocalSize.
            float x = rx - ry + map.Size.X - 1, y = rx + ry - map.Size.X - 1;
            _starts.Add(new Vector2((x / 2 - visible.Position.X) / visible.Size.X, (y / 2 - visible.Position.Y) / visible.Size.Y));
        }
    }

    private static Rect2I? Rect(string text)
    {
        var parts = text.Split(',');
        if (parts.Length != 4) return null;
        var values = new int[4];
        for (var i = 0; i < 4; i++)
            if (!int.TryParse(parts[i].Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out values[i])) return null;
        return new Rect2I(values[0], values[1], values[2], values[3]);
    }

    private static int Integer(RefCounted? ini, string section, string key, int fallback)
    {
        if (ini == null) return fallback;
        using var value = ini.Call("get_integer", section, key, fallback);
        return value.AsInt32();
    }

    private static bool Bool(RefCounted? ini, string section, string key, bool fallback)
    {
        if (ini == null) return fallback;
        using var value = ini.Call("get_bool", section, key, fallback);
        return value.AsBool();
    }

    // ---------------------------------------------------------------- seats

    private void FitSlotsToMap()
    {
        var limit = _starts.Count;
        foreach (var slot in _slots)
            if (slot.Start >= limit) slot.Start = SkirmishSlot.Random;
        if (limit == 0) return;
        // Too many seats for the new map: open the last computer seats first.
        for (var i = _slots.Length - 1; i > 0 && _slots.Count(slot => slot.IsOccupied) > limit; i--)
            if (_slots[i].IsOccupied && _slots[i].Kind != SkirmishSlotKind.Human) Open(_slots[i]);
    }

    private static void Open(SkirmishSlot slot)
    {
        slot.Kind = SkirmishSlotKind.Open;
        slot.Country = slot.Color = slot.Start = SkirmishSlot.Random;
        slot.Team = SkirmishSlot.NoTeam;
    }

    private void OnSlotChanged(SkirmishSlotCard card)
    {
        var slot = card.Slot;
        if (!slot.IsOccupied) Open(slot);
        else if (_starts.Count > 0 && _slots.Count(other => other.IsOccupied) > _starts.Count)
        {
            Open(slot);
            _status.Text = $"该地图最多支持 {_starts.Count} 名玩家。";
        }
        RefreshSlots();
    }

    private void OnStartClicked(int start)
    {
        var human = _slots[0];
        if (_slots.Any(slot => slot != human && slot.IsOccupied && slot.Start == start)) return;
        human.Start = human.Start == start ? SkirmishSlot.Random : start;
        RefreshSlots();
    }

    private void RefreshSlots()
    {
        var starts = new HashSet<int>(_slots.Where(slot => slot.IsOccupied && slot.Start >= 0).Select(slot => slot.Start));
        var colors = new HashSet<int>(_slots.Where(slot => slot.IsOccupied && slot.Color >= 0).Select(slot => slot.Color));
        foreach (var card in _cards) card.Refresh(_starts.Count, starts, colors);
        var labels = new Dictionary<int, (string Text, Color? Ring)>();
        foreach (var slot in _slots)
            if (slot.IsOccupied && slot.Start >= 0 && slot.Start < _starts.Count)
                labels[slot.Start] = (slot.Kind == SkirmishSlotKind.Human ? PlayerName : SkirmishSetup.KindLabel(slot.Kind),
                    slot.Color >= 0 ? SkirmishSetup.Colors[slot.Color].Value : null);
        _preview.SetLabels(labels);
        RefreshStartButton();
    }

    private string LaunchError()
    {
        if (_core == null || !_core.HasMethod("begin_skirmish_loading")) return "请重新编译原生库以启用遭遇战。";
        if (_map == null || _starts.Count < 2) return "请选择至少有两个出生点的遭遇战地图。";
        if (_mode.Selected < 0 || _mode.IsItemDisabled(_mode.Selected)) return "该模式暂未开放。";
        var occupied = _slots.Where(slot => slot.IsOccupied).ToArray();
        if (occupied.Length < 2) return "请至少添加一名电脑对手。";
        if (occupied.Length > _starts.Count) return "玩家人数超过地图出生点数量。";
        if (_slots[0].Kind != SkirmishSlotKind.Human || occupied.Count(slot => slot.Kind == SkirmishSlotKind.Human) != 1)
            return "遭遇战需要一名本地玩家。";
        if (occupied.Any(slot => slot.Country != SkirmishSlot.Random && !_countries.Any(country => country.Index == slot.Country)))
            return "请选择可用国家。";
        if (occupied.Any(slot => slot.Start != SkirmishSlot.Random && (slot.Start < 0 || slot.Start >= _starts.Count)))
            return "请选择地图中的出生点。";
        if (occupied.Where(slot => slot.Color >= 0).GroupBy(slot => slot.Color).Any(group => group.Count() > 1))
            return "玩家颜色不能重复。";
        if (occupied.Where(slot => slot.Start >= 0).GroupBy(slot => slot.Start).Any(group => group.Count() > 1))
            return "出生点不能重复。";
        if (_slots[0].Team >= 0 && occupied.All(slot => slot.Team == _slots[0].Team))
            return "不能与所有电脑结盟，请保留至少一个敌方队伍。";
        return "";
    }

    private void RefreshStartButton()
    {
        var error = LaunchError();
        StartButton.Disabled = error.Length > 0;
        StartButton.TooltipText = error;
        UpdateScanStatus();
    }

    private void StartGame()
    {
        var error = LaunchError();
        if (error.Length > 0) { _status.Text = error; RefreshStartButton(); return; }
        var players = new int[SkirmishSetup.SlotCount * 5];
        for (var i = 0; i < _slots.Length; i++)
        {
            var slot = _slots[i];
            players[i * 5] = (int)slot.Kind;
            players[i * 5 + 1] = slot.Country;
            players[i * 5 + 2] = slot.Color;
            players[i * 5 + 3] = slot.Start;
            players[i * 5 + 4] = slot.Team;
        }
        using var options = new Godot.Collections.Dictionary {
            ["players"] = players, ["money"] = Mathf.RoundToInt(_money.Value), ["units"] = Mathf.RoundToInt(_units.Value),
            ["game_speed"] = 6 - Mathf.RoundToInt(_speed.Value), ["seed"] = (long)(uint)Time.GetTicksUsec()
        };
        foreach (var (key, checkbox) in _options) options[key] = checkbox.ButtonPressed;
        CancelScan();
        StartRequested?.Invoke(SelectedMap, options);
    }

    private void UpdateScanStatus()
    {
        var text = Scanning ? $"正在查找遭遇战地图…已找到 {_maps.Count} 张"
            : _maps.Count == 0 ? "未找到可用的遭遇战地图，请检查游戏资源目录。" : $"共 {_maps.Count} 张遭遇战地图";
        var error = LaunchError();
        _status.Text = !Scanning && error.Length > 0 ? error : text;
        _mapPopupStatus.Text = text;
    }

    // ---------------------------------------------------------------- layout

    private void BuildLayout()
    {
        AddChild(new ColorRect { Color = SkirmishStyle.Background, MouseFilter = MouseFilterEnum.Ignore }.FullRect());
        var page = new VBoxContainer();
        page.AddThemeConstantOverride("separation", 18);
        AddChild(Margin(page, 24, 24, 20, 20).FullRect());
        var header = new HBoxContainer();
        page.AddChild(header);
        var titles = new VBoxContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        titles.AddThemeConstantOverride("separation", 2);
        header.AddChild(titles);
        var title = new Label { Text = "遭遇战" };
        title.AddThemeFontSizeOverride("font_size", 30);
        titles.AddChild(title);
        var subtitle = new Label { Text = "部署玩家，选择战场，开始你的战斗。" };
        subtitle.AddThemeFontSizeOverride("font_size", 15);
        subtitle.AddThemeColorOverride("font_color", SkirmishStyle.TextDim);
        titles.AddChild(subtitle);
        var back = new Button { Name = "BackButton", Text = "返回主菜单", CustomMinimumSize = new Vector2(140, 42),
            SizeFlagsVertical = SizeFlags.ShrinkCenter };
        back.AddThemeFontSizeOverride("font_size", 17);
        back.Pressed += () => BackRequested?.Invoke();
        header.AddChild(back);
        var body = new HBoxContainer { SizeFlagsVertical = SizeFlags.ExpandFill };
        body.AddThemeConstantOverride("separation", 20);
        page.AddChild(body);

        var left = new VBoxContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill, SizeFlagsStretchRatio = 1.7f };
        left.AddThemeConstantOverride("separation", 8);
        body.AddChild(left);
        var grid = new GridContainer { Columns = 4, SizeFlagsVertical = SizeFlags.ExpandFill };
        grid.AddThemeConstantOverride("h_separation", 10);
        grid.AddThemeConstantOverride("v_separation", 10);
        left.AddChild(grid);
        for (var i = 0; i < _cards.Length; i++)
        {
            var card = new SkirmishSlotCard(_slots[i]) { PlayerName = PlayerName, Name = $"Slot{i + 1}" };
            card.Changed += () => OnSlotChanged(card);
            _cards[i] = card;
            grid.AddChild(card);
        }
        left.AddChild(BuildOptions());

        var right = new VBoxContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        right.AddThemeConstantOverride("separation", 12);
        var rightPanel = new PanelContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        var rightStyle = SkirmishStyle.Box(SkirmishStyle.Panel, 10, SkirmishStyle.CardBorder, 1);
        rightStyle.SetContentMarginAll(16);
        rightPanel.AddThemeStyleboxOverride("panel", rightStyle);
        rightPanel.AddChild(right);
        body.AddChild(rightPanel);

        var previewHeader = new HBoxContainer();
        right.AddChild(previewHeader);
        var previewTitle = new Label { Text = "战场预览", SizeFlagsHorizontal = SizeFlags.ExpandFill };
        previewTitle.AddThemeFontSizeOverride("font_size", 18);
        previewHeader.AddChild(previewTitle);
        var changeMap = OverlayButton("更换地图");
        changeMap.Name = "ChangeMapButton";
        changeMap.Pressed += OpenMapPopup;
        previewHeader.AddChild(changeMap);
        var previewHost = new Control { CustomMinimumSize = new Vector2(0, 220), SizeFlagsVertical = SizeFlags.ExpandFill };
        right.AddChild(previewHost);
        _preview = new SkirmishMapPreview { Name = "MapPreview" }.FullRect();
        _preview.StartClicked += OnStartClicked;
        previewHost.AddChild(_preview);

        var footer = new PanelContainer { CustomMinimumSize = new Vector2(0, 44) };
        var footerStyle = SkirmishStyle.Box(SkirmishStyle.Card, 6);
        footerStyle.ContentMarginLeft = footerStyle.ContentMarginRight = 10;
        footer.AddThemeStyleboxOverride("panel", footerStyle);
        right.AddChild(footer);
        var footerRow = new HBoxContainer();
        footerRow.AddThemeConstantOverride("separation", 12);
        footer.AddChild(footerRow);
        _mode = new OptionButton { TooltipText = "游戏模式", FitToLongestItem = false,
            CustomMinimumSize = new Vector2(112, 0) };
        SkirmishStyle.Flat(_mode, 18);
        footerRow.AddChild(_mode);
        footerRow.AddChild(new VSeparator());
        _mapTitle = new Label { SizeFlagsHorizontal = SizeFlags.ExpandFill, VerticalAlignment = VerticalAlignment.Center,
            ClipText = true };
        _mapTitle.AddThemeFontSizeOverride("font_size", 19);
        footerRow.AddChild(_mapTitle);

        var info = new VBoxContainer();
        info.AddThemeConstantOverride("separation", 8);
        right.AddChild(info);
        _status = new Label { AutowrapMode = TextServer.AutowrapMode.WordSmart };
        _status.AddThemeFontSizeOverride("font_size", 14);
        _status.AddThemeColorOverride("font_color", SkirmishStyle.TextDim);
        info.AddChild(_status);
        _mapDetails = new Label { AutowrapMode = TextServer.AutowrapMode.WordSmart };
        _mapDetails.AddThemeFontSizeOverride("font_size", 14);
        _mapDetails.AddThemeColorOverride("font_color", SkirmishStyle.Text);
        info.AddChild(_mapDetails);

        StartButton = new Button { Name = "StartButton", Text = "开始游戏", CustomMinimumSize = new Vector2(0, 64) };
        StartButton.AddThemeFontSizeOverride("font_size", 23);
        StartButton.AddThemeStyleboxOverride("normal", SkirmishStyle.Box(SkirmishStyle.Accent, 7));
        StartButton.AddThemeStyleboxOverride("hover", SkirmishStyle.Box(new Color(1f, 0.67f, 0.4f), 7));
        StartButton.AddThemeStyleboxOverride("pressed", SkirmishStyle.Box(new Color(0.8f, 0.43f, 0.2f), 7));
        StartButton.AddThemeStyleboxOverride("focus", SkirmishStyle.Box(new Color(0, 0, 0, 0), 4, SkirmishStyle.Text, 2));
        StartButton.AddThemeColorOverride("font_color", SkirmishStyle.PanelDark);
        StartButton.AddThemeColorOverride("font_hover_color", SkirmishStyle.PanelDark);
        StartButton.AddThemeColorOverride("font_pressed_color", SkirmishStyle.PanelDark);
        StartButton.AddThemeColorOverride("font_focus_color", SkirmishStyle.PanelDark);
        StartButton.AddThemeColorOverride("font_disabled_color", SkirmishStyle.TextDim);
        StartButton.AddThemeStyleboxOverride("disabled", SkirmishStyle.Box(SkirmishStyle.Card, 7));
        right.AddChild(StartButton);

        BuildMapPopup();
    }

    private Control BuildOptions()
    {
        var tabs = new TabContainer { CustomMinimumSize = new Vector2(0, 190) };
        tabs.AddThemeStyleboxOverride("panel", SkirmishStyle.Box(SkirmishStyle.Panel));
        tabs.AddThemeStyleboxOverride("tab_selected", Tab(SkirmishStyle.Panel));
        tabs.AddThemeStyleboxOverride("tab_unselected", Tab(SkirmishStyle.PanelDark));
        tabs.AddThemeStyleboxOverride("tab_hovered", Tab(SkirmishStyle.AccentDark));
        tabs.AddThemeStyleboxOverride("tab_focus", new StyleBoxEmpty());
        tabs.AddThemeColorOverride("font_selected_color", SkirmishStyle.Accent);
        tabs.AddThemeColorOverride("font_unselected_color", SkirmishStyle.TextDim);
        tabs.AddThemeColorOverride("font_hovered_color", Colors.White);
        tabs.AddThemeFontSizeOverride("font_size", 17);

        var basic = new VBoxContainer { Name = "基础设置" };
        basic.AddThemeConstantOverride("separation", 18);
        tabs.AddChild(Margin(basic, 18, 18, 16, 10, "基础设置"));
        var checks = new GridContainer { Columns = 3 };
        checks.AddThemeConstantOverride("h_separation", 22);
        checks.AddThemeConstantOverride("v_separation", 12);
        basic.AddChild(checks);
        foreach (var (key, text) in new[]
                 {
                     ("ShortGame", "快速游戏"), ("BridgeDestruction", "桥梁可摧毁"), ("BuildOffAlly", "盟友旁建设"),
                     ("MCVRedeploys", "基地重新部署"), ("SuperWeaponsAllowed", "超级武器"), ("Crates", "随机箱子")
                 })
        {
            var check = new CheckBox { Text = text, Name = key };
            // The menu theme's button boxes would draw the checkboxes as orange buttons.
            foreach (var state in new[] { "normal", "hover", "pressed", "hover_pressed", "focus", "disabled" })
                check.AddThemeStyleboxOverride(state, new StyleBoxEmpty());
            check.AddThemeFontSizeOverride("font_size", 16);
            check.AddThemeColorOverride("font_color", SkirmishStyle.Text);
            check.AddThemeColorOverride("font_pressed_color", SkirmishStyle.Text);
            check.AddThemeColorOverride("font_hover_color", Colors.White);
            check.AddThemeColorOverride("font_hover_pressed_color", Colors.White);
            _options[key] = check;
            checks.AddChild(check);
        }
        var sliders = new HBoxContainer();
        sliders.AddThemeConstantOverride("separation", 14);
        basic.AddChild(sliders);
        _money = Slider(sliders, "初始金钱：", out _moneyValue);
        _units = Slider(sliders, "初始部队：", out _unitsValue);

        var play = new VBoxContainer { Name = "游戏性" };
        play.AddThemeConstantOverride("separation", 18);
        tabs.AddChild(Margin(play, 18, 18, 16, 10, "游戏性"));
        var speedRow = new HBoxContainer();
        speedRow.AddThemeConstantOverride("separation", 14);
        play.AddChild(speedRow);
        _speed = Slider(speedRow, "游戏速度：", out _speedValue);
        _speed.MinValue = 0;
        _speed.MaxValue = 6;
        _speed.Step = 1;
        _speed.ValueChanged += value => _speedValue.Text = (Mathf.RoundToInt(value) + 1).ToString(CultureInfo.InvariantCulture);
        _money.ValueChanged += value => _moneyValue.Text = (Mathf.RoundToInt(value)).ToString(CultureInfo.InvariantCulture);
        _units.ValueChanged += value => _unitsValue.Text = (Mathf.RoundToInt(value)).ToString(CultureInfo.InvariantCulture);
        return tabs;
    }

    private static StyleBoxFlat Tab(Color color)
    {
        var box = SkirmishStyle.Box(color);
        box.ContentMarginLeft = box.ContentMarginRight = 22;
        box.ContentMarginTop = box.ContentMarginBottom = 8;
        return box;
    }

    private static HSlider Slider(HBoxContainer row, string caption, out Label value)
    {
        var label = new Label { Text = caption, VerticalAlignment = VerticalAlignment.Center };
        label.AddThemeFontSizeOverride("font_size", 16);
        label.AddThemeColorOverride("font_color", SkirmishStyle.Text);
        row.AddChild(label);
        var slider = new HSlider { Rounded = true, CustomMinimumSize = new Vector2(70, 0), SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ShrinkCenter };
        var track = SkirmishStyle.Box(SkirmishStyle.CardBorder, 3);
        track.ContentMarginTop = track.ContentMarginBottom = 3;
        slider.AddThemeStyleboxOverride("slider", track);
        var fill = SkirmishStyle.Box(SkirmishStyle.Accent, 3);
        fill.ContentMarginTop = fill.ContentMarginBottom = 3;
        slider.AddThemeStyleboxOverride("grabber_area", fill);
        slider.AddThemeStyleboxOverride("grabber_area_highlight", fill);
        row.AddChild(slider);
        var box = new PanelContainer { CustomMinimumSize = new Vector2(64, 32) };
        box.AddThemeStyleboxOverride("panel", SkirmishStyle.Box(SkirmishStyle.AccentDark, 5));
        value = new Label { HorizontalAlignment = HorizontalAlignment.Center, VerticalAlignment = VerticalAlignment.Center };
        value.AddThemeFontSizeOverride("font_size", 17);
        box.AddChild(value);
        row.AddChild(box);
        return slider;
    }

    private static Button OverlayButton(string text)
    {
        var button = new Button { Text = text, CustomMinimumSize = new Vector2(0, 38) };
        button.AddThemeFontSizeOverride("font_size", 18);
        var normal = SkirmishStyle.Box(SkirmishStyle.Card, 6);
        normal.ContentMarginLeft = normal.ContentMarginRight = 14;
        var hover = SkirmishStyle.Box(SkirmishStyle.AccentDark, 6);
        hover.ContentMarginLeft = hover.ContentMarginRight = 14;
        button.AddThemeStyleboxOverride("normal", normal);
        button.AddThemeStyleboxOverride("hover", hover);
        button.AddThemeStyleboxOverride("pressed", hover);
        button.AddThemeStyleboxOverride("focus", new StyleBoxEmpty());
        return button;
    }

    private static MarginContainer Margin(Control content, int left, int right, int top, int bottom, string name = "")
    {
        var margin = new MarginContainer { SizeFlagsHorizontal = content.SizeFlagsHorizontal,
            SizeFlagsVertical = content.SizeFlagsVertical, SizeFlagsStretchRatio = content.SizeFlagsStretchRatio };
        if (name.Length > 0) margin.Name = name;
        margin.AddThemeConstantOverride("margin_left", left);
        margin.AddThemeConstantOverride("margin_right", right);
        margin.AddThemeConstantOverride("margin_top", top);
        margin.AddThemeConstantOverride("margin_bottom", bottom);
        margin.AddChild(content);
        return margin;
    }

    private void BuildMapPopup()
    {
        _mapPopup = new PopupPanel { Name = "MapPopup" };
        _mapPopup.AddThemeStyleboxOverride("panel", SkirmishStyle.Box(SkirmishStyle.Panel, 8, SkirmishStyle.CardBorder, 1));
        AddChild(_mapPopup);
        var content = new VBoxContainer();
        content.AddThemeConstantOverride("separation", 10);
        _mapPopup.AddChild(Margin(content, 16, 16, 14, 14));
        var title = new Label { Text = "选择地图" };
        title.AddThemeFontSizeOverride("font_size", 24);
        content.AddChild(title);
        _mapPopupStatus = new Label();
        _mapPopupStatus.AddThemeFontSizeOverride("font_size", 16);
        _mapPopupStatus.AddThemeColorOverride("font_color", SkirmishStyle.TextDim);
        content.AddChild(_mapPopupStatus);
        _mapList = new ItemList { SizeFlagsVertical = SizeFlags.ExpandFill, CustomMinimumSize = new Vector2(560, 380) };
        _mapList.AddThemeFontSizeOverride("font_size", 18);
        _mapList.ItemActivated += _ => ChooseListedMap();
        content.AddChild(_mapList);
        var actions = new HBoxContainer { Alignment = BoxContainer.AlignmentMode.End };
        actions.AddThemeConstantOverride("separation", 12);
        content.AddChild(actions);
        var cancel = new Button { Text = "取消", CustomMinimumSize = new Vector2(120, 44) };
        cancel.Pressed += () => _mapPopup.Hide();
        actions.AddChild(cancel);
        var choose = new Button { Text = "确定", CustomMinimumSize = new Vector2(120, 44) };
        choose.Pressed += ChooseListedMap;
        actions.AddChild(choose);
    }

    private void OpenMapPopup()
    {
        UpdateScanStatus();
        var index = _map == null ? -1 : _maps.IndexOf(_map);
        if (index >= 0)
        {
            _mapList.Select(index);
            _mapList.EnsureCurrentIsVisible();
        }
        _mapPopup.PopupCentered();
        _mapList.GrabFocus();
    }

    private void ChooseListedMap()
    {
        var selected = _mapList.GetSelectedItems();
        if (selected.Length == 0) return;
        _mapPopup.Hide();
        SelectMap(_maps[selected[0]]);
    }
}

internal static class SkirmishControlExtensions
{
    public static T FullRect<T>(this T control) where T : Control
    {
        control.SetAnchorsPreset(Control.LayoutPreset.FullRect);
        return control;
    }
}
