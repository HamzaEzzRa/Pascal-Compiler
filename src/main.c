#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "parser.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Error: No target file specified for the compiler\n");
        return EXIT_FAILURE;
    }

    char *file_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            if (file_path == NULL)
                file_path = argv[i];
            else
                printf("Warning: only one source file compilation is supported, compiling source file at first given path \"%s\"\n",
                    file_path);
        }
        else if (argv[i][1] != '\0') {
            if (argv[i][1] == 't') { // Add an option '-t' for listing tokens and token types (To-Do)
                
            }
            else if (argv[i][1] == 's' && argv[i][2] == 't') { // Add an option '-st' for listing symbols and their attributes (To-Do)

            }
            else {
                printf("Error: illegal parameter: %s\n", argv[i]);
                return EXIT_FAILURE;
            }
        }
        else {
            printf("Error: illegal parameter: %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    } 

    // scan_file() -> tests the lexical analyser only
    // parse_file() -> tests the lexical, syntax and semantic analysers

    parse_file(file_path);
    
    return EXIT_SUCCESS;
}
