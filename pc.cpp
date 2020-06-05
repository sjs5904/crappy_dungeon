#include <stdlib.h>
#include <ncurses.h>
#include <string.h>

#include "dungeon.h"
#include "pc.h"
#include "utils.h"
#include "move.h"
#include "path.h"
#include "io.h"
#include "object.h"

uint32_t pc_is_alive(dungeon_t *d)
{
  return d->PC && d->PC->alive;
}

void place_pc(dungeon_t *d)
{
  d->PC->position[dim_y] = rand_range(d->rooms->position[dim_y],
                                     (d->rooms->position[dim_y] +
                                      d->rooms->size[dim_y] - 1));
  d->PC->position[dim_x] = rand_range(d->rooms->position[dim_x],
                                     (d->rooms->position[dim_x] +
                                      d->rooms->size[dim_x] - 1));
  d->PC->select[dim_y] = d->PC->position[dim_y];
  d->PC->select[dim_x] = d->PC->position[dim_x];

  pc_init_known_terrain(d->PC);
  pc_observe_terrain(d->PC, d);

  io_display(d);
}

void config_pc(dungeon_t *d)
{
  static dice pc_dice(0, 1, 4);

  d->PC = new pc;

  d->PC->symbol = '@';
  d->PC->inSearch = 0;

  place_pc(d);

  d->PC->speed = PC_SPEED;
  d->PC->alive = 1;
  d->PC->sequence_number = 0;
  d->PC->kills[kill_direct] = d->PC->kills[kill_avenged] = 0;
  d->PC->color.push_back(COLOR_WHITE);
  d->PC->damage = &pc_dice;
  d->PC->name = "Isabella Garcia-Shapiro";
  d->PC->hp = d->PC->maxHP = 30;
  d->PC->wet = false;
  d->PC->regen = 0;

  int i;
  object *temp;
  temp = NULL;
  for(i=0; i < 13; i++){
    d->PC->equipSlots.push_back(temp);
  }

  d->character_map[character_get_y(d->PC)][character_get_x(d->PC)] = d->PC;

  dijkstra(d);
  dijkstra_tunnel(d);
}

uint32_t pc_next_pos(dungeon_t *d, pair_t dir)
{
  static uint32_t have_seen_corner = 0;
  static uint32_t count = 0;

  dir[dim_y] = dir[dim_x] = 0;

  if (in_corner(d, d->PC)) {
    if (!count) {
      count = 1;
    }
    have_seen_corner = 1;
  }

  /* First, eat anybody standing next to us. */
  if (charxy(d->PC->position[dim_x] - 1, d->PC->position[dim_y] - 1)) {
    dir[dim_y] = -1;
    dir[dim_x] = -1;
  } else if (charxy(d->PC->position[dim_x], d->PC->position[dim_y] - 1)) {
    dir[dim_y] = -1;
  } else if (charxy(d->PC->position[dim_x] + 1, d->PC->position[dim_y] - 1)) {
    dir[dim_y] = -1;
    dir[dim_x] = 1;
  } else if (charxy(d->PC->position[dim_x] - 1, d->PC->position[dim_y])) {
    dir[dim_x] = -1;
  } else if (charxy(d->PC->position[dim_x] + 1, d->PC->position[dim_y])) {
    dir[dim_x] = 1;
  } else if (charxy(d->PC->position[dim_x] - 1, d->PC->position[dim_y] + 1)) {
    dir[dim_y] = 1;
    dir[dim_x] = -1;
  } else if (charxy(d->PC->position[dim_x], d->PC->position[dim_y] + 1)) {
    dir[dim_y] = 1;
  } else if (charxy(d->PC->position[dim_x] + 1, d->PC->position[dim_y] + 1)) {
    dir[dim_y] = 1;
    dir[dim_x] = 1;
  } else if (!have_seen_corner || count < 250) {
    /* Head to a corner and let most of the NPCs kill each other off */
    if (count) {
      count++;
    }
    if (!against_wall(d, d->PC) && ((rand() & 0x111) == 0x111)) {
      dir[dim_x] = (rand() % 3) - 1;
      dir[dim_y] = (rand() % 3) - 1;
    } else {
      dir_nearest_wall(d, d->PC, dir);
    }
  }else {
    /* And after we've been there, let's head toward the center of the map. */
    if (!against_wall(d, d->PC) && ((rand() & 0x111) == 0x111)) {
      dir[dim_x] = (rand() % 3) - 1;
      dir[dim_y] = (rand() % 3) - 1;
    } else {
      dir[dim_x] = ((d->PC->position[dim_x] > DUNGEON_X / 2) ? -1 : 1);
      dir[dim_y] = ((d->PC->position[dim_y] > DUNGEON_Y / 2) ? -1 : 1);
    }
  }

  /* Don't move to an unoccupied location if that places us next to a monster */
  if (!charxy(d->PC->position[dim_x] + dir[dim_x],
              d->PC->position[dim_y] + dir[dim_y]) &&
      ((charxy(d->PC->position[dim_x] + dir[dim_x] - 1,
               d->PC->position[dim_y] + dir[dim_y] - 1) &&
        (charxy(d->PC->position[dim_x] + dir[dim_x] - 1,
                d->PC->position[dim_y] + dir[dim_y] - 1) != d->PC)) ||
       (charxy(d->PC->position[dim_x] + dir[dim_x] - 1,
               d->PC->position[dim_y] + dir[dim_y]) &&
        (charxy(d->PC->position[dim_x] + dir[dim_x] - 1,
                d->PC->position[dim_y] + dir[dim_y]) != d->PC)) ||
       (charxy(d->PC->position[dim_x] + dir[dim_x] - 1,
               d->PC->position[dim_y] + dir[dim_y] + 1) &&
        (charxy(d->PC->position[dim_x] + dir[dim_x] - 1,
                d->PC->position[dim_y] + dir[dim_y] + 1) != d->PC)) ||
       (charxy(d->PC->position[dim_x] + dir[dim_x],
               d->PC->position[dim_y] + dir[dim_y] - 1) &&
        (charxy(d->PC->position[dim_x] + dir[dim_x],
                d->PC->position[dim_y] + dir[dim_y] - 1) != d->PC)) ||
       (charxy(d->PC->position[dim_x] + dir[dim_x],
               d->PC->position[dim_y] + dir[dim_y] + 1) &&
        (charxy(d->PC->position[dim_x] + dir[dim_x],
                d->PC->position[dim_y] + dir[dim_y] + 1) != d->PC)) ||
       (charxy(d->PC->position[dim_x] + dir[dim_x] + 1,
               d->PC->position[dim_y] + dir[dim_y] - 1) &&
        (charxy(d->PC->position[dim_x] + dir[dim_x] + 1,
                d->PC->position[dim_y] + dir[dim_y] - 1) != d->PC)) ||
       (charxy(d->PC->position[dim_x] + dir[dim_x] + 1,
               d->PC->position[dim_y] + dir[dim_y]) &&
        (charxy(d->PC->position[dim_x] + dir[dim_x] + 1,
                d->PC->position[dim_y] + dir[dim_y]) != d->PC)) ||
       (charxy(d->PC->position[dim_x] + dir[dim_x] + 1,
               d->PC->position[dim_y] + dir[dim_y] + 1) &&
        (charxy(d->PC->position[dim_x] + dir[dim_x] + 1,
                d->PC->position[dim_y] + dir[dim_y] + 1) != d->PC)))) {
    dir[dim_x] = dir[dim_y] = 0;
  }

  return 0;
}

uint32_t pc_in_room(dungeon_t *d, uint32_t room)
{
  if ((room < d->num_rooms)                                     &&
      (d->PC->position[dim_x] >= d->rooms[room].position[dim_x]) &&
      (d->PC->position[dim_x] < (d->rooms[room].position[dim_x] +
                                d->rooms[room].size[dim_x]))    &&
      (d->PC->position[dim_y] >= d->rooms[room].position[dim_y]) &&
      (d->PC->position[dim_y] < (d->rooms[room].position[dim_y] +
                                d->rooms[room].size[dim_y]))) {
    return 1;
  }

  return 0;
}

void pc_learn_terrain(pc *p, pair_t pos, terrain_type_t ter)
{
  p->known_terrain[pos[dim_y]][pos[dim_x]] = ter;
  p->visible[pos[dim_y]][pos[dim_x]] = 1;
}

void pc_reset_visibility(pc *p)
{
  uint32_t y, x;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      p->visible[y][x] = 0;
    }
  }
}

terrain_type_t pc_learned_terrain(pc *p, int16_t y, int16_t x)
{
  if (y < 0 || y >= DUNGEON_Y || x < 0 || x >= DUNGEON_X) {
    io_queue_message("Invalid value to %s: %d, %d", __FUNCTION__, y, x);
  }

  return p->known_terrain[y][x];
}

void pc_init_known_terrain(pc *p)
{
  uint32_t y, x;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      p->known_terrain[y][x] = ter_unknown;
      p->visible[y][x] = 0;
    }
  }
}

void pc_observe_terrain(pc *p, dungeon_t *d)
{
  pair_t where;
  int16_t y_min, y_max, x_min, x_max;

  y_min = p->position[dim_y] - PC_VISUAL_RANGE;
  if (y_min < 0) {
    y_min = 0;
  }
  y_max = p->position[dim_y] + PC_VISUAL_RANGE;
  if (y_max > DUNGEON_Y - 1) {
    y_max = DUNGEON_Y - 1;
  }
  x_min = p->position[dim_x] - PC_VISUAL_RANGE;
  if (x_min < 0) {
    x_min = 0;
  }
  x_max = p->position[dim_x] + PC_VISUAL_RANGE;
  if (x_max > DUNGEON_X - 1) {
    x_max = DUNGEON_X - 1;
  }

  for (where[dim_y] = y_min; where[dim_y] <= y_max; where[dim_y]++) {
    where[dim_x] = x_min;
    can_see(d, p->position, where, 1, 1);
    where[dim_x] = x_max;
    can_see(d, p->position, where, 1, 1);
  }
  /* Take one off the x range because we alreay hit the corners above. */
  for (where[dim_x] = x_min - 1; where[dim_x] <= x_max - 1; where[dim_x]++) {
    where[dim_y] = y_min;
    can_see(d, p->position, where, 1, 1);
    where[dim_y] = y_max;
    can_see(d, p->position, where, 1, 1);
  }       
}

int32_t is_illuminated(pc *p, int16_t y, int16_t x)
{
  return p->visible[y][x];
}

void pc_see_object(pc *p, object *o)
{
  if (o) {
    o->has_been_seen();
  }
}

void pc_addCarry(dungeon_t *d, pair_t curPos){
  //io_queue_message("Picked up item");
  if(d->PC->carrySlots.size() < 10){
    if(d->objmap[curPos[dim_y]][curPos[dim_x]]->get_symbol() == '&'){
      //objects are stacked
      d->PC->carrySlots.push_back(d->objmap[curPos[dim_y]][curPos[dim_x]]);
      d->objmap[curPos[dim_y]][curPos[dim_x]] = d->objmap[curPos[dim_y]][curPos[dim_x]]->next;
    }else{
      d->PC->carrySlots.push_back(d->objmap[curPos[dim_y]][curPos[dim_x]]);
      d->objmap[curPos[dim_y]][curPos[dim_x]] = NULL;
    }
  }else{
    attron(COLOR_PAIR(COLOR_RED));
    io_queue_message("No carry slots open");
    attroff(COLOR_PAIR(COLOR_RED));
  }
}

int pc_inspect(dungeon_t *d){
  attron(COLOR_PAIR(COLOR_BLUE));
  mvprintw(2, 30, " %-40s ", "Select an item to inspect.");
  attroff(COLOR_PAIR(COLOR_BLUE));
  io_list_inventory(d);

  uint16_t carryNum;
  carryNum = getch();

  if(carryNum == 27){
    return 0;
  }else if(carryNum >= 48 && carryNum < d->PC->carrySlots.size() + 48){
    std::string description;
    std::string name;
    description = d->PC->carrySlots[carryNum-48]->description;
    name = d->PC->carrySlots[carryNum-48]->name;
    clear();
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(1, 0, " %s ", "Hit escape to continue.");
    attroff(COLOR_PAIR(COLOR_RED));
    mvprintw(4, 0, " %s ", name.c_str());
    mvprintw(6, 0, " %s ", description.c_str());
    while (getch() != 27 /* escape */)
      ;
  }else{
  attron(COLOR_PAIR(COLOR_RED));
    mvprintw(2, 30, " %-40s ", "Invalid selection. Escape to continue.");
    attroff(COLOR_PAIR(COLOR_RED));
    while (getch() != 27 /* escape */)
      ;
  }

  return 0;
}

void pc_update(dungeon_t *d, object *o, int direction){
  int32_t basePC;
  uint32_t numberPC, sidesPC;
  //directions is -1 or 1 to determine if it should be added or subtracted from the current stats.
  basePC = (d->PC->damage->get_base() + o->damage.get_base()) * direction;
  numberPC = d->PC->damage->get_number() + (o->damage.get_number() * direction);
  sidesPC = d->PC->damage->get_sides() + (o->damage.get_sides() * direction);

  static dice pc_dice(basePC, numberPC, sidesPC);
  d->PC->damage = &pc_dice;

  d->PC->speed += (o->speed * direction);
  if(d->PC->speed < 0)
    d->PC->speed = 2;
  //io_queue_message("New damage is %u", d->PC->damage->roll());
  //io_queue_message("New speed is %u", d->PC->speed);
}

int pc_wear(dungeon_t *d){
  attron(COLOR_PAIR(COLOR_BLUE));
  mvprintw(2, 30, " %-40s ", "Select an item to wear.");
  attroff(COLOR_PAIR(COLOR_BLUE));
  io_list_inventory(d);

  uint16_t carryNum;
  carryNum = getch();

  if(carryNum == 27){
    return 0;
  }else if(carryNum >= 48 && carryNum < d->PC->carrySlots.size() + 48){
    object_type_t typeCarried;
    int i;

    typeCarried = d->PC->carrySlots[carryNum-48]->type;
    switch(typeCarried){
      case objtype_WEAPON:
        i=0;
        break;
      case objtype_OFFHAND:
        i=1;
        break;
      case objtype_RANGED:
        i=2;
        break;
      case objtype_ARMOR:
        i=3;
        break;
      case objtype_HELMET:
        i=4;
        break;
      case objtype_CLOAK:
        i=5;
        break;
      case objtype_GLOVES:
        i=6;
        break;
      case objtype_BOOTS:
        i=7;
        break;
      case objtype_AMULET:
        i=8;
        break;
      case objtype_LIGHT:
        i=9;
        break;
      case objtype_RING:
        i=10;
        break;
      default:
        attron(COLOR_PAIR(COLOR_RED));
        mvprintw(2, 30, " %-40s ", "Item not wearable. Escape to continue.");
        attroff(COLOR_PAIR(COLOR_RED));
        while (getch() != 27)
          ;
        return 0;
    }

    if(i==10 && d->PC->equipSlots[i] != NULL){
      i++;
    }

    if(d->PC->equipSlots[i] != NULL){
      //switch items
      object *temp;
      temp = d->PC->equipSlots[i];
      pc_update(d, d->PC->equipSlots[i], -1);
      d->PC->equipSlots[i] = d->PC->carrySlots[carryNum - 48];
      d->PC->carrySlots[carryNum - 48] = temp;
    }else{
      //set in equip
      d->PC->equipSlots[i] = d->PC->carrySlots[carryNum - 48];
      d->PC->carrySlots.erase(d->PC->carrySlots.begin() + carryNum - 48);
    }
    //update pc stats. thrid paramater is adding or subtracting stat.
    pc_update(d, d->PC->equipSlots[i], 1);

  }else{
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(2, 30, " %-40s ", "Invalid selection. Escape to continue.");
    attroff(COLOR_PAIR(COLOR_RED));
    while (getch() != 27 /* escape */)
      ;
  }
  return 0;
}

int pc_takeOff(dungeon_t *d){
  //check if we can
  if(d->PC->carrySlots.size() == 10){
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(3, 30, " %-40s ", "Inventory full. Escape to continue.");
    attroff(COLOR_PAIR(COLOR_RED));
    while (getch() != 27 /* escape */)
      ;
    return 0;
  }else{
    attron(COLOR_PAIR(COLOR_BLUE));
    mvprintw(2, 30, " %-40s ", "Select an item to take off. Escape to cancel.");
    attroff(COLOR_PAIR(COLOR_BLUE));
    io_list_equipment(d);

    uint16_t equipNum;
    equipNum = getch();

    if(equipNum == 27){
      return 0;
    }else if(equipNum >= 97 && equipNum <=108 && d->PC->equipSlots[equipNum - 97] != NULL){
      //change stats
      pc_update(d, d->PC->equipSlots[equipNum - 97], -1);

      d->PC->carrySlots.push_back(d->PC->equipSlots[equipNum - 97]);
      d->PC->equipSlots.erase(d->PC->equipSlots.begin() + equipNum - 97);
    }else{
      attron(COLOR_PAIR(COLOR_RED));
      mvprintw(2, 30, " %-40s ", "Invalid selection. Escape to continue.");
      attroff(COLOR_PAIR(COLOR_RED));
      while (getch() != 27 /* escape */)
        ;
    }

    return 0;
  }
}

int pc_dropItem(dungeon_t *d, std::string choice){
  //check if we can
  if(d->objmap[d->PC->position[dim_y]][d->PC->position[dim_x]] != NULL){
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(3, 30, " %-40s ", "Spot not available. Escape to continue.");
    attroff(COLOR_PAIR(COLOR_RED));
    while (getch() != 27 /* escape */)
      ;
    return 0;
  }else{
    attron(COLOR_PAIR(COLOR_BLUE));
    mvprintw(2, 30, " %-40s ", choice.c_str());
    attroff(COLOR_PAIR(COLOR_BLUE));
    io_list_inventory(d);

    uint16_t carryNum;
    carryNum = getch();

    if(carryNum == 27){
      return 0;
    }else if(carryNum >= 48 && carryNum < d->PC->carrySlots.size() + 48){
      d->objmap[d->PC->position[dim_y]][d->PC->position[dim_x]] = d->PC->carrySlots[carryNum-48];
      d->PC->carrySlots.erase(d->PC->carrySlots.begin() + carryNum - 48);

    }else{
      attron(COLOR_PAIR(COLOR_RED));
      mvprintw(2, 30, " %-40s ", "Invalid selection. Escape to continue.");
      attroff(COLOR_PAIR(COLOR_RED));
      while (getch() != 27 /* escape */)
        ;
    }

    return 0;
  }
}