using Godot;
using System;
using System.Collections.Generic;
using System.Threading.Tasks;

/// <summary>Both backends must render the same seeded workload at the same elapsed times.</summary>
public partial class TestBallComparison : TestRunner
{
    protected override async Task RunTestAsync()
    {
        if (DisplayServer.GetName() == "headless")
            throw new InvalidOperationException("TestBallComparison requires a rendering window.");
        var reference = new List<byte[]>();
        foreach (var backend in new[] { "test-ball", "test-ball-godot" })
        {
            Root.Size = new Vector2I(1920, 1080);
            await DrawFrame();
            using var scene = GD.Load<PackedScene>($"res://scenes/{backend}.tscn");
            var balls = scene.Instantiate<TestBallHost>();
            balls.ResizeWindow = false;
            balls.ShowStats = false;
            balls.UnlimitedFps = false;
            Root.AddChild(balls);
            balls.SetProcess(false);
            int frame = 0;
            foreach (double delta in new[] { 0, 0.25, 1, 8.75, 0.0 })
            {
                if (frame == 4)
                {
                    Root.Size = new Vector2I(960, 540);
                    await DrawFrame();
                }
                balls._Process(delta);
                Expect(balls.SubmittedCount == 10_000, $"{backend}: submit every ball");
                await DrawFrame();
                using var image = Root.GetTexture().GetImage();
                var pixels = image.GetData();
                if (backend == "test-ball") reference.Add(pixels);
                else
                {
                    var expected = reference[frame];
                    Expect(pixels.Length == expected.Length, "Matching image dimensions and format");
                    int maxError = 0, different = 0;
                    for (int i = 0; i < Math.Min(pixels.Length, expected.Length); ++i)
                    {
                        int error = Math.Abs(pixels[i] - expected[i]);
                        maxError = Math.Max(maxError, error);
                        if (error > 0) ++different;
                    }
                    // Allow at most two 8-bit levels for float/color conversion rounding.
                    Expect(maxError <= 2, $"Frame {frame}: image mismatch, max channel error {maxError}");
                    GD.Print($"Ball comparison frame {frame}: max channel error={maxError}, differing bytes={different}");
                }
                if (CaptureDirectory().Length > 0 && frame == 2)
                    Expect(image.SavePng(CaptureDirectory().PathJoin($"{backend}-comparison.png")) == Error.Ok, "Save comparison capture");
                ++frame;
            }
            balls.QueueFree();
            await NextFrame();
            await DrawFrame();
            using var cleared = Root.GetTexture().GetImage();
            var center = cleared.GetPixel(cleared.GetWidth() / 2, cleared.GetHeight() / 2);
            Expect(center.R < 0.5f && center.G < 0.5f && center.B < 0.5f, $"{backend}: release drawing");
        }
    }
}
