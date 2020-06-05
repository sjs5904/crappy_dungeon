#ifndef PC_H
# define PC_H

# include <stdint.h>

# include "dims.h"
# include "character.h"
# include "dungeon.h"
# include "object.h"

class pc : public character {
 public:
  ~pc() {}
  terrain_type_t known_terrain[DUNGEON_Y][DUNGEON_X];
  uint8_t visible[DUNGEON_Y][DUNGEON_X];
  std::vector<object*> carrySlots;
  std::vector<object*> equipSlots;
  int32_t regen;
  uint32_t maxHP;
  pair_t select;
  int inSearch;
};

void pc_delete(pc *pc);
uint32_t pc_is_alive(dungeon *d);
void config_pc(dungeon *d);
uint32_t pc_next_pos(dungeon *d, pair_t dir);
void place_pc(dungeon *d);
uint32_t pc_in_room(dungeon *d, uint32_t room);
void pc_learn_terrain(pc *p, pair_t pos, terrain_type_t ter);
terrain_type_t pc_learned_terrain(pc *p, int16_t y, int16_t x);
void pc_init_known_terrain(pc *p);
void pc_observe_terrain(pc *p, dungeon *d);
int32_t is_illuminated(pc *p, int16_t y, int16_t x);
void pc_reset_visibility(pc *p);
// void pc_see_object(character *the_pc, object *o);
void pc_see_object(pc *p, object *o);
void pc_addCarry(dungeon *d, pair_t curPos);
int pc_dropItem(dungeon *d, std::string choice);
int pc_takeOff(dungeon *d);
int pc_inspect(dungeon *d);
int pc_wear(dungeon *d);
void pc_update(dungeon *d, object *o, int direction);

#endif
