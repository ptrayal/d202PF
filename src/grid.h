/* Grid.h. The skeleton of the grid.  Davion MudBytes.net*/
 
#ifndef GRID_H
#define GRID_H
#ifndef MSL
	#define MSL MAX_STRING_LENGTH
#endif
 
#define DEFAULT_GRID_SIZE = 120		/* characters */
 
 
typedef struct grid_data GRID_DATA;
typedef struct grid_row  GRID_ROW;
typedef struct grid_cell GRID_CELL;
 
struct grid_data 
{	GRID_ROW *first_row;
	GRID_ROW *last_row;
 
	int width;
	char border_corner;
	char border_top;
	char border_left;
	char border_right;
	char border_internal;
	char border_bottom;
};
 
struct grid_row
{	GRID_DATA *grid;
	GRID_ROW *next;
	GRID_ROW *prev;
 
	GRID_CELL *first_cell;
	GRID_CELL *last_cell;
 
	int curr_width;
	int max_height;
	int columns;
 
	int padding_top;
	int padding_left;
	int padding_right;
	int padding_bottom;
};
 
struct grid_cell
{	GRID_ROW *row;
	GRID_CELL *next;
	GRID_CELL *prev;
 
	char contents[MSL];
	int width;
	int lines;
};
 
//Prototypes
void grid_to_char(GRID_DATA *grid, struct char_data *ch, bool destroy );
void row_to_char( GRID_ROW *row, struct char_data *ch );
void cell_set_linecount( GRID_CELL *cell );
void cell_append_contents (GRID_CELL *cell, char *fmt, ...);
void cell_set_contents (GRID_CELL *cell, char *fmt, ...);
GRID_CELL * row_append_cell (GRID_ROW *row, int width, char *fmt, ...);
void row_remove_cell( GRID_CELL *cell);
void row_add_cell( GRID_ROW *row, GRID_CELL *cell);
void grid_remove_row( GRID_ROW *row );
void grid_add_row(GRID_DATA *grid, GRID_ROW *row);
GRID_CELL * destroy_cell(GRID_CELL *cell);
GRID_CELL *create_cell(GRID_ROW *row, int width);
GRID_ROW *destroy_row(GRID_ROW *row);
GRID_ROW *create_row(GRID_DATA *grid);
GRID_DATA * destroy_grid(GRID_DATA *grid );
GRID_DATA * create_grid(int width);
#endif