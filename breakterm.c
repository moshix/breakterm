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
/*  v 0.7 3 lives. that's it.                              */
/*  v 0.8 bricks! and points!                              */
/*  v 0.9 fine tune game parameters (paddel movmt etc.)    */
/*  v 0.9.1 some minor tweaks                              */
/*  v 1.0  make sure all bricks are cleared before exiting */
/*  v 1.1  make paddle a solid block !                     */


#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define PADDLE_WIDTH 10
#define WINDOW_WIDTH 60
#define WINDOW_HEIGHT 20
#define BRICK_WIDTH 5
#define NUM_BRICKS ((WINDOW_WIDTH / (BRICK_WIDTH + 2) - 2))
#define PACING 110000 // how many milliseconds to wait between updates - configure here!

int paddle_x, paddle_dx = 0;
int ball_x, ball_y;
int ball_dx = 1, ball_dy = -1;
int score = 0;
int lives = 3;
int paddle_moved = 0;
int game_paused = 0;

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

int main() {
    signal(SIGINT, handle_quit_signal);  // Handle Ctrl-C
    /* the cycle is like any other game, input, game logic */
    init_game();
    print_welcome_message();
    getch();
    clear();
    refresh();
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
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);  // Score and paddle color
    init_pair(2, COLOR_CYAN, COLOR_BLACK);    // Message color
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Paddle color changed to yellow
    init_pair(4, COLOR_GREEN, COLOR_BLACK);   // Ball color
    init_pair(5, COLOR_BLUE, COLOR_BLACK);    // Bricks first row
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK); // Bricks second row
    init_pair(7, COLOR_CYAN, COLOR_BLACK);    // Bricks third row
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
        mvprintw(0, i, "#");
        mvprintw(WINDOW_HEIGHT - 1, i, "#");
    }
    for (int i = 1; i < WINDOW_HEIGHT - 1; ++i) {
        mvprintw(i, 0, "#");
        mvprintw(i, WINDOW_WIDTH - 1, "#");
    }
    attroff(COLOR_PAIR(2));
}

void draw_score_and_lives() {
    /* self-explanatory */
    attron(COLOR_PAIR(1));
    mvprintw(0, 2, "Score: %d  Lives: %d", score, lives);
    attroff(COLOR_PAIR(1));
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
                attron(COLOR_PAIR(5 + row));
                for (int i = 0; i < BRICK_WIDTH; i++) {
                    mvprintw(2 + row, start_x + col * (BRICK_WIDTH + 2) + i, "=");
                }
                attroff(COLOR_PAIR(5 + row));
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
    int brick_height = 3;      // Total number of brick rows
    for (int row = 0; row < brick_height; row++) {
        int start_x = (WINDOW_WIDTH - (NUM_BRICKS * (BRICK_WIDTH + 2) - 2)) / 2; // Calculated from draw_bricks
        for (int col = 0; col < NUM_BRICKS; col++) {
            if (bricks[row][col]) {
                int brick_left = start_x + col * (BRICK_WIDTH + 2);
                int brick_right = brick_left + BRICK_WIDTH;
                if (ball_y == brick_row_height + row && ball_x >= brick_left && ball_x < brick_right) {
                    bricks[row][col] = 0;  // Mark the brick as hit
                    ball_dy *= -1;  // Reflect the ball

                    // Clear the entire brick
                    for (int i = 0; i < BRICK_WIDTH; i++) {
                        mvprintw(brick_row_height + row, brick_left + i, " ");
                    }

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
    check_ball_collision();
    
    if (ball_y >= WINDOW_HEIGHT - 1) {
        lives--;    
        if (lives > 0) {
            clear();  // Clear the screen to remove stale outputs
            draw_borders();
            draw_score_and_lives();
            draw_paddle();
            draw_bricks();
            attron(COLOR_PAIR(2));
            mvprintw(WINDOW_HEIGHT / 2, (WINDOW_WIDTH / 2) - 10, "You lost one life!");
            refresh();
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


        if (are_all_bricks_cleared()) {
            game_paused = 1; // Pause the game to display the Game Over message
            clear();  // Clear the screen before displaying the game over message
            print_game_over();
            refresh();
            usleep(2000000);  // Pause to allow the player to see the Game Over message
            endwin();
            exit(0);  // Exit the game after displaying the Game Over message
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
    while (1) {
        ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 'x' || ch == 'X' || ch == 3) {  // 'q', 'x', or Ctrl-C
            quit_game();
            continue;
        } else if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            paddle_dx = 0;
            clear_paddle();
            switch (ch) {
                case KEY_LEFT:
                    if (paddle_x > 1) {
                        paddle_x -= 4; // how many asci move at the same time
                        paddle_dx = -1;
                        paddle_moved = 1;
                    }
                    break;
                case KEY_RIGHT:
                    if (paddle_x < WINDOW_WIDTH - PADDLE_WIDTH - 1) {
                        paddle_x += 4; // how many asci move at the same time
                        paddle_dx = 1;
                        paddle_moved = 1;
                    }
                    break;
            }

            // Clear the input buffer after processing the key press
            flushinp();
        }

        draw_borders();
        draw_score_and_lives();
        draw_paddle();
        draw_bricks();
        move_ball();
        refresh();
        usleep(PACING); // 100 milliseconds delay for game pacing
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
    mvprintw(WINDOW_HEIGHT / 2 - 1, (WINDOW_WIDTH / 2) - 9, "BREAKTERM");
    attroff(COLOR_PAIR(8));

    attron(COLOR_PAIR(9));
    mvprintw(WINDOW_HEIGHT / 2, (WINDOW_WIDTH / 2) - 3, "(2024)");
    attroff(COLOR_PAIR(9));

    attron(COLOR_PAIR(10));
    mvprintw(WINDOW_HEIGHT / 2 + 1, (WINDOW_WIDTH / 2) - 5, "by moshix");
    attroff(COLOR_PAIR(10));

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

void handle_quit_signal(int sig) {
    /* go out */
    quit_game();
}

