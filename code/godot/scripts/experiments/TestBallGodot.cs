using Godot;
using System;
using System.Diagnostics;

/// <summary>Conventional Godot scene: persistent Sprite2D nodes sharing one texture.</summary>
public partial class TestBallGodot : TestBallHost
{
    private readonly GodotBallField _field = new();
    private Sprite2D[] _sprites = [];
    private double _appliedRadius = -1;
    protected override string ExperimentName => "test-ball-godot";
    protected override string BackendName => "Godot C#";
    protected override string RenderingPath => "Sprite2D shared texture, position and modulate updates";

    protected override bool InitializeRenderer(Vector2 size)
    {
        if (!_field.Reset(size, BallCount, BallRadius)) return false;
        _sprites = new Sprite2D[BallCount];
        for (int i = 0; i < _sprites.Length; ++i)
        {
            var sprite = new Sprite2D { Texture = BallTexture, TextureFilter = TextureFilterEnum.Linear };
            _sprites[i] = sprite;
            AddChild(sprite);
        }
        UpdateSprites();
        return true;
    }

    protected override Vector3 AdvanceRenderer(double seconds, Vector2 size)
    {
        var start = Stopwatch.GetTimestamp();
        if (!_field.Advance(seconds, size)) return Vector3.Zero;
        var updated = Stopwatch.GetTimestamp();
        int count = UpdateSprites();
        var submitted = Stopwatch.GetTimestamp();
        return new Vector3((float)Stopwatch.GetElapsedTime(start, updated).TotalMilliseconds,
            (float)Stopwatch.GetElapsedTime(updated, submitted).TotalMilliseconds, count);
    }

    private int UpdateSprites()
    {
        var radiusChanged = _field.Balls[0].Radius != _appliedRadius;
        var scale = Vector2.One * (float)(_field.Balls[0].Radius * 2 / 64);
        for (int i = 0; i < _sprites.Length; ++i)
        {
            ref readonly var ball = ref _field.Balls[i];
            var sprite = _sprites[i];
            // Match the float cropped-rectangle representation used by SpriteDrawBatch.
            var radius = (float)ball.Radius;
            sprite.Position = new Vector2((float)(ball.X - ball.Radius) + radius,
                (float)(ball.Y - ball.Radius) + radius);
            sprite.Modulate = Color.FromHsv((float)ball.Hue, 0.8f, 1);
            if (radiusChanged) sprite.Scale = scale;
        }
        _appliedRadius = _field.Balls[0].Radius;
        return _field.Balls.Length;
    }

    protected override void ReleaseRenderer()
    {
        // Nodes are owned by this scene and freed by Godot with their parent.
        _sprites = [];
        _appliedRadius = -1;
    }
}
