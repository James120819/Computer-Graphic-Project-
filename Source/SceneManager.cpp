///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.empty()) return false;
	bool bFound = false;
	for (const auto& m : m_objectMaterials)
		{
			if (m.tag == tag)
			{
				material = m;
				bFound = true;
				break;
			}
		}
	return bFound;

}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		int textureID = FindTextureSlot(textureTag);

		if (textureID < 0) {
			return;
		}

		m_pShaderManager->setIntValue(g_UseTextureName, true);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	OBJECT_MATERIAL material{};

	if (!FindMaterial(materialTag, material)) {
		return;
	}
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture("./Debug/textures/wood_light_seamless.jpg", "wood");
	bReturn = CreateGLTexture("./Debug/textures/marble_light_seamless.jpg", "marble1");
	bReturn = CreateGLTexture("./Debug/textures/leather_black_seamless.jpg", "leather1");
	bReturn = CreateGLTexture("./Debug/textures/paper_textured_seamless.jpg", "paper");
	bReturn = CreateGLTexture("./Debug/textures/leather_brown_seamless.jpg", "leather2");
	bReturn = CreateGLTexture("./Debug/textures/paper_brown_seamless.jpg", "paper2");
	bReturn = CreateGLTexture("./Debug/textures/leather_tan_seamless.jpg", "leather3");
	bReturn = CreateGLTexture("./Debug/textures/marble_light2_seamless.jpg", "marble2");
	bReturn = CreateGLTexture("./Debug/textures/ground_textured_seamless.jpg", "ground");
	bReturn = CreateGLTexture("./Debug/textures/grass_textured1_seamless.jpg", "grass1");
	bReturn = CreateGLTexture("./Debug/textures/grass_textured2_seamless.jpg", "grass2");
	bReturn = CreateGLTexture("./Debug/textures/pattern_flowers_seamless.jpg", "pattern");
	bReturn = CreateGLTexture("./Debug/textures/fabric_textured_seamless.jpg", "fabric");
	bReturn = CreateGLTexture("./Debug/textures/wood_cherry_seamless.jpg", "wood2");
	BindGLTextures();
}

void SceneManager::SetupSceneLights()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);

	GLfloat light0_position[] = { 0.0f, 5.0f, 5.0f, 0.0f };
	GLfloat light0_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat light0_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
	GLfloat light0_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

	GLfloat light1_position[] = { -2.0f, 3.0f, -2.0f, 1.0f };
	GLfloat light1_ambient[] = { 0.1f, 0.0f, 0.0f, 1.0f };
	GLfloat light1_diffuse[] = { 0.8f, 0.1f, 0.1f, 1.0f };
	GLfloat light1_specular[] = { 1.0f, 0.4f, 0.4f, 1.0f };

	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
}

void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL wood;
	wood.ambientColor = glm::vec3(0.2f, 0.1f, 0.05f);
	wood.ambientStrength = 0.4f;
	wood.diffuseColor = glm::vec3(0.5f, 0.25f, 0.1f);
	wood.specularColor = glm::vec3(0.3f, 0.2f, 0.1f);
	wood.shininess = 8.0f;
	wood.tag = "wood";
	m_objectMaterials.push_back(wood);

	OBJECT_MATERIAL marble1;
	marble1.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	marble1.ambientStrength = 0.5f;
	marble1.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	marble1.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	marble1.shininess = 64.0f;
	marble1.tag = "marble1";
	m_objectMaterials.push_back(marble1);

	OBJECT_MATERIAL leather1;
	leather1.ambientColor = glm::vec3(0.2f, 0.1f, 0.1f);
	leather1.ambientStrength = 0.3f;
	leather1.diffuseColor = glm::vec3(0.4f, 0.2f, 0.2f);
	leather1.specularColor = glm::vec3(0.5f, 0.4f, 0.3f);
	leather1.shininess = 64.0f;
	leather1.tag = "leather1";
	m_objectMaterials.push_back(leather1);

	OBJECT_MATERIAL paper;
	paper.ambientColor = glm::vec3(0.4f, 0.4f, 0.3f);
	paper.ambientStrength = 0.3f;
	paper.diffuseColor = glm::vec3(0.8f, 0.8f, 0.7f);
	paper.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	paper.shininess = 4.0f;
	paper.tag = "paper";
	m_objectMaterials.push_back(paper);

	OBJECT_MATERIAL leather2;
	leather2.ambientColor = glm::vec3(0.15f, 0.1f, 0.05f);
	leather2.ambientStrength = 0.3f;
	leather2.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	leather2.specularColor = glm::vec3(0.4f, 0.3f, 0.2f);
	leather2.shininess = 12.0f;
	leather2.tag = "leather2";
	m_objectMaterials.push_back(leather2);

	OBJECT_MATERIAL paper2;
	paper2.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	paper2.ambientStrength = 0.3f;
	paper2.diffuseColor = glm::vec3(0.9f, 0.9f, 0.8f);
	paper2.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	paper2.shininess = 4.0f;
	paper2.tag = "paper2";
	m_objectMaterials.push_back(paper2);

	OBJECT_MATERIAL leather3;
	leather3.ambientColor = glm::vec3(0.1f, 0.05f, 0.05f);
	leather3.ambientStrength = 0.3f;
	leather3.diffuseColor = glm::vec3(0.35f, 0.2f, 0.2f);
	leather3.specularColor = glm::vec3(0.4f, 0.3f, 0.3f);
	leather3.shininess = 16.0f;
	leather3.tag = "leather3";
	m_objectMaterials.push_back(leather3);

	OBJECT_MATERIAL marble2;
	marble2.ambientColor = glm::vec3(0.35f, 0.35f, 0.35f);
	marble2.ambientStrength = 0.5f;
	marble2.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	marble2.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	marble2.shininess = 64.0f;
	marble2.tag = "marble2";
	m_objectMaterials.push_back(marble2);

	OBJECT_MATERIAL ground;
	ground.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	ground.ambientStrength = 0.4f;
	ground.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	ground.specularColor = glm::vec3(0.2f, 0.4f, 0.2f);
	ground.shininess = 8.0f;
	ground.tag = "ground";
	m_objectMaterials.push_back(ground);

	OBJECT_MATERIAL grass1;
	grass1.ambientColor = glm::vec3(0.1f, 0.3f, 0.1f);
	grass1.ambientStrength = 0.4f;
	grass1.diffuseColor = glm::vec3(0.2f, 0.5f, 0.2f);
	grass1.specularColor = glm::vec3(0.2f, 0.4f, 0.2f);
	grass1.shininess = 8.0f;
	grass1.tag = "grass1";
	m_objectMaterials.push_back(grass1);

	OBJECT_MATERIAL grass2;
	grass2.ambientColor = glm::vec3(0.15f, 0.35f, 0.15f);
	grass2.ambientStrength = 0.4f;
	grass2.diffuseColor = glm::vec3(0.25f, 0.55f, 0.25f);
	grass2.specularColor = glm::vec3(0.25f, 0.45f, 0.25f);
	grass2.shininess = 10.0f;
	grass2.tag = "grass2";
	m_objectMaterials.push_back(grass2);


	OBJECT_MATERIAL pattern;
	pattern.ambientColor = glm::vec3(0.3f, 0.2f, 0.2f);
	pattern.ambientStrength = 0.4f;
	pattern.diffuseColor = glm::vec3(0.6f, 0.3f, 0.3f);
	pattern.specularColor = glm::vec3(0.4f, 0.2f, 0.2f);
	pattern.shininess = 20.0f;
	pattern.tag = "pattern";
	m_objectMaterials.push_back(pattern);


	OBJECT_MATERIAL fabric;
	fabric.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	fabric.ambientStrength = 0.4f;
	fabric.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	fabric.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	fabric.shininess = 16.0f;
	fabric.tag = "fabric";
	m_objectMaterials.push_back(fabric);

	OBJECT_MATERIAL wood2;
	wood2.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	wood2.ambientStrength = 0.4f;
	wood2.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	wood2.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	wood2.shininess = 16.0f;
	wood2.tag = "wood2";
	m_objectMaterials.push_back(wood2);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	
}

void SceneManager::ApplyAndDraw(const std::string& texTag,
                                const std::string& matTag,
                                const glm::vec2& uv,
                                const glm::vec3& scale,
                                float rx, float ry, float rz,
                                const glm::vec3& pos,
                                const std::function<void()>& drawCall)
{

   SetShaderTexture(texTag);
   SetShaderMaterial(matTag);
   SetTextureUVScale(uv.x, uv.y);
   SetTransformations(scale, rx, ry, rz, pos);
   drawCall();
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	glm::vec3 scale, position;
	float xRot = 0.0f, yRot = 0.0f, zRot = 0.0f;

	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Table top surface using plane shape 
	SetShaderTexture("wood");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	SetTransformations(glm::vec3(2.0f, 1.0f, 2.0f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 0.2f));
	m_basicMeshes->DrawPlaneMesh();

	//Saucer base plate
	SetShaderTexture("marble1");
	SetShaderMaterial("marble1");
	SetTextureUVScale(1.0f, 1.0f);
	SetTransformations(glm::vec3(0.3f, 0.015f, 0.3f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.01f, 0.0f));
	m_basicMeshes->DrawCylinderMesh();

	//Half Sphere shape used for the centered middle of the saucer plate
	SetShaderTexture("marble1");
	SetShaderMaterial("marble1");
	SetTextureUVScale(1.0f, 1.0f);
	SetTransformations(glm::vec3(0.12f, 0.008f, 0.12f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.035f, 0.0f));
	m_basicMeshes->DrawHalfSphereMesh();

	

	//Using the upside down Cylinder for the cups body 
	SetShaderTexture("marble1");
	SetShaderMaterial("marble1");
	SetTextureUVScale(1.0f, 1.0f);
	SetTransformations(glm::vec3(0.18f, 0.27f, 0.18f), 180.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.30f, 0.0f));
	m_basicMeshes->DrawTaperedCylinderMesh(true, true, true);

	
	

	//Cup surface liquid using flattened cylinder shape
	SetShaderColor(0.1f, 0.05f, 0.01f, 1.0f);
	SetTransformations(glm::vec3(0.16f, 0.005f, 0.16f), 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.30f, 0.0f));
	m_basicMeshes->DrawCylinderMesh();

	
	
	//First half of the handle using half torus shape
	SetShaderTexture("marble1");
	SetShaderMaterial("marble1");
	SetTextureUVScale(1.0f, 1.0f);
	SetTransformations(glm::vec3(0.06f, 0.06f, 0.025f), 0.0f, 0.0f, 90.0f, glm::vec3(-0.20f, 0.215f, 0.0f));
	m_basicMeshes->DrawHalfTorusMesh();

	//second half of handle
	SetShaderTexture("marble1");
	SetShaderMaterial("marble1");
	SetTextureUVScale(1.0f, 1.0f);
	SetTransformations(glm::vec3(0.06f, 0.06f, 0.025f), 180.0f, 0.0f, 90.0f, glm::vec3(-0.20f, 0.215f, 0.0f));
	m_basicMeshes->DrawHalfTorusMesh();
////////////////////////////////////////////////////////////////Book Design //////////////////////////////////////////////////////
	//First Book
	SetShaderTexture("leather1");
	SetShaderMaterial("leather1");
	SetTextureUVScale(4.0f, 2.0f);
	SetTransformations(glm::vec3(0.5f, 0.07f, 0.4f), 0.0f, 90.0f, 0.0f, glm::vec3(0.52f, 0.035f, 0.09f));
	m_basicMeshes->DrawBoxMesh();

	//Paper texture for the first book
	SetShaderTexture("paper");
	SetShaderMaterial("paper");
	SetTextureUVScale(4.0f, 2.0f);
	SetTransformations(glm::vec3(0.27f, 0.001f, 0.16f), 0.0f, 90.0f, 0.0f, glm::vec3(0.47f, 0.035f, 0.08f));
	m_basicMeshes->DrawPlaneMesh();

	//Second Book
	SetShaderTexture("leather2");
	SetShaderMaterial("leather2");
	SetTextureUVScale(4.0f, 2.0f);
	SetTransformations(glm::vec3(0.5f, 0.09f, 0.4f), 0.0f, 90.0f, 0.0f, glm::vec3(0.52f, 0.12f, 0.09f));
	m_basicMeshes->DrawBoxMesh();

	//Paper texture for the second book
	SetShaderTexture("paper2");
	SetShaderMaterial("paper2");
	SetTextureUVScale(4.0f, 2.0f);
	SetTransformations(glm::vec3(0.26f, 0.014f, 0.21f), 0.0f, 90.0f, 0.0f, glm::vec3(0.52f, 0.12f, 0.09f));
	m_basicMeshes->DrawPlaneMesh();

	//Third Book 
	SetShaderTexture("leather3");
	SetShaderMaterial("leather3");
	SetTextureUVScale(4.0f, 2.0f);
	SetTransformations(glm::vec3(0.4f, 0.04f, 0.3f), 0.0f, 90.0f, 0.0f, glm::vec3(0.52f, 0.17f, 0.09f));
	m_basicMeshes->DrawBoxMesh();

/////////////////////////////////////////////////////Picture Frame Design////////////////////////////////////////////////////////
	//Picture frame 
	SetShaderTexture("paper");
	SetShaderMaterial("paper");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.25f, 0.01f, 0.89f), 90.0f, -45.0f, 0.0f, glm::vec3(0.52f, 0.46f, 0.09f));
	m_basicMeshes->DrawBoxMesh();

	//Wooden frame 
	SetShaderTexture("wood");
	SetShaderMaterial("wood");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.27f, 0.01f, 0.92f), 90.0f, -45.0f, 0.0f, glm::vec3(0.53f, 0.48f, 0.09f));
	m_basicMeshes->DrawBoxMesh();

	//Additional box added for wooden frame
	SetShaderTexture("wood");
	SetShaderMaterial("wood");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.27f, 0.01f, 0.92f), 90.0f, -45.0f, 0.0f, glm::vec3(0.53f, 0.48f, 0.09f));
	m_basicMeshes->DrawBoxMesh();

	//Adding in final wooden texture for wooden picture frame
	SetShaderTexture("wood");
	SetShaderMaterial("wood");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.27f, 0.01f, 0.90f), 90.0f, -45.0f, 0.0f, glm::vec3(0.53, 0.48f, 0.09f));
	m_basicMeshes->DrawBoxMesh();

	

//////////////////////////////////////////////////Plant Vase Design////////////////////////////////////////////////////////////
	/** Set shape figures for plant vase design.   ***/

	SetShaderTexture("marble1");
	SetShaderMaterial("marble1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.3f, 0.65f, 0.3f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 0.01f, -0.70f));
	m_basicMeshes->DrawTaperedCylinderMesh(true, true, true);

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.06f, 0.45f, 0.06f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.21f, 0.19f, -0.70f));
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.06f, 0.45f, 0.06f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 0.19f, -0.49f));
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.06f, 0.45f, 0.06f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.63f, 0.19f, -0.70f));
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.06f, 0.45f, 0.06f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 0.19f, -0.91f));
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.06f, 0.45f, 0.06f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.31f, 0.19f, -0.59f));
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.06f, 0.45f, 0.06f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.53f, 0.19f, -0.59f));
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.06f, 0.45f, 0.06f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.31f, 0.19f, -0.81f));
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.06f, 0.45f, 0.06f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.53f, 0.19f, -0.81f));
	m_basicMeshes->DrawCylinderMesh(true, true);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);
	SetTransformations(glm::vec3(0.26f, 0.008f, 0.26f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 0.625f, -0.70f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("marble1");
	SetShaderMaterial("marble1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.26f, 0.008f, 0.26f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 0.635f, -0.70f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("marble2");
	SetShaderMaterial("marble2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.24f, 0.10f, 0.24f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 0.645f, -0.70f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("ground");
	SetShaderMaterial("ground");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.22f, 0.10f, 0.22f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 0.755f, -0.70f));
	m_basicMeshes->DrawHalfSphereMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 0.82f, -0.70f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass2"); 
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.05f, 0.08f, 0.05), 0.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 1.54f, -0.70f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), -15.0f, 0.0f, 30.0f, glm::vec3(-0.36f, 0.82f, -0.67f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), -10.0f, 0.0f, 5.0f, glm::vec3(-0.38f, 0.82f, -0.72f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), 0.0f, 0.0f, -25.0f, glm::vec3(-0.45f, 0.82f, -0.68f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), 10.0f, 0.0f, 20.0f, glm::vec3(-0.39f, 0.82f, -0.66f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), -11.0f, 0.0f, -27.7f, glm::vec3(-0.37f, 0.82f, -0.70f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), -12.0f, 0.0f, -27.7f, glm::vec3(-0.39f, 0.82f, -0.66f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), -10.8f, 0.0f, -6.9f, glm::vec3(-0.44f, 0.82f, -0.66f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), 8.3f, 0.0f, 21.7f, glm::vec3(-0.47f, 0.82f, -0.70f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), -12.4f, 0.0f, 4.0f, glm::vec3(-0.45f, 0.82f, -0.74f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), 5.7f, 0.0f, 25.1f, glm::vec3(-0.39f, 0.82f, -0.74f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), -12.0f, 0.0f, -35.1f, glm::vec3(-0.35f, 0.82f, -0.68f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), 14.0f, 0.0f, 32.1f, glm::vec3(-0.49f, 0.82f, -0.73f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass1");
	SetShaderMaterial("grass1");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), -8.0f, 0.0f, 40.0f, glm::vec3(-0.38f, 0.82f, -0.77f));
	m_basicMeshes->DrawCylinderMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), -12.0f, 0.0f, -35.0f, glm::vec3(-0.60f, 1.36f, -0.84f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 15.0f, 0.0f, 20.0f, glm::vec3(-0.30f, 1.36f, -0.56f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), -8.0f, 0.0f, 40.0f, glm::vec3(-0.20f, 1.36f, -0.92f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), -10.0f, 0.0f, 20.0f, glm::vec3(-0.72f, 1.36f, -0.82f));
	m_basicMeshes->DrawSphereMesh();
	
	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 0.0f, 0.0f, 40.0f, glm::vec3(-0.42f, 1.36f, -0.50f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 8.0f, 0.0f, 30.0f, glm::vec3(-0.10f, 1.36f, -0.75f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), -5.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 1.36f, -0.92f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 5.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 1.36f, -0.45f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 0.0f, 0.0f, -15.0f, glm::vec3(-0.82f, 1.36f, -0.70f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 0.0f, 0.0f, 15.0f, glm::vec3(-0.02f, 1.36f, -0.70f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), -8.0f, 0.0f, 0.0f, glm::vec3(-0.42f, 1.28f, -0.82f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 0.0f, 0.0f, -10.0f, glm::vec3(-0.72f, 1.36f, -0.90f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 0.0f, 0.0f, 10.0f, glm::vec3(-0.12f, 1.36f, -0.50f));
	m_basicMeshes->DrawSphereMesh();

	SetShaderTexture("grass2");
	SetShaderMaterial("grass2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.07f, 0.07f, 0.07f), 0.0f, 0.0f, -20.0f, glm::vec3(-0.82f, 1.36f, -0.48f));
	m_basicMeshes->DrawSphereMesh();

/////////////////////////////////////////////////////Stacked Books//////////////////////////////////////////////////////

	SetShaderTexture("leather3");
	SetShaderMaterial("leather3");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.45f, 0.05f, 0.65f), 0.0f, 100.0f, 0.0f, glm::vec3(-0.75f, 0.01f, -0.15f));
	m_basicMeshes->DrawBoxMesh();

	SetShaderTexture("paper");
	SetShaderMaterial("paper");
	SetTextureUVScale(1.0f, 1.0f);
	SetTransformations(glm::vec3(0.35f, 0.002f, 0.20f), 0.0f, 10.0f, 0.0f, glm::vec3(-0.75f, 0.01f, -0.15f));
	m_basicMeshes->DrawPlaneMesh();

	SetShaderTexture("pattern");
	SetShaderMaterial("pattern");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.32f, 0.045f, 0.62f), 0.0f, 100.0f, 0.0f, glm::vec3(-0.75f, 0.065f, -0.15f));
	m_basicMeshes->DrawBoxMesh();

	SetShaderTexture("fabric");
	SetShaderMaterial("fabric");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.30f, 0.04f, 0.60f), 0.0f, 100.0f, 0.0f, glm::vec3(-0.75f, 0.12f, -0.15f));
	m_basicMeshes->DrawBoxMesh();

	SetShaderTexture("paper2");
	SetShaderMaterial("paper2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.14f, 0.004f, 0.32f), 0.0f, 100.0f, 0.0f, glm::vec3(-0.75f, 0.125f, -0.15f));
	m_basicMeshes->DrawPlaneMesh();

	SetShaderTexture("wood2");
	SetShaderMaterial("wood2");
	SetTextureUVScale(2.0f, 2.0f);
	SetTransformations(glm::vec3(0.005f, 0.68f, 0.005f), 90.0f, 100.0f, 0.0f, glm::vec3(-1.05f, 0.16f, 0.02f));
	m_basicMeshes->DrawCylinderMesh();
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
}




