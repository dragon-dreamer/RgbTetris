#### MEGA TETRIS board firmware code

This is full C++ source code for MEGA TETRIS board combined into Atmel Studio 6.2 solution.
Currently it has animated menu and supports several classic games (Snake, Tetris, Space Invaders, Asteroids, Maze); it has UART-based control protocol and debug mode.
Each game can be controlled using either buttons or accelerometer (this can be changed in settings menu). All games save high scores, which are then displayed in main menu (on a separate numeric display) and can be reset in settings menu.

**Main menu**
Runs after startup. You can scroll through options using "up" and "down" buttons and select option using "right" button. When selecting a game, numeric display will show current high score for that game.

**Options**
You can enter "options" menu from main menu. Options can be saved by pressing "left" button (main menu will be loaded after this). Scrolling through options is performed using "up" and "down" buttons. Checkbox values can be changed using "right" button. Brightness value can be changed using "forward" and "backwards" buttons. It's possible to reset options to their default values by pressing and holding "up" and "down" buttons before powering on the device.

**Games**
Each game starts with "3"... "2"... "1"... countdown and ends with displaying your final score (or new high score, if you've beaten old one). Every game can be paused by pressing "up" and "down" simultaneously. In pause menu, you can press "right" button to continue game or "left" button to exit to main menu. Each game supports either button or accelerometer control (sometimes mixed), which can be changed in options menu.

**"Snake" game**
Classic "Snake" game where you control a snake and eat food, which causes your snake to grow. Moving speed also increases as you earn more score. Numeric display will show your score.

**"Space invaders" game**
Something like classic "Space invaders" game. You shoot differently shaped space invaders and don't allow them to land on your shooting platform. They can shoot you, too! You have 5 lives. There're also some bosses in the game, which have a lot of lives and shoot you. These bosses don't fall down, but they're very strong and have some weak points which you can shoot. You can also damage a boss shooting anywhere at it, but in this case there's only small probability that your bullet will pass boss armor. Numeric display will show your score.

**"Tetris" game**
Classic "Tetris" game. The more score you earn, the faster is blocks fall speed. Some blocks are also added to the game after you reach certain score values.

**"Asteroids" game**
You control the plane which flies through a "cave" or something like that. Some asteroids fly towards your plane, you can shoot them down. Flying speed increases with time, the "cave" also becomes more tight. The ship is controlled using either "forward", "backwards", "left", "right" buttons or accelerometer. Use "up" button to shoot. Numeric display will show your score.

**"Maze" game**
Generates random scrollable mazes, which become more difficult each level. You have limited time to find exit. Control movement using either "forward", "backwards", "left", "right" buttons or accelerometer. Numeric display will show how many seconds you have to finish current level.

**Debugger mode**
Draws color changing dot on display. This dot can be moved either using buttons or accelerometer, depending on settings. Draws dot coordinates on number display, too. Can be stopped by pressing "up" and "down" buttons simultaneously.

**UART mode**
Starts UART and waits for data to be sent from other device (e.g. computer). You can exit this mode by pressing "up" and "down" buttons simultaneously. Detailed protocol description is available in uart.h file.
