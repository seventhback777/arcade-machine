#include "AboutScreen.h"
#include "Configuration.h"
#include "splashkit.h"
#include <string>
#include <cmath>
#include <vector>
#include <fstream>

#define TITLE_FONT_SIZE 38
#define TITLE_FONT_CHAR_WIDTH 32
#define TITLE_FONT_CHAR_HEIGHT 32
#define TITLE_FONT_Y ((((ARCADE_MACHINE_RES_Y / 2) + (TITLE_FONT_CHAR_HEIGHT / 2))) / 3)
#define STAR_COUNT 1024
#define DISTANCE_SHIFT 10

#define CONTRIBUTION_TIME (60 * 3)
#define CONTRIBUTION_FADE_TIME 30

static const char *title = "About The Thoth Tech Arcade Machine!";
static const char *description[] =  {
	"This arcade machine has been created",
	"by students undertaking capstone units",
	"in the School of Information Technology",
	"as a platform to designed to showcase",
	"games built by students using SplashKit.",
	"",
	"Lines of code"
};

static const std::string createdBy = "Created by";

AboutScreen::AboutScreen() {
	this->m_shouldQuit = false; // Initialise quit flag to false
	this->m_titleX = ARCADE_MACHINE_RES_X; // Set title X to the far right of the screen
	this->m_title = std::string(title); // Convert to std::string for convenient operations
	this->m_titleEnd = ((TITLE_FONT_CHAR_WIDTH * this->m_title.length()));
	this->m_titleEnd *= -1;
	/* Each character advances X by TITLE_FONT_CHAR_WIDTH; the title scrolls fully off-screen
	   when the first character reaches x = -(TITLE_FONT_CHAR_WIDTH * title.length()). */


	this->m_stars = std::vector<struct s_star>(); // Initialise the m_stars array
	this->m_contributorsIndex = 0;
	this->m_contributorTicker = 0;
	this->m_ticker = 0;

	for (int i=0; i<STAR_COUNT; ++i) {
		struct s_star star;
		star.x = (rand() % (ARCADE_MACHINE_RES_X - 0 + 1)  + 0); // Random X position
		star.y = (rand() % (ARCADE_MACHINE_RES_Y - 0 + 1) + 0); // Random Y position
		/* Classic bounded random: (rand() % (max - min + 1)) + min gives range [min, max]. */



		star.distance = (rand() % (DISTANCE_SHIFT - 1 + 1) + 1);

		double brightness = (rand() % (80 - 40) + 40);
		// Brightness range is therefore [40, 79]
		brightness /= 100;
		// Normalise brightness to [0.0, 1.0]
		star.c.a = brightness;
		star.c.r = 1;
		star.c.g = 1;
		star.c.b = 1;
		// Set ARGB; alpha channel is the sampled brightness
		this->m_stars.push_back(star);
	}
	// Generates all 1024 s_star structs (X, Y, brightness, distance) and appends to m_stars
	
	this->m_contributors = std::vector<std::string>(); // Initialise the m_contributors array
	std::ifstream contributors("stats" ARCADE_MACHINE_PATH_SEP "contributors.txt");
	// Open contributor list via ifstream; ARCADE_MACHINE_PATH_SEP handles OS path separators
	std::string line;
	while (std::getline(contributors, line)) {
		if (line.length() > 0)
			this->m_contributors.push_back(line);
	}
	contributors.close();
	// Read contributor names line by line and append to m_contributors
	
	this->m_linesOfCode = std::vector<std::string>(); // Initialise the m_linesOfCode array
	std::ifstream linesOfCode("stats" ARCADE_MACHINE_PATH_SEP "lines-of-code.txt");
	while (std::getline(linesOfCode, line)) {
		if (line.length() > 0)
			this->m_linesOfCode.push_back(line);
	}
	// Read lines-of-code counts line by line and append to m_linesOfCode
	
	this->m_gitContributions = std::vector<std::string>();
	std::ifstream contributions("stats" ARCADE_MACHINE_PATH_SEP "git.txt");
	while (std::getline(contributions, line)) {
		if (line.length() > 0)
			this->m_gitContributions.push_back(line);
	}
	// Same pattern as the two blocks above
}
// End of constructor

void AboutScreen::onExit() {
	if (music_playing())
		stop_music();
}
// Stop music playback on exit

void AboutScreen::readInput() {
	if (quit_requested() || key_down(ESCAPE_KEY))
		this->m_shouldQuit = true;
}
// Set m_shouldQuit to true when the user requests quit

void AboutScreen::tick() {
	this->shiftTitle(); // Advance title scroll position
	this->shiftStars(); // Advance star positions
	this->tickContributor();

	// Every 1/4 second.
	if (this->m_ticker % 15 == 0) {
		if (! music_playing())
			play_music("music_about");
	}
		// Check every 0.25 s (15 frames at 60 fps)
	this->m_ticker++; // Increment frame counter
}
// Per-frame update: scroll, stars, contributor ticker, and music

void AboutScreen::render() {
	clear_screen(COLOR_BLACK); // Clear screen to black

	this->renderStars(); // Render starfield
	this->renderTitle(); // Render scrolling title
	this->renderDescription(); // Render description text
	this->renderContributor(); // Render contributor name

	refresh_screen(); // Present the rendered frame
}
// Each call: clear, stars, title, description, contributor, refresh
void AboutScreen::shiftTitle() {
	this->m_titleX -= 6;

	if (this->m_titleX < this->m_titleEnd)
		this->m_titleX = ARCADE_MACHINE_RES_X;
}
// Shift title left 6 px per frame; wrap to right edge when fully off-screen

color AboutScreen::getRainbowShade(double x) {
	color c;
	double rd = (double)ARCADE_MACHINE_RES_X / 16; // Red divisor: smallest = fastest cycle
	double gd = (double)ARCADE_MACHINE_RES_X / 10; // Green divisor
	double bd = (double)ARCADE_MACHINE_RES_X / 6;  // Blue divisor: largest = slowest cycle
	double p = sin(x / rd) * 0.5;
	double g = sin(x / gd) * 0.5;
	double b = sin(x / bd) * 0.5;
	// Map each channel to [-0.5, 0.5] via sin
	c.r = 0.5 + p;
	c.g = 0.5 + g;
	c.b = 0.5 + b;
	// Shift to [0, 1]; differing cycle rates produce rainbow colours
	return c;
}
// Varying sin divisors per channel produce smoothly cycling rainbow colours
void AboutScreen::renderTitle() {
	double x = this->m_titleX; // Start at current scroll position

	for (int i=0; i<this->m_title.length(); ++i) {
		double y = TITLE_FONT_Y + (sin(x / 80) * 115);
		// Unique X per character produces wave shape via sin; amplitude +-115 px
		double fontSize = TITLE_FONT_SIZE + (sin(x / 120) * 8);
		// Same principle for font size; amplitude +-8 px
		color c = this->getRainbowShade(x);
		// Get rainbow colour keyed to character position
		draw_text(
			this->m_title.substr(i, 1), // Render one character at a time
			c, // Character colour
			"font_about", // Font
			fontSize, // Font size
			x, // X position
			y  // Y position
		);	

		x += TITLE_FONT_CHAR_WIDTH; // Advance X by one character width
	}
	// Characters are rendered one at a time
}
// Renders title with wave motion, size variation, and rainbow colouring

void AboutScreen::shiftStars() {
	for (int i=0; i<this->m_stars.size(); ++i) {
		this->m_stars[i].x += this->m_stars[i].distance;
		if (this->m_stars[i].x > ARCADE_MACHINE_RES_X)
			this->m_stars[i].x = -10;
	}
}
// Each call moves every star right by its distance; stars past the right edge wrap to x=-10

void AboutScreen::renderStars() {
	for (auto star : this->m_stars)
		fill_rectangle(star.c, star.x, star.y, star.distance / 2, star.distance / 4);
}
// fill_rectangle width/height scale with distance, giving a parallax depth effect

void AboutScreen::renderDescription() {
	double offset  = sin(((double)this->m_ticker / 16)) * 12; // Y oscillation, +-12 px
	double offsetX = sin((double)this->m_ticker / 24)  * 19; // X oscillation, +-19 px
	// Compute XY oscillation offsets for a floating text effect
	double y = 480 + offset;  // Base Y with oscillation applied
	double x = 140 + offsetX; // Base X with oscillation applied
	// Sin-driven offsets make the text float in a gentle periodic motion
	int maxIOffset = 0;
	for (int i=0; i<sizeof(description) / sizeof(description[0]); ++i) {
		maxIOffset = (i * 32);
		draw_text(description[i], COLOR_WHITE, "font_about", 18, x, y + maxIOffset);
	}
	/* description is a char* array; sizeof(arr)/sizeof(arr[0]) gives element count.
	   Each row is spaced 32 px apart (maxIOffset = i * 32). */
	
	y = y + maxIOffset + 32;
	for (int i=0; i<this->m_linesOfCode.size(); ++i) {
		maxIOffset = (i * 32);
		draw_text(this->m_linesOfCode[i], COLOR_WHITE, "font_about", 18, x, y + (maxIOffset));
	}
	// Render linesOfCode below description with the same 32 px row spacing
	y = y + maxIOffset + 64;
	draw_text("Contributions", COLOR_WHITE, "font_about", 18, x, y);
	y += 32;

	for (int i=0; i<this->m_gitContributions.size(); ++i) {
		maxIOffset = (i * 32);
		draw_text(this->m_gitContributions[i], COLOR_WHITE, "font_about", 18, x, y + (maxIOffset));
	}
	// Render git contributions below linesOfCode with the same row spacing
}
// Renders description, lines-of-code, and git contribution counts

void AboutScreen::loop() {
	while (! this->m_shouldQuit) {
		process_events(); // Process input events via SplashKit

		this->readInput(); // Check for quit request
		this->tick(); // Update positions, music, and timers
		this->render(); // Render the frame

		delay(1000 / 60); // Cap at 60 fps
	}

}
// Main render loop

void AboutScreen::main() {
	// Clear music and start the about screen music.
	stop_music();
	play_music("music_about");

	this->loop();

	// Tidy up once the loop is done.
	this->onExit();
}
// Entry point for the about screen

void AboutScreen::tickContributor() {
	this->m_contributorTicker++;
	if (this->m_contributorTicker >= CONTRIBUTION_TIME) {
		this->m_contributorTicker = 0; // Reset every 180 frames (3 s at 60 fps)
		this->m_contributorsIndex = (this->m_contributorsIndex + 1) % this->m_contributors.size(); // Wrap index to cycle
	}
}
// Cycles contributor display every 3 s

void AboutScreen::renderContributor() {
	int r = this->m_contributorTicker % CONTRIBUTION_TIME; // r stays in [0, 179]
	double ratio = 1; // Opacity multiplier
	double fontSize = 32; // Base font size
	double fontRatio = 1; // Font size multiplier

	if (r < CONTRIBUTION_FADE_TIME) {
		ratio = r / (double)CONTRIBUTION_FADE_TIME; // Fade in: ratio rises 0->1
		fontRatio = 1 + ((1 - ratio) * 4); // Shrink from 5x to 1x over the fade window
		// Fade-in: starts large and transparent, ends small and opaque
	} else if ((CONTRIBUTION_TIME - r) < CONTRIBUTION_FADE_TIME) {
		ratio = (CONTRIBUTION_TIME - r) / (double)CONTRIBUTION_FADE_TIME; // Fade out: ratio falls to 0
		fontRatio = ratio; // Shrink with opacity during fade-out
	}

	fontSize = fontSize * fontRatio; // Apply size multiplier
	if (ratio > 1.0)
		ratio = 1.0; // Clamp to prevent overexposure

	color c;
	c.r = ratio;
	c.g = ratio;
	c.b = ratio;


	draw_text(this->m_contributors[this->m_contributorsIndex], c, "font_about", fontSize, 1100, 600);

	double x = 0;
	double y = 0;
	fontSize = 24;
	for (int i=0; i<createdBy.length(); ++i) {
		y = sin((this->m_ticker + (i * 4)) / (double)16) * (double)9; // Per-character Y wave, +-9 px
		x = sin(this->m_ticker / (double) 50) * (double)225; // Group X oscillation, +-225 px
		double actualX = 1300 + x + (i * 28); // Per-character X with 28 px spacing

		fontRatio = sin((actualX + 190) / (double)80) * (double)1.5; // Varying size ~1-1.5x
		if (fontRatio < 1)
			fontRatio = (double)1; // Clamp minimum size

		color c = this->getRainbowShade(actualX - 350); // Rainbow colour keyed to X position
		draw_text(createdBy.substr(i, 1), c, "font_about", (double)24 * fontRatio, actualX, (double)500 + y);
	}
} // Render contributor name with rainbow colour, fade in/out, and position oscillation