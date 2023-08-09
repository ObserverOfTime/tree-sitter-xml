#pragma once

#include <ctype.h>
#include <tree_sitter/parser.h>

enum TokenType {
    PI_TARGET,
    PI_CONTENT,
    COMMENT,
    CHAR_DATA,
    XML_MODEL,
    XML_STYLESHEET,
};

/// Advance the lexer to the next token
static inline void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

/// Check if the character is valid in PITarget
/// @private
static inline bool is_valid_pi_char(int32_t chr) {
    return isalnum(chr) || chr == '_' || chr == ':' || chr == '.' || chr == '-' || chr == L'·';
}

/// Check if the lexer matches the given word
/// @private
static inline bool check_word(TSLexer *lexer, const char *const word) {
    for (int j = 0; word[j] != '\0'; ++j) {
        if (word[j] != lexer->lookahead) return false;
        advance(lexer);
    }
    return true;
}

/// Scan for the target of a PI node
static bool scan_pi_target(TSLexer *lexer, const bool *valid_symbols) {
    bool advanced_once = false, found_x_first = false;

    if (isalpha(lexer->lookahead) || lexer->lookahead == '_') {
        if (lexer->lookahead == 'x' || lexer->lookahead == 'X') {
            found_x_first = true;
            lexer->mark_end(lexer);
        }
        advanced_once = true;
        advance(lexer);
    }

    if (advanced_once) {
        while (is_valid_pi_char(lexer->lookahead)) {
            if (found_x_first &&
                (lexer->lookahead == 'm' || lexer->lookahead == 'M')) {
                advance(lexer);
                if (lexer->lookahead == 'l' || lexer->lookahead == 'L') {
                    advance(lexer);
                    if (is_valid_pi_char(lexer->lookahead)) {
                        found_x_first = false;
                        bool last_char_hyphen = lexer->lookahead == '-';
                        advance(lexer);
                        if (last_char_hyphen) {
                            if (valid_symbols[XML_MODEL] && check_word(lexer, "model")) return false;
                            if (valid_symbols[XML_STYLESHEET] && check_word(lexer, "stylesheet")) return false;
                        }
                    } else {
                        return false;
                    }
                }
            }

            found_x_first = false;
            advance(lexer);
        }

        lexer->mark_end(lexer);
        lexer->result_symbol = PI_TARGET;
        return true;
    }

    return false;
}

/// Scan for the content of a PI node
static bool scan_pi_content(TSLexer *lexer) {
    while (!lexer->eof(lexer) && lexer->lookahead != '\n' && lexer->lookahead != '?') advance(lexer);

    if (lexer->lookahead != '?') return false;

    lexer->mark_end(lexer);
    advance(lexer);

    if (lexer->lookahead == '>') {
        advance(lexer);
        while (lexer->lookahead == ' ') advance(lexer);
        if (lexer->lookahead == '\n') {
            advance(lexer);
        } else {
            return false;
        }
        lexer->result_symbol = PI_CONTENT;
        return true;
    }

    return false;
}

/// Scan for a Comment node
static bool scan_comment(TSLexer *lexer) {
    if (lexer->lookahead != '<') return false;
    advance(lexer);

    if (lexer->lookahead != '!') return false;
    advance(lexer);

    if (lexer->lookahead != '-') return false;
    advance(lexer);

    if (lexer->lookahead != '-') return false;
    advance(lexer);

    while (!lexer->eof(lexer)) {
        if (lexer->lookahead == '-') {
            advance(lexer);
            if (lexer->lookahead == '-') {
                advance(lexer);
                break;
            }
        } else {
            advance(lexer);
        }
    }

    if (lexer->lookahead == '>') {
        advance(lexer);
        lexer->mark_end(lexer);
        lexer->result_symbol = COMMENT;
        return true;
    }

    return false;
}

/// Define the boilerplate functions of the scanner
#define SCANNER_BOILERPLATE(name) \
    void *tree_sitter_##name##_external_scanner_create() { return NULL; } \
    \
    void tree_sitter_##name##_external_scanner_destroy(void *payload) {} \
    \
    void tree_sitter_##name##_external_scanner_reset(void *payload) {} \
    \
    unsigned tree_sitter_##name##_external_scanner_serialize(void *payload, char *buffer) { return 0; } \
    \
    void tree_sitter_##name##_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {}
