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
    char *s, *t, temp;
    char buf[MAX_STRING_LENGTH] = {'\0'};
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
        *buf = '\0';
        if (*string)
            switch (sscanf(string, " %d - %d ", &line_low, &line_high))
            {
            case 0:
                line_low = 1;
                line_high = 999999;
                break;
            case 1:
                line_high = line_low;
                break;
            }
        else
        {
            line_low = 1;
            line_high = 999999;
        }

        if (line_low < 1)
        {
            write_to_output(d, "Line numbers must be greater than 0.\r\n");
            return;
        }
        if (line_high < line_low)
        {
            write_to_output(d, "Invalid range.\r\n");
            return;
        }

        i = 1;
        total_len = 0;
        s = *d->str;

        while (s && i < line_low)
            if ((s = strchr(s, '\n')) != NULL)
            {
                i++;
                s++;
            }

        if (i < line_low || s == NULL)
        {
            write_to_output(d, "Line(s) out of range; no buffer listing.\r\n");
            return;
        }

        t = s;
        while (s && i <= line_high)
            if ((s = strchr(s, '\n')) != NULL)
            {
                i++;
                total_len++;
                s++;
                temp = *s;
                *s = '\0';

                char line_buf[64];
                snprintf(line_buf, sizeof(line_buf), "%4d:\r\n", (i - 1));
                SAFE_CAT(buf, line_buf);
                SAFE_CAT(buf, t);

                *s = temp;
                t = s;
            }

        if (s && t)
        {
            temp = *s;
            *s = '\0';
            SAFE_CAT(buf, t);
            *s = temp;
        }
        else if (t)
            SAFE_CAT(buf, t);

        page_string(d, buf, TRUE);
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
    int line_chars, cap_next = TRUE, cap_next_next = FALSE, color_chars = 0, i;
    char *flow, *start = NULL, temp;
    char formatted[MAX_STRING_LENGTH] = {'\0'};

    /* Fix memory overrun. */
    if (d->max_str > MAX_STRING_LENGTH)
    {
        log("SYSERR: format_text: max_str is greater than buffer size.");
        return;
    }

    /* XXX: Want to make sure the string doesn't grow either... */

    if ((flow = *ptr_string) == NULL)
        return;

    if (IS_SET(mode, FORMAT_INDENT))
    {
        strcpy(formatted, "   ");
        line_chars = 3;
    }
    else
    {
        *formatted = '\0';
        line_chars = 0;
    }

    while (*flow)
    {
        while (*flow && strchr("\n\r\f\t\v ", *flow))
            flow++;

        if (*flow)
        {
            start = flow;
            while (*flow && !strchr("\n\r\f\t\v .?!", *flow))
            {
                if (*flow == '@')
                {
                    if (*(flow + 1) == '@')
                        color_chars++;
                    flow++;
                }
                flow++;
            }

            if (cap_next_next)
            {
                cap_next_next = FALSE;
                cap_next = TRUE;
            }

            /*
             * This is so that if we stopped on a sentence .. we move off the
             * sentence delimiter.
             */
            while (strchr(".!?", *flow))
            {
                cap_next_next = TRUE;
                flow++;
            }

            temp = *flow;
            *flow = '\0';

            if (line_chars + strlen(start) + 1 - color_chars > PAGE_WIDTH)
            {
                strcat(formatted, "\r\n");
                color_chars = line_chars = 0;
                for (i = 0; start[i]; i++)
                    if (start[i] == '@')
                        color_chars += 2;
            }

            if (!cap_next)
            {
                if (line_chars > 0)
                {
                    strcat(formatted, " ");
                    line_chars++;
                }
            }
            else
            {
                cap_next = FALSE;
                CAP(start);
            }

            line_chars += strlen(start);
            strcat(formatted, start);

            *flow = temp;
        }

        if (cap_next_next && *flow)
        {
            if (line_chars + 3 - color_chars > PAGE_WIDTH)
            {
                strcat(formatted, "\r\n");
                color_chars = line_chars = 0;
            }
            else if (*flow == '\"' || *flow == '\'')
            {
                char buf[MAX_STRING_LENGTH] = {'\0'};
                sprintf(buf, "%c  ", *flow);
                strcat(formatted, buf);
                flow++;
                line_chars++;
            }
            else
            {
                strcat(formatted, "  ");
                line_chars += 2;
            }
        }
    }
    strcat(formatted, "\r\n");

    if (strlen(formatted) + 1 > maxlen)
        formatted[maxlen - 1] = '\0';
    RECREATE(*ptr_string, char, MIN(maxlen, strlen(formatted) + 1));
    strcpy(*ptr_string, formatted);
}

int replace_str(char **string, char *pattern, char *replacement, int rep_all, unsigned int max_size)
{
    char *replace_buffer = NULL;
    char *flow, *jetsam, temp;
    int len = 0, i = 0;

    if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
        return -1;

    CREATE(replace_buffer, char, max_size);
    i = 0;
    jetsam = *string;
    flow = *string;
    *replace_buffer = '\0';

    if (rep_all)
    {
        while ((flow = (char *)strstr(flow, pattern)) != NULL)
        {
            i++;
            temp = *flow;
            *flow = '\0';
            if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size)
            {
                i = -1;
                break;
            }
            strcat(replace_buffer, jetsam);
            strcat(replace_buffer, replacement);
            *flow = temp;
            flow += strlen(pattern);
            jetsam = flow;
        }
        strcat(replace_buffer, jetsam);
    }
    else
    {
        if ((flow = (char *)strstr(*string, pattern)) != NULL)
        {
            i++;
            flow += strlen(pattern);
            len = ((char *)flow - (char *)*string) - strlen(pattern);
            strncpy(replace_buffer, *string, len);
            strcat(replace_buffer, replacement);
            strcat(replace_buffer, flow);
        }
    }

    if (i <= 0)
    {
        free(replace_buffer);
        return 0;
    }
    else
    {
        RECREATE(*string, char, strlen(replace_buffer) + 3);
        strcpy(*string, replace_buffer);
    }
    free(replace_buffer);
    return i;
}

#endif
