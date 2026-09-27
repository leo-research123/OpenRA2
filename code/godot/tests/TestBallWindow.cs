using Godot;
using System;
using System.Threading.Tasks;

/// <summary>Checks pixels from the C++ RenderingServer path in a real window.</summary>
public partial class TestBallWindow : TestRunner
{
    private readonly record struct BallPixels(int Count, Vector2 Center, Color Color);

    private BallPixels FindBall(Image image)
    {
        int count = 0;
        double xSum = 0, ySum = 0;
        for (int y = 0; y < image.GetHeight(); y++)
        for (int x = 0; x < image.GetWidth(); x++)
        {
            var pixel = image.GetPixel(x, y);
            if (Math.Max(pixel.R, Math.Max(pixel.G, pixel.B)) < 0.5f) continue;
            count++;
            xSum += x + 0.5;
            ySum += y + 0.5;
        }
        var center = count == 0 ? Vector2.Zero : new Vector2((float)(xSum / count), (float)(ySum / count));
        return new BallPixels(count, center,
            count == 0 ? Colors.Black : image.GetPixel((int)center.X, (int)center.Y));
    }

    private async Task<BallPixels> ReadBall(string captureName = "")
    {
        await DrawFrame();
        using var image = Root.GetTexture().GetImage();
        var directory = CaptureDirectory();
        if (captureName.Length > 0 && directory.Length > 0)
            Expect(image.SavePng(directory.PathJoin(captureName)) == Error.Ok, "Save test-ball capture");
        return FindBall(image);
    }

    protected override async Task RunTestAsync()
    {
        if (DisplayServer.GetName() == "headless")
            throw new InvalidOperationException("TestBallWindow requires a real rendering window.");
        var originalTitle = Root.Title;
        var originalScaleMode = Root.ContentScaleMode;
        var originalScaleSize = Root.ContentScaleSize;
        using var scene = GD.Load<PackedScene>("res://scenes/test-ball.tscn");
        for (int run = 0; run < 3; run++)
        {
            Root.Size = new Vector2I(560, 400);
            await DrawFrame();
            var ball = scene.Instantiate<TestBall>();
            ball.BallCount = 1;
            ball.BallRadius = 20;
            ball.ResizeWindow = false;
            ball.ShowStats = false;
            ball.UnlimitedFps = false;
            Root.AddChild(ball);
            ball.SetProcess(false);
            // _Process below is the same host entry point as normal playback;
            // explicit elapsed time makes the rendered-pixel assertions repeatable.
            var size = ball.GetViewportRect().Size;
            var start = await ReadBall(run == 0 ? "test-ball-start.png" : "");
            Expect(start.Count > 1100 && start.Count < 1400, "One radius-20 circle must be visible");
            Expect(start.Center.DistanceTo(size * 0.5f) < 1, "Ball starts at viewport center");

            ball._Process(0.25);
            var moved = await ReadBall();
            Expect(moved.Center.DistanceTo(start.Center + new Vector2(65, 45)) < 1,
                "C++ movement must reach rendered pixels");
            Expect(moved.Count > 1100 && moved.Count < 1400, "Old circle commands must be cleared");
            Expect(Math.Abs(moved.Color.G - start.Color.G) > 0.1f, "C++ color must change");

            var timeToRightWall = (size.X - 20 - (size.X * 0.5 + 65)) / 260;
            ball._Process(timeToRightWall);
            var wall = await ReadBall();
            Expect(Math.Abs(wall.Center.X - (size.X - 20)) < 1, "Ball reaches right edge without clipping");
            ball._Process(0.1);
            var reflected = await ReadBall(run == 0 ? "test-ball-bounce.png" : "");
            Expect(Math.Abs(reflected.Center.X - (size.X - 46)) < 1, "Ball moves inward after wall contact");

            Root.Size = new Vector2I(320, 240);
            await DrawFrame();
            ball._Process(0);
            var resized = await ReadBall(run == 0 ? "test-ball-resized.png" : "");
            size = ball.GetViewportRect().Size;
            Expect(resized.Count > 1100 && resized.Count < 1400, "Resize must retain one full-size circle");
            Expect(resized.Center.X >= 19.5 && resized.Center.X <= size.X - 19.5 &&
                resized.Center.Y >= 19.5 && resized.Center.Y <= size.Y - 19.5, "Resized bounds contain ball");

            ball.QueueFree();
            await NextFrame();
            var cleared = await ReadBall();
            Expect(cleared.Count == 0, "Exit must free the native CanvasItem and remove its drawing");
            Expect(Root.Title == originalTitle && Root.ContentScaleMode == originalScaleMode &&
                Root.ContentScaleSize == originalScaleSize, "Exit must restore host window settings");
        }

        Root.Size = new Vector2I(1920, 1080);
        await DrawFrame();
        var stress = scene.Instantiate<TestBall>();
        stress.ResizeWindow = false;
        stress.ShowStats = false;
        Root.AddChild(stress);
        stress.SetProcess(false);
        stress._Process(0);
        await DrawFrame();
        using var before = Root.GetTexture().GetImage();
        var initial = FindBall(before);
        Expect(stress.SubmittedCount == 10_000, "Native path must submit all ten thousand circles");
        Expect(initial.Count > 200_000 && initial.Count < 350_000, "Stress frame must contain the expected small-circle coverage");
        stress._Process(0.25);
        await DrawFrame();
        using var after = Root.GetTexture().GetImage();
        int changed = 0;
        for (int y = 0; y < before.GetHeight(); y += 4)
        for (int x = 0; x < before.GetWidth(); x += 4)
            if (!before.GetPixel(x, y).IsEqualApprox(after.GetPixel(x, y))) ++changed;
        Expect(changed > 10_000, "Stress load must change across the rendered frame");
        if (CaptureDirectory().Length > 0)
            Expect(after.SavePng(CaptureDirectory().PathJoin("test-ball-10000.png")) == Error.Ok, "Save stress capture");
        Root.Size = new Vector2I(960, 540);
        await DrawFrame();
        stress._Process(0);
        Expect(stress.SubmittedCount == 10_000, "Resize must preserve the complete stress load");
        stress.QueueFree();
        await NextFrame();
        Expect((await ReadBall()).Count == 0, "Stress CanvasItem must also be freed");
    }
}
