#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#include <random>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// -------------------------------------------------
// Global Variables
// -------------------------------------------------
int Width = 512;
int Height = 512;
std::vector<float> OutputImage;
vec3 lightPos(-4.0f, 4.0f, -3.0f);
vec3 lightColor(1.0f, 1.0f, 1.0f);
// -------------------------------------------------

// class Ray
class Ray {
public:
	vec3 origin;
	vec3 direction;
};

// class Surface and virtual bool intersect
class Surface {
public:
	vec3 ka, kd, ks;
	float specularPower;

	virtual bool intersect(const Ray& ray, float tMin, float tMax, float& t) const = 0;
	virtual vec3 getNormal(const vec3& point) const = 0;
};

// make Sphere using discriminant
class Sphere : public Surface {
public:
	vec3 center;
	float radius;

	Sphere(const vec3& c, float r, const vec3& ka, const vec3& kd, const vec3& ks, float specularPower)
		: center(c), radius(r) {
		this->ka = ka;
		this->kd = kd;
		this->ks = ks;
		this->specularPower = specularPower;
	}

	bool intersect(const Ray& ray, float tMin, float tMax, float& t) const override {
		vec3 oc = ray.origin - center;
		float a = dot(ray.direction, ray.direction);
		float b = 2.0f * dot(oc, ray.direction);
		float c = dot(oc, oc) - radius * radius;
		float discriminant = b * b - 4 * a * c;
		if (discriminant > 0) {
			float temp = (-b - std::sqrt(discriminant)) / (2.0f * a);
			if (temp < tMax && temp > tMin) {
				t = temp;
				return true;
			}
			temp = (-b + std::sqrt(discriminant)) / (2.0f * a);
			if (temp < tMax && temp > tMin) {
				t = temp;
				return true;
			}
		}
		return false;
	}

	vec3 getNormal(const vec3& point) const override {
		return normalize(point - center);
	}
};

// class Plane
class Plane : public Surface {
public:
	vec3 point;
	vec3 normal;

	Plane(const vec3& p, const vec3& n, const vec3& ka, const vec3& kd, const vec3& ks, float specularPower)
		: point(p), normal(normalize(n)) {
		this->ka = ka;
		this->kd = kd;
		this->ks = ks;
		this->specularPower = specularPower;
	}

	bool intersect(const Ray& ray, float tMin, float tMax, float& t) const override {
		float denom = dot(normal, ray.direction);
		if (abs(denom) > 1e-6) {
			vec3 p0l0 = point - ray.origin;
			t = dot(p0l0, normal) / denom;
			if (t >= tMin && t <= tMax) {
				return true;
			}
		}
		return false;
	}

	vec3 getNormal(const vec3& point) const override {
		return normal;
	}
};

// set Camera and get Ray
class Camera {
public:
	vec3 eye;
	vec3 u, v, w;
	float l, r, b, t, d;

	Camera() {
		eye = vec3(0.0f, 0.0f, 0.0f);
		u = vec3(1.0f, 0.0f, 0.0f);
		v = vec3(0.0f, 1.0f, 0.0f);
		w = vec3(0.0f, 0.0f, 1.0f);
		l = -0.1f;
		r = 0.1f;
		b = -0.1f;
		t = 0.1f;
		d = 0.1f;
	}

	Ray getRay(float i, float j, float u_offset = 0.5f, float v_offset = 0.5f) const {
		float u_coord = l + (r - l) * (i + u_offset) / Width;
		float v_coord = b + (t - b) * (j + v_offset) / Height;
		vec3 direction = normalize(u_coord * u + v_coord * v - d * w);
		return Ray{ eye, direction };
	}
};

// class Scene if hit object return white else return black
class Scene {
public:
	std::vector<Surface*> surfaces;

	vec3 trace(const Ray& ray, float tMin, float tMax) const {
		float closest_t = tMax;
		const Surface* hit_surface = nullptr;
		for (const auto& surface : surfaces) {
			float t;
			if (surface->intersect(ray, tMin, closest_t, t)) {
				closest_t = t;
				hit_surface = surface;
			}
		}
		if (hit_surface) {
			vec3 hitPoint = ray.origin + closest_t * ray.direction;
			vec3 normal = hit_surface->getNormal(hitPoint);
			return phongShading(ray, hitPoint, normal, *hit_surface);
		}
		return vec3(0.0f, 0.0f, 0.0f); // black
	}

	vec3 phongShading(const Ray& ray, const vec3& hitPoint, const vec3& normal, const Surface& surface) const {
		vec3 ambient = surface.ka;
		vec3 lightDir = normalize(lightPos - hitPoint);
		vec3 viewDir = normalize(ray.origin - hitPoint);
		vec3 halfDir = normalize(lightDir + viewDir);

		// Shadow check
		Ray shadowRay;
		shadowRay.origin = hitPoint + normal * 1e-4f;
		shadowRay.direction = lightDir;
		bool inShadow = false;
		for (const auto& s : surfaces) {
			float t;
			if (s->intersect(shadowRay, 0.0f, length(lightPos - hitPoint), t)) {
				inShadow = true;
				break;
			}
		}

		vec3 diffuse(0.0f);
		vec3 specular(0.0f);
		if (!inShadow) {
			diffuse = surface.kd * std::max(dot(normal, lightDir), 0.0f);
			specular = surface.ks * pow(std::max(dot(normal, halfDir), 0.0f), surface.specularPower);
		}

		return ambient + diffuse + specular;
	}
};

void render()
{
	//Create our image. We don't want to do this in 
	//the main loop since this may be too slow and we 
	//want a responsive display of our beautiful image.
	//Instead we draw to another buffer and copy this to the 
	//framebuffer using glDrawPixels(...) every refresh

	//Create camera and scene
	Camera camera;
	Scene scene;
	scene.surfaces.push_back(new Plane(vec3(0.0f, -2.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.2f, 0.2f, 0.2f), vec3(1.0f, 1.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f)); // Plane P
	scene.surfaces.push_back(new Sphere(vec3(-4.0f, 0.0f, -7.0f), 1.0f, vec3(0.2f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f)); // Sphere S1
	scene.surfaces.push_back(new Sphere(vec3(0.0f, 0.0f, -7.0f), 2.0f, vec3(0.0f, 0.2f, 0.0f), vec3(0.0f, 0.5f, 0.0f), vec3(0.5f, 0.5f, 0.5f), 32.0f)); // Sphere S2
	scene.surfaces.push_back(new Sphere(vec3(4.0f, 0.0f, -7.0f), 1.0f, vec3(0.0f, 0.0f, 0.2f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 0.0f), 0.0f)); // Sphere S3

	OutputImage.clear();
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0.0, 1.0);

	const int N = 64; // Number of samples per pixel
	for (int j = 0; j < Height; ++j)
	{
		for (int i = 0; i < Width; ++i)
		{
			vec3 color(0.0f);
			for (int s = 0; s < N; ++s)
			{
				float u_offset = dis(gen);
				float v_offset = dis(gen);
				Ray ray = camera.getRay(i, j, u_offset, v_offset);
				color += scene.trace(ray, 0.0f, std::numeric_limits<float>::max());
			}
			color /= static_cast<float>(N);

			// Perform gamma correction with ¥ã = 2.2
			float gamma = 2.2f;
			color = pow(color, vec3(1.0f / gamma));

			OutputImage.push_back(color.x); // R
			OutputImage.push_back(color.y); // G
			OutputImage.push_back(color.z); // B
		}
	}
}


void resize_callback(GLFWwindow*, int nw, int nh)
{
	//This is called in response to the window resizing.
	//The new width and height are passed in so we make 
	//any necessary changes:
	Width = nw;
	Height = nh;
	//Tell the viewport to use all of our screen estate
	glViewport(0, 0, nw, nh);

	//This is not necessary, we're just working in 2d so
	//why not let our spaces reflect it?
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, static_cast<double>(Width)
		, 0.0, static_cast<double>(Height)
		, 1.0, -1.0);

	//Reserve memory for our render so that we don't do 
	//excessive allocations and render the image
	OutputImage.reserve(Width * Height * 3);
	render();
}

int main(int argc, char* argv[])
{
	// -------------------------------------------------
	// Initialize Window
	// -------------------------------------------------

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//We have an opengl context now. Everything from here on out 
	//is just managing our window or opengl directly.

	//Tell the opengl state machine we don't want it to make 
	//any assumptions about how pixels are aligned in memory 
	//during transfers between host and device (like glDrawPixels(...) )
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	//We call our resize function once to set everything up initially
	//after registering it as a callback with glfw
	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, Width, Height);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// -------------------------------------------------------------
		//Rendering begins!
		glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
		//and ends.
		// -------------------------------------------------------------

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		//Close when the user hits 'q' or escape
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
