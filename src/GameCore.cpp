# include <GameCore.h>

// Standard libraries
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <GL/glew.h>
#include <glfw3.h>

// GLSL Wrangler
#include <glsw/glsw.h>

#include <tools/Logger.hpp>
#include <tools/Timer.hpp>
#include <tools/imgui.h>
#include <tools/TCamera.h>

#include <imgui/imgui_impl_glfw_gl3.h>

// Breakpoints that should ALWAYS trigger (EVEN IN RELEASE BUILDS) [x86]!
#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else 
#include <signal.h>
#define DEBUG_BREAK raise(SIGTRAP)
	// __builtin_trap() 
#endif

using namespace gamecore;

__ImplementSubInterface(IGameApp, rtti::Interface)

namespace gamecore {
	void initialize(IGameApp& app, const std::string& name);
	void initWindow(IGameApp& app, const std::string& name);
	void initExtension(IGameApp& app);
	void initGL(IGameApp& app);

	void update(IGameApp& app);
	void updateHUD(IGameApp& app);
	void renderHUD(IGameApp& app);

	void glfw_error_callback(int error, const char* description);
    void glfw_keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void glfw_reshape_callback(GLFWwindow* window, int width, int height);
    void glfw_mouse_callback(GLFWwindow* window, int button, int action, int mods);
    void glfw_motion_callback(GLFWwindow* window, double xpos, double ypos);
	void glfw_char_callback(GLFWwindow* window, unsigned int c);
	void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	void glfw_framesize_callback(GLFWwindow* window, int width, int height);

    const uint32_t SHADOW_WIDTH = 1024;
    const uint32_t WINDOW_WIDTH = 1280;
    const uint32_t WINDOW_HEIGHT = 720;

	int32_t m_WindowWidth = 0;
	int32_t m_WindowHeight = 0;
	int32_t m_FrameWidth = 0;
	int32_t m_FrameHeight = 0;

    bool m_bWireframe = false;
	bool m_bCloseApp = false;

	GLFWwindow* m_Window = nullptr;  

	void APIENTRY OpenglCallbackFunction(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		using namespace std;

		// ignore these non-significant error codes
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204 || id == 131184) 
			return;

		cout << "---------------------opengl-callback-start------------" << endl;
		cout << "message: " << message << endl;
		cout << "type: ";
		switch(type) {
		case GL_DEBUG_TYPE_ERROR:
			cout << "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			cout << "DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			cout << "UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			cout << "PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			cout << "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER:
			cout << "OTHER";
			break;
		}
		cout << endl;

		cout << "id: " << id << endl;
		cout << "severity: ";
		switch(severity){
		case GL_DEBUG_SEVERITY_LOW:
			cout << "LOW";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			cout << "MEDIUM";
			break;
		case GL_DEBUG_SEVERITY_HIGH:
			cout << "HIGH";
			break;
		}
		cout << endl;
		cout << "---------------------opengl-callback-end--------------" << endl;
		if (type == GL_DEBUG_TYPE_ERROR)
			DEBUG_BREAK;
	}


	void initialize(IGameApp& app, const std::string& name)
	{
		// window maanger
		initWindow(app, name);

		// OpenGL extensions
		initExtension(app);

		// OpenGL
		initGL(app);

		// ImGui initialize without callbacks
		ImGui_ImplGlfwGL3_Init(m_Window, false);

		// Load FontsR
		// (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
		// ImGuiIO& io = ImGui::GetIO();
		// io.Fonts->AddFontDefault();
		// io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
		// io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
		// io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
		// io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);

		// GLSW : shader file manager
		glswInit();
		glswSetPath("./shaders/", ".glsl");
#ifdef __APPLE__
		glswAddDirectiveToken("*", "#version 330 core");
#else
		glswAddDirectiveToken("*", "#version 450 core");
#endif

		// to prevent osx input bug
		fflush(stdout);
	}

	void initExtension(IGameApp& app)
	{
		glewExperimental = GL_TRUE;

		GLenum result = glewInit(); 
		if (result != GLEW_OK)
		{
			fprintf( stderr, "Error: %s\n", glewGetErrorString(result));
			exit( EXIT_FAILURE );
		}

		fprintf( stderr, "GLEW version : %s\n", glewGetString(GLEW_VERSION));

#ifndef __APPLE__
		assert(GLEW_ARB_direct_state_access);
#endif
	}

	void initGL(IGameApp& app)
	{
		// Clear the error buffer (it starts with an error).
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			std::cerr << "OpenGL error: " << err << std::endl;
		}

		std::printf("%s\n%s\n", 
			glGetString(GL_RENDERER),  // e.g. Intel HD Graphics 3000 OpenGL Engine
			glGetString(GL_VERSION)    // e.g. 3.2 INTEL-8.0.61
		);

#if _DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		if (glDebugMessageCallback) {
			std::cout << "Register OpenGL debug callback " << std::endl;
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
			glDebugMessageCallback(OpenglCallbackFunction, nullptr);
		}
		else
		{
			std::cout << "glDebugMessageCallback not available" << std::endl;
		}
#endif

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glDisable(GL_STENCIL_TEST);
		glClearStencil(0);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		glDisable(GL_MULTISAMPLE);
	}

	void initWindow(IGameApp& app, const std::string& name)
	{
		glfwSetErrorCallback(glfw_error_callback);

		// Initialise GLFW
		if( !glfwInit() )
		{
			fprintf( stderr, "Failed to initialize GLFW\n" );
			exit( EXIT_FAILURE );
		}
		
#ifdef __APPLE__
		GLuint minor = 1;
#else
		GLuint minor = 5;
#endif

		glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef _DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		
		auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, name.c_str(), NULL, NULL );
		if ( window == NULL ) {
			fprintf( stderr, "Failed to open GLFW window\n" );
			glfwTerminate();
			exit( EXIT_FAILURE );
		}

		glfwSetWindowUserPointer(window, &app);
		glfwMakeContextCurrent(window);

		// GLFW Events' Callback
		glfwSetWindowSizeCallback(window, glfw_reshape_callback);
		glfwSetKeyCallback(window, glfw_keyboard_callback);
		glfwSetMouseButtonCallback(window, glfw_mouse_callback);
		glfwSetCursorPosCallback(window, glfw_motion_callback);
		glfwSetCharCallback(window, glfw_char_callback);
		glfwSetScrollCallback(window, glfw_scroll_callback);
		glfwSetFramebufferSizeCallback(window, glfw_framesize_callback);

		m_Window = window;
	}

	// GLFW Callbacks_________________________________________________  
	void glfw_error_callback(int error, const char* description)
	{
		fprintf(stderr, "Error: %s\n", description);
	}

	void glfw_reshape_callback(GLFWwindow* window, int width, int height)
	{
		m_WindowWidth = (int32_t)width;
		m_WindowHeight = (int32_t)height;

		auto app = static_cast<IGameApp*>(glfwGetWindowUserPointer(window));
		if (app == nullptr) return;
		app->reshapeCallback(width, height);
	}

	void glfw_keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods) 
	{
		ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);

		bool bPressed = action != GLFW_RELEASE;
		if (bPressed) 
		{
			switch (key)
			{
			case GLFW_KEY_ESCAPE:
				m_bCloseApp = true;
				break;

			case GLFW_KEY_W:
				m_bWireframe = !m_bWireframe;
				break;

			case GLFW_KEY_T:
				{
					Timer &timer = Timer::getInstance();
					printf( "fps : %d [%.3f ms]\n", timer.getFPS(), timer.getElapsedTime());
				}
				break;

			default:
				break;
			}
		}

		auto app = static_cast<IGameApp*>(glfwGetWindowUserPointer(window));
		if (app == nullptr) return;
		app->keyboardCallback(key, bPressed);
	}

	void glfw_motion_callback(GLFWwindow* window, double xpos, double ypos)
	{
		int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		const bool bPressed = (state == GLFW_PRESS);

		auto app = static_cast<IGameApp*>(glfwGetWindowUserPointer(window));
		if (app == nullptr) return;
		app->motionCallback((float)xpos, (float)ypos, bPressed);
	}  

	void glfw_mouse_callback(GLFWwindow* window, int button, int action, int mods)
	{
		ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);

		double xpos = 0.0, ypos = 0.0;
		glfwGetCursorPos(window, &xpos, &ypos);

		auto app = static_cast<IGameApp*>(glfwGetWindowUserPointer(window));
		if (app == nullptr) return;
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			app->mouseCallback((float)xpos, (float)ypos, action == GLFW_PRESS);
	}

	void glfw_char_callback(GLFWwindow* window, unsigned int c)
	{
		ImGui_ImplGlfwGL3_CharCallback(window, c);

		auto app = static_cast<IGameApp*>(glfwGetWindowUserPointer(window));
		if (app == nullptr) return;
		app->charCallback(c);
	}

	void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);

		auto app = static_cast<IGameApp*>(glfwGetWindowUserPointer(window));
		if (app == nullptr) return;
		app->scrollCallback((float)xoffset, (float)yoffset);
	}

	void glfw_framesize_callback(GLFWwindow* window, int width, int height)
	{
		m_FrameWidth = (int32_t)width;
		m_FrameHeight = (int32_t)height;

		auto app = static_cast<IGameApp*>(glfwGetWindowUserPointer(window));
		if (app == nullptr) return;
		app->framesizeCallback(width, height);
	}
}

IGameApp::IGameApp() noexcept
{
}

IGameApp::~IGameApp() noexcept
{
}

void IGameApp::charCallback(uint32_t c) noexcept
{
}

void IGameApp::keyboardCallback(uint32_t c, bool bPressed) noexcept
{
}

void IGameApp::reshapeCallback(int32_t width, int32_t height) noexcept
{
}

void IGameApp::motionCallback(float xpos, float ypos, bool bPressed) noexcept
{
}

void IGameApp::mouseCallback(float xpos, float ypos, bool bPressed) noexcept
{
}

void IGameApp::framesizeCallback(int32_t width, int32_t height) noexcept
{
}

void IGameApp::scrollCallback(float xoffset, float yoffset) noexcept
{
}

bool IGameApp::isDone() const noexcept
{
	return !m_bCloseApp && glfwWindowShouldClose(m_Window) == 0;
}

void IGameApp::update() noexcept
{
}

void IGameApp::updateHUD() noexcept
{
}

void IGameApp::render() noexcept
{
}

void IGameApp::renderHUD() noexcept
{
}

bool IGameApp::isWireframe() const noexcept
{
	return m_bWireframe;
}

int32_t IGameApp::getWindowWidth() const noexcept
{
	return m_WindowWidth;
}

int32_t IGameApp::getWindowHeight() const noexcept
{
	return m_WindowHeight;
}

int32_t IGameApp::getFrameWidth() const noexcept
{
	return m_FrameWidth;
}

int32_t IGameApp::getFrameHeight() const noexcept
{
	return m_FrameHeight;
}

void gamecore::update(IGameApp& app)
{
	Timer::getInstance().update();
	app.update();
}

void gamecore::updateHUD(IGameApp& app)
{
	ImGui_ImplGlfwGL3_NewFrame();
	app.updateHUD();
}

bool gamecore::updateApplication(IGameApp& app)
{
	app.update();
	updateHUD(app);
	app.render();
	app.renderHUD();
	renderHUD(app);

	/* Swap front and back buffers */
	glfwSwapBuffers(m_Window);

	/* Poll for and process events */
	glfwPollEvents();

	return app.isDone();
}

void gamecore::runApplication(IGameApp& app, std::string name)
{
	Timer::getInstance().start();

	initialize(app, name);

	app.startup();

    glfwGetFramebufferSize(m_Window, &m_FrameWidth, &m_FrameHeight);
    glfw_framesize_callback(m_Window, m_FrameWidth, m_FrameHeight);
    glfw_reshape_callback(m_Window, WINDOW_WIDTH, WINDOW_HEIGHT);

	do {
	}
	while (updateApplication(app));

	terminateApplication(app);
}

void gamecore::terminateApplication(IGameApp& app)
{
	ImGui_ImplGlfwGL3_Shutdown();
	glswShutdown();  
	Logger::getInstance().close();
	glfwTerminate();
}

void gamecore::renderHUD(IGameApp& app)
{
	// restore some state
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	ImGui::Render();
	ImGui::EndFrame();
	app.renderHUD();
}
