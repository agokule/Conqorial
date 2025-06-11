#include "cq_ui.h"

// Country id is the index of the country in the match
// to show the pyramid for
void show_population_pyramid_renderer(AppState &state, CountryId country_id) {
    const Country &country = state.match.get_country(country_id);
    if (&country.get_pyramid() != &state.pyramid_renderer.get_pyramid())
        state.pyramid_renderer.set_pyramid(country);

    state.pyramid_renderer.render(country.get_urbanization_level());
}

