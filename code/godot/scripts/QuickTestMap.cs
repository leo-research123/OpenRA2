/// <summary>Keep the quick-test shortcut on the shared bundled-map loading path.</summary>
public static class QuickTestMap
{
    public const string ResourcePath = BundledTestMaps.QuickTestPath;

    public static bool Prepare(out string filename, out string error) =>
        BundledTestMaps.Prepare(ResourcePath, out filename, out error);
}
