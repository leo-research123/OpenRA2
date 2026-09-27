using Godot;
using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

// Uses the production shader and C++ packets. Expected pixels/Z words were
// produced by fixed gamemd.exe instructions, not a second shader implementation.
public partial class TestShapeGpuReference : TestRunner
{
    private static byte[] ExpandWords(byte[] input, bool color)
    {
        var output = new byte[input.Length * 2];
        for (int i = 0; i < input.Length / 2; i++)
        {
            uint value = BitConverter.ToUInt16(input, i * 2);
            if (color)
            {
                uint r = (value >> 11) & 31, g = (value >> 5) & 63, b = value & 31;
                // EXE fixtures are RGB565. Independently normalize their
                // channels for exact comparison with the RGBA8 GPU buffer.
                value = (uint)Math.Round(r * 255.0 / 31.0)
                    | ((uint)Math.Round(g * 255.0 / 63.0) << 8)
                    | ((uint)Math.Round(b * 255.0 / 31.0) << 16) | 0xFF000000u;
            }
            BitConverter.GetBytes(value).CopyTo(output, i * 4);
        }
        return output;
    }

    protected override Task RunTestAsync()
    {
        var fixtures = OS.GetCmdlineUserArgs().Where(s => s.StartsWith("--packets=")).ToArray();
        if (fixtures.Length == 0) throw new ArgumentException("At least one --packets file is required");
        foreach (var argument in fixtures)
            CheckFixture(argument["--packets=".Length..]);
        return Task.CompletedTask;
    }

    private void CheckFixture(string path)
    {
        var tokens = File.ReadAllText(path).Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries);
        int cursor = 0;
        string Next() => tokens[cursor++];
        byte[] Bytes() => Convert.FromHexString(Next());
        Expect(Next() == "SHP_GPU_PACKETS_V1", "Unsupported pixel fixture format");
        int width = int.Parse(Next()), height = int.Parse(Next());
        var initial = ExpandWords(Bytes(), true);
        var initialZ = ExpandWords(Bytes(), false);
        var light = ExpandWords(Bytes(), false);
        var palette = ExpandWords(Bytes(), true);
        int count = int.Parse(Next());
        using var rd = RenderingServer.CreateLocalRenderingDevice();
        if (rd == null) throw new InvalidOperationException("This test requires a real RenderingDevice");
        // Match RA2MapRenderer::create_pipeline. Compiling loose GLSL here hid
        // stale imported SPIR-V: the test passed while the game drew black sparks.
        using var source = GD.Load<RDShaderFile>("res://shaders/map_draw.glsl");
        using var spirv = source.GetSpirV();
        var compileError = spirv.GetStageCompileError(RenderingDevice.ShaderStage.Compute);
        if (compileError.Length != 0) throw new InvalidOperationException(compileError);
        var shader = rd.ShaderCreateFromSpirV(spirv);
        var pipeline = rd.ComputePipelineCreate(shader);
        try
        {
            for (int c = 0; c < count; c++)
            {
                string name = Next();
                var parameters = Bytes(); var texels = Bytes();
                var expected = ExpandWords(Bytes(), true); var expectedZ = ExpandWords(Bytes(), false);
                var bytes = new[] { parameters, texels, initial, initialZ, light, palette };
                var buffers = bytes.Select(b => rd.StorageBufferCreate((uint)b.Length, b)).ToArray();
                var uniforms = new Godot.Collections.Array<RDUniform>();
                for (int binding = 0; binding < buffers.Length; binding++)
                {
                    var uniform = new RDUniform { UniformType = RenderingDevice.UniformType.StorageBuffer, Binding = binding };
                    uniform.AddId(buffers[binding]); uniforms.Add(uniform);
                }
                var set = rd.UniformSetCreate(uniforms, shader, 0);
                try
                {
                    long list = rd.ComputeListBegin();
                    rd.ComputeListBindComputePipeline(list, pipeline);
                    rd.ComputeListBindUniformSet(list, set, 0);
                    rd.ComputeListSetPushConstant(list, new byte[4], 4);
                    rd.ComputeListDispatch(list, (uint)(BitConverter.ToInt32(parameters, 8) + 7) / 8,
                        (uint)(BitConverter.ToInt32(parameters, 12) + 7) / 8, 1);
                    rd.ComputeListEnd(); rd.Submit(); rd.Sync();
                    var actual = rd.BufferGetData(buffers[2]);
                    var capture = OS.GetCmdlineUserArgs().FirstOrDefault(s => s.StartsWith("--capture-dir="));
                    if (capture != null) File.WriteAllBytes(Path.Combine(capture[14..], name + ".rgba"), actual);
                    CheckPixels(name, "color", actual, expected, width);
                    CheckPixels(name, "depth", rd.BufferGetData(buffers[3]), expectedZ, width);
                }
                finally
                {
                    rd.FreeRid(set);
                    foreach (var buffer in buffers) rd.FreeRid(buffer);
                    foreach (var uniform in uniforms) uniform.Dispose();
                }
            }
        }
        finally { rd.FreeRid(pipeline); rd.FreeRid(shader); rd.Free(); }
        GD.Print($"Original SHP differential: {count} cases, {count * width * height} pixels and Z words, {Failures} failures");
    }

    private void CheckPixels(string name, string kind, byte[] actual, byte[] expected, int width)
    {
        if (actual.SequenceEqual(expected)) return;
        int i = Enumerable.Range(0, Math.Min(actual.Length, expected.Length)).FirstOrDefault(i => actual[i] != expected[i]);
        int pixel = i / 4;
        int different = Enumerable.Range(0, Math.Min(actual.Length, expected.Length) / 4)
            .Count(p => BitConverter.ToUInt32(actual, p * 4) != BitConverter.ToUInt32(expected, p * 4));
        Expect(false, $"{name}: {kind} has {different} different pixels; first at ({pixel % width},{pixel / width}), "
            + $"GPU=0x{BitConverter.ToUInt32(actual, pixel * 4):X8}, EXE=0x{BitConverter.ToUInt32(expected, pixel * 4):X8}");
    }
}
