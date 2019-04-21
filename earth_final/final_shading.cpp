#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_new.h"
#include "camera.h"
#include "obj.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

struct Particle{
	glm::vec3 pos, speed;
	unsigned char r,g,b,a;
	float size, angle, weight;
	float life;
	float cameradistance; 

	bool operator<(const Particle& that) const {
		return this->cameradistance > that.cameradistance;
	}
};

const int MaxParticles = 100000;
Particle ParticlesContainer[MaxParticles];
int LastUsedParticle = 0;

int FindUnusedParticle(){

	for(int i=LastUsedParticle; i<MaxParticles; i++){
		if (ParticlesContainer[i].life < 0){
			LastUsedParticle = i;
			return i;
		}
	}

	for(int i=0; i<LastUsedParticle; i++){
		if (ParticlesContainer[i].life < 0){
			LastUsedParticle = i;
			return i;
		}
	}

	return 0;
}

void SortParticles(){
	std::sort(&ParticlesContainer[0], &ParticlesContainer[MaxParticles]);
}

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
float heightScale = 0.07;

// camera
Camera camera(glm::vec3(-15.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(0.0f, 10.0f, 10.0f);

float random(float x){
	return static_cast <float> (rand() / (static_cast <float> (RAND_MAX/x)));
}

glm::vec3 drawBezier(double t,glm::vec3 PT1,glm::vec3 PT4) {

	glm::vec3 P;
	glm::vec3 PT2 = glm::vec3 (-1.0f,0.8f,2.3f);
	glm::vec3 PT3 = glm::vec3 (-2.0f,0.9f,2.4f);

	P.x = pow((1 - t), 3) * PT1.x + 3 * t * pow((1 - t), 2) * PT2.x + 3 * (1 - t) * pow(t, 2)* PT3.x + pow(t, 3)* PT4.x;
	P.y = pow((1 - t), 3) * PT1.y + 3 * t * pow((1 - t), 2) * PT2.y + 3 * (1 - t) * pow(t, 2)* PT3.y + pow(t, 3)* PT4.y;
	P.z = pow((1 - t), 3) * PT1.z + 3 * t * pow((1 - t), 2) * PT2.z + 3 * (1 - t) * pow(t, 2)* PT3.z + pow(t, 3)* PT4.z;

	return P;
}
glm::vec3 drawLinear(double t,glm::vec3 PT1,glm::vec3 PT2) {

	glm::vec3 P;

	P.x = (1 - t) * PT1.x + t * PT2.x;
	P.y = (1 - t) * PT1.y + t * PT2.y;
	P.z = (1 - t) * PT1.z + t * PT2.z;

	return P;
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental=GL_TRUE;
    GLenum err=glewInit();
    if(err!=GLEW_OK)
    {
        std::cout<<"glewInit failed, aborting."<<std::endl;
    }

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
    
    glEnable(GL_CULL_FACE);
	
    Shader colorShader("color.vs", "color.fs");
    Shader lampShader("lamp.vs", "lamp.fs");
    Shader textureShader("texture.vs","texture.fs");
    
    Shader tessShader("shader/v.vs","shader/tesscs.tcs","shader/tesses.tes", "shader/geo.gs","shader/f.fs");


    Shader particleShader("particle.vs","particle.fs");
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f);

    OBJ sphere("earth2.obj");
    OBJ cube("earth.obj");
    
    GLint MaxPatchVertices = 0;
    glGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
    printf("Max supported patch vertices %d\n", MaxPatchVertices);	
    glPatchParameteri(GL_PATCH_VERTICES, 3);

    unsigned int diffuseMap2 = loadTexture("marsdiff.jpg");
    unsigned int heightMap2 = loadTexture("marsbump.jpg");
	unsigned int diffuseMap = loadTexture("highdiff.jpg");
    unsigned int heightMap = loadTexture("highbump.jpg");
	unsigned int text = loadTexture("fire.png");

    //PARTICLE!!!!
    GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	camera.Front=glm::normalize(glm::vec3(3.0f, 0.0f, -1.0f));

	static GLfloat* g_particule_position_size_data = new GLfloat[MaxParticles * 4];
	static GLubyte* g_particule_color_data         = new GLubyte[MaxParticles * 4];

	for(int i=0; i<MaxParticles; i++){
		ParticlesContainer[i].life = -1.0f;
		ParticlesContainer[i].cameradistance = -1.0f;
	}

	static const GLfloat g_vertex_buffer_data[] = { 
		 -0.5f, -0.5f, 0.0f,
		  0.5f, -0.5f, 0.0f,
		 -0.5f,  0.5f, 0.0f,
		  0.5f,  0.5f, 0.0f,
	};
	GLuint billboard_vertex_buffer;
	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint particles_position_buffer;
	glGenBuffers(1, &particles_position_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

	GLuint particles_color_buffer;
	glGenBuffers(1, &particles_color_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
	glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

	srand (static_cast <unsigned> (time(0)));
	float t0 = glfwGetTime();

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glm::vec3 Rig=camera.Right;
    while (!glfwWindowShouldClose(window))
    {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 rot = glm::mat4(1.0f);
        glm::mat4 rotx = glm::mat4(1.0f);
        glm::mat4 roty = glm::mat4(1.0f);
        glm::mat4 rotz = glm::mat4(1.0f);
        glm::mat4 tran = glm::mat4(1.0f);
        glm::mat4 scal = glm::mat4(1.0f);
        
		if(glfwGetTime()-t0<4){
		camera.Position= drawLinear((glfwGetTime()-t0)/4,glm::vec3(-15.0f, 0.0f, 5.0f),glm::vec3(-8.0f, 0.0f, 3.0f));
		}
		
		if(glfwGetTime()-t0>=4 && glfwGetTime()-t0<13){
		camera.Position= drawLinear((glfwGetTime()-t0-4)/9,glm::vec3(-8.0f, 0.0f, 3.0f),glm::vec3(-1.0f, 0.8f, 2.5f));
		camera.Front=glm::normalize(drawLinear((glfwGetTime()-t0-4)/9,glm::normalize(glm::vec3(3.0f, 0.0f, -1.0f)),glm::normalize(glm::vec3(-0.81,-0.28,-0.51))));
		camera.Right = glm::normalize(drawLinear((glfwGetTime()-t0-4)/9,Rig,glm::normalize(glm::vec3(0.0f, 1.0f, -1.0f))));
		camera.Up    = glm::normalize(glm::cross(camera.Right, camera.Front));
		}

		if(glfwGetTime()-t0>=13 && glfwGetTime()-t0<17){
		camera.Position= drawLinear((glfwGetTime()-t0-13)/4,glm::vec3(-1.0f, 0.8f, 2.5f),glm::vec3(-0.6f, 0.8f, 2.42f));
		camera.Front=glm::normalize(drawLinear((glfwGetTime()-t0-13)/4,glm::normalize(glm::vec3(-0.81f,-0.28f,-0.51f)),glm::normalize(glm::vec3(2.0f,1.0f,-0.4f))));
		camera.Right = glm::normalize(drawLinear((glfwGetTime()-t0-13)/4,glm::normalize(glm::vec3(0.0f, 1.0f, -1.0f)),glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f))));
		camera.Up    = glm::normalize(glm::cross(camera.Right, camera.Front));
		}

		if(glfwGetTime()-t0>=17 && glfwGetTime()-t0<22){
		camera.Position= drawLinear((glfwGetTime()-t0-17)/5,glm::vec3(-0.6f, 0.8f, 2.42f),glm::vec3(-0.26f, 0.8f, 2.48f));
		camera.Front=glm::normalize(drawLinear((glfwGetTime()-t0-17)/5,glm::normalize(glm::vec3(2.0f,1.0f,-0.4f)),glm::normalize(glm::vec3(2.0f,-0.1f,0.0f))));
		camera.Right = glm::normalize(drawLinear((glfwGetTime()-t0-17)/5,glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)),glm::normalize(glm::vec3(1.0f, -1.0f, 0.7f))));
		camera.Up    = glm::normalize(glm::cross(camera.Right, camera.Front));
		}

		if(glfwGetTime()-t0>=22 && glfwGetTime()-t0<45){
		camera.Position= drawBezier((glfwGetTime()-t0-22)/23,glm::vec3(-0.26f, 0.8f, 2.48f),glm::vec3(-3.0f, -0.2f, 0.5f));
		camera.Front=glm::normalize(drawLinear((glfwGetTime()-t0-22)/23,glm::normalize(glm::vec3(2.0f,-0.1f,0.0f)),glm::normalize(glm::vec3(2.0f,0.1f,3.0f))));
		camera.Right = glm::normalize(drawLinear((glfwGetTime()-t0-22)/23,glm::normalize(glm::vec3(1.0f, -1.0f, 0.7f)),glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f))));
		camera.Up    = glm::normalize(glm::cross(camera.Right, camera.Front));
		}

		if(glfwGetTime()-t0>=45 && glfwGetTime()-t0<60){
		camera.Position= drawLinear((glfwGetTime()-t0-45)/15,glm::vec3(-3.0f, -0.2f, 0.5f),glm::vec3(-8.0f, -1.0f, -8.0f));
		camera.Front=glm::normalize(drawLinear((glfwGetTime()-t0-45)/15,glm::normalize(glm::vec3(2.0f,0.1f,3.0f)),glm::normalize(glm::vec3(2.0f,0.1f,2.0f))));
		camera.Right = glm::normalize(drawLinear((glfwGetTime()-t0-45)/15,glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)),glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f))));
		camera.Up    = glm::normalize(glm::cross(camera.Right, camera.Front));
		}

        glm::mat4 view = camera.GetViewMatrix();

		//lamp object
        lampShader.use();
        lampShader.setMat4("projection", projection);
        lampShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        lampShader.setMat4("model", model);

        glBindVertexArray(cube.VAO);
        glDrawElements(GL_TRIANGLES, cube.indices.size(), GL_UNSIGNED_INT, 0);

		// //textured cube
        tessShader.use();
        tessShader.setVec3("lightPos", lightPos);
        tessShader.setVec3("viewPos", camera.Position);
        tessShader.setMat4("projection", projection);
        tessShader.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f,-1.0f,0.0f));
        model = glm::scale(model, glm::vec3(3.07f));
        model = glm::rotate(model, glm::radians(150.0f), glm::vec3(0.0, 1.0, 0.0));
        tessShader.setMat4("model", model);
        
		
        tessShader.setVec3("centerPos", glm::vec3(0.0f,-1.0f,0.0f));
        tessShader.setFloat("heightScale", heightScale);
		if(glfwGetTime()-t0<18){
		tessShader.setInt("diffuseMap", 0);
        tessShader.setInt("depthMap", 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap2);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, heightMap2);
		}
		if(glfwGetTime()-t0>=18){
		tessShader.setInt("diffuseMap", 0);
        tessShader.setInt("depthMap", 1);
		glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, heightMap);
		}

        glBindVertexArray(sphere.VAO);
        glDrawElements(GL_PATCHES, sphere.indices.size(), GL_UNSIGNED_INT, 0);

        glm::mat4 ProjectionMatrix = projection;
		glm::mat4 ViewMatrix = view;

		glm::vec3 CameraPosition(glm::inverse(ViewMatrix)[3]);

		glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		
		if(glfwGetTime()-t0<27){
			glDepthMask(GL_FALSE);
			glBindVertexArray(VertexArrayID);

			int maxParticles = 20000 ;
			if(glfwGetTime()-t0<22){
				int newparticles = (int)(deltaTime*maxParticles);
				if (newparticles > (int)(0.016f*maxParticles))
					newparticles = (int)(0.016f*maxParticles);
				float maxTheta = 0.1;
				for(int i=0; i<newparticles; i++){
					int particleIndex = FindUnusedParticle();
					ParticlesContainer[particleIndex].life = 3.0f; 
					ParticlesContainer[particleIndex].pos = glm::vec3(0,-1.0f,0.0f);

					float spread = 2.5f;
					glm::vec3 maindir = glm::vec3(0.0f, 0.0f, 0.0f);
					float phi = random(6.28);
					float theta = random(min(3.14, (glfwGetTime() - t0)*0.1)) ;
					glm::vec3 randomdir = glm::vec3(-cos(theta), sin(theta)*sin(phi), sin(theta)*cos(phi));

					randomdir=glm::normalize(randomdir);
					
					ParticlesContainer[particleIndex].speed = maindir + randomdir*spread;

					ParticlesContainer[particleIndex].r = rand() % 256;
					ParticlesContainer[particleIndex].g = rand() % 256;
					ParticlesContainer[particleIndex].b = rand() % 256;
					ParticlesContainer[particleIndex].a = (rand() % 256) / 3;

					ParticlesContainer[particleIndex].size = 0.3;
					
				}
			}



			// Simulate all particles
			int ParticlesCount = 0;
			for(int i=0; i<MaxParticles; i++){

				Particle& p = ParticlesContainer[i]; 

				if(p.life > 0.0f){

					p.life -= deltaTime;
					if (p.life > 0.0f){

						p.speed += 1.27f*(glm::vec3(0.0f,-1.0f,0.0f)-p.pos) * (float)deltaTime * 0.5f;
						p.pos += p.speed * (float)deltaTime;
						glm::vec3 temp =  p.pos - CameraPosition;
						p.cameradistance = dot(temp, temp);

						g_particule_position_size_data[4*ParticlesCount+0] = p.pos.x;
						g_particule_position_size_data[4*ParticlesCount+1] = p.pos.y;
						g_particule_position_size_data[4*ParticlesCount+2] = p.pos.z;
													
						g_particule_position_size_data[4*ParticlesCount+3] = p.size;
													
						g_particule_color_data[4*ParticlesCount+0] = p.r;
						g_particule_color_data[4*ParticlesCount+1] = p.g;
						g_particule_color_data[4*ParticlesCount+2] = p.b;
						g_particule_color_data[4*ParticlesCount+3] = p.a;

					}else{
						p.cameradistance = -1.0f;
					}

					ParticlesCount++;

				}
			}

			//SortParticles();

			glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
			glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data);

			glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
			glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, g_particule_color_data);


			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

			particleShader.use();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, text);
			particleShader.setInt("myTextureSampler", 0);

			particleShader.setVec3("CameraRight_worldspace", ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
			particleShader.setVec3("CameraUp_worldspace", ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);

			particleShader.setMat4("VP", ViewProjectionMatrix);

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
			glVertexAttribPointer(
				0,
				3,
				GL_FLOAT,
				GL_FALSE,
				0,
				(void*)0
			);
			
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
			glVertexAttribPointer(
				1,
				4,
				GL_FLOAT,
				GL_FALSE,
				0,
				(void*)0
			);

			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
			glVertexAttribPointer(
				2,
				4,
				GL_UNSIGNED_BYTE,
				GL_TRUE,
				0,
				(void*)0
			);

			glVertexAttribDivisor(0, 0);
			glVertexAttribDivisor(1, 1);
			glVertexAttribDivisor(2, 1);

			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDepthMask(GL_TRUE);
		}
        

        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    sphere.flush();
    cube.flush();
	glDeleteTextures(1, &diffuseMap);
	glDeleteTextures(1, &heightMap);
	glDeleteTextures(1, &diffuseMap2);
	glDeleteTextures(1, &heightMap2);
	glDeleteTextures(1, &text);

	delete[] g_particule_position_size_data;

	glDeleteBuffers(1, &particles_color_buffer);
	glDeleteBuffers(1, &particles_position_buffer);
	glDeleteBuffers(1, &billboard_vertex_buffer);
	glDeleteVertexArrays(1, &VertexArrayID);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS){
        camera.Position=glm::vec3(0.0f, 0.0f, 3.0f);
        float lastX = SCR_WIDTH / 2.0f;
        float lastY = SCR_HEIGHT / 2.0f;
        camera.Yaw        = -90.0f;
        camera.Pitch       =  0.0f;
        bool firstMouse = true;
        camera.ProcessMouseMovement(0,0);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
