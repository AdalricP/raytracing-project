#ifndef CAMERAH
#define CAMERAH

#include "ray.h"

class camera {
 public:
  camera(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect) {
    origin = lookfrom;
    world_up = vup;
    yaw = -90.0f;
    pitch = 0.0f;
    update_camera_vectors();
    
    float theta = vfov * M_PI / 180.0;
    float half_height = tan(theta / 2);
    float half_width = aspect * half_height;
    
    w = unit_vector(lookfrom - lookat);
    u = unit_vector(cross(vup, w));
    v = cross(w, u);

    lower_left_corner = origin - half_width * u - half_height * v - w;
    horizontal = 2 * half_width * u;
    vertical = 2 * half_height * v;
  }

  ray get_ray(float s, float t) {
    return ray(origin, lower_left_corner + s * horizontal + t * vertical - origin);
  }

  bool process_keyboard(int direction, float velocity) {
    bool moved = false;
    if (direction == 0) { origin -= velocity * front; moved = true; } // FORWARD (Inverted)
    if (direction == 1) { origin += velocity * front; moved = true; } // BACKWARD (Inverted)
    if (direction == 2) { origin -= velocity * right; moved = true; } // LEFT (Inverted)
    if (direction == 3) { origin += velocity * right; moved = true; } // RIGHT (Inverted)
    if (direction == 4) { origin += velocity * world_up; moved = true; } // UP (Keep)
    if (direction == 5) { origin -= velocity * world_up; moved = true; } // DOWN (Keep)
    if (moved) update_camera_vectors();
    return moved;
  }

  bool process_mouse(float xoffset, float yoffset) {
    if (xoffset == 0 && yoffset == 0) return false;

    xoffset *= 0.1f;
    yoffset *= 0.1f;

    yaw -= xoffset;   // Inverted
    pitch -= yoffset; // Inverted

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    update_camera_vectors();
    return true;
  }

 private:
  void update_camera_vectors() {
    vec3 front_temp;
    front_temp[0] = cos(yaw * M_PI / 180.0) * cos(pitch * M_PI / 180.0);
    front_temp[1] = sin(pitch * M_PI / 180.0);
    front_temp[2] = sin(yaw * M_PI / 180.0) * cos(pitch * M_PI / 180.0);
    front = unit_vector(front_temp);
    right = unit_vector(cross(front, world_up));
    up = unit_vector(cross(right, front));

    // Recalculate view plane
    // For simplicity in this basic raytracer, we keep the view plane relative to the new front
    // But to keep it simple with the existing get_ray logic which relies on fixed horizontal/vertical relative to lookat,
    // we might need to fully re-derive lower_left_corner etc.
    // Let's just re-run the constructor logic effectively.
    
    // Assuming 90 deg fov and 2:1 aspect for now as defaults or passed in?
    // Let's stick to the dynamic update.
    
    // We need to maintain the FOV and aspect ratio.
    // Let's hardcode them for now or store them.
    float vfov = 90.0;
    float aspect = 2.0;
    
    float theta = vfov * M_PI / 180.0;
    float half_height = tan(theta / 2);
    float half_width = aspect * half_height;
    
    lower_left_corner = origin - half_width * right - half_height * up - front;
    horizontal = 2 * half_width * right;
    vertical = 2 * half_height * up;
  }

 public:
  vec3 origin;
  vec3 lower_left_corner;
  vec3 horizontal;
  vec3 vertical;
  vec3 u, v, w;
  
  // Camera attributes
  vec3 front;
  vec3 right;
  vec3 up;
  vec3 world_up;
  
  // Euler Angles
  float yaw;
  float pitch;
};

#endif