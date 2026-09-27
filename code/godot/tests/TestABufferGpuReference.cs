using Godot;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

// Original x86 outputs are the oracle. Geometry/coverage cases additionally
// exercise the bridge contract; they do not claim full original-frame parity.
public partial class TestABufferGpuReference : TestRunner
{
    private const uint Covered = 0x10000;
    private readonly List<Rid> _resources = new();
    private RenderingDevice _rd = null!;
    private Rid _shader, _pipeline;
    private byte[] _reference = Array.Empty<byte>();
    private int _cases, _pixels;
    private sealed record Packet(int[] Parameters, uint[] Source);

    private Rid Keep(Rid rid)
    {
        if (!rid.IsValid) throw new InvalidOperationException("GPU resource creation failed");
        _resources.Add(rid);
        return rid;
    }

    private static byte[] Bytes(Array words)
    {
        var bytes = new byte[words.Length * 4];
        Buffer.BlockCopy(words, 0, bytes, 0, bytes.Length);
        return bytes;
    }

    private Rid Storage(Array words)
    {
        var bytes = Bytes(words);
        return Keep(_rd.StorageBufferCreate((uint)bytes.Length, bytes));
    }

    private (Rid, Rid) Pipeline(string name)
    {
        using var source = GD.Load<RDShaderFile>($"res://shaders/{name}.glsl");
        using var spirv = source.GetSpirV();
        var error = spirv.GetStageCompileError(RenderingDevice.ShaderStage.Compute);
        if (error.Length != 0) throw new InvalidOperationException(error);
        var shader = Keep(_rd.ShaderCreateFromSpirV(spirv));
        return (shader, Keep(_rd.ComputePipelineCreate(shader)));
    }

    private Rid Bind(Rid shader, params Rid[] buffers)
    {
        var uniforms = new Godot.Collections.Array<RDUniform>();
        for (int binding = 0; binding < buffers.Length; ++binding)
        {
            var uniform = new RDUniform { Binding = binding,
                UniformType = RenderingDevice.UniformType.StorageBuffer };
            uniform.AddId(buffers[binding]);
            uniforms.Add(uniform);
        }
        try { return Keep(_rd.UniformSetCreate(uniforms, shader, 0)); }
        finally { foreach (var uniform in uniforms) uniform.Dispose(); }
    }

    private static Packet Make(int width, int height, int operation, int origin = 0,
        int sourceWidth = 0, int sourceHeight = 0, int x = 0, int y = 0, bool holes = false)
    {
        int sw = sourceWidth == 0 ? width : sourceWidth;
        int sh = sourceHeight == 0 ? height : sourceHeight;
        var parameters = new int[20];
        new[] { width, height, sw, sh, x, y, 0, 0, width, height }.CopyTo(parameters, 0);
        parameters[16] = origin;
        parameters[19] = operation;
        // Reset deliberately has only a dummy word: it must never sample it.
        var source = operation == 0 ? new uint[1]
            : Enumerable.Range(0, sw * sh).Select(i => (uint)(i % 256)
                | (holes && i % 5 == 0 ? 0u : Covered) | 0xAB000000u).ToArray();
        return new(parameters, source);
    }

    private uint[] Expected(uint[] initial, params Packet[] packets)
    {
        var expected = (uint[])initial.Clone();
        foreach (var packet in packets)
        {
            var p = packet.Parameters;
            // Traverse destination pixels, independent of the shader dispatch.
            for (int y = 0; y < p[1]; ++y)
            for (int x = 0; x < p[0]; ++x)
            {
                int sx = x - p[4], sy = y - p[5];
                if (sx < 0 || sy < 0 || sx >= p[2] || sy >= p[3]
                    || x < p[6] || y < p[7] || x >= p[6] + p[8] || y >= p[7] + p[9]) continue;
                int row = ((y - p[16]) % p[1] + p[1]) % p[1];
                int index = row * p[0] + x, op = p[19];
                if (op == 0) { expected[index] = 127; continue; }
                if (op < 1 || op > 3) continue;
                uint sample = packet.Source[sy * p[2] + sx];
                if ((sample & Covered) == 0) continue;
                // Lookup actual original instruction observations, no copied formulas.
                int offset = 8 + (op - 1) * 65536 + (int)expected[index] * 256 + (int)(sample & 0xFF);
                expected[index] = _reference[offset];
            }
        }
        return expected;
    }

    private void Dispatch(long list, Rid pipeline, Rid set, int index, int width, int height)
    {
        _rd.ComputeListBindComputePipeline(list, pipeline);
        _rd.ComputeListBindUniformSet(list, set, 0);
        _rd.ComputeListSetPushConstant(list, Bytes(new[] { index }), 4);
        _rd.ComputeListDispatch(list, (uint)(width + 7) / 8, (uint)(height + 7) / 8, 1);
        _rd.ComputeListAddBarrier(list);
    }

    private void Check(string name, Rid light, uint[] expected)
    {
        var actual = _rd.BufferGetData(light);
        var wanted = Bytes(expected);
        int differences = 0, first = -1;
        for (int i = 0; i < expected.Length; ++i)
            if (BitConverter.ToUInt32(actual, i * 4) != expected[i])
            { ++differences; if (first < 0) first = i; }
        Expect(actual.Length == wanted.Length && differences == 0,
            $"{name}: {differences} different pixels, first index {first}");
        ++_cases;
        _pixels += expected.Length;
    }

    private void Run(string name, uint[] initial, Packet[] packets, bool drawConsumer = false)
    {
        int resourceStart = _resources.Count;
        try
        {
            var parameters = Storage(packets.SelectMany(p => p.Parameters).ToArray());
            var light = Storage(initial);
            var sets = packets.Select(p => Bind(_shader, parameters, Storage(p.Source), light)).ToArray();
            var expected = Expected(initial, packets);
            int width = packets[0].Parameters[0], height = packets[0].Parameters[1];
            Rid drawPipeline = default, drawSet = default, colors = default, depth = default;
            uint[] palette = Array.Empty<uint>();
            if (drawConsumer)
            {
                // 128 shades, intensity 254 => shade == light for these <=127 inputs.
                // Real map_draw reads the producer's buffer after the barrier.
                var (drawShader, pipeline) = Pipeline("map_draw");
                drawPipeline = pipeline;
                var p = Make(width, height, 0, packets[0].Parameters[16]).Parameters;
                p[10] = 1; p[11] = 254; p[12] = 128;
                colors = Storage(new uint[initial.Length]);
                depth = Storage(Enumerable.Repeat(0xFFFFu, initial.Length).ToArray());
                palette = Enumerable.Range(0, 129 * 256).Select(i => 0xFF000000u | (uint)i).ToArray();
                drawSet = Bind(drawShader, Storage(p),
                    Storage(Enumerable.Repeat(Covered | 1u, initial.Length).ToArray()), colors, depth, light, Storage(palette));
            }
            long list = _rd.ComputeListBegin();
            for (int i = 0; i < packets.Length; ++i)
                Dispatch(list, _pipeline, sets[i], i, packets[i].Parameters[2], packets[i].Parameters[3]);
            if (drawConsumer) Dispatch(list, drawPipeline, drawSet, 0, width, height);
            _rd.ComputeListEnd(); _rd.Submit(); _rd.Sync();
            Check(name, light, expected);
            if (drawConsumer)
            {
                var expectedColors = new uint[initial.Length];
                for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    int row = ((y - packets[0].Parameters[16]) % height + height) % height;
                    uint intensity = expected[row * width + x];
                    if (intensity > 127) throw new InvalidOperationException("Consumer test requires light <= 127");
                    expectedColors[y * width + x] = palette[256 + intensity * 256 + 1];
                }
                Check(name + " consumer color", colors, expectedColors);
                Check(name + " preserved Z", depth, Enumerable.Repeat(0xFFFFu, initial.Length).ToArray());
            }
        }
        finally
        {
            for (int i = _resources.Count - 1; i >= resourceStart; --i) _rd.FreeRid(_resources[i]);
            _resources.RemoveRange(resourceStart, _resources.Count - resourceStart);
        }
    }

    private void RunBatchedFixture(string path)
    {
        using var reader = new BinaryReader(File.OpenRead(path));
        if (Encoding.ASCII.GetString(reader.ReadBytes(8)) != "LIGHTBT1")
            throw new InvalidDataException("Expected production LIGHTBT1 packets");
        var (shader, pipeline) = Pipeline("map_abuffer_batch");
        int cases = reader.ReadInt32();
        for (int c = 0; c < cases; ++c)
        {
            int start = _resources.Count;
            try
            {
                int width = reader.ReadInt32(), height = reader.ReadInt32(), count = reader.ReadInt32();
                int atlasCount = reader.ReadInt32(), binCount = reader.ReadInt32(), batchCount = reader.ReadInt32();
                uint[] ReadWords(int n) => Enumerable.Range(0, n).Select(_ => reader.ReadUInt32()).ToArray();
                var initial = ReadWords(width * height);
                var parameters = ReadWords(count * 20).Select(v => unchecked((int)v)).ToArray();
                var atlas = ReadWords(atlasCount);
                var bins = ReadWords(binCount);
                var offsets = ReadWords(batchCount);
                var packets = new Packet[count];
                for (int i = 0; i < count; ++i)
                {
                    var p = parameters.AsSpan(i * 20, 20).ToArray();
                    packets[i] = new Packet(p, atlas.AsSpan(p[17], p[2] * p[3]).ToArray());
                }
                var light = Storage(initial);
                var set = Bind(shader, Storage(parameters), Storage(atlas), light, Storage(bins));
                long list = _rd.ComputeListBegin();
                _rd.ComputeListBindComputePipeline(list, pipeline);
                _rd.ComputeListBindUniformSet(list, set, 0);
                foreach (uint offset in offsets)
                {
                    _rd.ComputeListSetPushConstant(list, Bytes(new[] { width, height, (width + 31) / 32, (int)offset }), 16);
                    _rd.ComputeListDispatch(list, (uint)(width + 7) / 8, (uint)(height + 7) / 8, 1);
                    _rd.ComputeListAddBarrier(list);
                }
                _rd.ComputeListEnd(); _rd.Submit(); _rd.Sync();
                Check($"production batch {c}: {count} packets, {batchCount} dispatches", light, Expected(initial, packets));
            }
            finally
            {
                for (int i = _resources.Count - 1; i >= start; --i) _rd.FreeRid(_resources[i]);
                _resources.RemoveRange(start, _resources.Count - start);
            }
        }
        if (reader.BaseStream.Position != reader.BaseStream.Length) throw new InvalidDataException("Trailing batch fixture data");
    }

    protected override Task RunTestAsync()
    {
        var argument = OS.GetCmdlineUserArgs().FirstOrDefault(s => s.StartsWith("--reference="));
        var path = argument == null ? ProjectSettings.GlobalizePath("res://../tests/fixtures/abuffer_reference.bin")
            : argument["--reference=".Length..];
        _reference = File.ReadAllBytes(path);
        if (_reference.Length != 8 + 3 * 65536 || Encoding.ASCII.GetString(_reference, 0, 8) != "ABUFREF1")
            throw new InvalidDataException("Expected ABUFREF1 fixture");
        using var rd = RenderingServer.CreateLocalRenderingDevice();
        _rd = rd ?? throw new InvalidOperationException("This test requires a real RenderingDevice");
        try
        {
            (_shader, _pipeline) = Pipeline("map_abuffer");
            var exhaustive = Enumerable.Range(0, 65536).Select(i => (uint)(i / 256)).ToArray();
            for (int op = 1; op <= 3; ++op)
                Run($"all old/source pairs op={op}", exhaustive, new[] { Make(256, 256, op) });

            var initial = Enumerable.Range(0, 17 * 13).Select(i => (uint)(i * 53 % 256)).ToArray();
            foreach (int origin in new[] { -27, -1, 0, 5, 31 })
            for (int op = 0; op <= 3; ++op)
            {
                var packet = Make(17, 13, op, origin, 21, 15, -3, -2, true);
                new[] { 1, 1, 14, 10 }.CopyTo(packet.Parameters, 6);
                Run($"clip/coverage/ring op={op} origin={origin}", initial, new[] { packet });
            }
            // Empty/offscreen rectangles and unsupported operations leave state intact.
            var empty = Make(17, 13, 3); empty.Parameters[8] = 0;
            Run("empty clip", initial, new[] { empty });
            Run("offscreen", initial, new[] { Make(17, 13, 3, x: 17, y: 13) });
            Run("unknown op", initial, new[] { Make(17, 13, 99) });

            var sequence = new[] {
                Make(17, 13, 0, 5), Make(17, 13, 1, 5), Make(17, 13, 2, 5, holes: true),
                Make(17, 13, 3, 5, 19, 15, -1, -1, true), Make(17, 13, 3, 5),
                Make(17, 13, 0, 5, 3, 3, 4, 4), Make(17, 13, 3, 5)
            };
            Run("ordered overlapping writes and rectangle reset", initial, sequence);
            var alpha = Make(17, 13, 3, 5);
            Array.Fill(alpha.Source, Covered | 64u);
            Run("reset then AlphaShape twice then lit palette", initial,
                new[] { Make(17, 13, 0, 5), alpha, alpha }, drawConsumer: true);
            var batchArgument = OS.GetCmdlineUserArgs().FirstOrDefault(s => s.StartsWith("--lighting-batch="));
            if (batchArgument != null) RunBatchedFixture(batchArgument["--lighting-batch=".Length..]);
            GD.Print($"ABuffer GPU reference: {_cases} buffer comparisons, {_pixels} words, {Failures} failures");
        }
        finally
        {
            for (int i = _resources.Count - 1; i >= 0; --i) _rd.FreeRid(_resources[i]);
            _resources.Clear();
            _rd.Free();
        }
        return Task.CompletedTask;
    }
}
