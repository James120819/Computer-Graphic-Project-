///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <algorithm>

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 20;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	//Mouse input
	glfwSetCursorPosCallback(window, Mouse_Position_Callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Enabling sticky keys 
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	//enabling callback
	glfwSetScrollCallback(window, ViewManager::Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

void ViewManager::Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	if (g_pCamera)
	{
		g_pCamera->Zoom -= (float)yOffset;
		if (g_pCamera->Zoom < 1.0f)
			g_pCamera->Zoom = 1.0f;
		if (g_pCamera->Zoom > 90.0f)
			g_pCamera->Zoom = 90.0f;
	}
}
/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// process camera zooming in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}
		//process camera panning up and down
		if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
		{
			g_pCamera->ProcessKeyboard(UP, gDeltaTime);
		}
		if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
		{
			g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);

		}

		if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
		{
			bOrthographicProjection = !bOrthographicProjection;
			std::cout << (bOrthographicProjection ? "Orthographic" : "Perspective") << " projection enabled." << std::endl;

			while (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
			{
				glfwPollEvents();
			}
		}
		
	}



////////////////////////////////////////////////////
bool ViewManager::KeyPressedOnce(int key) {
    int state = glfwGetKey(m_window, key);
    if (state == GLFW_PRESS && !m_keyOnce[key]) {
        m_keyOnce[key] = true;
        return true;
    }
    if (state == GLFW_RELEASE) {
        m_keyOnce[key] = false;
    }
    return false;
}

// Updates the window title to show the selected light and move speed scale
void ViewManager::SetWindowTitleWithSelection() {
    std::ostringstream oss;
    oss << "Graphics Project  |  Selected Light: "
        << (m_selectedPointLight + 1)
        << "  |  Move speed x" << m_moveSpeedScale;
    glfwSetWindowTitle(m_window, oss.str().c_str());
}
/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix
	if (bOrthographicProjection)
	{
		float orthoSize = 5.0f;
		float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
		projection = glm::ortho(-orthoSize * aspectRatio, orthoSize * aspectRatio,
			-orthoSize, orthoSize,
			0.1f, 100.0f);
	}
	else
	{
		projection = glm::perspective(glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f, 100.0f);
	}
	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}

}

void ViewManager::HandleInteractiveShortcuts(GLFWwindow* window) {                          
    //                                                  
    float dt = 0.016f;                                                                       
    float base = 3.0f;                                                                       
    float speed = base * m_moveSpeedScale * dt;                                              
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 2.0f;                

    //Camera                                                            
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) m_camera.MoveForward(speed);           
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) m_camera.MoveBackward(speed);          
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) m_camera.MoveLeft(speed);              
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) m_camera.MoveRight(speed);             
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) m_camera.MoveDown(speed);              
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) m_camera.MoveUp(speed);               

    //Speed scale                                              
    if (KeyPressedOnce(GLFW_KEY_Z)) {                                                       
        m_moveSpeedScale = std::max(0.25f, m_moveSpeedScale * 0.5f);                         
        SetWindowTitleWithSelection();                                                       
    }                                                                                        
    if (KeyPressedOnce(GLFW_KEY_X)) {                                                       
        m_moveSpeedScale = std::min(8.0f,  m_moveSpeedScale * 2.0f);                         
        SetWindowTitleWithSelection();                                                      
    }                                                                                        

                                      
    if (KeyPressedOnce(GLFW_KEY_1)) { m_selectedPointLight = 0; SetWindowTitleWithSelection(); } 
    if (KeyPressedOnce(GLFW_KEY_2)) { m_selectedPointLight = 1; SetWindowTitleWithSelection(); } 
 if (KeyPressedOnce(GLFW_KEY_3)) { m_selectedPointLight = 2; SetWindowTitleWithSelection(); } 
    if (KeyPressedOnce(GLFW_KEY_4)) { m_selectedPointLight = 3; SetWindowTitleWithSelection(); } 

    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS)  m_pointLightPos[m_selectedPointLight].x -= speed; 
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)  m_pointLightPos[m_selectedPointLight].x += speed; 
    if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS)  m_pointLightPos[m_selectedPointLight].z -= speed; 
    if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS)  m_pointLightPos[m_selectedPointLight].z += speed; 
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP)   == GLFW_PRESS) m_pointLightPos[m_selectedPointLight].y += speed; 
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) m_pointLightPos[m_selectedPointLight].y -= speed; 

    if (KeyPressedOnce(GLFW_KEY_L)) { m_dirLightOn     = !m_dirLightOn; }                    
    if (KeyPressedOnce(GLFW_KEY_F)) { m_flashlightOn   = !m_flashlightOn; }                  
    if (KeyPressedOnce(GLFW_KEY_T)) { m_pointLightOn[m_selectedPointLight] = !m_pointLightOn[m_selectedPointLight]; } 

if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || KeyPressedOnce(GLFW_KEY_EQUAL)) {          
        m_pointIntensity[m_selectedPointLight] = std::min(3.0f, m_pointIntensity[m_selectedPointLight] + 0.05f); 
    }                                                                                                     
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || KeyPressedOnce(GLFW_KEY_MINUS)) {       
        m_pointIntensity[m_selectedPointLight] = std::max(0.0f, m_pointIntensity[m_selectedPointLight] - 0.05f); 
    }                                                                                                     

    if (glfwGetKey(window, GLFW_KEY_SEMICOLON) == GLFW_PRESS)  m_ambientBoost = std::max(0.0f, m_ambientBoost - 0.001f); 
    if (glfwGetKey(window, GLFW_KEY_APOSTROPHE) == GLFW_PRESS) m_ambientBoost = std::min(0.3f,  m_ambientBoost + 0.001f); 

    UploadInteractiveUniforms();                                                            
}                                                                                             
