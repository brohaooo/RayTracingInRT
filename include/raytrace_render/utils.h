#ifndef RTIRT_H
#define RTIRT_H



#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>  // 包含常量定义，如 glm::pi
#include <glm/gtx/norm.hpp>  // 包含 length2 函数


// Usings

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

// Constants

const float infinity = std::numeric_limits<float>::infinity();
const float pi = glm::pi<float>();

// Utility Functions:

// random float generator
#include <random>

inline float random_float() {
	// Returns a random real in [0,1).
	static std::uniform_real_distribution<float> distribution(0.0, 1.0);
	static std::mt19937 generator;
	return distribution(generator);
}

inline float random_float(float min, float max) {
	// Returns a random real in [min,max).
	std::uniform_real_distribution<float> distribution(min, max);
	static std::mt19937 generator;
	return distribution(generator);
}

inline int random_int(int min, int max) {
	// Returns a random integer in [min,max].
	return static_cast<int>(random_float(min, max + 1));
}


// random vec3 generator

static glm::vec3 vec3_random() {
	return glm::vec3(random_float(), random_float(), random_float());
}

static glm::vec3 vec3_random(float min, float max) {
	return glm::vec3(random_float(min, max), random_float(min, max), random_float(min, max));
}


// random vec3 generator sampled in an area
inline glm::vec3 random_in_unit_sphere() {
	while (true) {
		auto p = vec3_random(-1, 1);
		if (glm::length2(p) >= 1) continue;
		return p;
	}
}

inline glm::vec3 random_unit_vector() {
	return glm::normalize(random_in_unit_sphere());
}

inline glm::vec3 random_in_hemisphere(const glm::vec3& normal) {
	glm::vec3 in_unit_sphere = random_in_unit_sphere();
	if (glm::dot(in_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
		return in_unit_sphere;
	else
		return -in_unit_sphere;
}

inline glm::vec3 random_in_unit_disk() {
	while (true) {
		auto p = glm::vec3(random_float(-1, 1), random_float(-1, 1), 0);
		if (glm::length2(p) >= 1) continue;
		return p;
	}
}

// physical ray functions
inline glm::vec3 reflect(const glm::vec3& v, const glm::vec3& n) {
	return v - 2 * glm::dot(v, n) * n;
}

// close to zero check
inline bool near_zero(const glm::vec3& v) {
	// Return true if the vector is close to zero in all dimensions.
	const auto s = 1e-8;
	return (fabs(v.x) < s) && (fabs(v.y) < s) && (fabs(v.z) < s);
}



// Common Headers

#include "interval.h"
#include "ray.h"


#endif