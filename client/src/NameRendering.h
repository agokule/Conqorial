#pragma once

#include "Map.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "typedefs.h"
#include "Country.h"
#include <map>
#include <unordered_map>
#include <vector>

struct CountryRegion {
    float centroid_x;
    float centroid_y;
    float area;
    SDL_Color color;
};

// Data structure for regions with largest inscribed rectangle
struct RegionWithRectangle : public CountryRegion {
    // Region boundary and tiles
    std::vector<std::pair<int, int>> tiles;
    
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

void render_country_labels(SDL_Renderer* renderer, ImDrawList* draw_list, 
                         const Map& map, const SDL_FRect& view_rect,
                         const std::map<CountryId, Country>& countries,
                         RegionCache& cache, bool update_cache);


