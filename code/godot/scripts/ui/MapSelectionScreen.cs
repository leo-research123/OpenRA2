using Godot;
using System;
using System.Collections.Generic;

public partial class MapSelectionScreen : Control
{
    public enum Category { Campaign, NonCampaign, All }
    public event Action? BackRequested;
    public event Action<string>? MapChosen;
    public ItemList Maps { get; private set; } = null!;
    public Button LoadButton { get; private set; } = null!;
    public OptionButton MapType { get; private set; } = null!;
    public bool Scanning => _scan != null;
    public string SelectedMap => Maps.GetSelectedItems() is { Length: > 0 } selected
        ? _visibleEntries[selected[0]].Filename : "";

    private Label _status = null!;
    private Label _details = null!;
    private IEnumerator<MapCatalog.Entry?>? _scan;
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
        MapType.ItemSelected += _ => RebuildList();
        _status = GetNode<Label>("Content/Status");
        _details = GetNode<Label>("Content/Details");
        Maps.ItemSelected += _ => UpdateSelection();
        Maps.ItemActivated += _ => LoadSelected();
        LoadButton.Pressed += LoadSelected;
        GetNode<Button>("Content/Actions/BackButton").Pressed += () => BackRequested?.Invoke();
        SetProcess(false);
    }

    public void Present(RefCounted core, string directory)
    {
        CancelScan();
        Maps.Clear();
        _entries.Clear();
        _visibleEntries.Clear();
        MapType.Select((int)Category.Campaign);
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
                    MapType.Disabled = false;
                    UpdateCount();
                    Maps.MouseFilter = MouseFilterEnum.Stop;
                    Maps.FocusMode = FocusModeEnum.All;
                    if (_visibleEntries.Count > 0) Maps.GrabFocus();
                    else MapType.GrabFocus();
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
        if (selected.Length == 0) return;
        var entry = _visibleEntries[selected[0]];
        _details.Text = $"{entry.Filename}  ·  {entry.Theater}  ·  {entry.Width} × {entry.Height}";
    }

    private void AddVisibleEntry(MapCatalog.Entry entry)
    {
        if (MapType.Selected != (int)Category.All &&
            entry.IsCampaign != (MapType.Selected == (int)Category.Campaign)) return;
        _visibleEntries.Add(entry);
        var label = entry.Name.Equals(entry.Filename, StringComparison.OrdinalIgnoreCase)
            ? entry.Filename : $"{entry.Name}  ·  {entry.Filename}";
        var index = Maps.AddItem(label);
        Maps.SetItemTooltip(index, $"{entry.Filename}\n{entry.Theater}  ·  {entry.Width} × {entry.Height}");
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
        MapChosen?.Invoke(SelectedMap);
    }

    public void CancelScan()
    {
        _scan?.Dispose();
        _scan = null;
        SetProcess(false);
    }

    public override void _ExitTree() => CancelScan();
}
