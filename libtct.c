#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdarg.h>

typedef enum {false=0, true} bool;

#define TCT_START_SIGN "{{"
#define TCT_END_SIGN "}}"
#define TCT_START_SIGN_LEN (sizeof(TCT_START_SIGN)-1)
#define TCT_END_SIGN_LEN (sizeof(TCT_END_SIGN)-1)

typedef struct _tct_arguments {
    struct _tct_arguments *next;
    char data[0];
} tct_arguments;

typedef struct _tct_section {
    char *data;
    size_t length;
    struct _tct_section *next;
} tct_section; 

tct_arguments* tct_add_argument_(tct_arguments *next_argument, char *name, const char *format, ...) {
    va_list argp;
    tct_arguments *argument;
    size_t arg_len, name_len;

    name_len = strlen(name);
    va_start(argp, format);
    arg_len = vsnprintf(NULL, 0, format, argp);
    va_end(argp);
    argument = calloc(1, sizeof(tct_arguments) + name_len+ 1 + arg_len + 1);
    strcpy(argument->data, name);
    va_start(argp, format);
    vsnprintf(&argument->data[name_len + 1], arg_len + 1, format, argp);
    va_end(argp);
    argument->next = next_argument;

    return argument;
}
#define tct_add_argument(a, ...) {a = tct_add_argument_(a, __VA_ARGS__);}

tct_arguments* tct_find_arguments(tct_arguments *arguments, char *name, size_t name_len) {
    while(arguments)
        if(memcmp(arguments->data, name, name_len) == 0 && arguments->data[name_len] == 0)
            return arguments;
        else
            arguments = arguments->next;
    return NULL;
}
#define tct_find_argument(a, n) tct_find_arguments(a, n, strlen(n))

char* tct_get_valuen(tct_arguments *arguments, char *name, size_t name_len) {
    tct_arguments *argument = tct_find_arguments(arguments, name, name_len);
    return argument ? &argument->data[name_len + 1] : "";
}
#define tct_get_value(a, n) tct_get_valuen(a, n, strlen(n))

static bool tct_find_symbol(char *template, char** start_, char** end_) {
    char* start;
    char* end;

    *start_ = NULL;
    *end_ = NULL;

    start = template;
    while(*start) {
        if(memcmp(start, TCT_START_SIGN, TCT_START_SIGN_LEN) != 0) {
            start++; continue;
        }
        end = start + TCT_START_SIGN_LEN;
        while(*end) {
            if(memcmp(end, TCT_END_SIGN, TCT_END_SIGN_LEN) != 0) {
                end++; continue;
            }
            *start_ = start;
            *end_ = end;
            return true;
        }
        break;
    }
    return false;
}

void tct_free_argument(tct_arguments* arguments) {
    while (arguments) {
        tct_arguments *argument = arguments;
        arguments = argument->next;
        free(argument);
    }
}

char* tct_render(char *template, tct_arguments *argument) {
#define IS_WHITESPACE(c) (c==' ' || c=='\t' || c=='\r' || c=='\n') 
    tct_section *section_start, *section_current;
    char *start, *end;

    char *result, *write;
    size_t result_len;

    section_start = calloc(1, sizeof(tct_section));
    section_current = section_start;

    result_len = strlen(template);
    while (tct_find_symbol(template, &start, &end)) {
        char *trim_start, *trim_end;

        section_current->data = template;
        section_current->length = start - template;
        result_len += section_current->length;
        section_current->next = calloc(1, sizeof(tct_section));
        section_current = section_current->next;

        trim_start = start + TCT_START_SIGN_LEN;
        trim_end = end;
        while (IS_WHITESPACE(trim_start[0])) trim_start++;
        while (IS_WHITESPACE(trim_end[-1])) trim_end--;
        section_current->data = tct_get_valuen(argument, trim_start, trim_end - trim_start);
        section_current->length = strlen(section_current->data);
        result_len += section_current->length;
        section_current->next = calloc(1, sizeof(tct_section));
        section_current = section_current->next;

        template = end + TCT_END_SIGN_LEN;
    }
    section_current->data = template;
    section_current->length = strlen(template);

    result = malloc(result_len + 1);
    write = result;

    while (section_start) {
        tct_section *section = section_start;
        memcpy(write, section->data, section->length);
        write += section->length;
        section_start = section->next;
        free(section);
    }
    *write = 0;

    return result;
#undef IS_WHITESPACE
}

int main() {
    char *template = "Welcome to {{ project_name }}!\n"
        "{{ project_name }}({{ project_abbreviation }}) is a micro template "
        "engine for {{ language }}. It has C-style usage, fewer lines of code "
        "and just needs a standard library. The {{ project_name }} project is "
        "released under the terms of the {{ license }} license.\n";
    char *result;

    tct_arguments *args = NULL;
    tct_add_argument(args, "project_abbreviation", "%s", "TCT");
    tct_add_argument(args, "project_name", "%s", "Tiny C Template engine");
    tct_add_argument(args, "language", "%s", "C Language");
    tct_add_argument(args, "license", "%s", "MIT");

    result = tct_render(template, args);
    fputs(result, stdout);

    tct_free_argument(args);
    free(result);

    return 0;
}
