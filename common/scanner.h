#pragma once

#include <ctype.h>
#include <tree_sitter/parser.h>

enum TokenType {
    PITarget,
    PIContent,
    Comment,
    CharData,
    XmlModel,
    XmlStylesheet,
};

/// Advance the lexer to the next token
static inline void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

static inline bool is_valid_pi_char(int32_t chr) {
    return isalnum(chr) || chr == '_' || chr == ':' || chr == '.' ||
           chr == '-' || chr == L'·';
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
                            // scan for stylesheet/model and disallow that
                            if (valid_symbols[XmlModel]) {
                                const char *const word = "model";
                                int j = 0;
                                while (word[j] != '\0') {
                                    if (word[j] != lexer->lookahead) {
                                        break;
                                    }
                                    j++;
                                    advance(lexer);
                                }
                                if (word[j] == '\0') {
                                    return false;
                                }
                            }
                            if (valid_symbols[XmlStylesheet]) {
                                const char *const word = "stylesheet";
                                int j = 0;
                                while (word[j] != '\0') {
                                    if (word[j] != lexer->lookahead) {
                                        break;
                                    }
                                    j++;
                                    advance(lexer);
                                }
                                if (word[j] == '\0') {
                                    return false;
                                }
                            }
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
        lexer->result_symbol = PITarget;
        return true;
    }

    return false;
}

/// Scan for the content of a PI node
static bool scan_pi_content(TSLexer *lexer) {
    bool advanced_once = false;

    while (!lexer->eof(lexer) &&
           lexer->lookahead != '\n' &&
           lexer->lookahead != '?') {
        advanced_once = true;
        advance(lexer);
    }

    if (lexer->lookahead == '?') {
        lexer->mark_end(lexer);
        advance(lexer);
        if (lexer->lookahead == '>') {
            advance(lexer);
            return false;
        }
    }

    if (advanced_once) {
        lexer->mark_end(lexer);
        lexer->result_symbol = PIContent;
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
        lexer->result_symbol = Comment;
        return true;
    }

    return false;
}

/// Define the boilerplate functions of the scanner
/// @param name the name of the language
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
