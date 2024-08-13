/* breakterm, a terminal based breakout game               */
/* (c) 2024 by moshix                                      */
/* This game was made specificaly to have a game on        */
/*   z/OS Unix System Services (USS). It requires          */
/*   libncurses to compile and run. But it also builds and */
/*   and runs on every other *NIX                          */
/*  v 0.1 humble beginnings                                */
/*  v 0.2 handle ball and paddle                           */
/*  v 0.3 angle deflection                                 */
/*  v 0.4 colors!                                          */
/*  v 0.5 quittting logic                                  */
/*  v 0.6 Score keeping                                    */
/*  v 0.7 Introduce concept of player lives                */
/*  v 0.8 bricks! and points!                              */
/*  v 0.9 fine tune game parameters (paddel movmt etc.)    */
/*  v 0.9.1 some minor tweaks                              */
/*  v 1.0  make sure all bricks are cleared before exiting */
/*  v 1.1  make paddle a solid block !                     */
/*  v 1.1  More narrow bricks, for more bricks per row     */
/*  v 1.2  More fluid paddle movement                      */
/*  v 1.3  Happier brick colors                            */
/*  v 1.4  Seperate paddle and ball timers for fluid play  */
/*  v 1.5  Detect boring totally vertical ball w/o bricks  */
/*  v 1.6  Add P or p to pause/unpause the game            */
/*  v 1.7  B or b for boss key. Shows boring screen        */
/*  v 1.8  Add infinite levels! Each level ball 15% faster */
/*  v 1.9  pass lives at program invocaton -l 5 and ...    */
/*          acceleration at next game level with -a 20 (%) */

#define VERSION "1.9"  // Version of the code

/* keep includes to a minimum */
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>


/* change these values here to customize your Breakterm game!  */
#define PADDLE_WIDTH 8
#define WINDOW_WIDTH 60
#define WINDOW_HEIGHT 20
#define BRICK_WIDTH 4
#define NUM_BRICKS ((WINDOW_WIDTH / (BRICK_WIDTH + 2) - 2))
#define PACING 110000 // how many milliseconds to wait between updates - configure here!
#define PADDLE_VELOCITY 6 // how many chars the paddle moves per input
#define BALL_TIMER 3 // how fast the ball should be moving. Heavily connectiona and hw dependent


/* externals here */
int lives = 4;  // Default lives
float ball_acceleration_percent = 15.0;  // Default ball acceleration percentage

int paddle_x, paddle_dx = 0;
int ball_x, ball_y;
int ball_dx = 1, ball_dy = -1;
int score = 0;
int paddle_moved = 0;
int game_paused = 0;  // Game starts unpaused
int level = 1;        // Global variable for the level

char bricks[3][NUM_BRICKS];  // 3 rows of bricks

/* put function protos here */
void init_game();
void reset_ball();
void draw_borders();
void draw_score_and_lives();
void clear_paddle();
void draw_paddle();
void draw_bricks();
void check_ball_collision();
void move_ball();
void game_loop();
void print_game_over();
void print_welcome_message();
void quit_game();
void handle_quit_signal(int sig);
int are_all_bricks_cleared();
void print_message(const char *message, int seconds);
void show_directory_listing(); /* for boss key B or b ! */
void reset_level(); /* a level has finished */


/* entry point to game */
int main(int argc, char *argv[]) {
/* the cycle is like any other game, input, game logic */
    signal(SIGINT, handle_quit_signal);  // Handle Ctrl-C
    int opt;
    while ((opt = getopt(argc, argv, "l:a:")) != -1) {
            switch (opt) {
                case 'l':
                    lives = atoi(optarg);
                    break;
                case 'a':
                    ball_acceleration_percent = atof(optarg);
                    break;
                default:
                    fprintf(stderr, "Usage: %s [-l lives] [-a ball_acceleration_percent]\n", argv[0]);
                    exit(EXIT_FAILURE);
            }
        }



    init_game();
    print_welcome_message();
    getch();
    clear();
    game_loop();
    return 0;
}







void init_game() {
    initscr();
    start_color();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    /* color definitions */
    init_pair(1, COLOR_WHITE, COLOR_BLACK);  // Score and paddle color
    init_pair(2, COLOR_CYAN, COLOR_BLACK);    // Message color
    init_pair(3, COLOR_WHITE, COLOR_BLACK);  // Paddle color changed to yellow
    init_pair(4, COLOR_GREEN, COLOR_BLACK);   // Ball color
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Bricks first row
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK); // Bricks second row
    init_pair(7, COLOR_RED, COLOR_BLACK);    // Bricks third row
    init_pair(8, COLOR_YELLOW, COLOR_BLACK);  // Welcome message "BREAKTERM"
    init_pair(9, COLOR_GREEN, COLOR_BLACK);   // Welcome message "(2024)"
    init_pair(10, COLOR_BLUE, COLOR_BLACK);   // Welcome message "by moshix"

    paddle_x = WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2;
    reset_ball();
    memset(bricks, 1, sizeof(bricks));  // Initialize all bricks to existent
}

void reset_ball() {
    /* each new game starts with ball on the paddle */
    ball_x = paddle_x + PADDLE_WIDTH / 2 - 1;  // Center ball on the paddle
    ball_y = WINDOW_HEIGHT - 3;
    ball_dy = -1;  // make sure the ball only starts upwards
}

void draw_borders() {
    /* define playing field. can be changed in definitions at the top */
    attron(COLOR_PAIR(2));
    for (int i = 0; i < WINDOW_WIDTH; ++i) {
        mvprintw(0, i, "_"); // this is the roof
        mvprintw(WINDOW_HEIGHT - 1, i, "_"); // this is the floor
    }
    for (int i = 1; i < WINDOW_HEIGHT - 1; ++i) {
        mvprintw(i, 0, "|"); //left wall
        mvprintw(i, WINDOW_WIDTH - 1, "|"); //right wall
    }
    attroff(COLOR_PAIR(2));
}

void draw_score_and_lives() {
    attron(COLOR_PAIR(1));
    mvprintw(0, 2, "Score: %d  Lives: %d  Level: %d", score, lives, level);
    attroff(COLOR_PAIR(1));
}




void move_paddle(int direction) {
    clear_paddle();
    if (direction == KEY_LEFT && paddle_x > 1) {
        paddle_x -= 6;  // Substantially increased value for faster movement
        if (paddle_x < 1) paddle_x = 1;
    } else if (direction == KEY_RIGHT && paddle_x < WINDOW_WIDTH - PADDLE_WIDTH - 1) {
        paddle_x += 6;  // Substantially increased value for faster movement
        if (paddle_x > WINDOW_WIDTH - PADDLE_WIDTH - 1) paddle_x = WINDOW_WIDTH - PADDLE_WIDTH - 1;
    }
    draw_paddle();
}

void clear_paddle() {
    /* constantly redraw the paddle */
    for (int i = 0; i < PADDLE_WIDTH; ++i) {
        mvprintw(WINDOW_HEIGHT - 2, paddle_x + i, " ");
    }
}
void draw_paddle() {
    attron(COLOR_PAIR(3) | A_REVERSE);  // Use reverse video and yellow color
    for (int i = 0; i < PADDLE_WIDTH; ++i) {
        mvprintw(WINDOW_HEIGHT - 2, paddle_x + i, " ");
    }
    attroff(COLOR_PAIR(3) | A_REVERSE);
}

void draw_bricks() {
    int start_x = (WINDOW_WIDTH - (NUM_BRICKS * (BRICK_WIDTH + 2) - 2)) / 2;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < NUM_BRICKS; col++) {
            if (bricks[row][col]) {
                attron(COLOR_PAIR(5 + row) | A_REVERSE);
                for (int i = 0; i < BRICK_WIDTH; i++) {
                    mvprintw(2 + row * 2, start_x + col * (BRICK_WIDTH + 2) + i, " ");
                }
                attroff(COLOR_PAIR(5 + row) | A_REVERSE);
            }
        }
    }
}

void check_ball_collision() {
    if (ball_x <= 1 || ball_x >= WINDOW_WIDTH - 2) {
        ball_dx *= -1;
    }
    if (ball_y <= 1) {
        ball_dy *= -1;
    }

    // Collision with the paddle
    if (ball_y == WINDOW_HEIGHT - 3 && ball_x >= paddle_x && ball_x < paddle_x + PADDLE_WIDTH) {
        ball_dy = -1; // Reverse the vertical direction
        int center_offset = (ball_x - (paddle_x + PADDLE_WIDTH / 2));
        ball_dx += center_offset / 3;  // Adjust horizontal direction based on where it hits the paddle
        if (paddle_dx != 0) {
            ball_dx += paddle_dx;  // Add spin if the paddle is moving
        }
    }

    // Check collision with bricks
    int brick_row_height = 2;  // Assuming bricks start at row 2
    for (int row = 0; row < 3; row++) {
        int start_x = (WINDOW_WIDTH - (NUM_BRICKS * (BRICK_WIDTH + 2) - 2)) / 2; // Calculated from draw_bricks
        for (int col = 0; col < NUM_BRICKS; col++) {
            if (bricks[row][col]) {
                int brick_top = brick_row_height + row * 2;
                int brick_left = start_x + col * (BRICK_WIDTH + 2);
                int brick_right = brick_left + BRICK_WIDTH;
                if (ball_y == brick_top && ball_x >= brick_left && ball_x < brick_right) {
                    // Remove the whole brick when any part is hit
                    for (int i = 0; i < BRICK_WIDTH; i++) {
                        mvprintw(brick_top, brick_left + i, " ");
                    }
                    ball_dy *= -1;  // Reflect the ball

                    // Mark the brick as hit
                    bricks[row][col] = 0;

                    // Update the score based on the row
                    if (row == 0) {
                        score += 75;  // Top row
                    } else if (row == 1) {
                        score += 50;  // Middle row
                    } else if (row == 2) {
                        score += 25;  // Bottom row
                    }
                }
            }
        }
    }
}


void move_ball() {
    if (game_paused) return;

    attron(COLOR_PAIR(4));
    mvprintw(ball_y, ball_x, " "); // Clear old position
    ball_x += ball_dx;
    ball_y += ball_dy;

    // Check for perfectly vertical movement and no bricks in the path
    if (ball_dx == 0 && ball_dy == 1) {
        int vertical_clear = 1;
        for (int row = (ball_y / 2) - 1; row >= 0; row--) {
            int col = ball_x / (BRICK_WIDTH + 1);
            if (bricks[row][col]) {
                vertical_clear = 0;
                break;
            }
        }
        if (vertical_clear) {
            ball_dx = (ball_x < WINDOW_WIDTH / 2) ? 1 : -1; // Add slight spin towards the center
        }
    }

    check_ball_collision();

    if (ball_y >= WINDOW_HEIGHT - 1) {
        lives--;
        if (lives > 0) {
            clear();  // Clear the screen to remove stale outputs
            draw_borders();
            draw_score_and_lives();
            draw_paddle();
            draw_bricks();
            print_message("You lost one life!", 2); // Display message for 2 seconds
            usleep(2000000);  // Display message for 2 seconds

            clear();  // Clear the screen again to remove the message
            draw_borders();
            draw_score_and_lives();
            draw_paddle();
            draw_bricks();  // Redraw the game state to continue play
            reset_ball();
            paddle_moved = 0;
        } else {
            game_paused = 1; // Pause the game to display the Game Over message
            clear();  // Clear the screen before displaying the game over message
            print_game_over();
            refresh();
            usleep(2000000);  // Pause to allow the player to see the Game Over message

            endwin();
            exit(0);  // Exit the game after displaying the Game Over message
        }
    }

    // Check if all bricks are cleared
    if (are_all_bricks_cleared()) {
        game_paused = 1; // Pause the game to display the level complete message
        clear();  // Clear the screen before displaying the message
        print_message("You finished another level!", 3); // Display message for 3 seconds
        usleep(3200000);  // Pause to allow the player to see the message

        // Increase ball speed by BALL_ACCELERATION_PERCENT
        float acceleration_factor = 1 + (ball_acceleration_percent / 100.0);
        ball_dx *= acceleration_factor;
        ball_dy *= acceleration_factor;

        level++;  // Increment the level
        reset_level();  // Reset the level
        game_paused = 0;
        return;

    }

    mvprintw(ball_y, ball_x, "O");
    attroff(COLOR_PAIR(4));
    refresh();  // Make sure all changes are shown on screen
}



int are_all_bricks_cleared() {
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < NUM_BRICKS; col++) {
            if (bricks[row][col]) {
                return 0;  // There are still some bricks left
            }
        }
    }
    return 1;  // All bricks are cleared
}


void game_loop() {
    int ch;
    nodelay(stdscr, TRUE);
    timeout(20);  // Set timeout for getch() to 20 milliseconds for more responsive paddle movement
    while (1) {
        ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 'x' || ch == 'X' || ch == 3) {  // 'q', 'x', or Ctrl-C
            quit_game();
            continue;
        } else if (ch == 'p' || ch == 'P') {
            game_paused = !game_paused;  // Toggle game pause state
            if (game_paused) {
                print_message("Game Paused. Press P to resume.", 0);  // Display paused message
            } else {
                clear();
                draw_borders();
                draw_score_and_lives();
                draw_paddle();
                draw_bricks();
                refresh();
            }
            continue;
        } else if (ch == 'b' || ch == 'B') {
            show_directory_listing();  // Display directory listing
            continue;
        } else if (!game_paused && (ch == KEY_LEFT || ch == KEY_RIGHT)) {
            move_paddle(ch);
        }

        // Move the ball separately with a different timing
        if (!game_paused) {
            static int ball_timer = 0;
            if (ball_timer++ >= 3) {  // Adjust this value to control the ball speed (lower value for faster ball)
                move_ball();
                ball_timer = 0;
            }

            draw_borders();
            draw_score_and_lives();
            draw_bricks();
            refresh();
        }
        usleep(10000);  // Shorter delay to make paddle movement more fluid
    }
    endwin();
}





void print_game_over() {
    attron(COLOR_PAIR(2));
    mvprintw(WINDOW_HEIGHT / 2 - 3, (WINDOW_WIDTH / 2) - 21, "  _____                         ____                 ");
    mvprintw(WINDOW_HEIGHT / 2 - 2, (WINDOW_WIDTH / 2) - 21, " / ____|                       / __ \\                ");
    mvprintw(WINDOW_HEIGHT / 2 - 1, (WINDOW_WIDTH / 2) - 21, "| |  __  __ _ _ __ ___   ___  | |  | |_   _____ _ __ ");
    mvprintw(WINDOW_HEIGHT / 2, (WINDOW_WIDTH / 2) - 21, "| | |_ |/ _` | '_ ` _ \\ / _ \\ | |  | \\ \\ / / _ \\ '__|");
    mvprintw(WINDOW_HEIGHT / 2 + 1, (WINDOW_WIDTH / 2) - 21, "| |__| | (_| | | | | | |  __/ | |__| |\\ V /  __/ |   ");
    mvprintw(WINDOW_HEIGHT / 2 + 2, (WINDOW_WIDTH / 2) - 21, " \\_____|\\__,_|_| |_| |_|\\___|  \\____/  \\_/ \\___|_|   ");
    attroff(COLOR_PAIR(2));
    refresh();
}

void print_welcome_message() {
    attron(COLOR_PAIR(8));
    mvprintw(WINDOW_HEIGHT / 2 - 1, (WINDOW_WIDTH / 2) - 9, "BREAKTERM - v%s", VERSION);
    attroff(COLOR_PAIR(8));

    attron(COLOR_PAIR(9));
    mvprintw(WINDOW_HEIGHT / 2, (WINDOW_WIDTH / 2) - 3, "(2024)");
    attroff(COLOR_PAIR(9));

    attron(COLOR_PAIR(10));
    mvprintw(WINDOW_HEIGHT / 2 + 1, (WINDOW_WIDTH / 2) - 5, "by moshix");
    attroff(COLOR_PAIR(10));
    mvprintw(WINDOW_HEIGHT - 5, 2, "Press P to Pause, B for Boss Screen, Q to Quit");
    mvprintw(WINDOW_HEIGHT-4, 2, "You start with 4 lives and at level 1. Each time \n  you finish a level, the ball speeds up by 15pct! \n\n  Can you survive the next level?");
    refresh();
}

void quit_game() {
    flushinp();  // Remove any buffered key presses
    game_paused = 1;
    attron(COLOR_PAIR(2));
    mvprintw(WINDOW_HEIGHT / 2, (WINDOW_WIDTH / 2) - 20, "You really want to quit this amazing game? (Y/N)");
    attroff(COLOR_PAIR(2));
    refresh();

    int ch;
    while ((ch = getch())) {
        if (ch == 'Y' || ch == 'y') {
            clear();
            refresh();
            usleep(1200000);  // 2 seconds pause
            endwin();
            exit(0);
        } else if (ch == 'N' || ch == 'n') {
            clear();
            draw_borders();
            draw_score_and_lives();
            draw_paddle();
            draw_bricks();
            refresh();
            game_paused = 0;
            break;
        }
    }
}

void show_directory_listing() {
    int ch;  // Declare ch within the function
    clear();
    mvprintw(1, 0, "total 64");
    mvprintw(2, 0, "drwxr-xr-x 2 banking prod 4096 Jun 22 15:00 .");
    mvprintw(3, 0, "drwxr-xr-x 5 banking prod 4096 Jun 22 14:00 ..");
    mvprintw(4, 0, "-rw-r--r-- 1 banking prod  512 Jun 22 14:00 README.md");
    mvprintw(5, 0, "-rw-r--r-- 1 banking prod 1024 Jun 22 14:05 main.c");
    mvprintw(6, 0, "-rw-r--r-- 1 banking prod 2048 Jun 22 14:10 utils.c");
    mvprintw(7, 0, "-rw-r--r-- 1 banking prod 4096 Jun 22 14:15 config.h");
    mvprintw(8, 0, "-rw-r--r-- 1 banking prod  256 Jun 22 14:20 Makefile");
    mvprintw(9, 0, "-rw-r--r-- 1 banking prod  512 Jun 22 14:25 LICENSE");
    mvprintw(10, 0, "-rw-r--r-- 1 banking prod  512 Jun 22 14:30 banking.c");
    mvprintw(11, 0, "-rw-r--r-- 1 banking prod 1024 Jun 22 14:35 graphics.c");
    mvprintw(12, 0, "-rw-r--r-- 1 banking prod  512 Jun 22 14:40 input.h");
    mvprintw(13, 0, "-rw-r--r-- 1 banking prod  256 Jun 22 14:45 trading.c");
    mvprintw(15, 0, "Press B to return to Breakterm...");
    refresh();


    while ((ch = getch()) != 'b' && ch != 'B')
    ;  // Wait for B or b to be pressed
        clear();
        draw_borders();
        draw_score_and_lives();
        draw_paddle();
        draw_bricks();
        refresh();
}



void print_message(const char *message, int seconds) {
    attron(COLOR_PAIR(2));
    mvprintw(WINDOW_HEIGHT / 2, (WINDOW_WIDTH - strlen(message)) / 2, "%s", message);
    attroff(COLOR_PAIR(2));
    refresh();
    if (seconds > 0) {
        sleep(seconds);
        clear();
        draw_borders();
        draw_score_and_lives();
        draw_paddle();
        draw_bricks();
    }
}


void reset_level() {
    clear();
    draw_borders();
    draw_score_and_lives();
    draw_paddle();
    memset(bricks, 1, sizeof(bricks));  // Reset all bricks
    reset_ball();
    refresh();
}


void handle_quit_signal(int sig) {
    /* go out */
    quit_game();
}
