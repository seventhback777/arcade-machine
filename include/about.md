1.AboutScreen.h
The developer contribution list page will scroll through the contribution information of developers and obtain information from the data collected by the previous statistics script.

2.ArcadeMachine.h
The master control type of the entire "arcade front-end program" - organizing the startup animation, main menu, game list, option menu, button clicks, sounds and layout all together.

3.Audio.h
The music player plays the main menu music by default upon startup. It can switch songs and adjust the volume, ensuring that games/menus have a unified music management interface.

4.Button.h
The parent class of all buttons provides the basic attributes and behaviors for the buttons.

5.ButtonNode.h
Button nodes are used to form a bidirectional circular linked list of a bunch of buttons. This enables the menu cursor to move in a loop between buttons, just like in a real arcade machine. You can push in one direction without going all the way, making there no sense of boundaries.

6.cell.h
The grid cell, as the smallest drawing unit in the machine, supports storing four types: EMPTY,BMP,SPT(Animation Sprite), and BTN. It also supports functions such as center alignment, how large this cell should occupy (merging other base cells), etc. A very important drawing unit.

7.ConfigData.h
A very important header file is the "Data model manual" for each game. It is through this that the menu system knows which games are available and how to start them. Including: All metadata of each game.

Basic information: m_id

                   m_repo

                   m_language

                   m_folder

Display information: m_image

                   m_title

                   m_genre

                   m_rating

                   m_author

                   m_description

Executable file path

                   The executable path of Win

                   The executable path of Linux

                   The executable path of MacOS

It can open the config.txt of each game, read this information from it (here we have to mention config.txt, which is the information that every game uploader must provide), and then convert this information from text information into object attributes for the convenience of using it for config later.

8.Configuration.h
Global configuration constants, uniformly managing environment-related parameters such as "resolution, path separator, CPU architecture, operating system type, and executable file extension".

9.Database.h
The database encapsulation for arcade menu/statistics provides common operation methods such as table creation, addition, deletion, modification, query, and printing, making it convenient for direct invocation elsewhere.

10.GameData.h
To record each round encapsulation, wrote in the database a is fixed: gameData, it exists fields: gamaName, startTime, endTime, rating, highScore. It will even provide aggregated statistics: average score, total duration, highest score, etc.

11.GameScreenButton.h
The specific implementation class on the game selection menu completes the implementation of button images, drawing, click behavior, etc. In the menu interface, each game/option will be displayed with a GameScreenButton.

12.GridLayout.h
This actually forms a linkage with the previous grid header file. The two work together to achieve a very good drawing effect. It can divide the entire screen into x rows and y columns. Each cell is a grid, managing the drawing content of each cell, drawing uniformly, automatically calculating scaling, position, etc. based on the cell size, and finally drawing all of them. And it provides the functions of cleaning the screen and releasing memory.

13.Helper.h
This is a very important toolbox, involving the reading of game directories, etc. 

(1) There is a function called string getFoldName(string entryPath), which uses the path of the passed game file (usually the config.txt of a certain game) to take the directory where it is located as the "game folder name" Facilitate the subsequent reading of resources by ConfigData.

(2) vector<string>getConfigFiles(string dir) recursively traverses the folder, collects all config.txt files, and returns a list of paths.

(3) vector<ConfigData> ConfigDataList()
Call getConfigFiles(string dir), specify the path for it ("./games/games"), scan it once, then read each config.txt in, convert it into a ConfigData object, and fill it in:
  folder
  id
  The fields parsed from the configuration file
Regarding the specified path ("./games/games"), it is necessary to mention the special directory of the game repository. For the storage of games, a special format must be met. It must be written as config.txt and then uniformly placed in the "games" folder. Only in this way can vector<ConfigData> ConfigDataList() correctly find the game and load the relevant configuration. That's why it is hard-coded here.

(4)void resetScreen(GridLayout grid)
A demonstration tool for drawing grids for testing.

(5) void gridLayoutExample()
Open a 600x800 window without loading appContainer.png. Create a 5x5 grid and conduct several layout experiments.

14.Menu.h
The desktop of the arcade machine.

15.MenuButton.h
The button class of the menu interface inherits from the previous Button.

16.Option.h
The "Settings/Options Menu" of the arcade machine, the Settings interface.

17.OptionButton.h
A dedicated button for the Settings menu.

18.Process.h
Start an external process (basically a game) on Linux/ Raspberry PI and check if it is still running.
1.spawnProcess(std::string directory,std::string fileName)
Generate a new process
Parameters: The first one is placed in the directory where the running file is located, and the second one is placed in the file name of the running program.
Return value: pid_t(ID of the new process).
2.processRunning(pid_t processId)
Check if the process is alive and return T/F.

Why is this file necessary? Because the arcade machine needs to continuously listen to whether the application is still running, if the program is not running, it needs to exit and return to the main menu.

19.Rating.h
Scoring component.

20.Selector.h
Cursor selector component.

21.Splashscreen.h
The opening animation component plays the logo and provides an opening animation.

22.Table.h
The description of a table in the Database provides a blueprint for the tables in the previous database.

23.Tip.h
The screen prompts the implementation of bubbles/small pop-up Windows, which will automatically disappear after a few seconds.