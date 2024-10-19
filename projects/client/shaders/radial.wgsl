const TAU = 6.28318;


@group(0) @binding(0) var<uniform> time: f32;
@group(1) @binding(0) var<uniform> iRes: vec2u;

@fragment
fn fs_main(@location(0) fragUV: vec2<f32>) -> @location(0) vec4f {

    let uv = fragUV - 0.5;
    let distance = 0.5;
    let r = abs(length(uv) - distance);
    let a = fract(atan2(uv.y, uv.x) / TAU);

    let c = r;
    return vec4f(fract(time), c, c, 1);
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) fragUV: vec2<f32>,
};

@vertex
fn vs_main(
    @builtin(vertex_index) VertexIndex: u32
) -> VertexOutput {
    var pos = array(
        vec2(-1.0, 3),
        vec2(3, -1.0),
        vec2(-1.0, -1.0)
    );

    var uv = array(
        vec2(0.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0),
    );

    var output: VertexOutput;
    output.position = vec4(pos[VertexIndex], 0.0, 1.0);
    output.fragUV = uv[VertexIndex] * 2.0;
    return output;
}