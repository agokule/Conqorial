#include "NameRendering.h"
#include "Logging.h"
#include "Country.h"
#include "Map.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include <map>
#include <queue>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

// Find the largest inscribed rectangle within a region
void find_largest_rectangle(const std::vector<std::pair<int, int>>& region_tiles, 
                          int map_width, int map_height, 
                          int& out_x, int& out_y, int& out_width, int& out_height) {
    if (region_tiles.empty()) {
        out_x = out_y = out_width = out_height = 0;
        return;
    }
    
    // Create a grid representation of the region
    std::vector<std::vector<bool>> grid(map_height, std::vector<bool>(map_width, false));
    for (const auto& [x, y] : region_tiles) {
        if (x >= 0 && x < map_width && y >= 0 && y < map_height) {
            grid[y][x] = true;
        }
    }
    
    // Distance maps for horizontal and vertical distances
    std::vector<std::vector<DistanceCell>> distance(map_height, 
                                                 std::vector<DistanceCell>(map_width, {0, 0}));
    
    // Calculate horizontal distances (distance to left edge)
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            if (!grid[y][x]) {
                distance[y][x].horizontal_distance = 0;
            } else {
                distance[y][x].horizontal_distance = (x > 0) ? 
                    distance[y][x-1].horizontal_distance + 1 : 1;
            }
        }
    }
    
    // Calculate vertical distances (distance to top edge)
    for (int x = 0; x < map_width; x++) {
        for (int y = 0; y < map_height; y++) {
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
    
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            if (!grid[y][x]) continue;
            
            int min_height = distance[y][x].vertical_distance;
            
            // Expand horizontally from this cell
            for (int width = 1; width <= distance[y][x].horizontal_distance; width++) {
                // Check height constraint as we expand horizontally
                min_height = std::min(min_height, distance[y][x - width + 1].vertical_distance);
                int area = width * min_height;
                
                // Update if this is the largest rectangle so far
                if (area > max_area) {
                    max_area = area;
                    out_x = x - width + 1;
                    out_y = y - min_height + 1;
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
            std::queue<std::pair<int, int>> queue;
            queue.push({x, y});
            visited[index] = true;
            
            // Flood fill to find region
            while (!queue.empty()) {
                auto [curr_x, curr_y] = queue.front();
                queue.pop();
                region.tiles.emplace_back(curr_x, curr_y);
                
                const int dx[] = {-1, 1, 0, 0};
                const int dy[] = {0, 0, -1, 1};
                for (int i = 0; i < 4; ++i) {
                    int nx = curr_x + dx[i];
                    int ny = curr_y + dy[i];
                    if (nx < 0 || nx >= map_width || ny < 0 || ny >= map_height) continue;
                    
                    int neighbor_index = ny * map_width + nx;
                    if (!visited[neighbor_index] && map.get_tile(nx, ny).owner == country_id) {
                        visited[neighbor_index] = true;
                        queue.push({nx, ny});
                    }
                }
            }
            
            if (region.tiles.size() < min_region_area) continue;
            
            // Calculate centroid
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
            region.color.r = countries.at(country_id).get_color().r;
            region.color.g = countries.at(country_id).get_color().g;
            region.color.b = countries.at(country_id).get_color().b;
            region.color.a = countries.at(country_id).get_color().a;
            
            // Only add regions with non-trivial rectangles
            if (region.rect_width >= 3 && region.rect_height >= 2) {
                output.push_back(region);
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
    static const float min_font_size = 8.0f;
    static const float max_font_size = 36.0f;

    // Calculate conversion factors between map and screen coordinates
    float map_to_screen_x = view_rect.w / map_width;
    float map_to_screen_y = view_rect.h / map_height;
    
    // Update cache if needed (we'll use it just for visualization)
    if (update_cache) {
        for (const auto& [country_id, country] : countries) {
            if (country_id == 0) continue;
            
            CQ_LOG_DEBUG << "Updating region cache for country " << (short)country_id << '\n';
            std::vector<RegionWithRectangle> regions;
            find_country_regions_with_rectangles(map, country_id, regions, countries);
            cache[country_id] = regions;
        }
    }
    
    // Process each country
    for (const auto& [country_id, country] : countries) {
        if (country_id == 0) continue; // Skip neutral territory
        
        // Find regions with largest inscribed rectangles
        std::vector<RegionWithRectangle> regions_with_rectangles;
        find_country_regions_with_rectangles(map, country_id, regions_with_rectangles, countries);
        
        // Prepare text content
        const std::string &country_name = country.get_name();
        std::string troops_str = std::to_string(country.get_troops());
        std::stringstream text;
        text << country_name << "\n" << troops_str;
        std::string formatted_text = text.str();
        
        // For each region, fit text in its largest inscribed rectangle
        for (const auto& region : regions_with_rectangles) {
            // Convert rectangle to screen coordinates
            float screen_rect_x = view_rect.x + region.rect_x * map_to_screen_x;
            float screen_rect_y = view_rect.y + region.rect_y * map_to_screen_y;
            float screen_rect_width = region.rect_width * map_to_screen_x;
            float screen_rect_height = region.rect_height * map_to_screen_y;
            
            // Skip if rectangle is outside view
            if (screen_rect_x + screen_rect_width < view_rect.x || 
                screen_rect_x > view_rect.x + view_rect.w ||
                screen_rect_y + screen_rect_height < view_rect.y || 
                screen_rect_y > view_rect.y + view_rect.h) {
                continue;
            }
            
            // Calculate optimal font size to fit text in rectangle
            float base_font_size = ImGui::GetFontSize();
            ImVec2 base_text_size = ImGui::CalcTextSize(formatted_text.c_str());
            
            // Calculate aspect ratio of text and rectangle
            float text_aspect_ratio = base_text_size.x / base_text_size.y;
            float rect_aspect_ratio = screen_rect_width / screen_rect_height;
            
            // Calculate font size based on rectangle dimensions and text aspect ratio
            float font_size;
            if (text_aspect_ratio > rect_aspect_ratio) {
                // Width-constrained
                font_size = (screen_rect_width * 0.85f) / (base_text_size.x / base_font_size);
            } else {
                // Height-constrained
                font_size = (screen_rect_height * 0.85f) / (base_text_size.y / base_font_size);
            }
            
            // Clamp font size to reasonable limits
            font_size = std::clamp(font_size, min_font_size, max_font_size);
            
            // Skip rendering if font would be too small
            if (font_size < min_font_size) continue;
            
            // Calculate actual text size with calculated font size
            ImVec2 text_size;
            text_size.x = base_text_size.x * (font_size / base_font_size);
            text_size.y = base_text_size.y * (font_size / base_font_size);
            
            // Center text in rectangle
            float text_x = screen_rect_x + (screen_rect_width - text_size.x) * 0.5f;
            float text_y = screen_rect_y + (screen_rect_height - text_size.y) * 0.5f;
            
            // Choose text color based on background brightness
            ImU32 text_color;
            float lightness = (region.color.r * 0.299f + region.color.g * 0.587f + region.color.b * 0.114f) / 255.0f;
            if (lightness > 0.5f) {
                text_color = IM_COL32(0, 0, 0, 255); // Black text on light background
            } else {
                text_color = IM_COL32(255, 255, 255, 255); // White text on dark background
            }
            
            // For debugging - draw rectangle outline
            // draw_list->AddRect(
            //     ImVec2(screen_rect_x, screen_rect_y),
            //     ImVec2(screen_rect_x + screen_rect_width, screen_rect_y + screen_rect_height),
            //     IM_COL32(255, 255, 255, 120)
            // );
            
            // Draw text
            draw_list->AddText(nullptr, font_size, ImVec2(text_x, text_y), text_color, formatted_text.c_str());
        }
    }
}
