using Godot;
using System;
using System.Collections.Generic;

public partial class MapSelectionScreen : Control
{
    public enum Category
    {
        Campaign, NonCampaign, All, Test, RedAlert2Campaign, YurisRevengeCampaign,
        RedAlert2Allied, RedAlert2Soviet, YurisRevengeAllied, YurisRevengeSoviet, OtherCampaign
    }
    public event Action? BackRequested;
    public event Action<MapCatalog.Entry>? MapChosen;
    public ItemList Maps { get; private set; } = null!;
    public Button LoadButton { get; private set; } = null!;
    public OptionButton MapType { get; private set; } = null!;
    public bool Scanning => _scan != null;
    public int CampaignDifficulty { get; private set; } = 1;
    public PanelContainer DifficultyPanel { get; private set; } = null!;
    public IReadOnlyList<Button> DifficultyButtons => _difficultyButtons;
    private readonly Button[] _difficultyButtons = new Button[3];
    private Label _difficultyHint = null!;
    public string SelectedMap => Maps.GetSelectedItems() is { Length: > 0 } selected
        ? _visibleEntries[selected[0]].Filename : "";

    private Label _status = null!;
    private Label _details = null!;
    private IEnumerator<MapCatalog.Entry?>? _scan;
    private bool _catalogReady;
    private string _catalogDirectory = "";
    private readonly List<MapCatalog.Entry> _entries = new();
    private readonly List<MapCatalog.Entry> _visibleEntries = new();
    private const string SelectionHint = "选择地图后点击“加载地图”，也可双击列表中的地图。";

    public override void _Ready()
    {
        Maps = GetNode<ItemList>("Content/Maps");
        LoadButton = GetNode<Button>("Content/Actions/LoadButton");
        MapType = GetNode<OptionButton>("Content/Header/MapType");
        MapType.AddItem("战役地图", (int)Category.Campaign);
        MapType.AddItem("非战役地图", (int)Category.NonCampaign);
        MapType.AddItem("所有地图", (int)Category.All);
        MapType.AddItem("测试地图", (int)Category.Test);
        MapType.AddItem("标准版 · 全部战役", (int)Category.RedAlert2Campaign);
        MapType.AddItem("尤里的复仇 · 全部战役", (int)Category.YurisRevengeCampaign);
        MapType.AddItem("标准版 · 盟军战役", (int)Category.RedAlert2Allied);
        MapType.AddItem("标准版 · 苏军战役", (int)Category.RedAlert2Soviet);
        MapType.AddItem("尤里的复仇 · 盟军战役", (int)Category.YurisRevengeAllied);
        MapType.AddItem("尤里的复仇 · 苏军战役", (int)Category.YurisRevengeSoviet);
        MapType.AddItem("其他战役", (int)Category.OtherCampaign);
        MapType.ItemSelected += _ => RebuildList();
        _status = GetNode<Label>("Content/Status");
        _details = GetNode<Label>("Content/Details");
        Maps.ItemSelected += _ => UpdateSelection();
        Maps.ItemActivated += _ => LoadSelected();
        LoadButton.Pressed += LoadSelected;
        GetNode<Button>("Content/Actions/BackButton").Pressed += () => BackRequested?.Invoke();
        BuildDifficultySelector();
        SetProcess(false);
    }

    private void BuildDifficultySelector()
    {
        DifficultyPanel = new PanelContainer { Name = "CampaignDifficulty", Visible = false };
        var style = SkirmishStyle.Box(SkirmishStyle.Panel, 8, SkirmishStyle.CardBorder, 1);
        style.SetContentMarginAll(14);
        DifficultyPanel.AddThemeStyleboxOverride("panel", style);
        var content = GetNode<VBoxContainer>("Content");
        content.AddChild(DifficultyPanel);
        content.MoveChild(DifficultyPanel, content.GetNode("Actions").GetIndex());
        var row = new HBoxContainer();
        row.AddThemeConstantOverride("separation", 20);
        DifficultyPanel.AddChild(row);
        var copy = new VBoxContainer { SizeFlagsHorizontal = SizeFlags.ExpandFill };
        row.AddChild(copy);
        var title = new Label { Text = "战役难度" };
        title.AddThemeFontSizeOverride("font_size", 20);
        copy.AddChild(title);
        _difficultyHint = new Label();
        _difficultyHint.AddThemeFontSizeOverride("font_size", 15);
        _difficultyHint.AddThemeColorOverride("font_color", SkirmishStyle.TextDim);
        copy.AddChild(_difficultyHint);
        var group = new ButtonGroup();
        var labels = new[] { "简单", "普通", "困难" };
        for (var i = 0; i < labels.Length; i++)
        {
            var difficulty = i;
            var button = new Button { Name = $"Difficulty{i}", Text = labels[i], ToggleMode = true,
                ButtonGroup = group, CustomMinimumSize = new Vector2(100, 44), SizeFlagsVertical = SizeFlags.ShrinkCenter };
            button.AddThemeFontSizeOverride("font_size", 18);
            button.AddThemeStyleboxOverride("pressed", SkirmishStyle.Box(SkirmishStyle.AccentDark, 6, SkirmishStyle.Accent, 1));
            button.AddThemeColorOverride("font_pressed_color", SkirmishStyle.Accent);
            button.Pressed += () => SelectDifficulty(difficulty);
            _difficultyButtons[i] = button;
            row.AddChild(button);
        }
        SelectDifficulty(CampaignDifficulty);
    }

    private void SelectDifficulty(int difficulty)
    {
        CampaignDifficulty = difficulty;
        _difficultyButtons[difficulty].ButtonPressed = true;
        _difficultyHint.Text = new[] { "适合熟悉任务与操作。", "标准战役挑战。", "适合熟练指挥官的更高挑战。" }[difficulty];
    }

    public void Present(RefCounted core, string directory)
    {
        if (_catalogReady && _catalogDirectory == directory)
        {
            UpdateCount();
            UpdateSelection();
            FocusList();
            return;
        }
        CancelScan();
        _catalogReady = false;
        _catalogDirectory = directory;
        Maps.Clear();
        DifficultyPanel.Hide();
        _entries.Clear();
        _visibleEntries.Clear();
        MapType.Disabled = true;
        Maps.MouseFilter = MouseFilterEnum.Ignore;
        Maps.FocusMode = FocusModeEnum.None;
        LoadButton.Disabled = true;
        _details.Text = SelectionHint;
        _status.Text = "正在查找地图…";
        _scan = MapCatalog.Discover(core, directory).GetEnumerator();
        SetProcess(true);
        GetNode<Button>("Content/Actions/BackButton").GrabFocus();
    }

    public override void _Process(double delta)
    {
        if (_scan == null) return;
        try
        {
            // Native resources are owner-thread only. Spread metadata reads over
            // frames instead of racing map loading or blocking all menu input.
            var deadline = Time.GetTicksUsec() + 4000;
            do
            {
                if (!_scan.MoveNext())
                {
                    CancelScan();
                    _catalogReady = true;
                    MapType.Disabled = false;
                    UpdateCount();
                    Maps.MouseFilter = MouseFilterEnum.Stop;
                    Maps.FocusMode = FocusModeEnum.All;
                    FocusList();
                    UpdateSelection();
                    return;
                }
                var entry = _scan.Current;
                if (entry == null) continue;
                _entries.Add(entry);
                AddVisibleEntry(entry);
                UpdateCount();
            } while (Time.GetTicksUsec() < deadline);
        }
        catch (Exception exception)
        {
            CancelScan();
            Maps.Clear();
            _entries.Clear();
            _visibleEntries.Clear();
            LoadButton.Disabled = true;
            _status.Text = "读取地图列表失败：" + exception.Message;
        }
    }

    public bool SelectMap(string filename)
    {
        if (Scanning) return false;
        var index = _visibleEntries.FindIndex(entry => entry.Filename.Equals(filename, StringComparison.OrdinalIgnoreCase));
        if (index < 0) return false;
        Maps.Select(index);
        Maps.EnsureCurrentIsVisible();
        UpdateSelection();
        return true;
    }

    private void UpdateSelection()
    {
        var selected = Maps.GetSelectedItems();
        LoadButton.Disabled = Scanning || selected.Length == 0;
        DifficultyPanel.Visible = !Scanning && selected.Length > 0 &&
            _visibleEntries[selected[0]].IsCampaign && !_visibleEntries[selected[0]].IsTest;
        if (selected.Length == 0) return;
        var entry = _visibleEntries[selected[0]];
        _details.Text = $"{entry.Name}  ·  {entry.Classification}\n{entry.DisplayFilename}  ·  {entry.Theater}  ·  {entry.Width} × {entry.Height}";
    }

    private void AddVisibleEntry(MapCatalog.Entry entry)
    {
        var campaign = !entry.IsTest && entry.IsCampaign;
        var visible = (Category)MapType.GetSelectedId() switch
        {
            Category.Campaign => campaign,
            Category.NonCampaign => !entry.IsTest && !entry.IsCampaign,
            Category.Test => entry.IsTest,
            Category.All => true,
            Category.RedAlert2Campaign => campaign && entry.Version == MapCatalog.CampaignVersion.RedAlert2,
            Category.YurisRevengeCampaign => campaign && entry.Version == MapCatalog.CampaignVersion.YurisRevenge,
            Category.RedAlert2Allied => campaign && entry.Version == MapCatalog.CampaignVersion.RedAlert2 && entry.Side == MapCatalog.CampaignSide.Allied,
            Category.RedAlert2Soviet => campaign && entry.Version == MapCatalog.CampaignVersion.RedAlert2 && entry.Side == MapCatalog.CampaignSide.Soviet,
            Category.YurisRevengeAllied => campaign && entry.Version == MapCatalog.CampaignVersion.YurisRevenge && entry.Side == MapCatalog.CampaignSide.Allied,
            Category.YurisRevengeSoviet => campaign && entry.Version == MapCatalog.CampaignVersion.YurisRevenge && entry.Side == MapCatalog.CampaignSide.Soviet,
            Category.OtherCampaign => campaign &&
                (entry.Version == MapCatalog.CampaignVersion.Unknown || entry.Side == MapCatalog.CampaignSide.Unknown),
            _ => false
        };
        if (!visible) return;
        _visibleEntries.Add(entry);
        var label = entry.Name.Equals(entry.DisplayFilename, StringComparison.OrdinalIgnoreCase)
            ? entry.DisplayFilename : $"{entry.Name}  ·  {entry.DisplayFilename}";
        var index = Maps.AddItem(label);
        Maps.SetItemTooltip(index, $"{entry.Name}\n{entry.Classification}\n{entry.DisplayFilename}\n{entry.Theater}  ·  {entry.Width} × {entry.Height}");
    }

    private void RebuildList()
    {
        Maps.Clear();
        _visibleEntries.Clear();
        _details.Text = SelectionHint;
        foreach (var entry in _entries) AddVisibleEntry(entry);
        UpdateCount();
        UpdateSelection();
    }

    private void UpdateCount()
    {
        _status.Text = Scanning ? $"正在查找地图…当前分类已找到 {_visibleEntries.Count} 张"
            : _entries.Count == 0 ? "未找到可用地图。请返回主菜单检查游戏资源目录。"
            : _visibleEntries.Count == 0 ? "当前分类下没有地图，请切换地图类型查看。"
            : $"找到 {_visibleEntries.Count} 张地图";
    }

    private void LoadSelected()
    {
        if (!IsVisibleInTree() || Scanning || SelectedMap.Length == 0) return;
        MapChosen?.Invoke(_visibleEntries[Maps.GetSelectedItems()[0]]);
    }

    public void ShowLoadError(string error) => _status.Text = error;

    private void FocusList()
    {
        if (_visibleEntries.Count > 0) Maps.GrabFocus();
        else MapType.GrabFocus();
    }

    public void InvalidateCatalog()
    {
        CancelScan();
        _catalogReady = false;
        _catalogDirectory = "";
        Maps.Clear();
        _entries.Clear();
        _visibleEntries.Clear();
        LoadButton.Disabled = true;
        _details.Text = SelectionHint;
        DifficultyPanel.Hide();
    }

    public void CancelScan()
    {
        _scan?.Dispose();
        _scan = null;
        SetProcess(false);
    }

    public override void _ExitTree() => CancelScan();
}
