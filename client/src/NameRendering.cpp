#include "NameRendering.h"
#include "Logging.h"
#include "Country.h"
#include "Map.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "imgui.h"
#include <map>
#include <algorithm>
#include <string>
#include <vector>

// Initialize static member
uint64_t CacheTimestamp::last_update = 0;

uint64_t CacheTimestamp::get_current_time() {
    return SDL_GetTicks();
}

bool CacheTimestamp::should_update(uint64_t interval_ms) {
    uint64_t current_time = get_current_time();
    if (current_time - last_update > interval_ms) {
        last_update = current_time;
        return true;
    }
    return false;
}

// Find the largest inscribed rectangle within a region
void find_largest_rectangle(const std::vector<std::pair<int, int>>& region_tiles, 
                          int map_width, int map_height, 
                          int& out_x, int& out_y, int& out_width, int& out_height) {
    if (region_tiles.empty()) {
        out_x = out_y = out_width = out_height = 0;
        return;
    }
    
    // Find bounding box of the region to reduce grid size
    int min_x = map_width, min_y = map_height, max_x = 0, max_y = 0;
    for (const auto& [x, y] : region_tiles) {
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }
    
    // Add a small margin to ensure we don't go out of bounds
    min_x = std::max(0, min_x);
    min_y = std::max(0, min_y);
    max_x = std::min(map_width - 1, max_x);
    max_y = std::min(map_height - 1, max_y);
    
    // Calculate grid dimensions
    int grid_width = max_x - min_x + 1;
    int grid_height = max_y - min_y + 1;
    
    // Early exit for tiny regions
    if (grid_width <= 2 || grid_height <= 2) {
        out_x = min_x;
        out_y = min_y;
        out_width = grid_width;
        out_height = grid_height;
        return;
    }
    
    // Create a grid representation of the region (only for the bounding box)
    std::vector<std::vector<bool>> grid(grid_height, std::vector<bool>(grid_width, false));
    
    // Use a flat map for faster lookups
    std::vector<bool> flat_grid(map_width * map_height, false);
    for (const auto& [x, y] : region_tiles) {
        if (x >= 0 && x < map_width && y >= 0 && y < map_height) {
            flat_grid[y * map_width + x] = true;
            
            // Also fill the smaller grid
            if (x >= min_x && x <= max_x && y >= min_y && y <= max_y) {
                grid[y - min_y][x - min_x] = true;
            }
        }
    }
    
    // Distance maps for horizontal and vertical distances
    std::vector<std::vector<DistanceCell>> distance(grid_height, 
                                                 std::vector<DistanceCell>(grid_width, {0, 0}));
    
    // Calculate horizontal distances (distance to left edge)
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            if (!grid[y][x]) {
                distance[y][x].horizontal_distance = 0;
            } else {
                distance[y][x].horizontal_distance = (x > 0) ? 
                    distance[y][x-1].horizontal_distance + 1 : 1;
            }
        }
    }
    
    // Calculate vertical distances (distance to top edge)
    for (int x = 0; x < grid_width; x++) {
        for (int y = 0; y < grid_height; y++) {
            if (!grid[y][x]) {
                distance[y][x].vertical_distance = 0;
            } else {
                distance[y][x].vertical_distance = (y > 0) ? 
                    distance[y-1][x].vertical_distance + 1 : 1;
            }
        }
    }
    
    // Find largest rectangle
    int max_area = 0;
    out_x = out_y = out_width = out_height = 0;
    
    // Use a heuristic to prioritize more square-like rectangles
    // by giving a slight bonus to rectangles with better aspect ratios
    const float aspect_ratio_weight = 0.05f; // Small weight to avoid distorting the results too much
    
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            if (!grid[y][x]) continue;
            
            int min_height = distance[y][x].vertical_distance;
            
            // Expand horizontally from this cell
            for (int width = 1; width <= distance[y][x].horizontal_distance; width++) {
                // Check height constraint as we expand horizontally
                min_height = std::min(min_height, distance[y][x - width + 1].vertical_distance);
                int area = width * min_height;
                
                // Skip tiny rectangles early
                if (area < max_area / 2) continue;
                
                // Calculate aspect ratio score (1.0 for square, less for elongated rectangles)
                float aspect_ratio = static_cast<float>(std::min(width, min_height)) / 
                                    static_cast<float>(std::max(width, min_height));
                
                // Adjust area with aspect ratio bonus
                float adjusted_area = static_cast<float>(area) * (1.0f + aspect_ratio_weight * aspect_ratio);
                
                // Update if this is the largest rectangle so far
                if (adjusted_area > max_area) {
                    max_area = adjusted_area;
                    out_x = min_x + x - width + 1;
                    out_y = min_y + y - min_height + 1;
                    out_width = width;
                    out_height = min_height;
                }
            }
        }
    }
}

// Find regions for a country with their largest inscribed rectangles
void find_country_regions_with_rectangles(const Map& map, CountryId country_id,
                                        std::vector<RegionWithRectangle>& output,
                                        const std::map<CountryId, Country>& countries) {
    const int min_region_area = 25;
    const int map_width = map.get_width();
    const int map_height = map.get_height();
    std::vector<bool> visited(map_width * map_height, false);
    
    // Pre-allocate memory for region tiles to avoid reallocations
    std::vector<std::pair<int, int>> temp_tiles;
    temp_tiles.reserve(map_width * map_height / 4); // Reasonable initial capacity
    
    for (int y = 0; y < map_height; ++y) {
        for (int x = 0; x < map_width; ++x) {
            int index = y * map_width + x;
            if (visited[index] || map.get_tile(x, y).owner != country_id) continue;
            
            RegionWithRectangle region;
            temp_tiles.clear();
            
            // Use a more efficient flood fill with a pre-allocated queue
            std::vector<std::pair<int, int>> queue;
            queue.reserve(map_width * map_height / 8);
            queue.push_back({x, y});
            visited[index] = true;
            
            size_t queue_pos = 0;
            while (queue_pos < queue.size()) {
                auto [curr_x, curr_y] = queue[queue_pos++];
                temp_tiles.emplace_back(curr_x, curr_y);
                
                // Unrolled neighbor checking for better performance
                // Check left neighbor
                if (curr_x > 0) {
                    int nx = curr_x - 1;
                    int ny = curr_y;
                    int neighbor_index = ny * map_width + nx;
                    if (!visited[neighbor_index] && map.get_tile(nx, ny).owner == country_id) {
                        visited[neighbor_index] = true;
                        queue.push_back({nx, ny});
                    }
                }
                
                // Check right neighbor
                if (curr_x < map_width - 1) {
                    int nx = curr_x + 1;
                    int ny = curr_y;
                    int neighbor_index = ny * map_width + nx;
                    if (!visited[neighbor_index] && map.get_tile(nx, ny).owner == country_id) {
                        visited[neighbor_index] = true;
                        queue.push_back({nx, ny});
                    }
                }
                
                // Check top neighbor
                if (curr_y > 0) {
                    int nx = curr_x;
                    int ny = curr_y - 1;
                    int neighbor_index = ny * map_width + nx;
                    if (!visited[neighbor_index] && map.get_tile(nx, ny).owner == country_id) {
                        visited[neighbor_index] = true;
                        queue.push_back({nx, ny});
                    }
                }
                
                // Check bottom neighbor
                if (curr_y < map_height - 1) {
                    int nx = curr_x;
                    int ny = curr_y + 1;
                    int neighbor_index = ny * map_width + nx;
                    if (!visited[neighbor_index] && map.get_tile(nx, ny).owner == country_id) {
                        visited[neighbor_index] = true;
                        queue.push_back({nx, ny});
                    }
                }
            }
            
            if (temp_tiles.size() < min_region_area) continue;
            
            // Move tiles to region
            region.tiles = std::move(temp_tiles);
            temp_tiles.reserve(map_width * map_height / 4); // Restore capacity
            
            // Calculate centroid with running sum to avoid second loop
            double sum_x = 0.0, sum_y = 0.0;
            for (const auto& [x_pos, y_pos] : region.tiles) {
                sum_x += x_pos + 0.5;
                sum_y += y_pos + 0.5;
            }
            region.centroid_x = static_cast<float>(sum_x / region.tiles.size());
            region.centroid_y = static_cast<float>(sum_y / region.tiles.size());
            
            // Find largest inscribed rectangle
            find_largest_rectangle(region.tiles, map_width, map_height, 
                                 region.rect_x, region.rect_y, 
                                 region.rect_width, region.rect_height);
            
            region.area = static_cast<float>(region.tiles.size());
            
            // Avoid map lookup with .at() for better performance
            const auto& country = countries.find(country_id)->second;
            const auto& color = country.get_color();
            region.color.r = color.r;
            region.color.g = color.g;
            region.color.b = color.b;
            region.color.a = color.a;
            
            // Only add regions with non-trivial rectangles
            if (region.rect_width >= 3 && region.rect_height >= 2) {
                output.push_back(std::move(region));
            }
        }
    }
}

void render_country_labels(SDL_Renderer* renderer, ImDrawList* draw_list,
                         const Map& map, const SDL_FRect& view_rect,
                         const std::map<CountryId, Country>& countries,
                         RegionCache& cache, bool update_cache) {
    static const float map_width = static_cast<float>(map.get_width());
    static const float map_height = static_cast<float>(map.get_height());

    // Get ImGui scaling factor to ensure we respect UI scaling
    float imgui_scale = ImGui::GetIO().FontGlobalScale;
    float min_font_size = 8.0f * imgui_scale;
    float max_font_size = 36.0f * imgui_scale;
    
    // Early exit if no countries or if view is invalid
    if (countries.empty() || view_rect.w <= 0 || view_rect.h <= 0) {
        return;
    }

    // Calculate conversion factors between map and screen coordinates
    float map_to_screen_x = view_rect.w / map_width;
    float map_to_screen_y = view_rect.h / map_height;
    
    // Update cache if needed
    if (update_cache) {
        cache.clear(); // Clear the cache to avoid stale data
        
        for (const auto& [country_id, country] : countries) {
            if (country_id == 0) continue;
            
            CQ_LOG_DEBUG << "Updating region cache for country " << (short)country_id << '\n';
            std::vector<RegionWithRectangle> regions;
            regions.reserve(8); // Pre-allocate for typical number of regions
            find_country_regions_with_rectangles(map, country_id, regions, countries);
            
            if (!regions.empty()) {
                cache[country_id] = std::move(regions);
            }
        }
    }
    
    // Cache for text formatting to avoid repeated string operations
    struct TextCache {
        std::string formatted_text;
        ImVec2 base_text_size;
    };
    std::unordered_map<CountryId, TextCache> text_cache;
    text_cache.reserve(countries.size());
    
    // Process each country using the cache
    for (const auto& [country_id, country] : countries) {
        if (country_id == 0) continue; // Skip neutral territory
        
        // Use cached regions if available, otherwise compute them
        const std::vector<RegionWithRectangle>* regions_ptr;
        auto cache_it = cache.find(country_id);
        
        if (cache_it != cache.end()) {
            regions_ptr = &cache_it->second;
        } else {
            // If not in cache, skip this country for this frame
            // It will be added to the cache on the next update_cache=true call
            continue;
        }
        
        // Skip if no regions
        if (regions_ptr->empty()) continue;
        
        // Prepare text content (cached per country)
        auto& text_entry = text_cache[country_id];
        if (text_entry.formatted_text.empty()) {
            const std::string &country_name = country.get_name();
            std::string troops_str = std::to_string(country.get_troops());
            text_entry.formatted_text = country_name + "\n" + troops_str;
            text_entry.base_text_size = ImGui::CalcTextSize(text_entry.formatted_text.c_str());
        }
        
        float base_font_size = ImGui::GetFontSize();
        
        // For each region, fit text in its largest inscribed rectangle
        for (const auto& region : *regions_ptr) {
            // Convert rectangle to screen coordinates
            float screen_rect_x = view_rect.x + region.rect_x * map_to_screen_x;
            float screen_rect_y = view_rect.y + region.rect_y * map_to_screen_y;
            float screen_rect_width = region.rect_width * map_to_screen_x;
            float screen_rect_height = region.rect_height * map_to_screen_y;
            
            // Fast AABB check - skip if rectangle is outside view
            if (screen_rect_x + screen_rect_width < view_rect.x || 
                screen_rect_x > view_rect.x + view_rect.w ||
                screen_rect_y + screen_rect_height < view_rect.y || 
                screen_rect_y > view_rect.y + view_rect.h) {
                continue;
            }
            
            // Calculate aspect ratio of text and rectangle
            float text_aspect_ratio = text_entry.base_text_size.x / text_entry.base_text_size.y;
            float rect_aspect_ratio = screen_rect_width / screen_rect_height;
            
            // Calculate font size based on rectangle dimensions and text aspect ratio
            float font_size;
            if (text_aspect_ratio > rect_aspect_ratio) {
                // Width-constrained
                font_size = (screen_rect_width * 0.85f) / (text_entry.base_text_size.x / base_font_size);
            } else {
                // Height-constrained
                font_size = (screen_rect_height * 0.85f) / (text_entry.base_text_size.y / base_font_size);
            }
            
            // Clamp font size to reasonable limits
            font_size = std::clamp(font_size, min_font_size, max_font_size);
            
            // Skip rendering if font would be too small
            if (font_size < min_font_size) continue;
            
            // Calculate actual text size with calculated font size
            ImVec2 text_size;
            text_size.x = text_entry.base_text_size.x * (font_size / base_font_size);
            text_size.y = text_entry.base_text_size.y * (font_size / base_font_size);
            
            // Center text in rectangle
            float text_x = screen_rect_x + (screen_rect_width - text_size.x) * 0.5f;
            float text_y = screen_rect_y + (screen_rect_height - text_size.y) * 0.5f;
            
            // Choose text color based on background brightness (pre-computed)
            static std::unordered_map<uint32_t, ImU32> color_cache;
            
            // Create a color key for caching
            uint32_t color_key = (static_cast<uint32_t>(region.color.r) << 24) | 
                                (static_cast<uint32_t>(region.color.g) << 16) | 
                                (static_cast<uint32_t>(region.color.b) << 8) | 
                                static_cast<uint32_t>(region.color.a);
            
            ImU32 text_color;
            auto color_it = color_cache.find(color_key);
            
            if (color_it != color_cache.end()) {
                text_color = color_it->second;
            } else {
                float lightness = (region.color.r * 0.299f + region.color.g * 0.587f + region.color.b * 0.114f) / 255.0f;
                text_color = (lightness > 0.5f) ? IM_COL32(0, 0, 0, 255) : IM_COL32(255, 255, 255, 255);
                color_cache[color_key] = text_color;
            }
            
            // Draw text
            draw_list->AddText(nullptr, font_size, ImVec2(text_x, text_y), text_color, text_entry.formatted_text.c_str());
        }
    }
}

