#include "Menu.h"

#ifndef _WIN32
#include "Process.h"
#include "ConfigData.h"
#endif

#include <cmath>
#include <iostream>

#if __cplusplus >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

Menu::Menu()
{
    this->m_tip = nullptr;
    this->m_db = new Database();
    Table *gameDataTable = new Table("gameData", {{"gameName", "TEXT"}, {"startTime", "TEXT"}, {"endTime", "TEXT"}, {"rating", "TEXT"}, {"highScore", "TEXT"}});
    this->m_db->createTable(gameDataTable);
    this->m_rating = Rating();
}

Menu::Menu(std::vector<ConfigData> configs)
{
    this->m_tip = nullptr;
    this->m_games = configs;
    this->m_db = new Database();
    Table *gameDataTable = new Table("gameData", {{"gameName", "TEXT"}, {"startTime", "TEXT"}, {"endTime", "TEXT"}, {"rating", "TEXT"}, {"highScore", "TEXT"}});
    this->m_db->createTable(gameDataTable);
    this->m_rating = Rating();
#ifdef _WIN32
    m_handle = FindWindowA(NULL, "arcade-machine");
#endif
}

Menu::~Menu()
{
    std::cout << "Destructor called on Menu\n";
    std::cout << "Menu: clearing memory...\n";

    // Walk the circular doubly-linked list and free every ButtonNode and its Button.
    // getButtons() always returned an empty m_btns, so ArcadeMachine's destructor
    // never freed these — they must be freed here.
    if (m_button) {
        ButtonNode *current = m_button->getNext();
        while (current != m_button) {
            ButtonNode *next = current->getNext();
            delete current->button;
            delete current;
            current = next;
        }
        // Delete the head node last (after all others are gone)
        delete m_button->button;
        delete m_button;
        m_button = nullptr;
    }

    delete m_db;
    delete m_tip;
}

/** 
* @brief Gets the game images from the config files and returns vector of game images.
* 
* @param configs Vector of config data.
* @return vector of game images.
*/
std::vector<std::string> Menu::getGameSprites(std::vector<ConfigData> configs)
{
    std::vector<std::string> gameImages;

    for (int i = 0; i < configs.size(); i++)
    {
        // Get image dir and image name from games vector.
        std::string image = configs[i].folder() + "/" + configs[i].image();
        gameImages.push_back(image);
    }

    return gameImages;
}


/**
* @brief Create a grid object
*/
void Menu::createGrid()
{
    // Instantiate grid object
    GridLayout grid(8, 14);
    this->m_grid = grid;
    // Update the background
    this->m_grid.setBackground(bitmap_named(this->m_background));
}

/**
* @brief Create a list of games.
* 
*/
void Menu::createButtons()
{
    // Call function to get game images.
    m_gameImages = getGameSprites(m_games);
    GameData gameStats;

    for (int i = 0; i < m_gameImages.size(); i++)
    {
        std::string image = m_gameImages[i];

        // Load the game thumbnail bitmap with specific error reporting.
        if (m_games[i].image().empty()) {
            std::cerr << "[Menu] image load failed (empty path): " << m_games[i].title() << std::endl;
        } else if (!fs::exists(image)) {
            std::cerr << "[Menu] image load failed (file not found): " << image << std::endl;
        } else {
            if (!has_bitmap(image))
                load_bitmap(image, image);
            if (!has_bitmap(image))
                std::cerr << "[Menu] image load failed (splashkit error): " << image << std::endl;
            else
                std::cerr << "[Menu] image loaded: " << image << std::endl;
        }

        if (i == 0)
        {
            this->m_button = new ButtonNode(new GameScreenButton(Button::GAME, image));
            this->m_button->config = m_games[0];
            this->m_button->stats = gameStats.getStats(this->m_db, m_games[0].title());
        }
        else
        {
            this->m_button->addBefore(new ButtonNode(new GameScreenButton(Button::GAME, image)));
            this->m_button->getPrev()->config = m_games[i];
            this->m_button->getPrev()->stats = gameStats.getStats(this->m_db, m_games[i].title());
        }
    }
}

/** 
* @brief create a tip to display to the user.
*/
void Menu::createTip()
{
    bitmap bmpTip = bitmap_named("information");
    bitmap_set_cell_details(bmpTip, 50, 50, 4, 3, 12);
    animation_script infoScript = animation_script_named("info-script");
    animation anim = create_animation(infoScript, "rotate");
    drawing_options opt = option_with_animation(anim);

    std::string tipText[3] = {
        "Use the left and right arrow keys to cycle through the carousel", 
        "Press escape to return to the main menu", 
        "Press enter to start the game"
    };
    this->m_tip = new Tip(tipText[rand()%3], bmpTip, anim, opt, 3000, 25);
}

/**
* @brief draw the game buttons to the window, using the carousel layout
*/
void Menu::updateCarousel()
{
    // If menu is sliding then clear the grid.
    if (this->m_menuSliding)
    {
        this->m_grid.clearGrid();
    }
    else {
        if (this->m_button && !this->m_inGame)
        {
            this->m_grid.updateCell(this->m_button->getPrev()->button, 2, 0, 1, false);
            this->m_grid.updateCell(this->m_button->button, 2, 5, 1, false);
            this->m_grid.updateCell(this->m_button->getNext()->button, 2, 10, 1, false);
        }
    }
}

/**
* @brief m_handle carousel input
*/
void Menu::carouselHandler()
{
    /// Check for input in selector class.
    this->m_button = this->m_selectorGamesMenu.checkKeyInput(this->m_button, m_gameMenu);
    this->m_action = this->m_selectorGamesMenu.checkForSelection(this->m_button, m_gameMenu);

    checkGameExit();

    // ESC while in game: kill the game process
    // Use key_typed() directly since game window may have stolen focus from selector
    if (m_inGame && key_typed(ESCAPE_KEY)) {
        std::cerr << "[Menu] user pressed ESC, killing game (PID: " << this->m_processId << ")" << std::endl;
        killProcess(this->m_processId);
        m_launching = false;
        m_inGame = false;
    }

    if (this->m_button)
    {
        if (this->m_action == "escape" && m_overlayActive)
        {
            m_overlayActive = false;
        }
        else if (this->m_action == "return")
        {
            if (m_overlayActive)
            {
                // Set the center of the game
                this->m_x = m_centreX;
                this->m_y = m_centreY;

                // fade to black
                fade(0, 1, 0.1);

                // fill with black
                fill_rectangle(rgba_color(0.0, 0.0, 0.0, 1.0), 0, 0, ARCADE_MACHINE_RES_X, ARCADE_MACHINE_RES_Y);
                // clear grid
                this->m_grid.clearGrid();
                // set new background
                this->m_grid.setBackground(bitmap_named("in_game_bgnd"));
                //turn off overlay
                this->m_overlayActive = false;
                // turn off menu music
                fade_music_out(1000);
                // fade back in
                fade(1, 0, 0.1);
                m_gameData.setStartTime(time(0));
                m_gameData.setGameName(this->m_button->config.title());

#ifdef _WIN32
                {
                    // Build persistent strings before passing to CreateProcessA.
                    // Storing .c_str() of a temporary causes a dangling pointer —
                    // the temporary is destroyed at the end of the assignment statement.
                    std::string gamePath   = this->m_button->config.folder() + "/" + this->m_button->config.win_exe();
                    std::string gameDir    = this->m_button->config.folder();
                    // CreateProcessA may write into lpCommandLine, so use a mutable buffer.
                    std::string gameExeStr = this->m_button->config.win_exe();
                    std::vector<char> gameExeBuf(gameExeStr.begin(), gameExeStr.end());
                    gameExeBuf.push_back('\0');

                    startGame(gamePath.c_str(), gameExeBuf.data(), gameDir.c_str());
                }
#else
                try {
                    startGame(this->m_button->config.getExecutablePath());
                } catch(const std::runtime_error &error) {
                    write_line(error.what());
                }
#endif

                return;
            }
            m_overlayActive = true;
        }
    }
}

/**
* @brief draw the menu page
*/
void Menu::drawMenuPage()
{
    // if the game has ended, go back to games menu
    if(!this->m_inGame && this->m_gameStarted)
    {
        this->m_gameStarted = false;
        backToGamesMenu();
        m_gameData.setRating(m_rating.getRating());
        m_gameData.writeData(this->m_db);
        this->m_button->stats = m_gameData.getStats(this->m_db, m_gameData.getGameName());
    }
    
    this->m_grid.drawGrid();
    
    // Wait for selector to key input to determine slide direction.
    if (m_selectorGamesMenu.getSlideLeft())
        drawUpdateSlideLeft();
    else if (m_selectorGamesMenu.getSlideRight())
        drawUpdateSlideRight();

    if (m_overlayActive && !m_menuSliding)
        drawOverlay(m_button->config, m_button->stats);
    if (!m_inGame && this->m_tip)
        this->m_tip->draw();

    // Show launching animation until game process exits.
    if (m_inGame && m_launching) {
        int dots = (current_ticks() / 400) % 4;
        std::string anim = std::string(dots, '.');
        std::string msg = "Starting " + this->m_button->config.title() + anim;
        draw_text(msg, COLOR_WHITE, "font_text", 40,
                  m_centreX - 200, m_centreY - 20);
    }

    // Show launch error message (fork failure or crash).
    if (!m_inGame && !m_launchError.empty()) {
        draw_text(m_launchError, COLOR_RED, "font_text", 30,
                  m_centreX - 300, m_centreY - 20);
        // Clear after 3 seconds so the menu returns to normal.
        if (current_ticks() - m_launchTime > 3000)
            m_launchError = "";
    }

    updateCarousel();
    carouselHandler();
}

/**
* @brief Method to update the sprite positions and draw sprite.
* 
* @param buttonSprite The buttons sprite.
* @param position The position to move the sprite.
* @return ** void 
*/
void Menu::updateSlide(sprite buttonSprite, int position)
{
    // Show the base layer of sprite.
    sprite_show_layer(buttonSprite, 0);
    // Set the x position of sprite.
    sprite_set_x(buttonSprite, position);
    // Set the y position of sprite.
    sprite_set_y(buttonSprite, m_posY);
    // draw sprite to screen.
    draw_sprite(buttonSprite);
    // Updatse sprite.
    update_sprite(buttonSprite);
}

/**
* @brief Slide the game buttons on left key input.
* 
* @return ** void 
*/
void Menu::drawUpdateSlideLeft()
{
    this->m_menuSliding = true;

    // Get sprites of buttons on display.
    this->m_newButton1 = this->m_button->getNext()->button->btn();
    this->m_newButton2 = this->m_button->button->btn();
    this->m_newButton3 = this->m_button->getPrev()->button->btn();

    // Increment the x position of sprite.
    this->m_pos1 += m_speed;
    this->m_pos2 += m_speed;
    this->m_pos3 += m_speed;

    // Update and draw sprite.
    updateSlide(this->m_newButton1, this->m_pos1);
    updateSlide(this->m_newButton2, this->m_pos2);
    updateSlide(this->m_newButton3, this->m_pos3);

    // If sprite reaches position.
    if (this->m_pos1 > 1300)
    {
        // Set selector bool back to false.
        m_selectorGamesMenu.setSlideLeft(false);
        this->m_menuSliding = false;
        // Reset positions.
        this->m_pos1 = m_posX;
        this->m_pos2 = this->m_pos1 - m_posX;
        this->m_pos3 = this->m_pos2 - m_posX;
    }
}

/**
* @brief Slide the game buttons on right key input.
* 
* @return ** void 
*/
void Menu::drawUpdateSlideRight()
{
    this->m_menuSliding = true;

    // Get sprites of buttons on display.
    this->m_newButton1 = this->m_button->getPrev()->button->btn();
    this->m_newButton2 = this->m_button->button->btn();
    this->m_newButton3 = this->m_button->getNext()->button->btn();

    // Decrease the x position of sprite.
    this->m_pos1 -= m_speed;
    this->m_pos4 -= m_speed;
    this->m_pos5 -= m_speed;

    // Update and draw sprite.
    updateSlide(this->m_newButton1, this->m_pos1);
    updateSlide(this->m_newButton2, this->m_pos4);
    updateSlide(this->m_newButton3, this->m_pos5);

    if (this->m_pos1 <= 20)
    {
        // Set selector bool back to false.
        m_selectorGamesMenu.setSlideRight(false);
        this->m_menuSliding = false;
        // Reset positions.
        this->m_pos1 = m_posX;
        this->m_pos4 = m_posX * 2;
        this->m_pos5 = m_posX * 3;
    }
}

/**
* @brief Draw an overlay over the game, using data from the config.
* 
* @param config the game config.
*/
void Menu::drawOverlay(ConfigData config, GameData stats)
{
    int xOffset = (current_window_width() / 2) + (current_window_width() / 14);
    int yStart = current_window_height() / 6;
    int yOffset = current_window_height() / 40;

    fill_rectangle(rgba_color(0.0, 0.0, 0.0, 0.8), (current_window_width() / 2), 0, (current_window_width() / 2), current_window_height());
    draw_text(config.title(), COLOR_WHITE, "font_title", yOffset * 3, xOffset, yStart);
    yStart += yOffset * 3;
    draw_text("Author: " + config.author(), COLOR_WHITE, "font_text", yOffset, xOffset, yStart + (1 * yOffset));
    draw_text("Genre: " + config.genre(), COLOR_WHITE, "font_text", yOffset, xOffset, yStart + (2 * yOffset));
    draw_text("Language: " + config.language(), COLOR_WHITE, "font_text", yOffset, xOffset, yStart + (3 * yOffset));
    draw_text("Classification: " + config.rating(), COLOR_WHITE, "font_text", yOffset, xOffset, yStart + (4 * yOffset));
    draw_text("Repository: " + config.repo(), COLOR_WHITE, "font_text", yOffset, xOffset, yStart + (5 * yOffset));
    draw_text("Playtime: " + std::to_string(stats.getStartTime()/60) + "mins", COLOR_WHITE, "font_text", yOffset, xOffset, yStart + (6*yOffset));
    draw_text("Rating: ", COLOR_WHITE, "font_text", yOffset, xOffset, yStart + (7 * yOffset));
    string rating = string(stats.getRating(), 'J');
    rating += string(5-rating.size(),'I');
    draw_text(rating, COLOR_WHITE, "font_star", yOffset+10, xOffset + 90,  yStart + (7 * yOffset));
}

#ifdef _WIN32
/**
* @brief  Find the game window and bring it to focus, if it exists
* 
* @param windowName the name of the window
* @param timeout time in ms to search for the window
* @return true/false if window was found.
*/
bool Menu::focusWindow(std::string windowName, int timeout)
{
    LPCSTR gameWindow =  windowName.c_str();
    HWND gameWindowHandle = NULL;

    int timeElapsed;
    auto startTime = std::chrono::steady_clock::now();

    //Find the window m_handle
    do {
        gameWindowHandle = FindWindowEx(NULL,NULL,NULL, gameWindow);
        timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
        delay(250);
    }
    while (gameWindowHandle == NULL && timeElapsed <= timeout);

    //Maximise the Window
    if (gameWindowHandle != NULL)
    {
        ShowWindow(gameWindowHandle, SW_SHOWMAXIMIZED);
        return true;
    }
    else
    {
        write_line("Unable to find gameWindow m_handle");
        return false;
    }
    return true;
}

/**
* @brief Starts up the selected game by starting a new process.
* 
* @param gamePath The filepath of the game to open.
* @param gameExe The executable of the game.
* @param gameDirectory // The directory of the game.
* @return ** void 
*/
void Menu::startGame(LPCSTR gamePath,LPSTR gameExe, LPCSTR gameDirectory)
{
    if (!this->m_inGame)
    {
        // Additional info
        STARTUPINFOA startupInfo;

        // Set the size of the structures
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        ZeroMemory(&m_processInfo, sizeof(m_processInfo));

        // Start the program up
        WINBOOL gameProcess = CreateProcessA
        (
            gamePath,               // the path
            gameExe,                // Command line
            NULL,                   // Process m_handle not inheritable
            NULL,                   // Thread m_handle not inheritable
            FALSE,                  // Set m_handle inheritance to FALSE
            NORMAL_PRIORITY_CLASS,     // Don't open file in a separate console
            NULL,                    // Use parent's environment block
            gameDirectory,           // Use parent's starting directory
            &startupInfo,            // Pointer to STARTUPINFO structure
            &m_processInfo           // Pointer to PROCESS_INFORMATION structure
        );

        OpenProcess(PROCESS_QUERY_INFORMATION,TRUE, gameProcess);

        std::string windowName = gameExe;
        //Remove the extension from the application name (.exe)
        windowName = windowName.substr(0, windowName.find("."));
        //Focus the window
        focusWindow(windowName);

        this->m_inGame = true;
    }
}
#else
void Menu::startGame(struct s_ExecutablePath path) {
    if (!this->m_inGame) {
        std::cerr << "[Menu] starting game: " << path.file << " from " << path.path << std::endl;
        m_launching = true;
        m_launchError = "";

        this->m_processId = spawnProcess(path.path, path.file);

        if (this->m_processId == -1) {
            std::cerr << "[Menu] failed to start game: fork failed" << std::endl;
            m_launching = false;
            m_launchTime = current_ticks();
            m_launchError = "Failed to start game — see terminal for details";
        } else {
            std::cerr << "[Menu] game process spawned (PID: " << this->m_processId << ")" << std::endl;
            this->m_inGame = true;
            this->m_launchTime = current_ticks();
        }
    }
}
#endif


/**
* @brief Waits for game to exit.
* 
* @return ** void 
*/
void Menu::checkGameExit()
{
    if (this->m_inGame == true)
    {
#ifdef _WIN32
        this->m_gameStarted = true;
        // Check if game has been exited.
        this->m_programExit = GetExitCodeProcess(m_processInfo.hProcess, &m_exitCode);
        if ((this->m_programExit) && (STILL_ACTIVE != m_exitCode))
        {
            this->m_inGame = false;
            
            int highScore = 0;
            m_gameData.setEndTime(time(0));
            m_gameData.setHighScore(highScore);
        }
#else
        if (! processRunning(this->m_processId, m_lastExitCode)) {
            unsigned int runTime = current_ticks() - m_launchTime;
            if (m_lastExitCode != 0) {
                std::cerr << "[Menu] game crashed: " << this->m_button->config.title()
                          << " (PID: " << this->m_processId << ", exit code: " << m_lastExitCode
                          << ", ran for " << runTime << "ms)" << std::endl;
                m_launchError = this->m_button->config.title() + " crashed (exit code: " + std::to_string(m_lastExitCode) + ")";
                m_launchTime = current_ticks();
            } else {
                std::cerr << "[Menu] game exited normally: " << this->m_button->config.title()
                          << " (PID: " << this->m_processId << ", ran for " << runTime << "ms)" << std::endl;
            }
            m_launching = false;
            this->m_inGame = false;
            int highScore = 0;
            m_gameData.setEndTime(time(0));
            m_gameData.setHighScore(highScore);
        }
#endif
    }
}

/** 
* @brief Fade back to games menu
*/
void Menu::backToGamesMenu()
{
    // fade to black
    fade(0, 1, 0.1);
    fill_rectangle(rgba_color(0.0, 0.0, 0.0, 1.0), 0, 0, ARCADE_MACHINE_RES_X, ARCADE_MACHINE_RES_Y);
    this->m_grid.setBackground(bitmap_named("games_dashboard"));
    // fade to normal
    fade(1, 0, 0.1);
}

/**
* @brief Creates a fading effect
* 
* @param alphaStart The starting alpha value.
* @param alphaEnd The ending alpha value.
* @param alphaStep The alpha value to increment/decrement by.
*/
void Menu::fade(double alphaStart, double alphaEnd, double alphaStep)
{
    if (alphaStart > alphaEnd)
        alphaStep = -std::abs(alphaStep);
    // Calculate the number of steps required to complete the fade.
    double difference = std::abs(alphaEnd - alphaStart);
    int steps = difference / std::abs(alphaStep);

    for (int i = 0; i < steps; i++)
    {
        clear_screen();
        this->m_grid.drawGrid();
        // Alpha value manipulates to the opacity of the rectangle.
        fill_rectangle(rgba_color(0.0, 0.0, 0.0, alphaStart), 0, 0, ARCADE_MACHINE_RES_X, ARCADE_MACHINE_RES_Y);
        // Update the alpha value.
        alphaStart += alphaStep;
        refresh_screen(60);

        delay(50);
    }
}