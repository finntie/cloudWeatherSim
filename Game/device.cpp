#include "device.h"


#include <glad/include/glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>

#include <cloudWeatherSim/cloud_weather_sim.hpp>


// GLFW error callback
void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

bool changeWindowSize{ false };
void keyCallbackDevice(GLFWwindow* win, int key, int scan, int action, int mods)
{
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
	{
		changeWindowSize = true;
	}
}

device::device()
{
	if (!glfwInit())
	{
		printf("Failed to load glfw\n");
		return;
	}

	glfwSetErrorCallback(error_callback);

	// Create window
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(_DEBUG)
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	

#if defined(_DEBUG)
	m_window = glfwCreateWindow(1960, 1080, "CloudWeatherSimulation Example", NULL, NULL);
#else
	//m_window = glfwCreateWindow(1960, 1080, "CloudWeatherSimulation Example", NULL, NULL); // Windowed
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	//glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE); // Disable minimizing when unfocusing.
	m_window = glfwCreateWindow(mode->width, mode->height, "CloudWeatherSimulation Example", glfwGetPrimaryMonitor(), NULL);
#endif
	if (!m_window)
	{
		printf("Failed to create a window\n");
		return;
	}
	glfwMakeContextCurrent(m_window);
	glfwSetKeyCallback(m_window, keyCallbackDevice);

	// Init glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to load GLAD\n");
		return;
	}
	glfwSwapInterval(1);
}

device::~device()
{
	glfwTerminate();
	glfwDestroyWindow(m_window);
}

void device::renderInit()
{
	// Change window if F11 was pressed
	if (changeWindowSize)
	{
		GLFWmonitor* monitor = glfwGetWindowMonitor(m_window);
		if (!monitor)
		{
			GLFWmonitor* prim = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(prim);
			glfwSetWindowMonitor(m_window, prim, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else
		{
			glfwSetWindowMonitor(m_window, nullptr, 100, 100, 1960, 1080, GLFW_DONT_CARE);
		}
		changeWindowSize = false;
	}


	glfwGetFramebufferSize(m_window, &m_width, &m_height);

	glViewport(0, 0, m_width, m_height);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}


void device::renderFinish()
{
	glfwSwapBuffers(m_window);
	glfwPollEvents();
}

bool device::windowShouldClose()
{
	if (m_window)
	{
		return glfwWindowShouldClose(m_window);
	}	
	return true;
}
