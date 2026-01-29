/*

improved-edit.c   Routines specific to the improved editor.

*/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "improved-edit.h"

void send_editor_help(struct descriptor_data *d)
{
    if (using_improved_editor)
        write_to_output(d, "Instructions: /s or @ to save, /h for more options.\r\n");
    else
        write_to_output(d, "Instructions: Type @ on a line by itself to end.\r\n");
}

#if CONFIG_IMPROVED_EDITOR

int improved_editor_execute(struct descriptor_data *d, char *str)
{
    char actions[MAX_INPUT_LENGTH] = {'\0'};

    if (*str != '/')
        return STRINGADD_OK;

    strncpy(actions, str + 2, sizeof(actions) - 1);
    actions[sizeof(actions) - 1] = '\0';
    *str = '\0';

    switch (str[1])
    {
    case 'a':
        return STRINGADD_ABORT;
    case 'c':
        if (*(d->str))
        {
            free(*d->str);
            *(d->str) = NULL;
            write_to_output(d, "Current buffer cleared.\r\n");
        }
        else
            write_to_output(d, "Current buffer empty.\r\n");
        break;
    case 'd':
        parse_action(PARSE_DELETE, actions, d);
        break;
    case 'e':
        parse_action(PARSE_EDIT, actions, d);
        break;
    case 'f':
        if (*(d->str))
            parse_action(PARSE_FORMAT, actions, d);
        else
            write_to_output(d, "Current buffer empty.\r\n");
        break;
    case 'i':
        if (*(d->str))
            parse_action(PARSE_INSERT, actions, d);
        else
            write_to_output(d, "Current buffer empty.\r\n");
        break;
    case 'h':
        parse_action(PARSE_HELP, actions, d);
        break;
    case 'l':
        if (*d->str)
            parse_action(PARSE_LIST_NORM, actions, d);
        else
            write_to_output(d, "Current buffer empty.\r\n");
        break;
    case 'n':
        if (*d->str)
            parse_action(PARSE_LIST_NUM, actions, d);
        else
            write_to_output(d, "Current buffer empty.\r\n");
        break;
    case 'r':
        parse_action(PARSE_REPLACE, actions, d);
        break;
    case 's':
        return STRINGADD_SAVE;
    default:
        write_to_output(d, "Invalid option.\r\n");
        break;
    }
    return STRINGADD_ACTION;
}

/*
 * Handle some editor commands.
 */
void parse_action(int command, char *string, struct descriptor_data *d)
{
    int indent = 0, rep_all = 0, flags = 0, replaced, i, line_low, line_high, j = 0;
    unsigned int total_len;
    char *s, *t;
    // This may be needed, but gives compiler warnings, so commenting out.
    // char buf2[MAX_STRING_LENGTH] = {'\0'};

    /* --- Helper macro for safe concatenation --- */
#define SAFE_CAT(dst, src) strncat((dst), (src), sizeof(dst) - strlen(dst) - 1)
#define SAFE_PRINT(buf, fmt, ...) snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, __VA_ARGS__)

    switch (command)
    {

    case PARSE_HELP:
        write_to_output(d,
                        "Editor command formats: /<letter>\r\n\r\n"
                        "/a         -  aborts editor\r\n"
                        "/c         -  clears buffer\r\n"
                        "/d#        -  deletes a line #\r\n"
                        "/e# <text> -  changes the line at # with <text>\r\n"
                        "/f         -  formats text\r\n"
                        "/fi        -  indented formatting of text\r\n"
                        "/h         -  list text editor commands\r\n"
                        "/i# <text> -  inserts <text> before line #\r\n"
                        "/l         -  lists buffer\r\n"
                        "/n         -  lists buffer with line numbers\r\n"
                        "/r 'a' 'b' -  replace 1st occurrence of text <a> in buffer with text <b>\r\n"
                        "/ra 'a' 'b'-  replace all occurrences of text <a> within buffer with text <b>\r\n"
                        "              usage: /r[a] 'pattern' 'replacement'\r\n"
                        "/s         -  saves text\r\n");
        break;

    case PARSE_FORMAT:
        if (STATE(d) == CON_TRIGEDIT)
        {
            int format_script(struct descriptor_data * d);
            write_to_output(d, "Script %sformatted.\r\n", format_script(d) ? "" : "not ");
            return;
        }
        while (isalpha(string[j]) && j < 2)
        {
            if (string[j++] == 'i' && !indent)
            {
                indent = TRUE;
                flags += FORMAT_INDENT;
            }
        }
        format_text(d->str, flags, d, d->max_str);
        write_to_output(d, "Text formatted with%s indent.\r\n", (indent ? "" : "out"));
        break;

    case PARSE_REPLACE:
        while (isalpha(string[j]) && j < 2)
            if (string[j++] == 'a' && !indent)
                rep_all = 1;

        if ((s = strtok(string, "'")) == NULL ||
                (s = strtok(NULL, "'")) == NULL ||
                (t = strtok(NULL, "'")) == NULL ||
                (t = strtok(NULL, "'")) == NULL)
        {
            write_to_output(d, "Invalid or incomplete replace format.\r\n");
            return;
        }

        if (!*d->str)
            return;

        total_len = (strlen(t) - strlen(s)) + strlen(*d->str);

        if (total_len > d->max_str)
        {
            write_to_output(d, "Not enough space left in buffer.\r\n");
            return;
        }

        replaced = replace_str(d->str, s, t, rep_all, d->max_str);
        if (replaced > 0)
            write_to_output(d, "Replaced %d occurrence%s of '%s' with '%s'.\r\n",
                            replaced, (replaced != 1 ? "s" : ""), s, t);
        else if (replaced == 0)
            write_to_output(d, "String '%s' not found.\r\n", s);
        else
            write_to_output(d, "ERROR: Replacement string causes overflow, aborted.\r\n");
        break;

    case PARSE_DELETE:
        switch (sscanf(string, " %d - %d ", &line_low, &line_high))
        {
        case 0:
            write_to_output(d, "Specify a line number or range to delete.\r\n");
            return;
        case 1:
            line_high = line_low;
            break;
        case 2:
            if (line_high < line_low)
            {
                write_to_output(d, "Invalid range.\r\n");
                return;
            }
            break;
        }

        i = 1;
        total_len = 1;
        if ((s = *d->str) == NULL)
        {
            write_to_output(d, "Buffer is empty.\r\n");
            return;
        }
        else if (line_low > 0)
        {
            while (s && i < line_low)
                if ((s = strchr(s, '\n')) != NULL)
                {
                    i++;
                    s++;
                }
            if (s == NULL || i < line_low)
            {
                write_to_output(d, "Line(s) out of range; not deleting.\r\n");
                return;
            }
            t = s;
            while (s && i < line_high)
                if ((s = strchr(s, '\n')) != NULL)
                {
                    i++;
                    total_len++;
                    s++;
                }
            if (s && (s = strchr(s, '\n')) != NULL)
            {
                while (*(++s))
                    *(t++) = *s;
            }
            else
                total_len--;
            *t = '\0';
            RECREATE(*d->str, char, strlen(*d->str) + 3);
            write_to_output(d, "%d line%s deleted.\r\n", total_len,
                            (total_len != 1 ? "s" : ""));
        }
        else
        {
            write_to_output(d, "Line numbers must be > 0.\r\n");
            return;
        }
        break;

    /* --- Updated section fixing sprintf(buf, "%s...", buf) bug --- */
    case PARSE_LIST_NUM:
    {
        char outbuf[MAX_STRING_LENGTH];
        size_t used = 0;
        int line = 1;
        char *s, *e;

        outbuf[0] = '\0';

        if (*string)
        {
            switch (sscanf(string, " %d - %d ", &line_low, &line_high))
            {
            case 0:
                line_low = 1;
                line_high = INT_MAX;
                break;
            case 1:
                line_high = line_low;
                break;
            }
        }
        else
        {
            line_low = 1;
            line_high = INT_MAX;
        }

        if (line_low < 1 || line_high < line_low)
        {
            write_to_output(d, "Invalid range.\r\n");
            return;
        }

        if (!*d->str)
        {
            write_to_output(d, "Buffer is empty.\r\n");
            return;
        }

        s = *d->str;

        while (s && *s && line <= line_high)
        {
            e = strchr(s, '\n');
            if (!e)
                e = s + strlen(s);

            if (line >= line_low)
            {
                used += snprintf(outbuf + used, sizeof(outbuf) - used,
                                 "%4d: %.*s\r\n",
                                 line,
                                 (int)(e - s),
                                 s);

                if (used >= sizeof(outbuf) - 1)
                    break;
            }

            if (*e == '\0')
                break;

            s = e + 1;
            line++;
        }

        if (used == 0)
        {
            write_to_output(d, "Line(s) out of range; no buffer listing.\r\n");
            return;
        }

        page_string(d, outbuf, TRUE);
        break;
    }


    default:
        write_to_output(d, "Invalid option.\r\n");
        mudlog(BRF, ADMLVL_IMPL, TRUE, "SYSERR: invalid command passed to parse_action");
        return;
    }

#undef SAFE_CAT
#undef SAFE_PRINT
}



/*
 * Re-formats message type formatted char *.
 * (for strings edited with d->str) (mostly olc and mail)
 */
void format_text(char **ptr_string, int mode, struct descriptor_data *d, unsigned int maxlen)
{
    int line_chars = 0, cap_next = TRUE, cap_next_next = FALSE;
    int color_chars = 0;
    char *flow, *start = NULL;
    char formatted[MAX_STRING_LENGTH];
    size_t used = 0;
    char temp;

    if (!ptr_string || !*ptr_string)
        return;

    if (maxlen > MAX_STRING_LENGTH)
        maxlen = MAX_STRING_LENGTH;

    flow = *ptr_string;
    formatted[0] = '\0';

    if (IS_SET(mode, FORMAT_INDENT))
    {
        snprintf(formatted, sizeof(formatted), "   ");
        used = 3;
        line_chars = 3;
    }

    while (*flow && used < maxlen - 1)
    {
        while (*flow && strchr("\n\r\f\t\v ", *flow))
            flow++;

        if (!*flow)
            break;

        start = flow;
        while (*flow && !strchr("\n\r\f\t\v .?!", *flow))
        {
            if (*flow == '@' && *(flow + 1) == '@')
                color_chars += 2;
            flow++;
        }

        if (cap_next_next)
        {
            cap_next_next = FALSE;
            cap_next = TRUE;
        }

        while (strchr(".!?", *flow))
        {
            cap_next_next = TRUE;
            flow++;
        }

        temp = *flow;
        *flow = '\0';

        if ((line_chars + (int)strlen(start) + 1 - color_chars) > PAGE_WIDTH)
        {
            used += snprintf(formatted + used, maxlen - used, "\r\n");
            line_chars = color_chars = 0;
        }

        if (!cap_next && line_chars > 0)
        {
            used += snprintf(formatted + used, maxlen - used, " ");
            line_chars++;
        }
        else
        {
            cap_next = FALSE;
            CAP(start);
        }

        used += snprintf(formatted + used, maxlen - used, "%s", start);
        line_chars += strlen(start);

        *flow = temp;

        if (cap_next_next && *flow)
        {
            if ((line_chars + 3 - color_chars) > PAGE_WIDTH)
            {
                used += snprintf(formatted + used, maxlen - used, "\r\n");
                line_chars = color_chars = 0;
            }
            else
            {
                used += snprintf(formatted + used, maxlen - used,
                                 (*flow == '"' || *flow == '\'') ? "%c  " : "  ",
                                 *flow);
                if (*flow == '"' || *flow == '\'')
                    flow++;
                line_chars += 2;
            }
        }
    }

    used += snprintf(formatted + used, maxlen - used, "\r\n");

    RECREATE(*ptr_string, char, used + 1);
    strcpy(*ptr_string, formatted);
}

int replace_str(char **string, char *pattern, char *replacement,
                int rep_all, unsigned int max_size)
{
    char *src, *pos;
    size_t pat_len, rep_len, src_len;
    size_t used = 0;
    int count = 0;

    if (!string || !*string || !pattern || !replacement)
        return 0;

    src = *string;
    src_len = strlen(src);
    pat_len = strlen(pattern);
    rep_len = strlen(replacement);

    if (pat_len == 0)
        return 0;

    /* First pass: count replacements */
    for (pos = src; (pos = strstr(pos, pattern)); pos += pat_len)
    {
        count++;
        if (!rep_all)
            break;
    }

    if (count == 0)
        return 0;

    if (src_len + count * (rep_len - pat_len) > max_size)
        return -1;

    char *result;
    CREATE(result, char, max_size + 1);

    pos = src;
    while ((src = strstr(pos, pattern)))
    {
        size_t chunk = src - pos;
        memcpy(result + used, pos, chunk);
        used += chunk;

        memcpy(result + used, replacement, rep_len);
        used += rep_len;

        pos = src + pat_len;

        if (!rep_all)
            break;
    }

    strcpy(result + used, pos);

    RECREATE(*string, char, strlen(result) + 1);
    strcpy(*string, result);
    free(result);

    return count;
}


#endif
