#include <unistd.h>
#include <iostream>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>

#include "io.h"
#include "move.h"
#include "path.h"
#include "pc.h"
#include "utils.h"
#include "dungeon.h"
#include "descriptions.h"
#include "object.h"
#include "npc.h"

static dungeon *the_dungeon;

typedef struct io_message {
  char msg[71];
  struct io_message *next;
} io_message_t;

static io_message_t *io_head, *io_tail;

void io_init_terminal(void)
{
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

void io_reset_terminal(void)
{
  endwin();

  while (io_head) {
    io_tail = io_head;
    io_head = io_head->next;
    free(io_tail);
  }
  io_tail = NULL;
}

void io_queue_message(const char *format, ...)
{
  io_message_t *tmp;
  va_list ap;

  if (!(tmp = (io_message_t *) malloc(sizeof (*tmp)))) {
    perror("malloc");
    exit(1);
  }

  tmp->next = NULL;

  va_start(ap, format);

  vsnprintf(tmp->msg, sizeof (tmp->msg), format, ap);

  va_end(ap);

  if (!io_head) {
    io_head = io_tail = tmp;
  } else {
    io_tail->next = tmp;
    io_tail = tmp;
  }
}

static void io_print_message_queue(uint32_t y, uint32_t x)
{
  while (io_head) {
    io_tail = io_head;
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(y, x, "%-80s", io_head->msg);
    attroff(COLOR_PAIR(COLOR_CYAN));
    io_head = io_head->next;
    if (io_head) {
      attron(COLOR_PAIR(COLOR_CYAN));
      mvprintw(y, x + 70, "%10s", " --more-- ");
      attroff(COLOR_PAIR(COLOR_CYAN));
      refresh();
      getch();
    }
    free(io_tail);
  }
  io_tail = NULL;
}

void io_display_tunnel(dungeon *d)
{
  uint32_t y, x;
  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (charxy(x, y) == d->PC) {
        mvaddch(y + 1, x, charxy(x, y)->symbol);
      } else if (hardnessxy(x, y) == 255) {
        mvaddch(y + 1, x, '*');
      } else {
        mvaddch(y + 1, x, '0' + (d->pc_tunnel[y][x] % 10));
      }
    }
  }
  refresh();
}

void io_display_distance(dungeon *d)
{
  uint32_t y, x;
  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (charxy(x, y)) {
        mvaddch(y + 1, x, charxy(x, y)->symbol);
      } else if (hardnessxy(x, y) != 0) {
        mvaddch(y + 1, x, ' ');
      } else {
        mvaddch(y + 1, x, '0' + (d->pc_distance[y][x] % 10));
      }
    }
  }
  refresh();
}

static char hardness_to_char[] =
  "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

void io_display_hardness(dungeon *d)
{
  uint32_t y, x;
  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      mvaddch(y + 1, x, (d->hardness[y][x]                             ?
                         hardness_to_char[1 + (int) ((d->hardness[y][x] /
                                                      4.2))] : ' '));
    }
  }
  refresh();
}

static void io_redisplay_visible_monsters(dungeon *d)
{
  pair_t pos;
  uint32_t color;
  uint32_t illuminated;

  for (pos[dim_y] = -PC_VISUAL_RANGE;
       pos[dim_y] <= PC_VISUAL_RANGE;
       pos[dim_y]++) {
    for (pos[dim_x] = -PC_VISUAL_RANGE;
         pos[dim_x] <= PC_VISUAL_RANGE;
         pos[dim_x]++) {
      if ((d->PC->position[dim_y] + pos[dim_y] < 0) ||
          (d->PC->position[dim_y] + pos[dim_y] >= DUNGEON_Y) ||
          (d->PC->position[dim_x] + pos[dim_x] < 0) ||
          (d->PC->position[dim_x] + pos[dim_x] >= DUNGEON_X)) {
        continue;
      }
      if ((illuminated = is_illuminated(d->PC,
                                        d->PC->position[dim_y] + pos[dim_y],
                                        d->PC->position[dim_x] + pos[dim_x]))) {
        attron(A_BOLD);
      }
      if (d->character_map[d->PC->position[dim_y] + pos[dim_y]]
                          [d->PC->position[dim_x] + pos[dim_x]] &&
          can_see(d, d->PC->position,
                  d->character_map[d->PC->position[dim_y] + pos[dim_y]]
                                  [d->PC->position[dim_x] +
                                   pos[dim_x]]->position, 1, 0)) {
        attron(COLOR_PAIR((color = d->character_map[d->PC->position[dim_y] +
                                                    pos[dim_y]]
                                                   [d->PC->position[dim_x] +
                                                    pos[dim_x]]->get_color())));
        mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                d->PC->position[dim_x] + pos[dim_x],
                character_get_symbol(d->character_map[d->PC->position[dim_y] +
                                                      pos[dim_y]]
                                                     [d->PC->position[dim_x] +
                                                      pos[dim_x]]));
        attroff(COLOR_PAIR(color));
      } else if (d->objmap[d->PC->position[dim_y] + pos[dim_y]]
                          [d->PC->position[dim_x] + pos[dim_x]] &&
                 (can_see(d, d->PC->position,
                          d->objmap[d->PC->position[dim_y] + pos[dim_y]]
                                   [d->PC->position[dim_x] +
                                    pos[dim_x]]->get_position(), 1, 0) ||
                 d->objmap[d->PC->position[dim_y] + pos[dim_y]]
                          [d->PC->position[dim_x] + pos[dim_x]]->have_seen())) {
        attron(COLOR_PAIR(d->objmap[d->PC->position[dim_y] + pos[dim_y]]
                                   [d->PC->position[dim_x] +
                                    pos[dim_x]]->get_color()));
        mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                d->PC->position[dim_x] + pos[dim_x],
                d->objmap[d->PC->position[dim_y] + pos[dim_y]]
                         [d->PC->position[dim_x] + pos[dim_x]]->get_symbol());
        attroff(COLOR_PAIR(d->objmap[d->PC->position[dim_y] + pos[dim_y]]
                                    [d->PC->position[dim_x] +
                                     pos[dim_x]]->get_color()));
      } else {
        switch (pc_learned_terrain(d->PC,
                                   d->PC->position[dim_y] + pos[dim_y],
                                   d->PC->position[dim_x] +
                                   pos[dim_x])) {
        case ter_wall:
        case ter_wall_immutable:
        case ter_unknown:
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], ' ');
          break;
        case ter_floor:
        case ter_floor_room:
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], '.');
          break;
        case ter_water:
          attron(COLOR_PAIR(COLOR_BLUE));
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], '.');
          attroff(COLOR_PAIR(COLOR_BLUE));
          break;
        case ter_lava:
          attron(COLOR_PAIR(COLOR_RED));
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], '.');
          attroff(COLOR_PAIR(COLOR_RED));
          break;
        case ter_floor_hall:
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], '#');
          break;
        case ter_debug:
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], '*');
          break;
        case ter_stairs_up:
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], '<');
          break;
        case ter_stairs_down:
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], '>');
          break;
        default:
          mvaddch(d->PC->position[dim_y] + pos[dim_y] + 1,
                  d->PC->position[dim_x] + pos[dim_x], '0');
        }
      }
      attroff(A_BOLD);
    }
  }

  refresh();
}

static int compare_monster_distance(const void *v1, const void *v2)
{
  const character *const *c1 = (const character *const *) v1;
  const character *const *c2 = (const character *const *) v2;

  return (the_dungeon->pc_distance[(*c1)->position[dim_y]]
                                  [(*c1)->position[dim_x]] -
          the_dungeon->pc_distance[(*c2)->position[dim_y]]
                                  [(*c2)->position[dim_x]]);
}

static character *io_nearest_visible_monster(dungeon *d)
{
  character **c, *n;
  uint32_t x, y, count, i;

  c = (character **) malloc(d->num_monsters * sizeof (*c));


  for (count = 0, y = 1; y < DUNGEON_Y - 1; y++) {
    for (x = 1; x < DUNGEON_X - 1; x++) {
      if (d->character_map[y][x] && d->character_map[y][x] != d->PC) {
        c[count++] = d->character_map[y][x];
      }
    }
  }


  the_dungeon = d;
  qsort(c, count, sizeof (*c), compare_monster_distance);

  for (n = NULL, i = 0; i < count; i++) {
    if (can_see(d, character_get_pos(d->PC), character_get_pos(c[i]), 1, 0)) {
      n = c[i];
      break;
    }
  }

  free(c);

  return n;
}

void io_display(dungeon *d)
{
  pair_t pos;
  uint32_t illuminated;
  uint32_t color;
  character *c;
  int32_t visible_monsters;

  clear();
  for (visible_monsters = -1, pos[dim_y] = 0;
       pos[dim_y] < DUNGEON_Y;
       pos[dim_y]++) {
    for (pos[dim_x] = 0; pos[dim_x] < DUNGEON_X; pos[dim_x]++) {
      if ((illuminated = is_illuminated(d->PC,
                                        pos[dim_y],
                                        pos[dim_x]))) {
        attron(A_BOLD);
      }
      if(d->PC->select[dim_y] == pos[dim_y] &&
          d->PC->select[dim_x] == pos[dim_x] && d->PC->inSearch){

        attron(A_BOLD);
        mvaddch(pos[dim_y] + 1, pos[dim_x], 'O');
        attroff(A_BOLD);
      } else if (d->character_map[pos[dim_y]]
                          [pos[dim_x]] &&
          can_see(d,
                  character_get_pos(d->PC),
                  character_get_pos(d->character_map[pos[dim_y]]
                                                    [pos[dim_x]]), 1, 0)) {
        visible_monsters++;
        attron(COLOR_PAIR((color = d->character_map[pos[dim_y]]
                                                   [pos[dim_x]]->get_color())));
        mvaddch(pos[dim_y] + 1, pos[dim_x],
                character_get_symbol(d->character_map[pos[dim_y]]
                                                     [pos[dim_x]]));
        attroff(COLOR_PAIR(color));
      } else if (d->objmap[pos[dim_y]]
                          [pos[dim_x]] &&
                 (d->objmap[pos[dim_y]]
                           [pos[dim_x]]->have_seen() ||
                  can_see(d, character_get_pos(d->PC), pos, 1, 0))) {
        attron(COLOR_PAIR(d->objmap[pos[dim_y]]
                                   [pos[dim_x]]->get_color()));
        mvaddch(pos[dim_y] + 1, pos[dim_x],
                d->objmap[pos[dim_y]]
                         [pos[dim_x]]->get_symbol());
        attroff(COLOR_PAIR(d->objmap[pos[dim_y]]
                                    [pos[dim_x]]->get_color()));
      } else {
        switch (pc_learned_terrain(d->PC,
                                   pos[dim_y],
                                   pos[dim_x])) {
        case ter_wall:
        case ter_wall_immutable:
        case ter_unknown:
          mvaddch(pos[dim_y] + 1, pos[dim_x], ' ');
          break;
        case ter_floor:
        case ter_floor_room:
          mvaddch(pos[dim_y] + 1, pos[dim_x], '.');
          break;
        case ter_water:
          attron(COLOR_PAIR(COLOR_BLUE));
          mvaddch(pos[dim_y] + 1, pos[dim_x], '.');
          attroff(COLOR_PAIR(COLOR_BLUE));
          break;
        case ter_lava:
          attron(COLOR_PAIR(COLOR_RED));
          mvaddch(pos[dim_y] + 1, pos[dim_x], '.');
          attroff(COLOR_PAIR(COLOR_RED));
          break;
        case ter_floor_hall:
          mvaddch(pos[dim_y] + 1, pos[dim_x], '#');
          break;
        case ter_debug:
          mvaddch(pos[dim_y] + 1, pos[dim_x], '*');
          break;
        case ter_stairs_up:
          mvaddch(pos[dim_y] + 1, pos[dim_x], '<');
          break;
        case ter_stairs_down:
          mvaddch(pos[dim_y] + 1, pos[dim_x], '>');
          break;
        default:
 /* Use zero as an error symbol, since it stands out somewhat, and it's *
  * not otherwise used.                                                 */
          mvaddch(pos[dim_y] + 1, pos[dim_x], '0');
        }
      }
      if (illuminated) {
        attroff(A_BOLD);
      }
    }
  }

  mvprintw(23, 0, "PC position is (%3d,%2d).",
           character_get_x(d->PC), character_get_y(d->PC));

  mvprintw(22, 1, "%d known %s.", visible_monsters,
           visible_monsters > 1 ? "monsters" : "monster");
  if ((c = io_nearest_visible_monster(d))) {
    mvprintw(22, 30, "Nearest visible monster: %c at %d %c by %d %c.",
             c->symbol,
             abs(c->position[dim_y] - d->PC->position[dim_y]),
             ((c->position[dim_y] - d->PC->position[dim_y]) <= 0 ?
              'N' : 'S'),
             abs(c->position[dim_x] - d->PC->position[dim_x]),
             ((c->position[dim_x] - d->PC->position[dim_x]) <= 0 ?
              'E' : 'W'));
  } else {
    mvprintw(22, 30, "Nearest visible monster: NONE.");
  }

  io_print_message_queue(0, 0);

  refresh();
}

static void io_redisplay_non_terrain(dungeon *d, pair_t cursor)
{
  pair_t pos;
  uint32_t color;
  uint32_t illuminated;

  for (pos[dim_y] = 0; pos[dim_y] < DUNGEON_Y; pos[dim_y]++) {
    for (pos[dim_x] = 0; pos[dim_x] < DUNGEON_X; pos[dim_x]++) {
      if ((illuminated = is_illuminated(d->PC,
                                        pos[dim_y],
                                        pos[dim_x]))) {
        attron(A_BOLD);
      }
      if (cursor[dim_y] == pos[dim_y] && cursor[dim_x] == pos[dim_x]) {
        mvaddch(pos[dim_y] + 1, pos[dim_x], '*');
      } else if (d->character_map[pos[dim_y]][pos[dim_x]]) {
        attron(COLOR_PAIR((color = d->character_map[pos[dim_y]]
                                                   [pos[dim_x]]->get_color())));
        mvaddch(pos[dim_y] + 1, pos[dim_x],
                character_get_symbol(d->character_map[pos[dim_y]][pos[dim_x]]));
        attroff(COLOR_PAIR(color));
      } else if (d->objmap[pos[dim_y]][pos[dim_x]]) {
        attron(COLOR_PAIR(d->objmap[pos[dim_y]][pos[dim_x]]->get_color()));
        mvaddch(pos[dim_y] + 1, pos[dim_x],
                d->objmap[pos[dim_y]][pos[dim_x]]->get_symbol());
        attroff(COLOR_PAIR(d->objmap[pos[dim_y]][pos[dim_x]]->get_color()));
      }
      attroff(A_BOLD);
    }
  }

  refresh();
}

void io_display_no_fog(dungeon *d)
{
  uint32_t y, x;
  uint32_t color;
  character *c;

  clear();
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      if (d->character_map[y][x]) {
        attron(COLOR_PAIR((color = d->character_map[y][x]->get_color())));
        mvaddch(y + 1, x, character_get_symbol(d->character_map[y][x]));
        attroff(COLOR_PAIR(color));
      } else if (d->objmap[y][x]) {
        attron(COLOR_PAIR(d->objmap[y][x]->get_color()));
        mvaddch(y + 1, x, d->objmap[y][x]->get_symbol());
        attroff(COLOR_PAIR(d->objmap[y][x]->get_color()));
      } else {
        switch (mapxy(x, y)) {
        case ter_wall:
        case ter_wall_immutable:
          mvaddch(y + 1, x, ' ');
          break;
        case ter_floor:
        case ter_floor_room:
          mvaddch(y + 1, x, '.');
          break;
        case ter_floor_hall:
          mvaddch(y + 1, x, '#');
          break;
        case ter_debug:
          mvaddch(y + 1, x, '*');
          break;
        case ter_stairs_up:
          mvaddch(y + 1, x, '<');
          break;
        case ter_stairs_down:
          mvaddch(y + 1, x, '>');
          break;
        default:
          mvaddch(y + 1, x, '0');
        }
      }
    }
  }

  mvprintw(23, 1, "PC position is (%2d,%2d).",
           d->PC->position[dim_x], d->PC->position[dim_y]);
  mvprintw(22, 1, "%d known %s.", d->num_monsters,
           d->num_monsters > 1 ? "monsters" : "monster");
  if ((c = io_nearest_visible_monster(d))) {
    mvprintw(22, 30, "Nearest visible monster: %c at %d %c by %d %c.",
             c->symbol,
             abs(c->position[dim_y] - d->PC->position[dim_y]),
             ((c->position[dim_y] - d->PC->position[dim_y]) <= 0 ?
              'N' : 'S'),
             abs(c->position[dim_x] - d->PC->position[dim_x]),
             ((c->position[dim_x] - d->PC->position[dim_x]) <= 0 ?
              'E' : 'W'));
  } else {
    mvprintw(22, 30, "Nearest visible monster: NONE.");
  }

  io_print_message_queue(0, 0);
  refresh();
}

void io_display_monster_list(dungeon *d)
{
  mvprintw(11, 33, " HP:    XXXXX ");
  mvprintw(12, 33, " Speed: XXXXX ");
  mvprintw(14, 27, " Hit any key to continue. ");
  refresh();
  getch();
}

uint32_t io_look(dungeon *d)
{
  pair_t dest;
  int c;
  fd_set readfs;
  struct timeval tv;

  io_display_no_fog(d);

  mvprintw(0, 0, "Choose a location.  't' to teleport to; 'r' for random.");

  dest[dim_y] = d->PC->position[dim_y];
  dest[dim_x] = d->PC->position[dim_x];

  mvaddch(dest[dim_y] + 1, dest[dim_x], '*');
  refresh();

  do {
    do{
      FD_ZERO(&readfs);
      FD_SET(STDIN_FILENO, &readfs);

      tv.tv_sec = 0;
      tv.tv_usec = 125000;

      io_redisplay_non_terrain(d, dest);
    } while (!select(STDIN_FILENO + 1, &readfs, NULL, NULL, &tv));
    switch (mappair(dest)) {
    case ter_wall:
    case ter_wall_immutable:
    case ter_unknown:
      mvaddch(dest[dim_y] + 1, dest[dim_x], ' ');
      break;
    case ter_floor:
    case ter_floor_room:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '.');
      break;
    case ter_floor_hall:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '#');
      break;
    case ter_debug:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '*');
      break;
    case ter_stairs_up:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '<');
      break;
    case ter_stairs_down:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '>');
      break;
    default:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '0');
    }
    switch ((c = getch())) {
    case '7':
    case 'y':
    case KEY_HOME:
      if (dest[dim_y] != 1) {
        dest[dim_y]--;
      }
      if (dest[dim_x] != 1) {
        dest[dim_x]--;
      }
      break;
    case '8':
    case 'k':
    case KEY_UP:
      if (dest[dim_y] != 1) {
        dest[dim_y]--;
      }
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      if (dest[dim_y] != 1) {
        dest[dim_y]--;
      }
      if (dest[dim_x] != DUNGEON_X - 2) {
        dest[dim_x]++;
      }
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      if (dest[dim_x] != DUNGEON_X - 2) {
        dest[dim_x]++;
      }
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      if (dest[dim_y] != DUNGEON_Y - 2) {
        dest[dim_y]++;
      }
      if (dest[dim_x] != DUNGEON_X - 2) {
        dest[dim_x]++;
      }
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      if (dest[dim_y] != DUNGEON_Y - 2) {
        dest[dim_y]++;
      }
      break;
    case '1':
    case 'b':
    case KEY_END:
      if (dest[dim_y] != DUNGEON_Y - 2) {
        dest[dim_y]++;
      }
      if (dest[dim_x] != 1) {
        dest[dim_x]--;
      }
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      if (dest[dim_x] != 1) {
        dest[dim_x]--;
      }
      break;
    }
  } while (c != 't' && c != 27);
  
  if (d->character_map[dest[dim_y]][dest[dim_x]] != NULL && d->character_map[dest[dim_y]][dest[dim_x]] != d->PC) {
    system("clear");
    ((npc *) (d->character_map[dest[dim_y]][dest[dim_x]]))->md.printName();
    ((npc *) (d->character_map[dest[dim_y]][dest[dim_x]]))->md.printDesc();
    std::cout << std::endl;
    std::cout << "escape to continue." << std::endl;
    do {
      switch ((c = getch())) {
        case 27:
        break;
      }
    } while (c != 27);
  }

  dijkstra(d);
  dijkstra_tunnel(d);

  io_display(d);

  return 0;
}

uint32_t io_teleport_pc(dungeon *d)
{
  pair_t dest;
  int c;
  fd_set readfs;
  struct timeval tv;

  io_display_no_fog(d);

  mvprintw(0, 0, "Choose a location.  't' to teleport to; 'r' for random.");

  dest[dim_y] = d->PC->position[dim_y];
  dest[dim_x] = d->PC->position[dim_x];

  mvaddch(dest[dim_y] + 1, dest[dim_x], '*');
  refresh();

  do {
    do{
      FD_ZERO(&readfs);
      FD_SET(STDIN_FILENO, &readfs);

      tv.tv_sec = 0;
      tv.tv_usec = 125000; /* An eigth of a second */

      io_redisplay_non_terrain(d, dest);
    } while (!select(STDIN_FILENO + 1, &readfs, NULL, NULL, &tv));
    switch (mappair(dest)) {
    case ter_wall:
    case ter_wall_immutable:
    case ter_unknown:
      mvaddch(dest[dim_y] + 1, dest[dim_x], ' ');
      break;
    case ter_floor:
    case ter_floor_room:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '.');
      break;
    case ter_floor_hall:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '#');
      break;
    case ter_debug:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '*');
      break;
    case ter_stairs_up:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '<');
      break;
    case ter_stairs_down:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '>');
      break;
    default:
      mvaddch(dest[dim_y] + 1, dest[dim_x], '0');
    }
    switch ((c = getch())) {
    case '7':
    case 'y':
    case KEY_HOME:
      if (dest[dim_y] != 1) {
        dest[dim_y]--;
      }
      if (dest[dim_x] != 1) {
        dest[dim_x]--;
      }
      break;
    case '8':
    case 'k':
    case KEY_UP:
      if (dest[dim_y] != 1) {
        dest[dim_y]--;
      }
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      if (dest[dim_y] != 1) {
        dest[dim_y]--;
      }
      if (dest[dim_x] != DUNGEON_X - 2) {
        dest[dim_x]++;
      }
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      if (dest[dim_x] != DUNGEON_X - 2) {
        dest[dim_x]++;
      }
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      if (dest[dim_y] != DUNGEON_Y - 2) {
        dest[dim_y]++;
      }
      if (dest[dim_x] != DUNGEON_X - 2) {
        dest[dim_x]++;
      }
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      if (dest[dim_y] != DUNGEON_Y - 2) {
        dest[dim_y]++;
      }
      break;
    case '1':
    case 'b':
    case KEY_END:
      if (dest[dim_y] != DUNGEON_Y - 2) {
        dest[dim_y]++;
      }
      if (dest[dim_x] != 1) {
        dest[dim_x]--;
      }
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      if (dest[dim_x] != 1) {
        dest[dim_x]--;
      }
      break;
    }
  } while (c != 't' && c != 'r');

  if (c == 'r') {
    do {
      dest[dim_x] = rand_range(1, DUNGEON_X - 2);
      dest[dim_y] = rand_range(1, DUNGEON_Y - 2);
    } while (charpair(dest) || mappair(dest) < ter_floor);
  }

  if (charpair(dest) && charpair(dest) != d->PC) {
    io_queue_message("Teleport failed.  Destination occupied.");
  } else {  
    d->character_map[d->PC->position[dim_y]][d->PC->position[dim_x]] = NULL;
    d->character_map[dest[dim_y]][dest[dim_x]] = d->PC;

    d->PC->position[dim_y] = dest[dim_y];
    d->PC->position[dim_x] = dest[dim_x];
  }

  pc_observe_terrain(d->PC, d);
  dijkstra(d);
  dijkstra_tunnel(d);

  io_display(d);

  return 0;
}

#if o
/* Adjectives to describe our monsters */
static const char *adjectives[] = {
  "A menacing ",
  "A threatening ",
  "A horrifying ",
  "An intimidating ",
  "An aggressive ",
  "A frightening ",
  "A terrifying ",
  "A terrorizing ",
  "An alarming ",
  "A frightening ",
  "A dangerous ",
  "A glowering ",
  "A glaring ",
  "A scowling ",
  "A chilling ",
  "A scary ",
  "A creepy ",
  "An eerie ",
  "A spooky ",
  "A slobbering ",
  "A drooling ",
  " A horrendous ",
  "An unnerving ",
  "A cute little ",  /* Even though they're trying to kill you, */
  "A teeny-weenie ", /* they can still be cute!                 */
  "A fuzzy ",
  "A fluffy white ",
  "A kawaii ",       /* For our otaku */
  "Hao ke ai de "    /* And for our Chinese */
  /* And there's one special case (see below) */
};
#endif

static void io_scroll_monster_list(char (*s)[40], uint32_t count)
{
  uint32_t offset;
  uint32_t i;

  offset = 0;

  while (1) {
    for (i = 0; i < 13; i++) {
      mvprintw(i + 6, 19, " %-40s ", s[i + offset]);
    }
    switch (getch()) {
    case KEY_UP:
      if (offset) {
        offset--;
      }
      break;
    case KEY_DOWN:
      if (offset < (count - 13)) {
        offset++;
      }
      break;
    case 27:
      return;
    }

  }
}

static bool is_vowel(const char c)
{
  return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
          c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U');
}

static void io_list_monsters_display(dungeon *d,
                                     character **c,
                                     uint32_t count)
{
  uint32_t i;
  char (*s)[40]; /* pointer to array of 40 char */

  s = (char (*)[40]) malloc((count + 1) * sizeof (*s));

  mvprintw(3, 19, " %-40s ", "");
  snprintf(s[0], 40, "You know of %d monsters:", count);
  mvprintw(4, 19, " %-40s ", s);
  mvprintw(5, 19, " %-40s ", "");

  for (i = 0; i < count; i++) {
    snprintf(s[i], 40, "%3s%s (%c): %2d %s by %2d %s",
             (is_unique(c[i]) ? "" :
              (is_vowel(character_get_name(c[i])[0]) ? "An " : "A ")),
             character_get_name(c[i]),
             character_get_symbol(c[i]),
             abs(character_get_y(c[i]) - character_get_y(d->PC)),
             ((character_get_y(c[i]) - character_get_y(d->PC)) <= 0 ?
              "North" : "South"),
             abs(character_get_x(c[i]) - character_get_x(d->PC)),
             ((character_get_x(c[i]) - character_get_x(d->PC)) <= 0 ?
              "East" : "West"));
    if (count <= 13) {
      mvprintw(i + 6, 19, " %-40s ", s[i]);
    }
  }

  if (count <= 13) {
    mvprintw(count + 6, 19, " %-40s ", "");
    mvprintw(count + 7, 19, " %-40s ", "Hit escape to continue.");
    while (getch() != 27)
      ;
  } else {
    mvprintw(19, 19, " %-40s ", "");
    mvprintw(20, 19, " %-40s ",
             "Arrows to scroll, escape to continue.");
    io_scroll_monster_list(s, count);
  }

  free(s);
}

static void io_list_monsters(dungeon *d)
{
  character **c;
  uint32_t x, y, count;

  c = (character **) malloc(d->num_monsters * sizeof (*c));

  /* Get a linear list of monsters */
  for (count = 0, y = 1; y < DUNGEON_Y - 1; y++) {
    for (x = 1; x < DUNGEON_X - 1; x++) {
      if (d->character_map[y][x] && d->character_map[y][x] != d->PC &&
          can_see(d, character_get_pos(d->PC),
                  character_get_pos(d->character_map[y][x]), 1, 0)) {
        c[count++] = d->character_map[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  the_dungeon = d;
  qsort(c, count, sizeof (*c), compare_monster_distance);

  /* Display it */
  io_list_monsters_display(d, c, count);
  free(c);

  /* And redraw the dungeon */
  io_display(d);
}

void io_list_inventory(dungeon_t *d){
  attron(COLOR_PAIR(COLOR_CYAN));
  mvprintw(3, 30, " %-40s ", "Inventory: Escape to continue.");
  mvprintw(4, 30, " %-3s ", "0");
  mvprintw(5, 30, " %-3s ", "1");
  mvprintw(6, 30, " %-3s ", "2");
  mvprintw(7, 30, " %-3s ", "3");
  mvprintw(8, 30, " %-3s ", "4");
  mvprintw(9, 30, " %-3s ", "5");
  mvprintw(10, 30, " %-3s ", "6");
  mvprintw(11, 30, " %-3s ", "7");
  mvprintw(12, 30, " %-3s ", "8");
  mvprintw(13, 30, " %-3s ", "9");
  attroff(COLOR_PAIR(COLOR_CYAN));

  if(d->PC->carrySlots.size() > 0){
    uint16_t i;
    std::string name, strType;
    object_type_t type;
    for(i=0; i < d->PC->carrySlots.size(); i++){
      name = d->PC->carrySlots[i]->name;
      type = d->PC->carrySlots[i]->type;
      strType = object_typeName[type];
      attron(COLOR_PAIR(COLOR_CYAN));
      mvprintw(i+4, 64, " %s ", strType.c_str());
      attroff(COLOR_PAIR(COLOR_CYAN));
      attron(COLOR_PAIR(COLOR_GREEN));
      mvprintw(i+4, 74, " %s ", name.c_str());
    }
    attroff(COLOR_PAIR(COLOR_GREEN));
  }
}

void io_list_equipment(dungeon_t *d){
  attron(COLOR_PAIR(COLOR_CYAN));
  mvprintw(3, 30, " %-10s ", "Equipment: Escape to continue.");
  mvprintw(4, 30, " %-15s ", "a. WEAPON: ");
  mvprintw(5, 30, " %-15s ", "b. OFFHAND: ");
  mvprintw(6, 30, " %-15s ", "c. RANGED: ");
  mvprintw(7, 30, " %-15s ", "d. ARMOR: ");
  mvprintw(8, 30, " %-15s ", "e. HELMET: ");
  mvprintw(9, 30, " %-15s ", "f. CLOAK: ");
  mvprintw(10, 30, " %-15s ", "g. GLOVES: ");
  mvprintw(11, 30, " %-15s ", "h. BOOTS: ");
  mvprintw(12, 30, " %-15s ", "i. AMULET: ");
  mvprintw(13, 30, " %-15s ", "j. LIGHT: ");
  mvprintw(14, 30, " %-15s ", "k. RING: ");
  mvprintw(15, 30, " %-15s ", "l. RING: ");
  attroff(COLOR_PAIR(COLOR_CYAN));

  uint16_t i;
  std::string name;
  attron(COLOR_PAIR(COLOR_GREEN));
  for(i=0; i < d->PC->equipSlots.size(); i++){
    if(d->PC->equipSlots[i] != NULL){
      name = d->PC->equipSlots[i]->name;
      mvprintw(i+4, 75, " %s ", name.c_str());
    }
  }
  attroff(COLOR_PAIR(COLOR_CYAN));
}

int info(dungeon_t *d){
  d->PC->select[dim_y] = d->PC->position[dim_y];
  d->PC->select[dim_x] = d->PC->position[dim_x];
  uint16_t dir = 0;
  int invalid, selected;
  invalid = selected = 0;

  while(dir != 27){
    mvprintw(1, 30, " %-40s ", "Escape to continue. Spacebar to select.");
    switch (dir = getch()) {
    case KEY_UP:
      d->PC->select[dim_y] --;
      break;
    case KEY_RIGHT:
      d->PC->select[dim_x] ++;
      break;
    case KEY_DOWN:
      d->PC->select[dim_y] ++;
      break;
    case KEY_LEFT:
      d->PC->select[dim_x] --;
      break;
    case ' ':
      selected = 1;
      break;
    default:
      invalid = 1;
      break;
    }

    if(selected){
      if(d->character_map[d->PC->select[dim_y]][d->PC->select[dim_x]] != NULL){
        clear();
        mvprintw(0, 1, " %-40s ", "Escape to continue.");

        std::string strName;
        strName = d->character_map[d->PC->select[dim_y]][d->PC->select[dim_x]]->name;

        mvprintw(2, 1, "Name: %s", strName.c_str());
        mvprintw(3, 1, "Health: %d", d->character_map[d->PC->select[dim_y]][d->PC->select[dim_x]]->hp);
        mvprintw(4, 1, "Speed: %d", d->character_map[d->PC->select[dim_y]][d->PC->select[dim_x]]->speed);
        mvprintw(5, 1, "Base damage: %d", d->character_map[d->PC->select[dim_y]][d->PC->select[dim_x]]->damage->get_base());
        while(getch() != 27)
          ;

      }else if(d->objmap[d->PC->select[dim_y]][d->PC->select[dim_x]] != NULL){
        clear();
        mvprintw(0, 1, " %-40s ", "Escape to continue.");

        std::string strName, strDescription, strType;
        object_type_t type;
        strName = d->objmap[d->PC->select[dim_y]][d->PC->select[dim_x]]->name;
        strDescription = d->objmap[d->PC->select[dim_y]][d->PC->select[dim_x]]->description;
        type = d->objmap[d->PC->select[dim_y]][d->PC->select[dim_x]]->type;
        strType = object_typeName[type];

        mvprintw(2, 1, "Name: %s", strName.c_str());
        mvprintw(3, 1, "Type: %s", strType.c_str());
        mvprintw(4, 1, "Base damage: %d", d->objmap[d->PC->select[dim_y]][d->PC->select[dim_x]]->damage.get_base());
        mvprintw(5, 1, "Speed: %d", d->objmap[d->PC->select[dim_y]][d->PC->select[dim_x]]->speed);

        while(getch() != 27)
          ;
      }else{
        io_queue_message("Nothing.");
      }
      return 0;
    }else if(!invalid){
      io_display(d);
    }else{
      invalid = 0;
    }
  }
  return 0;
}

void io_handle_input(dungeon *d)
{
  uint32_t fail_code;
  int key;
  fd_set readfs;
  struct timeval tv;
  uint32_t fog_off = 0;
  pair_t tmp = { DUNGEON_X, DUNGEON_Y };

  do {
    do{
      FD_ZERO(&readfs);
      FD_SET(STDIN_FILENO, &readfs);

      tv.tv_sec = 0;
      tv.tv_usec = 125000;

      if (fog_off) {
        io_redisplay_non_terrain(d, tmp);
      } else {
        io_redisplay_visible_monsters(d);
      }
    } while (!select(STDIN_FILENO + 1, &readfs, NULL, NULL, &tv));
    fog_off = 0;
    switch (key = getch()) {
    case '7':
    case 'y':
    case KEY_HOME:
      fail_code = move_pc(d, 7);
      break;
    case '8':
    case 'k':
    case KEY_UP:
      fail_code = move_pc(d, 8);
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      fail_code = move_pc(d, 9);
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      fail_code = move_pc(d, 6);
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      fail_code = move_pc(d, 3);
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      fail_code = move_pc(d, 2);
      break;
    case '1':
    case 'b':
    case KEY_END:
      fail_code = move_pc(d, 1);
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      fail_code = move_pc(d, 4);
      break;
    case '5':
    case ' ':
    case '.':
    case KEY_B2:
      fail_code = 0;
      break;
    case '>':
      fail_code = move_pc(d, '>');
      break;
    case '<':
      fail_code = move_pc(d, '<');
      break;
    case 'Q':
      d->quit = 1;
      fail_code = 0;
      break;
    case 'T':
      /* Display the distances for tunnelers.             */
      io_display_tunnel(d);
      fail_code = 1;
      break;
    case 'D':
      /* Display the distances for non-tunnelers.         */
      io_display_distance(d);
      fail_code = 1;
      break;
    case 'H':
      /* Display the hardnesses.                          */
      io_display_hardness(d);
      fail_code = 1;
      break;
    case 's':
      /* Return to normal display after displaying some   *
       * special screen.                                                */
      io_display(d);
      fail_code = 1;
      break;
    case 'L':
      io_look(d);
      fail_code = 1;
      break;
    case 'g':
      /* Teleport the PC to a random place in the dungeon.              */
      io_teleport_pc(d);
      fail_code = 1;
      break;
    case 'w':
      pc_wear(d);
      io_display(d);
      break;
    case 't':
      pc_takeOff(d);
      io_display(d);
      break;
    case 'd':
      pc_dropItem(d, "Select an item to drop.");
      io_display(d);
      break;
    case 'x':
      pc_dropItem(d, "Select an item to expunge from the game.");
      d->objmap[d->PC->position[dim_y]][d->PC->position[dim_x]] = NULL;
      io_display(d);
      break;
    case 'i':
      io_list_inventory(d);

      while (getch() != 27)
        ;
      io_display(d);
      break;
    case 'e':
      io_list_equipment(d);

      while (getch() != 27)
        ;
      io_display(d);
      break;
    case 'I':
      pc_inspect(d);
      io_display(d);
      break;
    case 'm':
      io_list_monsters(d);
      fail_code = 1;
      break;
    case 'f':
      io_display_no_fog(d);
      fog_off = 1;
      fail_code = 1;
      break;
    case 'c':
      d->PC->inSearch = 1;
      info(d);
      d->PC->inSearch = 0;
      io_display(d);
      break;
    case 'q':
      io_queue_message("This is the first message.");
      io_queue_message("Since there are multiple messages, "
                       "you will see \"more\" prompts.");
      io_queue_message("You can use any key to advance through messages.");
      io_queue_message("Normal gameplay will not resume until the queue "
                       "is empty.");
      io_queue_message("Long lines will be truncated, not wrapped.");
      io_queue_message("io_queue_message() is variadic and handles "
                       "all printf() conversion specifiers.");
      io_queue_message("Did you see %s?", "what I did there");
      io_queue_message("When the last message is displayed, there will "
                       "be no \"more\" prompt.");
      io_queue_message("Have fun!  And happy printing!");
      fail_code = 0;
      break;
    default:
      mvprintw(0, 0, "Unbound key: %#o ", key);
      fail_code = 1;
    }
  } while (fail_code);
}
