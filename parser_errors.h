// parser_errors.h
#pragma once
#include <cstdlib>
#include <iostream>

#define RAISE_PRINT_EXIT(MSG)                                                  \
    do {                                                                       \
        std::cerr << RED "Shader Loader Error LINE NO:(" << ln_no              \
                  << "):" << char_no << " " << MSG << RESET;                   \
        std::exit(EXIT_FAILURE);                                               \
    } while (0)



#define RAISE_PRINT_EXIT_NO_LINE(MSG)                                        \
    do {                                                                     \
        std::cerr << MSG;                                                    \
        std::exit(EXIT_FAILURE);                                             \
    } while (0)

#define RAISE_EXPECTED_CHAR_BEFORE_NEWLINE(chr)                                \
    RAISE_PRINT_EXIT("Expected (" << (chr) << ") before newline")

#define RAISE_EXPECTED_TAG_OPEN(err_msg, c)                                    \
    RAISE_PRINT_EXIT(err_msg << " was expected before: " << (int)(c) << (c)    \
                               << "\n")

#define RAISE_NO_NEWLINE_INSIDE_TAGS                                           \
    RAISE_PRINT_EXIT("No newline character inside tags")

#define RAISE_SHADER_NAME_INVALID_START(c)                                     \
    RAISE_PRINT_EXIT("Shader name cannot start with special character or number: " << (c) << "\n")

#define RAISE_SHADER_NAME_HAS_WHITESPACE                                        \
    RAISE_PRINT_EXIT("Shader name has whitespace in between\n")

#define RAISE_SHADER_NAME_TOO_LONG(name_buf_sz, temp_name)                      \
    RAISE_PRINT_EXIT("Shader name exceeds the name buffer size"                \
                     << (name_buf_sz) << " name= " << (temp_name) << "\n")

#define RAISE_SHADER_NAME_INVALID_CHAR(c)                                      \
    RAISE_PRINT_EXIT("Shader has very special character in between: " << (c) << "\n")

#define RAISE_ELEMENT_TAG_MISMATCH(open_name, close_name, idx)                  \
    RAISE_PRINT_EXIT("Element(" << (idx) << "): open tag-" << (open_name)      \
                     << " does not match close tag-" << (close_name))

#define RAISE_SHADER_ALREADY_DEFINED(shdr_name, l1, l2)                              \
    RAISE_PRINT_EXIT("Shader: " << (shdr_name) << " Element: " << curnt_element->name \
                     << " is already defined line at (" << (l1) << ", "        \
                     << (l2) << ")")

#define RAISE_CLOSE_TAG_MISMATCH()                         \
    RAISE_PRINT_EXIT("Close tag subtag name:"                   \
                     << " does not match with open tag name"  << "\n")

#define RAISE_INSUFFICIENT_SPACE                                                \
    RAISE_PRINT_EXIT("INSUFFICIENT SPACE \n")

#define RAISE_FILE_NOT_FOUND                                                    \
    RAISE_PRINT_EXIT("File not found.")

#define RAISE_MISSING_SHADERS(handled_name, idx)                                \
    RAISE_PRINT_EXIT("Not all shaders are present. recent shader read was "     \
                     << (handled_name) << " -with shader index = " << (idx))

#define RAISE_UNKNOWN_ELEMENT(str)                                              \
    RAISE_PRINT_EXIT("'" << (str) << "' is not a Element of ShaderHandel\n")


#define RAISE_INVALID_TAG_NAME(name)                                         \
    RAISE_PRINT_EXIT_NO_LINE((name) << " is not a valid tag name.");

#define RAISE_INVALID_SUBTAG_NAME                                        \
    RAISE_PRINT_EXIT_NO_LINE((sbtg_name) << " is not a valid subtag name.");

#define RAISE_INACTIVE_SHADER_ACCESS(shdr, name)                                \
    RAISE_PRINT_EXIT_NO_LINE((shdr) << " is not a active shader of " << (name))

