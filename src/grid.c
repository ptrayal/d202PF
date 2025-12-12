/* grid.c. Where grids are created, displayed and destroyed. Davion. MudBytes.net */
 
#include "conf.h"
#include "sysdep.h"

#include <sys/types.h>
#include <sys/time.h>
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
 
#include "structs.h"
#include "utils.h"
#include "grid.h"
#include "comm.h"

// Change the function declaration at the beginning of the file
void row_to_char_debug(GRID_ROW *row, struct char_data *ch);

// Add this declaration before the implementation of row_to_char_debug
void grid_to_char_debug(GRID_DATA *grid, struct char_data *ch, bool destroy);

#define MAX_LINES_PER_CELL 20

/*
 * Returns how many visible characters (non-color) are in a string.
 * Example: "@RRed@n" => visible length = 3
 */
int visible_strlen(const char *str)
{
    int len = 0;
    for (const char *p = str; *p; p++) {
        if (*p == '@') {  // start of color code
            p++;          // skip the code letter
            continue;
        }
        len++;
    }
    return len;
}

/*
 * Copies up to `visible_limit` visible characters from src into dest.
 * Color codes are copied but not counted toward the limit.
 */
void visible_strncpy(char *dest, const char *src, int visible_limit, size_t dest_size)
{
    int visible = 0;
    const char *p = src;
    char *d = dest;
    while (*p && visible < visible_limit && (d - dest) < (int)dest_size - 1)
    {
        if (*p == '@') {
            // Copy color code literally
            *d++ = *p++;
            if (*p && (d - dest) < (int)dest_size - 1)
                *d++ = *p++;
            continue;
        }
        *d++ = *p++;
        visible++;
    }
    *d = '\0';
}


 
//Creation/Destruction
GRID_DATA * create_grid(int width)
{
    GRID_DATA *new_grid;

    new_grid = calloc(1, sizeof(*new_grid) );
    new_grid->border_corner = '+';
    new_grid->border_left = '|';
    new_grid->border_right = '|';
    new_grid->border_internal = '+';
    new_grid->border_top = '_';
    new_grid->border_bottom = '-';
    new_grid->width = width;
    return new_grid;
}

GRID_DATA * destroy_grid( GRID_DATA *grid )
{   GRID_ROW *row, *row_next = NULL;
    GRID_CELL *cell, *cell_next;
 
    if(grid->first_row)
        for(row = grid->first_row; row; row = row_next )
        {   row_next = row->next;
            if( row->first_cell )
                for( cell = row->first_cell ; cell ; cell = cell_next )
                {   cell_next = cell->next;
                    destroy_cell(cell);
                }
            destroy_row(row);
        }
    free(grid);
    return NULL;
}
GRID_ROW *create_row(GRID_DATA *grid)
{   GRID_ROW *new_row;
 
    new_row = calloc(1, sizeof(*new_row));
    new_row->grid = grid;
    if(grid)
        grid_add_row(grid, new_row);
    new_row->padding_top=0;
    new_row->padding_bottom=0;
    new_row->padding_left=1;
    new_row->padding_right=1;
    return new_row;
 
}
GRID_ROW * destroy_row( GRID_ROW *row )
{   if(row->grid)
        grid_remove_row(row);
    free(row);
    return NULL;
}
 
 
GRID_CELL *create_cell(GRID_ROW *row, int width)
{   GRID_CELL *new_cell;
    new_cell = calloc(1, sizeof(*new_cell));
    new_cell->contents[0] = '\0';
    new_cell->width = width;
    new_cell->row = row;
    if( row )
        row_add_cell(row,new_cell);
    return new_cell;
}
 
GRID_CELL * destroy_cell(GRID_CELL *cell)
{   if( cell->row )
        row_remove_cell(cell);
    free( cell );
    return NULL;
}
//List management
void grid_add_row(GRID_DATA *grid, GRID_ROW *row)
{   if( !grid->last_row )
    {   grid->first_row = row;
        grid->last_row = row;
        row->grid = grid;
        return;
    }
    row->prev = grid->last_row;
    grid->last_row->next = row;
    grid->last_row = row;
    row->grid = grid;
    return;
}
void grid_remove_row( GRID_ROW *row )
{   GRID_DATA *grid = row->grid;
    if(!grid)
        return;
    if( row == grid->first_row )
        grid->first_row = grid->first_row->next;
    if( row == grid->last_row )
        grid->last_row = row->prev;
    if( row->prev )
        row->prev->next = row->next;
    if( row->next )
        row->next->prev = row->prev;
    row->grid = NULL;
    row->next = NULL;
    row->prev = NULL;
}
 
void row_add_cell( GRID_ROW *row, GRID_CELL *cell)
{   if( row->curr_width + cell->width > row->grid->width )
        log("Warning: Added Cell Width Overflows Grid");
 
    if( !row->last_cell )
    {   row->first_cell = cell;
        row->last_cell = cell;
        row->columns++;
        return;
    }
    cell->prev = row->last_cell;
    row->last_cell->next = cell;
    row->last_cell = cell;
    cell->row = row;
    row->curr_width += cell->width; 
    row->columns++;
    return;
}
 
void row_remove_cell( GRID_CELL *cell)
{   GRID_ROW *row = cell->row;
    if(!row)
        return;
    if( cell == row->first_cell )
        row->first_cell = row->first_cell->next;
    if( cell == row->last_cell )
        row->last_cell = cell->prev;
    if( cell->prev )
        cell->prev->next = cell->next;
    if( cell->next )
        cell->next->prev = cell->prev;
    row->columns--;
    cell->row = NULL;
    cell->next = NULL;
    cell->prev = NULL;
}
//Meat
//IF You alaready have a cell use this to set the contents. If it has contents already, they are overwritten.
void cell_set_contents (GRID_CELL *cell, char *fmt, ...)
{
    char buf[MSL]={'\0'};
    va_list args;
    va_start (args, fmt);
    vsprintf (buf, fmt, args);
    va_end (args);
    sprintf(cell->contents, "%s", buf);
    cell_set_linecount(cell);
}

//If you already have a cell use this to append to the cells content.
void cell_append_contents(GRID_CELL *cell, const char *fmt, ...)
{
    char buf[MSL];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    size_t dest_len = strlen(cell->contents);
    size_t space_left = (MSL > dest_len) ? (MSL - dest_len - 1) : 0;

    if (space_left > 0)
        snprintf(cell->contents + dest_len, space_left + 1, "%s", buf);

    cell_set_linecount(cell);
}


//IF you don't already have a cell, but you have a row, use this to create a cell with the contents of fmt.
// helper: set cell contents from existing va_list
void cell_set_contents_v(GRID_CELL *cell, const char *fmt, va_list args)
{
    vsnprintf(cell->contents, sizeof(cell->contents), fmt, args);
    cell_set_linecount(cell);
}

//IF You don't already have a cell, but you have a row, use this to create a cell with the contents of fmt.
GRID_CELL *row_append_cell(GRID_ROW *row, int width, const char *fmt, ...)
{
    GRID_CELL *cell = create_cell(row, width);
    va_list args;
    va_start(args, fmt);
    cell_set_contents_v(cell, fmt, args);
    va_end(args);

    log("DEBUG: Created cell width=%d contents=[%s]", width, cell->contents);

    return cell;
}



// Low-level line counter
void cell_set_linecount(GRID_CELL *cell)
{
    int count = 1;  // At least one line
    for (const char *pos = cell->contents; *pos; pos++)
    {
        if (*pos == '\n')
            count++;
    }

    cell->lines = count;

    if (cell->row->max_height < count)
        cell->row->max_height = count;
}


//Counts colour codes to display offsets properly
int count_colour( char *str )
{   
    char c;
    int count = 0;
    while ( (c = *str++ ) != '\0'  )
    {
        if(c == '{' || c == '@')
            {
                count++;
                str++;
            }
    }
    return count*2;
}

/*
 * Converts one row of the grid into formatted text with proper alignment.
 * Handles wrapping and color codes correctly.
 */
void row_to_char(GRID_ROW *row, struct char_data *ch, bool is_last_row)
{
    if (!row || !ch)
        return;

    int num_cells = 0;
    for (GRID_CELL *c = row->first_cell; c; c = c->next)
        num_cells++;

    if (num_cells == 0)
        return;

    char wrapped[num_cells][20][MSL]; // up to 20 lines per cell
    int line_counts[num_cells];
    memset(wrapped, 0, sizeof(wrapped));
    memset(line_counts, 0, sizeof(line_counts));

    int i = 0;
    for (GRID_CELL *cell = row->first_cell; cell; cell = cell->next, i++)
    {
        const char *src = cell->contents;
        int width = cell->width - 2; // inner width (minus borders)
        int vis_len = 0;
        char *dest = wrapped[i][0];
        const char *p = src;

        while (*p)
        {
            // Handle color codes (copy but don't count)
            if (*p == '@' && *(p + 1))
            {
                *dest++ = *p++;
                *dest++ = *p++;
                continue;
            }

            *dest++ = *p++;
            vis_len++;

            // Wrap when visible length reaches width
            if (vis_len >= width)
            {
                *dest = '\0';
                line_counts[i]++;
                if (line_counts[i] >= 20)
                    break;
                dest = wrapped[i][line_counts[i]];
                vis_len = 0;
            }
        }

        if (vis_len > 0 || line_counts[i] == 0)
        {
            *dest = '\0';
            line_counts[i]++;
        }
    }

    // Determine how many lines the tallest cell has
    int max_lines = 0;
    for (int j = 0; j < num_cells; j++)
        if (line_counts[j] > max_lines)
            max_lines = line_counts[j];

    // Print each line of the row
    for (int line = 0; line < max_lines; line++)
    {
        GRID_CELL *cell = row->first_cell;

        send_to_char(ch, "|");
        for (int c = 0; c < num_cells && cell; c++, cell = cell->next)
        {
            const char *text = (line < line_counts[c]) ? wrapped[c][line] : "";
            char buf[MSL];
            visible_strncpy(buf, text, cell->width - 2, sizeof(buf));

            int pad = (cell->width - 2) - visible_strlen(buf);
            send_to_char(ch, " %s%-*s |", buf, pad, "");
        }
        send_to_char(ch, "\r\n");
    }
}


void grid_to_char(GRID_DATA *grid, struct char_data *ch, bool header)
{
    if (!grid || !ch)
        return;

    // Draw the very top border
    send_to_char(ch, "+");
    if (grid->first_row && grid->first_row->first_cell)
    {
        for (GRID_CELL *cell = grid->first_row->first_cell; cell; cell = cell->next)
        {
            for (int j = 0; j < cell->width; j++)
                send_to_char(ch, "-");
            send_to_char(ch, "+");
        }
    }
    send_to_char(ch, "\r\n");

    // Print each row followed by its horizontal border
    for (GRID_ROW *row = grid->first_row; row; row = row->next)
    {
        row_to_char(row, ch, (row->next == NULL));

        // draw a horizontal divider *after each row*
        send_to_char(ch, "+");
        for (GRID_CELL *cell = row->first_cell; cell; cell = cell->next)
        {
            for (int j = 0; j < cell->width; j++)
                send_to_char(ch, "-");
            send_to_char(ch, "+");
        }
        send_to_char(ch, "\r\n");
    }
}



// Change the function definition for grid_to_char_debug
void grid_to_char_debug(GRID_DATA *grid, struct char_data *ch, bool destroy)
{
    GRID_ROW *row;
    int i;

    log("Grid Content:");

    send_to_char(ch, "%c", grid->border_corner);
    for (i = 0; i < grid->width - 1; ++i)
        send_to_char(ch, "%c", grid->border_top);
    send_to_char(ch, "%c\r\n", grid->border_corner);

    for (row = grid->first_row; row; row = row->next)
    {
        row_to_char_debug(row, ch);  // Use the debug function for rows
        send_to_char(ch, "%c", grid->border_corner);
        for (i = 0; i < grid->width - 1; ++i)
            send_to_char(ch, "%c", grid->border_bottom);
        send_to_char(ch, "%c\r\n", grid->border_corner);
    }

    if (destroy)
        destroy_grid(grid);
}



void row_to_char_debug(GRID_ROW *row, struct char_data *ch)
{
    GRID_CELL *cell;

    log("Row Content:");

    for (cell = row->first_cell; cell; cell = cell->next)
    {
        log("Cell Content:");
        log("%s", cell->contents);
    }
}
