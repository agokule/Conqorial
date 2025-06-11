#include "Country.h"
#include "PopulationPyramid.h"
#include <vector>
#include "cq_utils.h"

class PopulationPyramidRenderer {
private:
    // A reference for the country whoose population pyramid to display
    const Country *country;

    int current_month = 0;
    int economy_param = 100;
    int density_param = 100;
    
    // For storing historical data
    std::vector<std::vector<unsigned>> male_history;
    std::vector<std::vector<unsigned>> female_history;
    std::vector<unsigned> total_population_history;
    std::vector<unsigned> birth_history;
    std::vector<double> birth_rate_history;
    std::vector<double> life_expectancy_history;
    PyramidUtils::EconomyResult economy {0, 0};
    
    static const int MAX_HISTORY = 1000;
public:
    PopulationPyramidRenderer();
    
    void record_current_state();
    void render_pyramid_chart();
    void render_population_trend();
    void render_controls(int urbanization_param);
    void render(int urbanization_param);
    void set_pyramid(const Country &country);

    const PopulationPyramid &get_pyramid() const;
};

