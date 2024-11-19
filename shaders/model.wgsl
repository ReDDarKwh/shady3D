
struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f,
	@location(2) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) normal: vec3f, 
	@location(1) color: vec3f,
};

struct FrameUniforms {
	m: mat3x3f,
    mvp: mat4x4f,
    time: f32,
};

@group(0) @binding(0) var<uniform> frame: FrameUniforms;
@group(1) @binding(0) var<uniform> iRes: vec2u;

@vertex
fn vs_main(
    in: VertexInput
) -> VertexOutput {

    var out: VertexOutput;
    out.position = frame.mvp * vec4f(in.position, 1.0);
    out.normal = frame.m * in.normal;
    out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    let n = normalize(in.normal);

    let lightColor1 = vec3f(1, 0.9, 0.6);
    let lightDirection1 = vec3f(-1.0, 0.0, 0.0);
    let shading1 = max(0.0, dot(n, -lightDirection1));
    let shading = shading1 * lightColor1;
    let color = shading;

	// Gamma-correction
    let corrected_color = pow(color, vec3f(2.2));
    return vec4f(corrected_color, 1);
}

