using Godot;

/// <summary>Host directory preference only. C++ retains archive discovery and priority.</summary>
public static class ResourceConfig
{
    public static string Resolve(string scenePath, string configPath)
    {
        var directory = AutomationRunner.Argument("--game-data=");
        if (directory.Length == 0) directory = scenePath;
        if (directory.Length == 0) directory = OS.GetEnvironment("RA2_GAME_DATA");
        if (directory.Length == 0)
        {
            using var config = new ConfigFile();
            if (config.Load(configPath) == Error.Ok)
            {
                using var saved = config.GetValue("resources", "directory", "");
                if (saved.VariantType == Variant.Type.String) directory = saved.AsString();
            }
        }
        if (directory.Length == 0)
            directory = OS.HasFeature("editor") ? "res://../../out/reference/RA2MDddcompact" : OS.GetExecutablePath().GetBaseDir();
        return Normalize(directory);
    }

    public static string Normalize(string directory)
    {
        if (directory.StartsWith("res://") || directory.StartsWith("user://"))
            directory = ProjectSettings.GlobalizePath(directory);
        else if (!directory.IsAbsolutePath())
            directory = ProjectSettings.GlobalizePath("res://").PathJoin(directory);
        return directory.SimplifyPath();
    }

    public static Error Save(string configPath, string directory)
    {
        using var config = new ConfigFile();
        config.Load(configPath);
        config.SetValue("resources", "directory", directory);
        return config.Save(configPath);
    }
}
