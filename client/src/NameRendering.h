#pragma once

#include "Map.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "typedefs.h"
#include "Country.h"
#include <map>
#include <unordered_map>
#include "utils.h"
#include <vector>

struct CountryRegion {
    float centroid_x;
    float centroid_y;
    float area;
    SDL_Color color;
};

// Data structure for regions with largest inscribed rectangle
struct RegionWithRectangle : public CountryRegion {
    // Number of tiles in the region (instead of storing all tiles)
    int tile_count;
    
    // Region bounds
    Coordinate min_bounds;
    Coordinate max_bounds;
    
    // Largest inscribed rectangle
    int rect_x, rect_y;       // Top-left corner
    int rect_width, rect_height; // Dimensions
};

// Helper structure for rectangle finding algorithm
struct DistanceCell {
    int horizontal_distance;
    int vertical_distance;
};


using RegionCache = std::unordered_map<CountryId, std::vector<RegionWithRectangle>>;

// Timestamp for cache invalidation
struct CacheTimestamp {
    static uint64_t last_update;
    static uint64_t get_current_time();
    static bool should_update(uint64_t interval_ms);
};

// Find the largest inscribed rectangle within a region
void find_largest_rectangle(const std::vector<bool>& grid, 
                          int grid_width, int grid_height,
                          const Coordinate& min_bounds, const Coordinate& max_bounds,
                          int& out_x, int& out_y, int& out_width, int& out_height);

void render_country_labels(SDL_Renderer* renderer, ImDrawList* draw_list, 
                         const Map& map, const SDL_FRect& view_rect,
                         const std::map<CountryId, Country>& countries,
                         RegionCache& cache, bool update_cache);

