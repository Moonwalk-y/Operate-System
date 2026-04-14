#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <testkit.h>
#include "labyrinth.h"

void printUsage();

typedef struct{
    char* map_path;
    char player_id;
    char* move_direction;
}Game;

int main(int argc, char *argv[]) {
    Game game = {0};
    int opt;
    int version_flag = 0;
    int map_flag = 0, player_flag = 0;
    struct option long_options[] ={
        {"map", required_argument, 0, 'm'},
        {"player", required_argument, 0, 'p'},
        {"move", required_argument, 0, 'd'},
        {"version", no_argument, 0,'v'},
        {0,0,0,0}
    };

    while((opt = getopt_long(argc, argv, "m:p:d:v", long_options, NULL)) != -1){
        switch(opt){
            case 'm':
                game.map_path = optarg;
                map_flag = 1;
                break;
            case 'p':
                game.player_id = *optarg;
                player_flag = 1;
                break;
            case 'd':
                game.move_direction = optarg;
                break;
            case 'v':
                version_flag = 1;
                break;
            case '?':
                return 1;
        }    
    }

    if(version_flag == 1){
        if(argc == 2){
            printf("Labyrinth Game 1.0\n");
            return 0;
        }else return 1;
    }

    if (!map_flag || !player_flag) {
        printUsage();
        return 1;
    }
    if (!isValidPlayer(game.player_id)) {
        fprintf(stderr, "Invalid player id!\n");
        return 1;
    }

    Labyrinth labyrinth;
    if (!loadMap(&labyrinth, game.map_path)) {
        fprintf(stderr, "Map file error!\n");
        return 1;
    }
    if (labyrinth.rows > MAX_ROWS || labyrinth.cols > MAX_COLS) {
        fprintf(stderr, "Map too large!\n");
        return 1;
    }
    if (!isConnected(&labyrinth)) {
        fprintf(stderr, "Map not connected!\n");
        return 1;
    }

    Position pos = findPlayer(&labyrinth, game.player_id);
    if (pos.row == -1) {
        Position empty = findFirstEmptySpace(&labyrinth);
        if (empty.row == -1) {
            fprintf(stderr, "No empty space for player!\n");
            return 1;
        }
        labyrinth.map[empty.row][empty.col] = game.player_id;
        pos = empty;
    }

    if (game.move_direction) {
        int dr = 0, dc = 0;
        if (strcmp(game.move_direction, "up") == 0) dr = -1;
        else if (strcmp(game.move_direction, "down") == 0) dr = 1;
        else if (strcmp(game.move_direction, "left") == 0) dc = -1;
        else if (strcmp(game.move_direction, "right") == 0) dc = 1;
        else { fprintf(stderr, "Invalid move direction!\n"); return 1; }
        int nr = pos.row + dr, nc = pos.col + dc;
        if (nr < 0 || nr >= labyrinth.rows || nc < 0 || nc >= labyrinth.cols) {
            fprintf(stderr, "Move out of bounds!\n");
            return 1;
        }
        char target = labyrinth.map[nr][nc];
        if (target != '.') {
            fprintf(stderr, "Cannot move to target!\n");
            return 1;
        }
        labyrinth.map[nr][nc] = game.player_id;
        labyrinth.map[pos.row][pos.col] = '.';
        for (int i = 0; i < labyrinth.rows; ++i) {
            for (int j = 0; j < labyrinth.cols; ++j) {
                putchar(labyrinth.map[i][j]);
            }
            putchar('\n');
        }
        return 0;
    } else {
        for (int i = 0; i < labyrinth.rows; ++i) {
            for (int j = 0; j < labyrinth.cols; ++j) {
                putchar(labyrinth.map[i][j]);
            }
            putchar('\n');
        }
        return 0;
    }
}

void printUsage() {
    printf("Usage:\n");
    printf("  labyrinth --map map.txt --player id\n");
    printf("  labyrinth -m map.txt -p id\n");
    printf("  labyrinth --map map.txt --player id --move direction\n");
    printf("  labyrinth --version\n");
}

bool isValidPlayer(char playerId) {
    int id = playerId - '0';
    if(id >= 0 && id <= 9) return true;
    return false;
}

bool loadMap(Labyrinth *labyrinth, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return false;
    char line[MAX_COLS + 2]; // +2 for newline and null
    int row = 0, col = 0;
    while (fgets(line, sizeof(line), fp)) {
        int len = strlen(line);
        if (line[len-1] == '\n') line[len-1] = '\0', len--;
        if (len == 0) continue;
        if (len > MAX_COLS) { fclose(fp); return false; }
        if (row == 0) col = len;
        else if (len != col) { fclose(fp); return false; }
        if (row >= MAX_ROWS) { fclose(fp); return false; }
        for (int i = 0; i < len; ++i) {
            char c = line[i];
            if (!(c == '#' || c == '.' || (c >= '0' && c <= '9'))) {
                fclose(fp); return false;
            }
            labyrinth->map[row][i] = c;
        }
        row++;
    }
    fclose(fp);
    labyrinth->rows = row;
    labyrinth->cols = col;
    return true;
}

Position findPlayer(Labyrinth *labyrinth, char playerId) {
    for (int i = 0; i < labyrinth->rows; ++i) {
        for (int j = 0; j < labyrinth->cols; ++j) {
            if (labyrinth->map[i][j] == playerId) {
                Position pos = {i, j};
                return pos;
            }
        }
    }
    Position pos = {-1, -1};
    return pos;
}

Position findFirstEmptySpace(Labyrinth *labyrinth) {
    for (int i = 0; i < labyrinth->rows; ++i) {
        for (int j = 0; j < labyrinth->cols; ++j) {
            char c = labyrinth->map[i][j];
            if (c == '.') {
                Position pos = {i, j};
                return pos;
            }
        }
    }
    Position pos = {-1, -1};
    return pos;
}

bool isEmptySpace(Labyrinth *labyrinth, int row, int col) {
    if (row < 0 || row >= labyrinth->rows || col < 0 || col >= labyrinth->cols) return false;
    char c = labyrinth->map[row][col];
    return (c == '.');
}

bool movePlayer(Labyrinth *labyrinth, char playerId, const char *direction) {
    // TODO: Implement this function
    return false;
}

bool saveMap(Labyrinth *labyrinth, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return false;
    for (int i = 0; i < labyrinth->rows; ++i) {
        for (int j = 0; j < labyrinth->cols; ++j) {
            fputc(labyrinth->map[i][j], fp);
        }
        fputc('\n', fp);
    }
    fclose(fp);
    return true;
}

// Check if all empty spaces are connected using DFS
void dfs(Labyrinth *labyrinth, int row, int col, bool visited[MAX_ROWS][MAX_COLS]) {
    if (row < 0 || row >= labyrinth->rows || col < 0 || col >= labyrinth->cols) return;
    if (visited[row][col]) return;
    char c = labyrinth->map[row][col];
    if (!(c == '.' || (c >= '0' && c <= '9'))) return;
    visited[row][col] = true;
    int dr[4] = {-1, 1, 0, 0};
    int dc[4] = {0, 0, -1, 1};
    for (int d = 0; d < 4; ++d) {
        dfs(labyrinth, row + dr[d], col + dc[d], visited);
    }
}

bool isConnected(Labyrinth *labyrinth) {
    bool visited[MAX_ROWS][MAX_COLS] = {0};
    // 找到第一个空地或玩家
    int found = 0;
    int start_row = -1, start_col = -1;
    for (int i = 0; i < labyrinth->rows && !found; ++i) {
        for (int j = 0; j < labyrinth->cols && !found; ++j) {
            char c = labyrinth->map[i][j];
            if (c == '.' || (c >= '0' && c <= '9')) {
                start_row = i; start_col = j; found = 1;
            }
        }
    }
    if (!found) return false;
    dfs(labyrinth, start_row, start_col, visited);
    // 检查所有空地和玩家都被访问
    for (int i = 0; i < labyrinth->rows; ++i) {
        for (int j = 0; j < labyrinth->cols; ++j) {
            char c = labyrinth->map[i][j];
            if ((c == '.' || (c >= '0' && c <= '9')) && !visited[i][j]) {
                return false;
            }
        }
    }
    return true;
}
