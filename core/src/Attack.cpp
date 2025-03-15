#include "Attack.h"
#include <iostream>

std::set<std::pair<unsigned, unsigned>> Attack::advance(Map &map) {
    double troop_cost_per_pixel = 1.0;

    std::cout << "Defender: " << (this->defender.has_value() ? "true" : "false") << std::endl;
    
    if (this->defender.has_value())
        troop_cost_per_pixel += (double)this->defender->troops / this->attacker.troops - 1;

    std::cout << "Troop cost per pixel: " << troop_cost_per_pixel << std::endl;
    std::cout << "Troops to attack: " << this->troops_to_attack << std::endl;

    unsigned pixels_to_capture {static_cast<unsigned int>(this->troops_to_attack / troop_cost_per_pixel)};

    std::cout << "Getting border, pixels to capture: " << pixels_to_capture << "\n";

    // get border between this and other
    std::set<std::pair<unsigned, unsigned>> border;
    int directions[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
    for (unsigned y = 0; y < map.get_height(); y++) {
        for (unsigned x = 0; x < map.get_width(); x++) {
            if (map.get_tile(x, y).owner == defender_id && map.get_tile(x, y).type != MapTileType::Water) {
                for (auto &dir : directions) {
                    int nx = x + dir[0];
                    int ny = y + dir[1];
                    if (nx >= 0 && nx < (int)map.get_width() && ny >= 0 && ny < (int)map.get_height()) {
                        MapTile neighbor = map.get_tile(nx, ny);
                        if (neighbor.owner == this->attacker.get_id() && neighbor.type != MapTileType::Water) {
                            border.insert({x, y});
                        }
                    }
                }
            }
        }
    }

    if (border.empty()) {
        std::cerr << "I thought there was a border but it is empty???" << std::endl;
        return {};
    }

    if (border.size() > pixels_to_capture) {
        std::cerr << "Not enough troops" << std::endl;
        return {};
    }

    for (const auto &tile : border) {
        map.set_tile(tile, this->attacker.get_id());
        this->attacker.troops -= troop_cost_per_pixel;
        this->troops_to_attack -= troop_cost_per_pixel;
    }
    return border;
}

