// Include the C++ wrapper instead of the raw header(s)
#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif // __EMSCRIPTEN__

#include <vector>
#include <thread>
#include <atomic>
#include <map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "./editorClient.hpp"
#include "./loader.hpp"
#include "./input.hpp"
// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/gtx/string_cast.hpp>

enum
{
	max_length = 1024
};

struct FrameUniforms
{
	glm::mat3x4 m; // at byte offset 64
	mat4x4 mvp;	   // at byte offset 0
	float time;	   // at byte offset 112
	float _pad0[3];
};

// Avoid the "wgpu::" prefix in front of all WebGPU symbols
using namespace wgpu;

class ShaderManager
{
public:
	ShaderManager(std::string shaderRootPath)
	{
		rootPath = shaderRootPath;
		for (const auto &entry : std::filesystem::recursive_directory_iterator(shaderRootPath))
		{
			const auto path = entry.path().string();
			UpdateShader(path);
		}
	}

	void UpdateShader(std::string shaderLocation)
	{
		const auto relativePath = shaderLocation.substr(
			rootPath.length() + 1, std::string::npos);

		std::ifstream file(shaderLocation);
		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		std::string shaderSource(size, ' ');
		file.seekg(0);
		file.read(shaderSource.data(), size);

		shaderMap[relativePath] = shaderSource;
	}

	std::string GetShader(std::string shaderName)
	{
		return shaderMap[shaderName];
	}

private:
	std::map<std::string, std::string> shaderMap;
	std::string rootPath;
};

class Application
{

public:
	void Resize(int width, int height);
	// Initialize everything and return true if it went all right
	bool Initialize();

	void Repair();

	// Uninitialize everything that was initialized
	void Terminate();

	// Draw a frame and handle events
	void MainLoop();
	void Tick();
	void LateTick();
	void Render();

	// Return true as long as the main loop should keep on running
	bool IsRunning();

	void InitializeLayouts();

	void InitializePipeline();

	void InitializeBindGroupsAndBuffers();

	void OnMouseMove(double xpos, double ypos);

	void UpdateCameraDirection(float xpos, float ypos);

	RequiredLimits GetRequiredLimits(Adapter adapter) const;

	ShaderManager *shaderManager;
	bool crashed;
	Input input;

private:
	Texture GetNextSurfaceTexture();
	TextureView GetNextSurfaceTextureView(Texture texture);

private:
	GLFWwindow *window;
	Device device;
	Queue queue;
	Surface surface;
	std::unique_ptr<ErrorCallback> uncapturedErrorCallbackHandle;
	TextureFormat surfaceFormat = TextureFormat::Undefined;
	RenderPipeline pipeline;
	SurfaceConfiguration config;
	Buffer frameUniformBuffer;
	Buffer sporadicUniformBuffer;
	Buffer vertexBuffer;
	uint64_t vertexBufferSize;
	PipelineLayout layout;
	BindGroupLayout frameBindGroupLayout;
	BindGroupLayout sporadicBindGroupLayout;
	BindGroup frameBindGroup;
	BindGroup sporadicBindGroup;
	int vertexCount;
	Texture depthTexture;
	TextureView depthTextureView;
	TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
	mat4x4 projectionMatrix;
	mat4x4 viewMatrix = mat4x4(1.0);
	float oldMouseX = 0;
	float oldMouseY = 0;
	float cameraYaw = 0;
	float cameraPitch = 0;
};

int main()
{

	try
	{
		asio::io_context io_context;

		Application app;

		Client client(io_context, "127.0.0.1", "43957");

		if (!app.Initialize())
		{
			return 1;
		}

		client.BindOnMessage([&app](auto str)
							 {
			app.shaderManager->UpdateShader(str);

			if(app.crashed) {
				app.Repair();
			} else {
				app.InitializePipeline();
			} });

#ifdef __EMSCRIPTEN__
		// Equivalent of the main loop when using Emscripten:
		auto callback = [](void *arg)
		{
			Application *pApp = reinterpret_cast<Application *>(arg);
			pApp->MainLoop(); // 4. We can use the application object
		};
		emscripten_set_main_loop_arg(callback, &app, 0, true);
#else  // __EMSCRIPTEN__
		while (app.IsRunning())
		{
			app.MainLoop();
			io_context.poll();
		}
#endif // __EMSCRIPTEN__

		app.Terminate();
		io_context.stop();
	}
	catch (std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}

void Application::Resize(int newWidth, int newHeight)
{
	if (newWidth == 0 || newWidth == 0)
	{
		return;
	}

	config.width = newWidth;
	config.height = newHeight;

	std::vector<uint32_t> res = {(uint32_t)newWidth, (uint32_t)newHeight};

	if (depthTexture)
	{
		depthTexture.destroy();
		depthTexture.release();
	}

	if (depthTextureView)
	{
		depthTextureView.release();
	}

	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::_2D;
	depthTextureDesc.format = depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 1;
	depthTextureDesc.size = {res[0], res[1], 1};
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (WGPUTextureFormat *)&depthTextureFormat;
	depthTexture = device.createTexture(depthTextureDesc);
	std::cout << "Depth texture: " << depthTexture << std::endl;

	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::_2D;
	depthTextureViewDesc.format = depthTextureFormat;
	depthTextureView = depthTexture.createView(depthTextureViewDesc);
	std::cout << "Depth texture view: " << depthTextureView << std::endl;

	float ratio = static_cast<float>(newWidth) / static_cast<float>(newHeight);
	float focalLength = 2.0;
	float nearr = 0.01f;
	float farr = 100.0f;
	float divider = 1 / (focalLength * (farr - nearr));
	projectionMatrix = transpose(mat4x4(
		1.0, 0.0, 0.0, 0.0,
		0.0, ratio, 0.0, 0.0,
		0.0, 0.0, farr * divider, -farr * nearr * divider,
		0.0, 0.0, 1.0 / focalLength, 0.0));

	queue.writeBuffer(sporadicUniformBuffer, 0, res.data(), 8);

	surface.configure(config);

	MainLoop();
}

bool Application::Initialize()
{

	shaderManager = new ShaderManager("../../shaders");

	auto width = 640 * 2;
	auto height = width / 2;
	// Open window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(width, height, "Learn WebGPU", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);

	input = Input(window);

	glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int, int action, int)
					   { static_cast<Application *>(glfwGetWindowUserPointer(window))->input.OnKey(key, action); });

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height)
								   { static_cast<Application *>(glfwGetWindowUserPointer(window))->Resize(width, height); });

	glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos)
							 { static_cast<Application *>(glfwGetWindowUserPointer(window))->OnMouseMove(xpos, ypos); });

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	oldMouseX = static_cast<float>(xpos);
	oldMouseY = static_cast<float>(ypos);

	Instance instance = wgpuCreateInstance(nullptr);
	surface = glfwGetWGPUSurface(instance, window);

	std::cout << "Requesting adapter..." << std::endl;

	RequestAdapterOptions adapterOpts = {};
	adapterOpts.compatibleSurface = surface;
	Adapter adapter = instance.requestAdapter(adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	instance.release();

	std::cout << "Requesting device..." << std::endl;
	DeviceDescriptor deviceDesc = {};
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeatureCount = 0;
	RequiredLimits requiredLimits = GetRequiredLimits(adapter);
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const *message, void * /* pUserData */)
	{
		std::cout << "Device lost: reason " << reason;
		if (message)
			std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	device = adapter.requestDevice(deviceDesc);
	std::cout << "Got device: " << device << std::endl;
	uncapturedErrorCallbackHandle = device.setUncapturedErrorCallback([](ErrorType type, char const *message)
																	  {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl; });

	queue = device.getQueue();

	// Configuration of the textures created for the underlying swap chain
	config.width = width;
	config.height = height;
	config.usage = TextureUsage::RenderAttachment;
	surfaceFormat = surface.getPreferredFormat(adapter);
	config.format = surfaceFormat;

	// And we do not need any particular view format:
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = PresentMode::Fifo;
	config.alphaMode = CompositeAlphaMode::Auto;

	InitializeLayouts();
	InitializePipeline();
	InitializeBindGroupsAndBuffers();

	Resize(width, height);
	adapter.release();
	return true;
}

RequiredLimits Application::GetRequiredLimits(Adapter adapter) const
{
	// Get adapter supported limits, in case we need them
	SupportedLimits supportedLimits;
	adapter.getLimits(&supportedLimits);

	// Don't forget to = Default
	RequiredLimits requiredLimits = Default;

	// We use at most 2 vertex attributes
	requiredLimits.limits.maxVertexAttributes = 4;
	// We should also tell that we use 1 vertex buffers
	requiredLimits.limits.maxVertexBuffers = 1;
	// Maximum size of a buffer is 15 vertices of 5 float each
	requiredLimits.limits.maxBufferSize = 10000 * sizeof(Loader::VertexAttributes);
	requiredLimits.limits.maxUniformBufferBindingSize = 144;

	requiredLimits.limits.maxVertexBufferArrayStride = sizeof(Loader::VertexAttributes);

	requiredLimits.limits.maxInterStageShaderComponents = 6;
	requiredLimits.limits.maxBindGroups = 2;
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;

	// These two limits are different because they are "minimum" limits,
	// they are the only ones we are may forward from the adapter's supported
	// limits.
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
	return requiredLimits;
}

void Application::Repair()
{
	surface.configure(config);
	InitializePipeline();
	crashed = false;
}

void Application::Terminate()
{
	pipeline.release();
	queue.release();
	surface.release();
	device.release();
	sporadicBindGroup.release();
	frameBindGroup.release();
	vertexBuffer.release();
	depthTextureView.release();
	depthTexture.release();

	delete shaderManager;
	glfwTerminate();
}

void Application::Tick()
{
	glfwPollEvents();

	if(input.IsDown("forward")){
		std::cout << "forward" << std::endl;
	}

}

void Application::LateTick()
{
	input.EndFrame();
}

void Application::Render()
{
	mat4x4 S = glm::scale(mat4x4(1.0), vec3(1.0f));
	mat4x4 T1 = glm::translate(mat4x4(1.0), vec3(0.0, 0.0, 0.0));
	mat4x4 R0 = glm::rotate(mat4x4(1.0), glm::mod(-static_cast<float>(glfwGetTime()), glm::two_pi<float>()), vec3(0.0, 1.0, 0.0));
	mat4x4 R1 = glm::rotate(mat4x4(1.0), -glm::half_pi<float>(), vec3(1.0, 0.0, 0.0));
	mat4x4 modelMatrix = T1 * R0 * S;
	glm::mat3x4 normalMatrix = glm::mat3x4(glm::inverseTranspose(modelMatrix));

	FrameUniforms uniforms = {normalMatrix, projectionMatrix * viewMatrix * modelMatrix, static_cast<float>(glfwGetTime())};

	queue.writeBuffer(frameUniformBuffer, 0, &uniforms, sizeof(FrameUniforms));

	// Get the next target texture view
	Texture target = GetNextSurfaceTexture();
	TextureView targetView = GetNextSurfaceTextureView(target);

	if (!targetView)
	{
		crashed = true;
		return;
	}

	// Create a command encoder for the draw call
	CommandEncoderDescriptor encoderDesc = {};
	encoderDesc.label = "My command encoder";
	CommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	// Create the render pass that clears the screen with our color
	RenderPassDescriptor renderPassDesc = {};

	// The attachment part of the render pass descriptor describes the target texture of the pass
	RenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = LoadOp::Clear;
	renderPassColorAttachment.storeOp = StoreOp::Store;
	renderPassColorAttachment.clearValue = WGPUColor{0.4, 0.1, 0.2, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	{
		// We now add a depth/stencil attachment:
		RenderPassDepthStencilAttachment depthStencilAttachment;
		// The view of the depth texture
		depthStencilAttachment.view = depthTextureView;

		// The initial value of the depth buffer, meaning "far"
		depthStencilAttachment.depthClearValue = 1.0f;
		// Operation settings comparable to the color attachment
		depthStencilAttachment.depthLoadOp = LoadOp::Clear;
		depthStencilAttachment.depthStoreOp = StoreOp::Store;
		// we could turn off writing to the depth buffer globally here
		depthStencilAttachment.depthReadOnly = false;

		// Stencil setup, mandatory but unused
		depthStencilAttachment.stencilClearValue = 0;
#ifdef WEBGPU_BACKEND_WGPU
		depthStencilAttachment.stencilLoadOp = LoadOp::Clear;
		depthStencilAttachment.stencilStoreOp = StoreOp::Store;
#else
		depthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
		depthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
#endif
		depthStencilAttachment.stencilReadOnly = true;

		renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
	}

	renderPassDesc.timestampWrites = nullptr;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	// Select which render pipeline to use
	renderPass.setPipeline(pipeline);
	renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexBufferSize);
	renderPass.setBindGroup(0, frameBindGroup, 0, nullptr);
	renderPass.setBindGroup(1, sporadicBindGroup, 0, nullptr);

	// Draw 1 instance of a 3-vertices shape
	renderPass.draw(vertexCount, 1, 0, 0);

	renderPass.end();
	renderPass.release();

	// Finally encode and submit the render pass
	CommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.finish(cmdBufferDescriptor);
	encoder.release();

	queue.submit(1, &command);
	command.release();

	// At the enc of the frame
#ifndef __EMSCRIPTEN__
	surface.present();
#endif

	targetView.release();
	target.release();

#if defined(WEBGPU_BACKEND_DAWN)
	device.tick();
#elif defined(WEBGPU_BACKEND_WGPU)
	device.poll(false);
#endif
}

void Application::MainLoop()
{
	Tick();
	Render();
	LateTick();
}

bool Application::IsRunning()
{
	return !glfwWindowShouldClose(window);
}

Texture Application::GetNextSurfaceTexture()
{

	SurfaceTexture surfaceTexture;
	surface.getCurrentTexture(&surfaceTexture);
	if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success)
	{
		return nullptr;
	}

	return surfaceTexture.texture;
}

TextureView Application::GetNextSurfaceTextureView(Texture texture)
{
	// Create a view for this surface texture
	TextureViewDescriptor viewDescriptor;
	viewDescriptor.label = "Surface texture view";
	viewDescriptor.format = texture.getFormat();
	viewDescriptor.dimension = TextureViewDimension::_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = TextureAspect::All;
	TextureView targetView = texture.createView(viewDescriptor);

	return targetView;
}

void Application::InitializeLayouts()
{

	BindGroupLayoutEntry bindingLayout = Default;

	// The binding index as used in the @binding attribute in the shader
	bindingLayout.binding = 0;
	// The stage that needs to access this resource
	bindingLayout.visibility = ShaderStage::Fragment | ShaderStage::Vertex;
	bindingLayout.buffer.type = BufferBindingType::Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(FrameUniforms);

	// Create a bind group layout
	BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayout;
	frameBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	bindingLayout.buffer.minBindingSize = 4 * 2;
	bindingLayout.visibility = ShaderStage::Fragment | ShaderStage::Vertex;

	sporadicBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

	std::vector<WGPUBindGroupLayout> layouts = {frameBindGroupLayout, sporadicBindGroupLayout};

	// Create the pipeline layout
	PipelineLayoutDescriptor pipelineLayoutDesc{};
	pipelineLayoutDesc.nextInChain = nullptr;
	pipelineLayoutDesc.bindGroupLayoutCount = layouts.size();
	pipelineLayoutDesc.bindGroupLayouts = layouts.data();

	layout = device.createPipelineLayout(pipelineLayoutDesc);
}

void Application::InitializePipeline()
{

	if (pipeline)
	{
		pipeline.release();
		pipeline = nullptr;
	}

	ShaderModuleDescriptor shaderDesc;

#ifdef WEBGPU_BACKEND_WGPU
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
#endif

	// We use the extension mechanism to specify the WGSL part of the shader module descriptor
	ShaderModuleWGSLDescriptor shaderCodeDesc{};
	// Set the chained struct's header
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
	// Connect the chain
	shaderDesc.nextInChain = &shaderCodeDesc.chain;

	const auto str = shaderManager->GetShader("model.wgsl");
	shaderCodeDesc.code = &str[0];
	ShaderModule shaderModule = device.createShaderModule(shaderDesc);

	RenderPipelineDescriptor pipelineDesc;

	std::vector<VertexAttribute> vertexAttribs(3);

	// Position
	vertexAttribs[0].shaderLocation = 0;
	vertexAttribs[0].format = VertexFormat::Float32x3;
	vertexAttribs[0].offset = 0;

	// Normal
	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = VertexFormat::Float32x3;
	vertexAttribs[1].offset = offsetof(Loader::VertexAttributes, normal);

	// Color
	vertexAttribs[2].shaderLocation = 2;
	vertexAttribs[2].format = VertexFormat::Float32x3;
	vertexAttribs[2].offset = offsetof(Loader::VertexAttributes, color);

	VertexBufferLayout vertexBufferLayout;
	vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
	vertexBufferLayout.attributes = vertexAttribs.data();
	vertexBufferLayout.arrayStride = sizeof(Loader::VertexAttributes);
	vertexBufferLayout.stepMode = VertexStepMode::Vertex;

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	pipelineDesc.primitive.cullMode = CullMode::Back;

	FragmentState fragmentState;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	BlendState blendState;
	blendState.color.srcFactor = BlendFactor::SrcAlpha;
	blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
	blendState.color.operation = BlendOperation::Add;
	blendState.alpha.srcFactor = BlendFactor::Zero;
	blendState.alpha.dstFactor = BlendFactor::One;
	blendState.alpha.operation = BlendOperation::Add;

	ColorTargetState colorTarget;
	colorTarget.format = surfaceFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.

	// We have only one target because our render pass has only one output color
	// attachment.
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;

	// We setup a depth buffer state for the render pipeline
	DepthStencilState depthStencilState = Default;
	// Keep a fragment only if its depth is lower than the previously blended one
	depthStencilState.depthCompare = CompareFunction::Less;
	// Each time a fragment is blended into the target, we update the value of the Z-buffer
	depthStencilState.depthWriteEnabled = true;
	// Store the format in a variable as later parts of the code depend on it

	depthStencilState.format = depthTextureFormat;
	// Deactivate the stencil alltogether
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

	pipelineDesc.depthStencil = &depthStencilState;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	pipelineDesc.layout = layout;

	pipeline = device.createRenderPipeline(pipelineDesc);

	shaderModule.release();
}

void Application::InitializeBindGroupsAndBuffers()
{
	std::vector<Loader::VertexAttributes> vertexData;
	Loader::LoadGeometryFromObj("resources/meshes/circle.obj", vertexData);

	{
		BufferDescriptor bufferDesc;
		bufferDesc.size = vertexData.size() * sizeof(Loader::VertexAttributes);
		bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
		bufferDesc.mappedAtCreation = false;
		vertexBuffer = device.createBuffer(bufferDesc);
		vertexBufferSize = bufferDesc.size;
		vertexCount = static_cast<int>(vertexData.size());
		queue.writeBuffer(vertexBuffer, 0, vertexData.data(), bufferDesc.size);
	}

	{
		BufferDescriptor bufferDesc;
		bufferDesc.size = sizeof(FrameUniforms);
		bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
		bufferDesc.mappedAtCreation = false;
		frameUniformBuffer = device.createBuffer(bufferDesc);
	}

	{
		BufferDescriptor bufferDesc;
		bufferDesc.size = 2 * 4;
		bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
		bufferDesc.mappedAtCreation = false;
		sporadicUniformBuffer = device.createBuffer(bufferDesc);
	}

	BindGroupEntry binding{};

	binding.binding = 0;
	// The buffer it is actually bound to
	binding.buffer = frameUniformBuffer;
	// We can specify an offset within the buffer, so that a single buffer can hold
	// multiple uniform blocks.
	binding.offset = 0;
	// And we specify again the size of the buffer.
	binding.size = sizeof(FrameUniforms);

	BindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.layout = frameBindGroupLayout;
	// There must be as many bindings as declared in the layout!
	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &binding;

	frameBindGroup = device.createBindGroup(bindGroupDesc);

	binding.buffer = sporadicUniformBuffer;
	binding.size = 4 * 2;
	bindGroupDesc.layout = sporadicBindGroupLayout;

	sporadicBindGroup = device.createBindGroup(bindGroupDesc);
}

void Application::OnMouseMove(double xpos, double ypos)
{
	UpdateCameraDirection(static_cast<float>(xpos), static_cast<float>(ypos));
}

void Application::UpdateCameraDirection(float xpos, float ypos)
{
	float s = 0.005f;

	float offsetX = (xpos - oldMouseX) * s;
	float offsetY = (ypos - oldMouseY) * s;

	oldMouseX = xpos;
	oldMouseY = ypos;

	cameraYaw += offsetX;
	cameraPitch -= offsetY;

	cameraPitch = glm::clamp(cameraPitch, -glm::radians<float>(89), glm::radians<float>(89));

	vec3 direction;
	direction.x = sin(cameraYaw) * cos(cameraPitch);
	direction.y = sin(cameraPitch);
	direction.z = cos(cameraYaw) * cos(cameraPitch);
	vec3 front = glm::normalize(direction);
	vec3 right = glm::normalize(glm::cross(front, vec3(0.0, 1.0, 0.0))); // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	vec3 up = glm::normalize(glm::cross(right, front));
	viewMatrix = glm::lookAt(glm::vec3(0), front, up);

	std::cout << cameraPitch << std::endl;
}
