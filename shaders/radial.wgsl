const TAU = 6.28318;
const PI = TAU / 2.0;


@group(0) @binding(0) var<uniform> time: f32;
@group(1) @binding(0) var<uniform> iRes: vec2u;

@fragment
fn fs_main(@location(0) fragUV: vec2<f32>) -> @location(0) vec4f {

    let center = vec2f(iRes) * 0.5;
    let uv = center - fragUV;
    let a = fract(atan2(uv.y, uv.x) / (TAU));
    let p = (cos(a * TAU * 7 + time) + 1) / 2.0;
    let c = pow(saturate((cos(((length(uv) + time * -100) - mix(200.0, 300.0, pow(p, 1.0))) * 0.02) + 1) / 2), 0.5);

    return vec4f(0.1 + (0.7 * (pow((cos(a * TAU * 7 + time * 1) + 1), 0.3)) / 2) * c, 0.01, saturate((sin(length(uv) * 0.01 - time * 4) + 1) / 2) * 1 - c, 1);
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
        vec2(0.0, f32(iRes.y)),
        vec2(f32(iRes.x), 0.0),
        vec2(0.0, 0.0),
    );

    var output: VertexOutput;
    output.position = vec4(pos[VertexIndex], 0.0, 1.0);
    output.fragUV = uv[VertexIndex] * 2.0;
    return output;
}