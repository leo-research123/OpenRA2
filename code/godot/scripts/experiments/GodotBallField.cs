using Godot;
using System;

// Managed implementation of the same display workload, with no native-core calls.
public sealed class GodotBallField
{
    public struct Ball
    {
        public double X, Y, VelocityX, VelocityY, Radius, Hue;
    }

    public Ball[] Balls { get; private set; } = [];
    private double _radius;

    private static double Random(ref uint seed)
    {
        seed = unchecked(seed * 1664525u + 1013904223u);
        return (seed >> 8) / 16777216.0;
    }

    private static bool ValidSize(Vector2 size) =>
        float.IsFinite(size.X) && float.IsFinite(size.Y) && size.X >= 0 && size.Y >= 0;

    public bool Reset(Vector2 size, int count, double radius)
    {
        if (!ValidSize(size) || count < 1 || count > 100_000 || !double.IsFinite(radius) || radius <= 0) return false;
        Balls = new Ball[count];
        _radius = radius;
        var fitted = Math.Min(radius, Math.Min(size.X * 0.5, size.Y * 0.5));
        uint seed = 0x42414c4c;
        for (int i = 0; i < count; ++i)
        {
            ref var ball = ref Balls[i];
            ball = new Ball { X = size.X * 0.5, Y = size.Y * 0.5, VelocityX = 260, VelocityY = 180, Radius = fitted };
            if (count == 1) continue;
            ball.X = fitted + Random(ref seed) * (size.X - 2 * fitted);
            ball.Y = fitted + Random(ref seed) * (size.Y - 2 * fitted);
            ball.VelocityX = 100 + Random(ref seed) * 200;
            ball.VelocityY = 100 + Random(ref seed) * 200;
            if (Random(ref seed) < 0.5) ball.VelocityX = -ball.VelocityX;
            if (Random(ref seed) < 0.5) ball.VelocityY = -ball.VelocityY;
            ball.Hue = Random(ref seed);
        }
        return true;
    }

    private static void AdvanceAxis(ref double position, ref double velocity, double extent, double radius, double seconds)
    {
        var span = extent - 2 * radius;
        if (span <= 0)
        {
            position = extent * 0.5;
            return;
        }
        position = Math.Clamp(position, radius, extent - radius);
        var speed = Math.Abs(velocity);
        var period = 2 * span;
        var phase = position - radius;
        if (velocity < 0) phase = period - phase;
        phase = (phase + seconds % (period / speed) * speed) % period;
        if (phase < span)
        {
            position = radius + phase;
            velocity = speed;
        }
        else
        {
            position = radius + period - phase;
            velocity = -speed;
        }
    }

    public bool Advance(double seconds, Vector2 size)
    {
        if (!double.IsFinite(seconds) || seconds < 0 || !ValidSize(size)) return false;
        var radius = Math.Min(_radius, Math.Min(size.X * 0.5, size.Y * 0.5));
        var hueStep = seconds % 6 / 6;
        for (int i = 0; i < Balls.Length; ++i)
        {
            ref var ball = ref Balls[i];
            ball.Radius = radius;
            AdvanceAxis(ref ball.X, ref ball.VelocityX, size.X, radius, seconds);
            AdvanceAxis(ref ball.Y, ref ball.VelocityY, size.Y, radius, seconds);
            ball.Hue = (ball.Hue + hueStep) % 1;
        }
        return true;
    }
}
