
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
    out.normal = in.normal;
	out.color = in.color;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

	let normal = normalize(in.normal);

	return vec4f(normal, 1);
}

