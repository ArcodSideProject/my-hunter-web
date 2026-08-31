/*
** text_input.c -- tiny reusable single-line text field for raylib.
** Not part of the original game; added for the scoreboard pseudo entry.
*/
#include "my.h"

// Appends typed characters into buf (capped at max_len, null-terminated)
// and handles backspace. Call once per frame while the field has focus.
void text_input_update(char *buf, int max_len)
{
    int c = GetCharPressed();
    while (c > 0) {
        int len = (int)strlen(buf);
        // Printable ASCII only, matching the server's accepted charset
        // (letters/digits/space/underscore/hyphen) closely enough --
        // the server re-validates regardless, this is just UX.
        if (c >= 32 && c < 127 && len < max_len) {
            buf[len] = (char)c;
            buf[len + 1] = '\0';
        }
        c = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        int len = (int)strlen(buf);
        if (len > 0) buf[len - 1] = '\0';
    }
}
