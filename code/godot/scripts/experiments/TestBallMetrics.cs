using Godot;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

// Optional bounded capture; interactive runs only keep the HUD's short averages.
public sealed class TestBallMetrics(double duration)
{
    private readonly record struct Sample(double FrameMs, float UpdateMs, float SubmitMs, int Submitted);
    private readonly List<Sample> _samples = new(16384);
    private double _started = -1;
    private Vector2 _viewport;
    private bool _resized;

    public bool Observe(double elapsed, double frameMs, Vector3 timing, Vector2 viewport)
    {
        if (elapsed < 5) return false;
        if (_started < 0)
        {
            _started = elapsed;
            _viewport = viewport;
            return false;
        }
        _resized |= viewport != _viewport;
        _samples.Add(new Sample(frameMs, timing.X, timing.Y, (int)timing.Z));
        return elapsed - _started >= duration;
    }

    private static object Distribution(IEnumerable<double> source)
    {
        var values = source.ToArray();
        Array.Sort(values);
        double Percentile(double q) => values[Math.Clamp((int)Math.Ceiling(q * values.Length) - 1, 0, values.Length - 1)];
        return new { mean = values.Average(), p50 = Percentile(0.5), p95 = Percentile(0.95),
            p99 = Percentile(0.99), max = values[^1] };
    }

    public object Summary() => new
    {
        warmup_seconds = 5,
        measured_seconds = _samples.Sum(s => s.FrameMs) / 1000,
        frames = _samples.Count,
        average_fps = 1000 / _samples.Average(s => s.FrameMs),
        frame_interval_ms = Distribution(_samples.Select(s => s.FrameMs)),
        update_ms = Distribution(_samples.Select(s => (double)s.UpdateMs)),
        submit_ms = Distribution(_samples.Select(s => (double)s.SubmitMs)),
        frames_over_60fps_budget = _samples.Count(s => s.FrameMs > 1000.0 / 60),
        submitted_min = _samples.Min(s => s.Submitted),
        submitted_max = _samples.Max(s => s.Submitted),
        viewport_at_start = new[] { _viewport.X, _viewport.Y },
        viewport_changed = _resized,
    };

    public string FrameCsv()
    {
        var csv = new StringBuilder("frame,frame_interval_ms,update_ms,submit_ms,submitted\n");
        for (int i = 0; i < _samples.Count; ++i)
        {
            var sample = _samples[i];
            csv.AppendLine(FormattableString.Invariant(
                $"{i},{sample.FrameMs:F6},{sample.UpdateMs:F6},{sample.SubmitMs:F6},{sample.Submitted}"));
        }
        return csv.ToString();
    }
}
