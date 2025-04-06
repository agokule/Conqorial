#include "NameRendering.h"
#include "Country.h"
#include "Map.h"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include <iomanip>
#include <map>
#include <queue>
#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

static void find_country_regions(const Map& map, CountryId country_id,
                               std::vector<CountryRegion>& output,
                               const std::map<CountryId, Country>& countries) {
    const int min_region_area = 25;
    const int map_width = map.get_width();
    const int map_height = map.get_height();
    std::vector<bool> visited(map_width * map_height, false);
    
    for (int y = 0; y < map_height; ++y) {
        for (int x = 0; x < map_width; ++x) {
            int index = y * map_width + x;
            if (visited[index] || map.get_tile(x, y).owner != country_id) continue;

            std::queue<std::pair<int, int>> queue;
            std::vector<std::pair<int, int>> region;
            std::unordered_set<int> region_tiles;
            queue.push({x, y});
            visited[index] = true;
            
            // Flood fill with tile tracking
            while (!queue.empty()) {
                auto [curr_x, curr_y] = queue.front();
                queue.pop();
                region.emplace_back(curr_x, curr_y);
                region_tiles.insert(curr_y * map_width + curr_x);

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

            if (region.size() < min_region_area) continue;

            // Calculate initial centroid
            CountryRegion cr;
            double sum_x = 0.0, sum_y = 0.0;
            for (const auto& [x_pos, y_pos] : region) {
                sum_x += x_pos + 0.5;
                sum_y += y_pos + 0.5;
            }
            cr.centroid_x = static_cast<float>(sum_x / region.size());
            cr.centroid_y = static_cast<float>(sum_y / region.size());

            // Adjust centroid to be within region boundaries
            int tile_x = static_cast<int>(cr.centroid_x);
            int tile_y = static_cast<int>(cr.centroid_y);
            if (!region_tiles.count(tile_y * map_width + tile_x)) {
                // Find nearest tile in region to original centroid
                float min_dist = FLT_MAX;
                std::pair<int, int> closest_tile = region[0];
                for (const auto& [x_pos, y_pos] : region) {
                    float dx = (x_pos + 0.5f) - cr.centroid_x;
                    float dy = (y_pos + 0.5f) - cr.centroid_y;
                    float dist = dx*dx + dy*dy;
                    if (dist < min_dist) {
                        min_dist = dist;
                        closest_tile = {x_pos, y_pos};
                    }
                }
                cr.centroid_x = closest_tile.first + 0.5f;
                cr.centroid_y = closest_tile.second + 0.5f;
                tile_x = closest_tile.first;
                tile_y = closest_tile.second;
            }

            // Ensure not in water
            MapTile centroid_tile = map.get_tile(tile_x, tile_y);
            if (centroid_tile.type == MapTileType::Water) {
                // Find closest non-water tile in region
                std::pair<int, int> best_tile;
                float min_dist = FLT_MAX;
                bool found = false;
                
                for (const auto& [x_pos, y_pos] : region) {
                    MapTile t = map.get_tile(x_pos, y_pos);
                    if (t.type == MapTileType::Water) continue;
                    
                    float dx = (x_pos + 0.5f) - cr.centroid_x;
                    float dy = (y_pos + 0.5f) - cr.centroid_y;
                    float dist = dx*dx + dy*dy;
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_tile = {x_pos, y_pos};
                        found = true;
                    }
                }

                if (found) {
                    cr.centroid_x = best_tile.first + 0.5f;
                    cr.centroid_y = best_tile.second + 0.5f;
                } else {
                    continue; // Skip water-only regions
                }
            }

            cr.area = static_cast<float>(region.size());
            cr.color.r = countries.at(country_id).get_color().r;
            cr.color.g = countries.at(country_id).get_color().g;
            cr.color.b = countries.at(country_id).get_color().b;
            cr.color.a = countries.at(country_id).get_color().a;
            output.push_back(cr);
        }
    }
}

void render_country_labels(SDL_Renderer* renderer, ImDrawList* draw_list,
                         const Map& map, const SDL_FRect& view_rect,
                         const std::map<CountryId, Country>& countries,
                         RegionCache& cache, bool update_cache) {
    static const float min_font_size = 12.0f;
    static const float max_font_size = 36.0f;
    static const float max_region_area = map.get_width() * map.get_height() * 0.5f;
    static const float map_width = static_cast<float>(map.get_width());
    static const float map_height = static_cast<float>(map.get_height());

    // Calculate zoom scale (inverse of viewport scale)
    float zoom_scale = std::clamp(view_rect.w / map_width, 0.5f, 4.0f);

    for (const auto& [country_id, country] : countries) {
        if (country_id == 0) continue;

        if (update_cache || cache.find(country_id) == cache.end()) {
            std::vector<CountryRegion> regions;
            find_country_regions(map, country_id, regions, countries);
            cache[country_id] = regions;
        }

        for (const auto& region : cache[country_id]) {
            // Combine region size and zoom scaling
            float area_scale = std::clamp(region.area / max_region_area, 0.2f, 1.0f);
            float font_size = (min_font_size + (max_font_size - min_font_size) * area_scale) * zoom_scale;
            font_size = std::clamp(font_size, 8.0f, 48.0f); // Absolute limits

            // Convert to screen coordinates
            float screen_x = view_rect.x + (region.centroid_x / map_width) * view_rect.w;
            float screen_y = view_rect.y + (region.centroid_y / map_height) * view_rect.h;

            // Final position validation
            int check_x = static_cast<int>((screen_x - view_rect.x) * map_width / view_rect.w);
            int check_y = static_cast<int>((screen_y - view_rect.y) * map_height / view_rect.h);
            if (check_x < 0 || check_x >= map_width || check_y < 0 || check_y >= map_height) continue;

            MapTile final_tile = map.get_tile(check_x, check_y);
            if (final_tile.owner != country_id || final_tile.type == MapTileType::Water) continue;

            // Prepare text and calculate size
            const std::string &country_name = country.get_name();
            const std::string &troops_str = std::to_string(country.get_troops());
            unsigned longest_len = std::max(country_name.length(), troops_str.length());
            std::stringstream text;
            text << std::setw((longest_len - country_name.length()) / 2 + 1) << country.get_name() << "\n"
                 << std::setw((longest_len - troops_str.length()) / 2 + 1) << troops_str;
            ImVec2 text_size = ImGui::CalcTextSize(text.str().c_str(), nullptr, false, -1.0f);

            // Viewport culling
            ImVec2 text_min(screen_x - text_size.x * 0.5f, screen_y - text_size.y * 0.5f);
            ImVec2 text_max(screen_x + text_size.x * 0.5f, screen_y + text_size.y * 0.5f);
            if (text_max.x < view_rect.x || text_min.x > view_rect.x + view_rect.w ||
                text_max.y < view_rect.y || text_min.y > view_rect.y + view_rect.h) continue;

            // Calculate contrast color
            ImU32 text_color = IM_COL32(region.color.r, region.color.g, region.color.b, 255);
            if ((region.color.r * 0.299 + region.color.g * 0.587 + region.color.b * 0.114) < 150) {
                text_color = IM_COL32_WHITE;
            }

            // Draw with scaled font
            draw_list->AddText(nullptr, font_size, text_min, text_color, text.str().c_str());
        }
    }
}


