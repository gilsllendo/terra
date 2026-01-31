#include "main.h"

char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);

        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);

        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);

        exit(74);
    }

    buffer[bytesRead] = '\0';
    fclose(file);

    return buffer;
}

int main(int argc, char **argv) {
    const char *filepath = NULL;
    PrintContext print = {0};

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--lexer-debug") == 0) {
            print.lexer_debug = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
        } else {
            filepath = argv[i];
        }
    }

    if (filepath == NULL) {
        fprintf(stderr, "Usage: terra [file] [options]\n");
        
        return 64;
    }

    char *source = read_file(filepath);

    VentContext vent;
    vent_context_init(&vent);

    TokenBuffer tokens;
    token_buffer_init(&tokens, &vent);

    Lexer lexer;
    lexer_init(&lexer, source, filepath, &tokens, &vent);
    lexer_run(&lexer);

    if (print.lexer_debug) {
        lexer_debug_print_tokens(&tokens, &print);
    }

    vent_flush(&vent);

    free(source);
    token_buffer_free(&tokens);
    vent_context_free(&vent);

    return vent.error_count ? 1 : 0;
}
