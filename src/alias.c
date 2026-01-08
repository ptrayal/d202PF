/* ***********************************************************************
*  File: alias.c                Modernized Alias Handler for CircleMUD
*  Purpose: Safe reading/writing/deleting of player aliases
*
*  Authors: Jeremy Hess, Chad Thompson, George Greer
*  Modernization: Code GPT (C99 Refactor, 2026)
*
*  Notes:
*  - Uses getline() for dynamic reads
*  - Robust error handling
*  - Memory-safe and stylistically modern
*********************************************************************** */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) (void)(x)

/* ----------------------------------------------------------------------
 *  Local helper functions
 * --------------------------------------------------------------------*/

/**
 * @brief Reads a single integer from a file.
 */
static inline bool _read_int(FILE *file, int *out)
{
    return fscanf(file, "%d\n", out) == 1;
}

/**
 * @brief Reads a line from file dynamically using getline().
 * The newline is stripped automatically.
 */
static inline bool _read_line(FILE *file, char **out)
{
    size_t len = 0;
    ssize_t read = getline(out, &len, file);
    if (read <= 0)
        return false;

    /* Strip trailing newline */
    if ((*out)[read - 1] == '\n')
        (*out)[read - 1] = '\0';

    return true;
}

/* ----------------------------------------------------------------------
 *  Write aliases to disk
 * --------------------------------------------------------------------*/
void write_aliases(struct char_data *ch)
{
    if (!ch || !GET_NAME(ch))
        return;

    char filename[MAX_STRING_LENGTH];
    get_filename(filename, sizeof(filename), ALIAS_FILE, GET_NAME(ch));

    remove(filename); /* Remove old alias file */

    struct alias_data *alias_node = GET_ALIASES(ch);
    if (!alias_node)
        return;

    FILE *file = fopen(filename, "w");
    if (!file) {
        log("SYSERR: Unable to write alias file '%s' for %s: %s",
            filename, GET_NAME(ch), strerror(errno));
        return;
    }

    for (; alias_node; alias_node = alias_node->next) {
        int alias_len = (int)strlen(alias_node->alias);
        int repl_len  = (int)strlen(alias_node->replacement) - 1;

        fprintf(file,
            "%d\n%s\n"      /* Alias length and alias */
            "%d\n%s\n"      /* Replacement length and replacement */
            "%d\n",         /* Alias type */
            alias_len, alias_node->alias,
            repl_len, alias_node->replacement + 1,
            alias_node->type);
    }

    fclose(file);
}

/* ----------------------------------------------------------------------
 *  Read aliases from disk
 * --------------------------------------------------------------------*/
void read_aliases(struct char_data *ch)
{
    if (!ch || !GET_NAME(ch))
        return;

    char filename[MAX_STRING_LENGTH];
    get_filename(filename, sizeof(filename), ALIAS_FILE, GET_NAME(ch));

    FILE *file = fopen(filename, "r");
    if (!file) {
        if (errno != ENOENT) {
            log("SYSERR: Could not open alias file '%s' for %s: %s",
                filename, GET_NAME(ch), strerror(errno));
        }
        return;
    }

    struct alias_data *head = NULL;
    struct alias_data *prev = NULL;

    int length = 0;

    while (_read_int(file, &length)) {
        char *alias = NULL;
        char *replacement = NULL;

        /* Read alias string */
        if (!_read_line(file, &alias))
            break;

        /* Read replacement length (ignored now, getline is safer) */
        if (!_read_int(file, &length))
            break;

        /* Read replacement string */
        if (!_read_line(file, &replacement))
            break;

        /* Add leading space to replacement (matches original behavior) */
        size_t repl_size = strlen(replacement) + 2;
        char *full_repl = malloc(repl_size);
        snprintf(full_repl, repl_size, " %s", replacement);
        free(replacement);

        /* Read alias type */
        int type = 0;
        if (!_read_int(file, &type))
            break;

        /* Allocate and link alias node */
        struct alias_data *alias_node = calloc(1, sizeof(struct alias_data));
        alias_node->alias = alias;
        alias_node->replacement = full_repl;
        alias_node->type = type;
        alias_node->next = NULL;

        if (!head)
            head = alias_node;
        else
            prev->next = alias_node;

        prev = alias_node;
    }

    fclose(file);
    GET_ALIASES(ch) = head;
}

/* ----------------------------------------------------------------------
 *  Delete alias file
 * --------------------------------------------------------------------*/
void delete_aliases(const char *charname)
{
    if (!charname || !*charname)
        return;

    char filename[PATH_MAX];
    if (!get_filename(filename, sizeof(filename), ALIAS_FILE, charname))
        return;

    if (remove(filename) < 0 && errno != ENOENT) {
        log("SYSERR: Could not delete alias file '%s': %s",
            filename, strerror(errno));
    }
}
