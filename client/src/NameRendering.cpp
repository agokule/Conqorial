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
void find_largest_rectangle(const std::vector<bool>& grid,
                          int grid_width, int grid_height,
                          const Coordinate& min_bounds, const Coordinate& max_bounds,
                          int& out_x, int& out_y, int& out_width, int& out_height) {
    if (grid.empty() || grid_width <= 0 || grid_height <= 0) {
        out_x = out_y = out_width = out_height = 0;
        return;
    }

    // Early exit for tiny regions (can keep this)
    if (grid_width <= 2 || grid_height <= 2) {
        out_x = min_bounds.x;
        out_y = min_bounds.y;
        out_width = grid_width;
        out_height = grid_height;
        return;
    }

    // Distance maps for horizontal and vertical distances (flat array)
    std::vector<DistanceCell> distance(grid_width * grid_height, {0, 0});

    // Variables to track the largest rectangle found so far
    float max_adjusted_area = 0; // Use float to match heuristic calculation
    out_x = out_y = out_width = out_height = 0;

    // Heuristic weight
    const float aspect_ratio_weight = 30.0f;

    // Single pass to calculate distances AND find the largest rectangle
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            int idx = y * grid_width + x;

            if (!grid[idx]) {
                distance[idx].horizontal_distance = 0;
                distance[idx].vertical_distance = 0;
                // No need to check for rectangles ending at an empty cell
                continue;
            }

            // Calculate horizontal distance (from left)
            distance[idx].horizontal_distance = (x > 0 && grid[y * grid_width + (x - 1)]) ?
                distance[y * grid_width + (x - 1)].horizontal_distance + 1 : 1;

            // Calculate vertical distance (from top)
            distance[idx].vertical_distance = (y > 0 && grid[(y - 1) * grid_width + x]) ?
                distance[(y - 1) * grid_width + x].vertical_distance + 1 : 1;

            // --- Rectangle Finding Logic (Integrated) ---
            // Now that we have distances for cell (x, y), check rectangles
            // ending at this cell (bottom-right corner).

            int min_height = distance[idx].vertical_distance;

            // Expand horizontally leftwards from this cell
            for (int width = 1; width <= distance[idx].horizontal_distance; width++) {
                // The current cell being checked is (x - width + 1, y)
                int check_idx = y * grid_width + (x - width + 1);

                // Update the minimum height encountered so far in this horizontal expansion
                min_height = std::min(min_height, distance[check_idx].vertical_distance);

                int area = width * min_height;

                // Skip tiny rectangles early (can adjust threshold if needed)
                // Note: max_adjusted_area is float, cast area for comparison potentially
                if (static_cast<float>(area) * (1.0f + aspect_ratio_weight) < max_adjusted_area / 2.0f) {
                     // Optimization: If even the max possible score is too small, break inner loop?
                     // Not strictly necessary, but could prune search slightly if aspect ratio weight is high.
                     // For now, just continue to next width.
                     continue;
                }


                // Calculate aspect ratio score
                float aspect_ratio = static_cast<float>(std::min(width, min_height)) /
                                     static_cast<float>(std::max(width, min_height));

                // Adjust area with aspect ratio bonus
                float adjusted_area = static_cast<float>(area) * (1.0f + aspect_ratio_weight * aspect_ratio);

                // Update if this is the largest rectangle so far
                if (adjusted_area > max_adjusted_area) {
                    max_adjusted_area = adjusted_area;
                    // Calculate top-left corner based on current bottom-right (x,y) and dimensions
                    out_x = min_bounds.x + x - width + 1;
                    out_y = min_bounds.y + y - min_height + 1;
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
    
    for (int y = 0; y < map_height; ++y) {
        for (int x = 0; x < map_width; ++x) {
            int index = y * map_width + x;
            if (visited[index] || map.get_tile(x, y).owner != country_id) continue;
            
            RegionWithRectangle region;
            
            // Initialize region bounds
            region.min_bounds = {map_width, map_height};
            region.max_bounds = {0, 0};
            
            // Use a more efficient flood fill with a pre-allocated queue
            std::vector<Coordinate> queue;
            queue.reserve(map_width * map_height / 8);
            queue.push_back({x, y});
            visited[index] = true;
            
            // Track region properties during flood fill
            double sum_x = 0.0, sum_y = 0.0;
            int tile_count = 0;
            
            // Create a grid for the region (will be sized based on bounds)
            std::vector<bool> region_grid;

            size_t queue_pos = 0;
            while (queue_pos < queue.size()) {
                Coordinate curr = queue[queue_pos++];
                
                // Update region bounds
                region.min_bounds.x = std::min(region.min_bounds.x, curr.x);
                region.min_bounds.y = std::min(region.min_bounds.y, curr.y);
                region.max_bounds.x = std::max(region.max_bounds.x, curr.x);
                region.max_bounds.y = std::max(region.max_bounds.y, curr.y);
                
                // Update centroid calculation
                sum_x += curr.x + 0.5;
                sum_y += curr.y + 0.5;
                tile_count++;
                
                // Unrolled neighbor checking for better performance
                // Check all four neighbors using direction arrays
                const int dx[4] = {-1, 1, 0, 0}; // left, right, top, bottom
                const int dy[4] = {0, 0, -1, 1}; // left, right, top, bottom
                
                for (int dir = 0; dir < 4; dir++) {
                    int nx = curr.x + dx[dir];
                    int ny = curr.y + dy[dir];
                    
                    // Check if neighbor is within bounds
                    if (nx >= 0 && nx < map_width && ny >= 0 && ny < map_height) {
                        int neighbor_index = ny * map_width + nx;
                        if (!visited[neighbor_index] && map.get_tile(nx, ny).owner == country_id) {
                            visited[neighbor_index] = true;
                            queue.push_back({nx, ny});
                        }
                    }
                }
            }
            
            if (tile_count < min_region_area) continue;
            
            // Store the tile count
            region.tile_count = tile_count;
            
            // Calculate centroid
            region.centroid_x = static_cast<float>(sum_x / tile_count);
            region.centroid_y = static_cast<float>(sum_y / tile_count);
            
            // Ensure bounds are within map limits
            region.min_bounds.x = std::max(0, region.min_bounds.x);
            region.min_bounds.y = std::max(0, region.min_bounds.y);
            region.max_bounds.x = std::min(map_width - 1, region.max_bounds.x);
            region.max_bounds.y = std::min(map_height - 1, region.max_bounds.y);
            
            // Calculate grid dimensions
            int grid_width = region.max_bounds.x - region.min_bounds.x + 1;
            int grid_height = region.max_bounds.y - region.min_bounds.y + 1;
            
            // Create a grid representation of the region
            region_grid.resize(grid_width * grid_height, false);
            
            // Fill the grid using the stored region tiles
            for(const auto& tile_coord : queue) {
                // No need to check owner again, already confirmed during flood fill
                 int grid_idx = (tile_coord.y - region.min_bounds.y) * grid_width + (tile_coord.x - region.min_bounds.x);
                 // Bounds check (shouldn't be necessary if bounds calculation is correct, but safer)
                 if (grid_idx >= 0 && grid_idx < region_grid.size()) {
                    region_grid[grid_idx] = true;
                 } else {
                    // Log error if this happens, indicates bug in bounds/indexing
                    CQ_LOG_DIST_ERROR << "Grid index out of bounds during region grid creation!\n";
                 }
            }
            
            // Find largest inscribed rectangle
            find_largest_rectangle(region_grid, grid_width, grid_height, 
                                 region.min_bounds, region.max_bounds,
                                 region.rect_x, region.rect_y, 
                                 region.rect_width, region.rect_height);
            
            region.area = static_cast<float>(tile_count);
            
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
        
        CQ_LOG_DEBUG << "Updating region cache for countries\n";
        for (const auto& [country_id, country] : countries) {
            if (country_id == 0) continue;
            
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
        TextCache text_entry {};
        const std::string &country_name = country.get_name();
        std::string troops_str = std::to_string(country.get_troops());
        text_entry.formatted_text = country_name + "\n" + troops_str;
        text_entry.base_text_size = ImGui::CalcTextSize(text_entry.formatted_text.c_str());
        
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

