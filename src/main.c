#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INVALID_CLI_ARGS (1)
#define OPENING_FILE_ERROR (2)
#define DEREFERENCING_NULL_PTR (3)
#define MEMORY_ALLOCATION_ERROR (4)
#define INVALID_COMMENTS (5)
#define INVALID_INSTRUCTION (6)

#define ALPHABET_SIZE (26)  // variable names A..Z

typedef enum { NOT, AND, OR, XOR, IN, OUT } operation_t;

int parse_instructions_from_file(FILE *fin);
int execute_instruction(char *instruction, int variables[ALPHABET_SIZE]);

int main(int argc, char *argv[]) {
    FILE *fin = NULL;
    int err = 0;

    if (argc != 2) {  // ./main filename.txt
        fprintf(stderr, "Invalid CLI args\n");
        return INVALID_CLI_ARGS;
    }
    fin = fopen(argv[1], "r");
    if (fin == NULL) {
        fprintf(stderr, "error opening the file");
        return OPENING_FILE_ERROR;
    }
    err = parse_instructions_from_file(fin);
    switch (err) {
        case EXIT_SUCCESS:
            printf("Execution finished successfully\n");
            break;
        default:
            printf("Exit code: %d\n", err);
            break;
    }

    fclose(fin);

    return EXIT_SUCCESS;
}

int parse_instructions_from_file(FILE *fin) {
    char c = 0, prev = ' ',  // store previous letter for removing multi spaces
        *instruction;
    size_t instruction_len = 0,
           instruction_cap =
               0;  // store capacity for O(1) amort. push back to string
    int err = 0, variables[ALPHABET_SIZE] = {0};
    char *for_realloc = NULL;
    int comment_value = 0, i = 0;
    if (fin == NULL) {
        return DEREFERENCING_NULL_PTR;
    }

    instruction = malloc(sizeof(char) * 16);  // INITIAL_CAPACITY
    if (instruction == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }
    instruction_len = 0;
    instruction_cap = 16;

    while ((c = fgetc(fin)) != EOF) {  // reading from file without handling EOF
        if (c == ';') {                // border principle, invalid symbol
            instruction[instruction_len++] = ' ';
            instruction[instruction_len++] = '\0';
            err = execute_instruction(instruction, variables);
            if (err) {
                free(instruction);
                return err;
            }
            instruction_len = 0;

        } else {  // border principle, valid symbol, add it to string
            if (instruction_len - 2 ==
                instruction_cap) {  // -1 for adding ' ' and '\0' symbol in
                                    // other case without realloc

                for_realloc = (char *)realloc(
                    instruction, sizeof(char) * instruction_cap * 2);
                if (for_realloc == NULL) {
                    free(instruction);
                    return MEMORY_ALLOCATION_ERROR;
                }
                instruction = for_realloc;
                for_realloc = NULL;
                instruction_cap *= 2;
            }
            if (c == '[') {
                comment_value++;
                if (comment_value > 52) {
                    free(instruction);
                    return INVALID_COMMENTS;
                }
                prev = c;
                continue;
            } else if (c == ']') {
                comment_value--;
                if (comment_value < 0) {
                    free(instruction);
                    return INVALID_COMMENTS;
                }
                prev = c;
                continue;
            }
            // string push back
            if (!(isspace(c) && isspace(prev)) && (comment_value == 0) &&
                !(instruction_len == 0 && isspace(c))) {
                if (c == '#') {  // single line comment
                    while ((c = fgetc(fin)) && c != '\n' && c != EOF);
                    if (c == EOF) {
                        break;
                    }
                    prev = ' ';
                    continue;
                }
                instruction[instruction_len++] = toupper(c);
            }
        }
        prev = c;
    }

    if (instruction_len != 0) {
        for (i = 0; i < instruction_len; ++i) {
            if (!isspace(instruction[i])) {
                free(instruction);
                return INVALID_INSTRUCTION;
            }
        }
    }

    free(instruction);

    return EXIT_SUCCESS;
}

int execute_instruction(char *instruction, int variables[ALPHABET_SIZE]) {
    char c = 0, *token = NULL, *for_realloc = NULL, operand_1 = 0,
         operand_2 = 0;
    size_t token_len = 0, token_cap = 0;
    operation_t op;
    size_t token_count = 0;
    int i = 0, operand_2_value = 0, operand_1_value = 0;
    if (instruction == NULL || variables == NULL) {
        return DEREFERENCING_NULL_PTR;
    }

    token = (char *)malloc(sizeof(char) * 16);
    if (token == NULL) {
        return MEMORY_ALLOCATION_ERROR;
    }
    token_cap = 16;

    while (*instruction) {
        c = *instruction;
        if (isspace(c) || c == ',') {  // border principle... again
            if (token_len == 0) {      // to elliminate spaces
                instruction++;
                continue;
            }

            switch (token_count) {
                case 0:  // 1 token: operation
                    if (strncmp("IN", token, 2) == 0) {
                        op = IN;
                    } else if (strncmp("OUT", token, 3) == 0) {
                        op = OUT;
                    } else if (strncmp("OR", token, 2) == 0) {
                        op = OR;
                    } else if (strncmp("XOR", token, 3) == 0) {
                        op = XOR;
                    } else if (strncmp("AND", token, 3) == 0) {
                        op = AND;
                    } else if (strncmp("NOT", token, 3) == 0) {
                        op = NOT;
                    } else {
                        free(token);
                        return INVALID_INSTRUCTION;
                    }
                    break;
                case 1:  // 2 token: 1 operand
                    if (token_len !=
                        1) {  // 1 operand should be variable should
                        free(token);
                        return INVALID_INSTRUCTION;
                    }
                    operand_1 = token[0];
                    if (!isalpha(operand_1)) {
                        free(token);
                        return INVALID_INSTRUCTION;
                    }
                    break;
                case 2:  // 3 token: 2 operand
                    if (token_len > 1) {
                        for (i = 0; i < token_len; ++i) {
                            if (!isdigit(token[i])) {
                                free(token);
                                return INVALID_INSTRUCTION;
                            }
                            operand_2 = '\0';
                            operand_2_value =
                                52;  // TODO: parse via gorner scheme
                        }
                    } else {
                        operand_2 = token[0];
                        if (!isalpha(operand_2)) {
                            free(token);
                            return INVALID_INSTRUCTION;
                        }
                    }
                    break;
                default:
                    free(token);
                    return INVALID_INSTRUCTION;  // more than 3 tokens
            }
            token_count++;

            token_len = 0;

        } else {
            if (token_len - 1 == token_cap) {
                for_realloc =
                    (char *)realloc(token, sizeof(char) * token_cap * 2);
                if (for_realloc == NULL) {
                    free(token);
                    return MEMORY_ALLOCATION_ERROR;
                }
                token = for_realloc;
                for_realloc = NULL;
                token_cap *= 2;
            }
            token[token_len++] = c;
        }
        instruction++;
    }

    if ((token_count != 3 && op != NOT) || (token_count != 2 && op == NOT)) {
        free(token);
        return INVALID_INSTRUCTION;
    }

    operand_1_value = variables[operand_1 - 'A'];
    operand_2_value =
        operand_2 == '\0' ? operand_2_value : variables[operand_2 - 'A'];
    switch (op) {
        case NOT:
            operand_1_value = ~operand_1_value;
            variables[operand_1 - 'A'] = operand_1_value;
            break;
        case OR:
            operand_1_value = operand_1_value | operand_2_value;
            variables[operand_1 - 'A'] = operand_1_value;
            break;
        case XOR:
            operand_1_value = operand_1_value ^ operand_2_value;
            variables[operand_1 - 'A'] = operand_1_value;
            break;
        case AND:
            operand_1_value = operand_1_value & operand_2_value;
            variables[operand_1 - 'A'] = operand_1_value;
            break;
        case IN:
            printf("enter variable %c value: ", operand_1);
            scanf("%d",
                  &(variables[operand_1 - 'A']));  // TODO: add gorner scheme
            break;
        case OUT:
            printf("%c = %d\n", operand_1,
                   operand_1_value);  // TODO: add gorner scheme
            break;
        default:
            free(token);
            return INVALID_INSTRUCTION;
    }

    free(token);

    return EXIT_SUCCESS;
}
