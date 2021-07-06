#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "map.h"
#include "utils.h"
#include "vector.h"

#define MAX_CELLS (100*100)
#define INITIAL_LENGTH 3
#define GROWTH_PER_APPLE 1

Snake create_snake(int x, int y, int max_parts) {
  Snake snake;

  snake.pos = (Coordinate){x, y};
  snake.direction = (Vector){0 ,0};
  
  snake.length = 1;
  snake.pending_length = INITIAL_LENGTH - 1;

  snake.parts = calloc(max_parts, sizeof(SnakePart));

  SnakePart head = {snake.pos, HEAD};
  snake.parts[0] = head;

  return snake;
}

void delete_snake(Snake *snake) {
  free(snake->parts);
}

MapCellState get_cell_state(Map* map, Coordinate coord) {
  return map->cells[coord.x][coord.y];
}

void set_cell_state(Map* map, Coordinate coord, MapCellState state) {
  map->cells[coord.x][coord.y] = state;
}

MapCell get_cell(Map* map, Coordinate coord) {
  MapCell cell = {coord, get_cell_state(map, coord)};
  return cell;
}

Map init_map(const char *map_file)
{
  Map map;
  map.map_file = (map_file != NULL) ? map_file : DEFAULT_MAP;
  return map;
}

int get_free_cells(Map* map, MapCell* free_cells_buffer)
{
  int free_cells_length = 0;
  MapCell free_cell;

  for (int x = 0; x < map->size.x; x++)
  {
    for (int y = 0; y < map->size.y; y++)
    { 
      Coordinate pos = {x, y};
      MapCellState state = get_cell_state(map, pos);
      if (state == FREE)
      {
        free_cell = (MapCell){pos, state};
        free_cells_buffer[free_cells_length++] = free_cell;
      }
    }
  }

  return free_cells_length;
}

bool is_snake_state(MapCellState state)
{
  return ((state == HEAD) || (state == BODY) || (state == BODY2));
}

bool is_snake_in_cell(Map *map, Coordinate coord)
{
  MapCellState cell_state = get_cell_state(map, coord);
  return is_snake_state(cell_state);
}

bool coord_in_map(Map* map, Coordinate coord) {
  Vector size = map->size;
  return ((coord.x >= 0) && (coord.x < size.x) && (coord.y >= 0) && (coord.y < size.y));
}

SnakePart make_snake_part(Coordinate coord, int length) {
  SnakePart part = {coord, ((length%2) == 0) ? BODY : BODY2};
  return part;
}

void spawn_apple(Map *map)
{
  MapCell free_cells[map->cells_length];
  int free_cells_length = get_free_cells(map, free_cells);

  MapCell random_cell;
  int random_cell_index = rand_range(0, free_cells_length);
  random_cell = free_cells[random_cell_index];

  set_cell_state(map, random_cell.pos, APPLE);
}

void process_map_symbol(MapCell *cells, int *cell_count, char character, int x, int y, int *snake_x, int *snake_y) {
  MapCell cell;
  cell.pos = (Coordinate){x, y};

  switch (character)
  {
  case 'x':
    if (*snake_x >= 0)
    {
      printf("Error: two snakes found\n");
      exit(1);
    }
    cell.state = HEAD;
    cells[(*cell_count)++] = cell;
    *snake_x = x;
    *snake_y = y;
    break;
  case ' ':
    cell.state = FREE;
    cells[(*cell_count)++] = cell;
    break;
  case '0':
    cell.state = WALL;
    cells[(*cell_count)++] = cell;
    break;
  case 'a':
    cell.state = APPLE;
    cells[(*cell_count)++] = cell;
    break;
  default:
    printf("Error: unknown symbol: \"%c\"\n", character);
    exit(1);
  }
}

void load_map(Map* map) {
  const char *filename = map->map_file;

  int cell_count = 0;

  FILE* file = fopen(filename, "r");

  if (file != NULL) {
    char character;
    int x = 0, y = 0, max_x = 0;
    
    MapCell map_cells[MAX_CELLS];
    
    int snake_x = -1, snake_y;

    while (true) {
      character = fgetc(file);
    
      if (character == EOF)
        break;
      
      if (character == '\n') {
        max_x = max(max_x, x);
        x = 0;
        y++;
      } else {
        process_map_symbol(map_cells, &cell_count, character, x, y, &snake_x, &snake_y);
        x++;
      }
    }

    fclose(file);
    
    if (snake_x == -1) {
      printf("Error: no snakes found\n");
      exit(1);
    }
    
    int max_y = y;
    map->size = (Vector){max_x, max_y};
    map->cells_length = cell_count;

    Coordinate cell_coord;
    MapCellState state;

    calloc2(map->cells, max_x, max_y);
    MapCell cell;
    for (int i = 0; i < cell_count; i++)
    {
      cell = map_cells[i];
      map->cells[cell.pos.x][cell.pos.y] = cell.state;
    }

    map->snake = create_snake(snake_x, snake_y, map->cells_length);
    
    spawn_apple(map);
  } else {
    printf("File not found: \"%s\"\n", filename);
    exit(1);
  }
}

void unload_map(Map* map) {
  free2(map->cells, map->size.x);
  delete_snake(&map->snake);
}

void redraw_snake(Map* map) {
  Snake* snake = &(map->snake);

  for (int x = 0; x < map->size.x; x++)
  {
    for (int y = 0; y < map->size.x; y++)
    {
      Coordinate pos = {x, y};
      MapCellState cell_state = get_cell_state(map, pos);

      if (! ((cell_state == FREE) || is_snake_state(cell_state)))
        continue;

      set_cell_state(map, pos, FREE);

      for (int snake_part_index = 0; snake_part_index < snake->length; snake_part_index++)
      {
        SnakePart part = snake->parts[snake_part_index];
        // printf("(%d, %d) %d\n", part.pos.x, part.pos.y, part.state);
        if (vector_eq(part.pos, pos))
        {
          set_cell_state(map, pos, part.state);
          break;
        }
      }
      
    }
  }
  
}

void update_map(Map* map, Coordinate new_pos) {
  Snake *snake = &(map->snake);

  bool grow = snake->pending_length > 0;

  MapCell* cells_to_update = snake->parts;

  for (int i = snake->length-1; i >= 0; i--)
  {
    Coordinate next_pos = (i == 0) ? new_pos : cells_to_update[i-1].pos;
    cells_to_update[i].pos = next_pos;
  }
  
  MapCellState tail_state;
  if (grow) {
    snake->parts[snake->length] = make_snake_part(snake->pos, snake->length);
  }

  snake->pos = snake->parts[0].pos;

  if (grow)
  {
    snake->length++;
    snake->pending_length--;
  }

  redraw_snake(map);
}

void grow_snake(Snake *snake)
{
  snake->pending_length += GROWTH_PER_APPLE;
}

MoveResult move(Map* map, Vector direction) {
  if (direction.x * direction.y != 0) {
    printf("Error: diagonal movement is not allowed\n");
    exit(1);
  }

  MoveResult result;
  Snake* snake = &(map->snake);

  // Disallow moving back
  if (vector_eq(direction, vector_scale(snake->direction, -1)))
  {
    result = IGNORED;
    return result;
  }

  snake->direction = direction;

  Coordinate pos = snake->pos;
  Coordinate new_pos = vector_sum(pos, direction);

  if (! coord_in_map(map, new_pos))
  {
    result = DIED;
    return result;
  }

  MapCellState new_cell = get_cell_state(map, new_pos);

  switch (new_cell) {
    case FREE:
      result = MOVED;
      break;
    case APPLE:
      result = EATEN;
      set_cell_state(map, new_pos, FREE);
      grow_snake(snake);
      spawn_apple(map);
      break;
    case WALL:
    case HEAD:
    case BODY:
    case BODY2:
      result = DIED;
      break;
  }
  
  printf("Length: %d\n", snake->length);
  printf("Snake pos: (%d, %d)\n", snake->pos.x, snake->pos.y);
  update_map(map, new_pos);
  
  return result;
}

MoveResult move_up(Map* map) {
  return move(map, (Vector){0, 1});
}

MoveResult move_right(Map* map) {
  return move(map, (Vector){1, 0});
}

MoveResult move_down(Map* map) {
  return move(map, (Vector){0, -1});
}

MoveResult move_left(Map* map) {
  return move(map, (Vector){-1, 0});
}
