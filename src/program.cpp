#include "Configuration.h"
#include "ArcadeMachine.h"
#include <iostream>

#if __cplusplus >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#define PLAY_INTRO true
#define LOAD_GAMES true

int main(void)
{
    // Load all resources
    set_resources_path("resources" ARCADE_MACHINE_PATH_SEP);

    if (!fs::exists("resources/bundles/resources.txt"))
        std::cerr << "[Bundle] resources.txt not found — check resources/ directory" << std::endl;

    load_resource_bundle("bundle", "resources.txt");

    if (has_resource_bundle("bundle"))
        std::cerr << "[Bundle] resource bundle loaded successfully" << std::endl;
    else
        std::cerr << "[Bundle] resource bundle failed to load (unknown error)" << std::endl;

    // Open window and toggle border off.
    // Note: bitmap/font checks must happen AFTER open_window() since SplashKit
    // needs a render context to load graphical resources.
    open_window("arcade-machine", ARCADE_MACHINE_RES_X, ARCADE_MACHINE_RES_Y);
    window_toggle_border("arcade-machine");

    // Instantiate Arcade Machine
    ArcadeMachine Arcade;

    // Check all critical bitmaps and fonts loaded correctly.
    // Must happen after open_window() — SplashKit needs a render context.
    std::vector<std::string> bitmaps = {
        "thoth", "intro_splashkit", "intro_thoth_tech", "intro_arcade_team",
        "games_dashboard", "back_ground", "in_game_bgnd", "rating_bg", "options_thoth",
        "btn_play", "btn_exit", "btn_opts", "play_hghlt", "exit_hghlt", "options_hghlt",
        "game_hghlt", "opts_home", "opts_sound", "opts_display", "opts_stats",
        "opts_home_hghlt", "opts_sound_hghlt", "opts_display_hghlt", "opts_stats_hghlt",
        "backCurrentGame", "backGame_notSelected", "changeSound", "sound_notSelected",
        "backMenu_notSelected", "backMenu", "cursor", "information", "star-gold", "star-black"
    };
    std::vector<std::string> fonts = {
        "font_btn", "font_title", "font_text", "font_star", "font_about"
    };
    int failCount = 0;
    for (const auto& name : bitmaps) {
        if (!has_bitmap(name)) {
            std::cerr << "[Bundle] bitmap not loaded: " << name << std::endl;
            failCount++;
        }
    }
    for (const auto& name : fonts) {
        if (!has_font(name)) {
            std::cerr << "[Bundle] font not loaded: " << name << std::endl;
            failCount++;
        }
    }
    if (failCount == 0)
        std::cerr << "[Bundle] all bitmaps and fonts loaded successfully" << std::endl;
    else
        std::cerr << "[Bundle] " << failCount << " resource(s) failed to load" << std::endl;

#if PLAY_INTRO == true
    // Play Thoth Tech intro
    Arcade.playThothTechIntro();
    Arcade.playArcadeTeamIntro();
#endif

#if LOAD_GAMES == true
    // Play SplashKit intro
    Arcade.playSplashKitIntro();
#endif
    
    // Prepare the main menu
    Arcade.prepareMainMenu();
    // Draw the main menu
    Arcade.mainMenu();

    // ISSUE - we never get here because the program exits forcibly from ArcadeMachine, rather than exiting from main and returning 0.
    std::cout << "we never reach this point to output?\n";

    free_resource_bundle("bundle");

    return 0;
}