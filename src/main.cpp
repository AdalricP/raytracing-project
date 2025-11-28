#include <iostream>
#include <SDL.h>
#include <vector>

#include "camera.h"
#include "float.h"
#include "hitable_list.h"
#include "material.h"
#include "sphere.h"

// Screen dimensions
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 600;
const int SAMPLES_PER_PIXEL = 1; // Reduced for real-time performance
const int MAX_DEPTH = 50;

vec3 random_in_unit_sphere() {
  vec3 p;
  do {
    p = 2.0 * vec3(drand48(), drand48(), drand48()) - vec3(1, 1, 1);
  } while (p.squared_length() >= 1.0);
  return p;
}

class lambertian : public material {
 public:
  lambertian(const vec3& a) : albedo(a) {}
  virtual bool scatter(const ray& r_in, const hit_record& rec,
                       vec3& attenuation, ray& scattered) const {
    vec3 target = rec.p + rec.normal + random_in_unit_sphere();
    scattered = ray(rec.p, target - rec.p);
    attenuation = albedo;
    (void)r_in;
    return true;
  }
  vec3 albedo;
};

bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted) {
  vec3 uv = unit_vector(v);
  float dt = dot(uv, n);
  float discriminant = 1.0 - ni_over_nt * ni_over_nt * (1 - dt * dt);
  if (discriminant > 0) {
    refracted = ni_over_nt * (uv - dt * n) - sqrt(discriminant) * n;
    return true;
  } else
    return false;
}

vec3 reflect(const vec3& v, const vec3& n) { return v - 2 * dot(v, n) * n; }
class metal : public material {
 public:
  metal(const vec3& a, float f) : albedo(a) {
    if (f < 1)
      fuzz = f;
    else
      fuzz = 1;
  }
  virtual bool scatter(const ray& r_in, const hit_record& rec,
                       vec3& attenuation, ray& scattered) const {
    vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
    scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere());
    attenuation = albedo;
    return (dot(scattered.direction(), rec.normal) > 0);
  }
  float fuzz;
  vec3 albedo;
};

float schlick(float cosine, float ref_idx) {
  float r0 = (1 - ref_idx) / (1 + ref_idx);
  r0 = r0 * r0;
  return r0 + (1 - r0) * pow((1 - cosine), 5);
}

class dielectric : public material {
 public:
  dielectric(float ri) : ref_idx(ri) {}
  virtual bool scatter(const ray& r_in, const hit_record& rec,
                       vec3& attenuation, ray& scattered) const {
    vec3 outward_normal;
    vec3 reflected = reflect(r_in.direction(), rec.normal);
    float ni_over_nt;
    attenuation = vec3(1.0, 1.0, 1.0);
    vec3 refracted;
    float reflect_prob;
    float cosine;

    if (dot(r_in.direction(), rec.normal) > 0) {
      outward_normal = -rec.normal;
      ni_over_nt = ref_idx;
      cosine = ref_idx * dot(r_in.direction(), rec.normal) /
               r_in.direction().length();
    } else {
      outward_normal = rec.normal;
      ni_over_nt = 1.0 / ref_idx;
      cosine = -dot(r_in.direction(), rec.normal) / r_in.direction().length();
    }

    if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted)) {
      reflect_prob = schlick(cosine, ref_idx);
    } else {
      scattered = ray(rec.p, reflected);
      reflect_prob = 1.0;
    }

    if (drand48() < reflect_prob) {
      scattered = ray(rec.p, reflected);
    } else {
      scattered = ray(rec.p, refracted);
    }

    return true;
  }
  float ref_idx;
};

vec3 color(const ray& r, hitable* world, int depth) {
  hit_record rec;
  if (world->hit(r, 0.001, MAXFLOAT, rec)) {
    ray scattered;
    vec3 attenuation;
    if (depth < MAX_DEPTH && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
      return attenuation * color(scattered, world, depth + 1);
    } else {
      return vec3(0, 0, 0);
    }
  } else {
    vec3 unit_direction = unit_vector(r.direction());
    float t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
  }
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow("Raytracer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!window) {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!texture) {
    std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  hitable* list[3];
  list[0] = new sphere(vec3(0, -100.5, -1), 100, new lambertian(vec3(0.8, 0.8, 0.8)));
  list[1] = new sphere(vec3(-0.6, 0, -1), 0.5, new lambertian(vec3(0.05, 0.05, 0.05)));
  list[2] = new sphere(vec3(0.6, 0, -1), 0.5, new metal(vec3(0.1, 0.1, 0.1), 0.0));
  hitable* world = new hitable_list(list, 3);

  vec3 lookfrom(0, 0, 0);
  vec3 lookat(0, 0, -1);
  vec3 vup(0, 1, 0);
  camera cam(lookfrom, lookat, vup, 90, float(SCREEN_WIDTH) / float(SCREEN_HEIGHT));

  bool quit = false;
  SDL_Event e;
  std::vector<uint32_t> pixels(SCREEN_WIDTH * SCREEN_HEIGHT);
  std::vector<vec3> accumulation_buffer(SCREEN_WIDTH * SCREEN_HEIGHT, vec3(0, 0, 0));
  int total_samples = 0;

  // Input state
  const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
  float speed = 0.1f;
  
  // Mouse capture
  SDL_SetRelativeMouseMode(SDL_TRUE);

  while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      } else if (e.type == SDL_MOUSEMOTION) {
        if (cam.process_mouse(e.motion.xrel, -e.motion.yrel)) {
            std::fill(accumulation_buffer.begin(), accumulation_buffer.end(), vec3(0, 0, 0));
            total_samples = 0;
        }
      }
    }

    // Keyboard input
    if (currentKeyStates[SDL_SCANCODE_ESCAPE]) quit = true;
    bool moved = false;
    if (currentKeyStates[SDL_SCANCODE_W]) moved |= cam.process_keyboard(0, speed);
    if (currentKeyStates[SDL_SCANCODE_S]) moved |= cam.process_keyboard(1, speed);
    if (currentKeyStates[SDL_SCANCODE_A]) moved |= cam.process_keyboard(2, speed);
    if (currentKeyStates[SDL_SCANCODE_D]) moved |= cam.process_keyboard(3, speed);
    if (currentKeyStates[SDL_SCANCODE_SPACE]) moved |= cam.process_keyboard(4, speed);
    if (currentKeyStates[SDL_SCANCODE_LSHIFT]) moved |= cam.process_keyboard(5, speed);

    if (moved) {
      std::fill(accumulation_buffer.begin(), accumulation_buffer.end(), vec3(0, 0, 0));
      total_samples = 0;
    }

    total_samples++;

    // Render
    #pragma omp parallel for schedule(dynamic)
    for (int j = SCREEN_HEIGHT - 1; j >= 0; j--) {
      for (int i = 0; i < SCREEN_WIDTH; i++) {
        vec3 col(0, 0, 0);
        for (int s = 0; s < SAMPLES_PER_PIXEL; s++) {
          float u = float(i + drand48()) / float(SCREEN_WIDTH);
          float v = float(j + drand48()) / float(SCREEN_HEIGHT);
          ray r = cam.get_ray(u, v);
          col += color(r, world, 0);
        }
        
        int pixel_index = (SCREEN_HEIGHT - 1 - j) * SCREEN_WIDTH + i;
        accumulation_buffer[pixel_index] += col;
        vec3 accumulated_col = accumulation_buffer[pixel_index] / float(total_samples * SAMPLES_PER_PIXEL);
        
        col = accumulated_col;
        col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

        int ir = int(255.99 * col[0]);
        int ig = int(255.99 * col[1]);
        int ib = int(255.99 * col[2]);

        // SDL uses ARGB
        // SDL uses ARGB
        uint32_t pixelColor = (255 << 24) | (ir << 16) | (ig << 8) | ib;
        pixels[pixel_index] = pixelColor;
      }
    }

    SDL_UpdateTexture(texture, NULL, pixels.data(), SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
