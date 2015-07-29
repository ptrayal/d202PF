/* grid.c. Where grids are created, displayed and destroyed. Davion. MudBytes.net */
 
#include "conf.h"
#include "sysdep.h"

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
 
#include "structs.h"
#include "utils.h"
#include "grid.h"
 
//Creation/Destruction
GRID_DATA * create_grid(int width)
{   GRID_DATA *new_grid;
 
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
    char buf[MSL];
    va_list args;
    va_start (args, fmt);
    vsprintf (buf, fmt, args);
    va_end (args);
    sprintf(cell->contents, "%s", buf);
    cell_set_linecount(cell);
}
//If you already have a cell use this to append to the cells content.
void cell_append_contents (GRID_CELL *cell, char *fmt, ...)
{
    char buf[MSL];
    va_list args;
    va_start (args, fmt);
    vsprintf (buf, fmt, args);
    va_end (args);
    strncat(cell->contents, buf, MSL );
    cell_set_linecount(cell);
}
//IF you don't already have a cell, but you have a row, use this to create a cell with the contents of fmt.
GRID_CELL * row_append_cell (GRID_ROW *row, int width, char *fmt, ...)
{
    char buf[MSL];
    GRID_CELL *cell;
 
    va_list args;
    va_start (args, fmt);
    vsprintf (buf, fmt, args);
    va_end (args);
 
    cell = create_cell(row, width);
    cell_set_contents(cell, buf);
    return cell;
}
//Low level line counter
void cell_set_linecount( GRID_CELL *cell )
{   int count = 0;
    char *pos = cell->contents;
    char *last_lr;
    while(*pos++)
        if(*pos == '\n') 
        {   last_lr = pos;
            count++; 
        }
    if( (*last_lr+1) != '\0' )
        count++;
 
    cell->lines = count;
 
    if( cell->row->max_height < count )
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

//Displaying of the Grid
void row_to_char( GRID_ROW *row, struct char_data *ch )
{   GRID_CELL *cell;
    char *** ptrs;
    char *tok;
    char buf[MSL], pad_buf[MSL];
    int i = 0, n = 0;
    int filler = UMIN(0, row->grid->width - row->curr_width - 1 );
    int actual_height = row->max_height + ( row->padding_top + row->padding_bottom );
    int alloced = 0;
    ptrs = calloc(row->columns, sizeof(*ptrs) );
    //Explode columns into individual lines.
    for( i = 0, cell = row->first_cell ; cell ; cell = cell->next, ++i )
    {   int colour_offset = 0, actual_width = cell->width - row->padding_right - row->padding_left - 1;
 
        sprintf(pad_buf,"%*s%-*.*s%*s", row->padding_left, " ", actual_width, actual_width, " ", row->padding_right, " ");
 
        ptrs[i] = calloc(actual_height, sizeof( *ptrs[i] ) );
 
        tok = strtok(cell->contents, "\n");
        //Append Padding to Top.
        for( n=0; n < row->padding_top ; ++n)
            { ptrs[i][n] = strdup(pad_buf); alloced++; }
 
        while(tok)
        {   colour_offset = count_colour(tok);
 
            sprintf(buf,"%*s%-*.*s%*s", row->padding_left, " ", actual_width+colour_offset, actual_width+colour_offset, tok, row->padding_right, " ");
            ptrs[i][n] = strdup(buf);
            tok = strtok(NULL, "\n");
            ++n;
        }
        //Add padding to bottom. This will also fill in any empty rows.     
        for( ;n < actual_height; ++n )
            ptrs[i][n] = strdup(pad_buf);
    }
 
    //Go through the exploded row, and send a line from each column at a time.
 
 
    for( n = 0; n < actual_height ; ++n )       
    {   for( i = 0 ; i < row->columns ; ++i )
        {   if( i == 0 )
                send_to_char(ch, "%c", row->grid->border_left );
            else
                send_to_char(ch, "%c", row->grid->border_internal );
 
            send_to_char(ch, "%s", ptrs[i][n]);
            free(ptrs[i][n]);
        }
        send_to_char(ch, "%-*c\r\n", filler, row->grid->border_right );
    }
    for( i = 0; i < row->columns ; ++i )
        free(ptrs[i]);
    free(ptrs);
 
}
//Display the whole grid, row by row, and destroy if necessary. Should only not destroy if you intend to display it multiple times in a single function.
void grid_to_char(GRID_DATA *grid, struct char_data *ch, bool destroy)
{   GRID_ROW *row;
    int i;
 
    send_to_char(ch, "%c", grid->border_corner);
    for( i = 0; i < grid->width-1; ++i )
        send_to_char(ch, "%c", grid->border_top );
    send_to_char(ch, "%c\r\n", grid->border_corner);
    for( row = grid->first_row ; row ; row = row->next )
    {   row_to_char(row, ch);
        send_to_char(ch, "%c", grid->border_corner);
        for( i = 0; i < grid->width-1; ++i )
            send_to_char(ch, "%c", grid->border_bottom );
        send_to_char(ch, "%c\r\n", grid->border_corner);
    }
    if( destroy )
        destroy_grid(grid);
}
 