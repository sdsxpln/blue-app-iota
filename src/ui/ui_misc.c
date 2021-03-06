#include "ui_misc.h"
#include <string.h>
#include "common.h"
#include "../iota/conversion.h"
#include "iota/addresses.h"

// go to state with menu index
void state_go(uint8_t state, uint8_t idx)
{
    ui_state.state = state;
    ui_state.menu_idx = idx;
}

void state_return(uint8_t state, uint8_t idx)
{
    state_go(state, idx);
}

void backup_state()
{
    ui_state.backup_state = ui_state.state;
    ui_state.backup_menu_idx = ui_state.menu_idx;
}

void restore_state()
{
    state_return(ui_state.backup_state, ui_state.backup_menu_idx);

    ui_state.backup_state = STATE_MENU_WELCOME;
    ui_state.backup_menu_idx = 0;
}

void abbreviate_addr(char *dest, const char *src, uint8_t len)
{
    // length 81 or 82 means full address with or without '\0'
    if (len != 81)
        return;

    // copy the abbreviated address over
    strncpy(dest, src, 4);
    strncpy(dest + 4, " ... ", 5);
    strncpy(dest + 9, src + 77, 4);
    dest[13] = '\0';
}

void ui_read_bundle(BUNDLE_CTX *bundle_ctx)
{
    const unsigned char *addr_bytes = bundle_get_address_bytes(bundle_ctx, 0);
    get_address_with_checksum(addr_bytes, ui_state.addr);

    ui_state.pay = bundle_ctx->payment;
    ui_state.bal = bundle_ctx->balance;
}

// len specifies max size of buffer
// if buffer doesn't fit whole int, returns null
void int_to_str(int64_t num, char *str, uint8_t len)
{
    // minimum buffer size of 2 (digit + \0)
    if (len < 2)
        return;

    int64_t i = 0;
    bool isNeg = false;

    // handle 0 first
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    if (num < 0) {
        isNeg = true;
        num = -num;
    }

    while (num != 0) {
        uint8_t rem = num % 10;
        str[i++] = rem + '0';
        num = num / 10;

        // ensure we have room for full int + null term
        if (i == len || (i == len - 1 && isNeg)) {
            str[0] = '\0';
            return;
        }
    }

    if (isNeg)
        str[i++] = '-';

    str[i--] = '\0';

    // reverse the string
    for (uint8_t j = 0; j < i; j++, i--) {
        char c = str[j];
        str[j] = str[i];
        str[i] = c;
    }
}

// write_display(&words, TYPE_STR, MID);
// write_display(&int_val, TYPE_INT, MID);
void write_display(void *o, uint8_t type, uint8_t pos)
{
    char *c_ptr = NULL;

    switch (pos) {
    case TOP_H:
        c_ptr = ui_text.half_top;
        break;
    case TOP:
        c_ptr = ui_text.top_str;
        break;
    case BOT:
        c_ptr = ui_text.bot_str;
        break;
    case BOT_H:
        c_ptr = ui_text.half_bot;
        break;
    case MID:
    default:
        c_ptr = ui_text.mid_str;
        break;
    }

    // NULL value sets line blank
    if (o == NULL) {
        c_ptr[0] = '\0';
        return;
    }

    // ledger does not support printing 64 bit ints
    // also does not support %i! - Use %d
    // use custom function to handle 64 bit ints
    if (type == TYPE_INT)
        int_to_str(*(int64_t *)o, c_ptr, 21);
    else if (type == TYPE_STR)
        snprintf(c_ptr, 21, "%s", (char *)o);
}


/* --------- STATE RELATED FUNCTIONS ----------- */
void clear_text()
{
    write_display(NULL, TYPE_STR, TOP_H);
    write_display(NULL, TYPE_STR, TOP);
    write_display(NULL, TYPE_STR, MID);
    write_display(NULL, TYPE_STR, BOT);
    write_display(NULL, TYPE_STR, BOT_H);
}

// Turns a single glyph on or off
void glyph_on(char *c)
{
    if (c != NULL)
        c[0] = '\0';
}

void glyph_off(char *c)
{
    if (c != NULL) {
        c[0] = '.';
        c[1] = '\0';
    }
}

void clear_glyphs()
{
    // turn off all glyphs
    glyph_off(ui_glyphs.glyph_bar_l);
    glyph_off(ui_glyphs.glyph_bar_r);
    glyph_off(ui_glyphs.glyph_cross);
    glyph_off(ui_glyphs.glyph_check);
    glyph_off(ui_glyphs.glyph_up);
    glyph_off(ui_glyphs.glyph_down);
    glyph_off(ui_glyphs.glyph_warn);
    glyph_off(ui_glyphs.glyph_load);
    glyph_off(ui_glyphs.glyph_dash);
}

void clear_display()
{
    clear_text();
    clear_glyphs();
}

// turns on 2 glyphs (often glyph on left + right)
void display_glyphs(char *c1, char *c2)
{
    clear_glyphs();

    // turn on ones we want
    glyph_on(c1);
    glyph_on(c2);
}

// combine glyphs with bars along top for confirm
void display_glyphs_confirm(char *c1, char *c2)
{
    clear_glyphs();

    // turn on ones we want
    glyph_on(ui_glyphs.glyph_bar_l);
    glyph_on(ui_glyphs.glyph_bar_r);
    glyph_on(c1);
    glyph_on(c2);
}


void write_text_array(char *array, uint8_t len)
{
    clear_display();
    clear_glyphs();

    if (ui_state.menu_idx > 0) {
        write_display(array + (21 * (ui_state.menu_idx - 1)), TYPE_STR, TOP_H);
        glyph_on(ui_glyphs.glyph_up);
    }

    write_display(array + (21 * ui_state.menu_idx), TYPE_STR, MID);

    if (ui_state.menu_idx < len - 1) {
        write_display(array + (21 * (ui_state.menu_idx + 1)), TYPE_STR, BOT_H);
        glyph_on(ui_glyphs.glyph_down);
    }
}

/* --------- FUNCTIONS FOR DISPLAYING BALANCE ----------- */
uint8_t get_num_digits(int64_t val)
{
    uint8_t i = 0;

    while (val > 0) {
        val /= 10;
        i++;
    }

    return i;
}

void str_add_units(char *str, uint8_t unit)
{
    char unit_str[] = " i\0\0 Ki\0 Mi\0 Gi\0 Ti\0";

    // if there isn't room for units don't write
    for (uint8_t i = 0; i < 17; i++) {
        if (str[i] == '\0') {
            strncpy(str + i, unit_str + (unit * 4), 4);
            return;
        }
    }
}

char *str_defn_to_ptr(uint8_t str_defn)
{
    char *str_ptr;

    switch (str_defn) {
    case TOP_H:
        str_ptr = ui_text.half_top;
        break;
    case TOP:
        str_ptr = ui_text.top_str;
        break;
    case BOT:
        str_ptr = ui_text.bot_str;
        break;
    case BOT_H:
        str_ptr = ui_text.half_bot;
        break;
    case MID:
    default:
        str_ptr = ui_text.mid_str;
        break;
    }

    return str_ptr;
}

bool char_is_num(char c)
{
    return c - '0' >= 0 && c - '0' <= 9;
}

void str_add_commas(char *str, uint8_t num_digits, bool full)
{
    // largest int that can fit with commas
    // and units at end. if bigger, don't write commas
    if (num_digits > 13)
        return;

    char tmp[21];
    memcpy(tmp, str, 21);

    // first place for a comma
    uint8_t comma_val = num_digits % 3, last_comma;

    if (comma_val == 0)
        comma_val = 3;

    if (full)
        last_comma = num_digits - 1;
    else
        last_comma = num_digits - 3;

    // i traces str, j traces tmp, k counts numbers
    for (int8_t i = 0, j = 0, k = 0; i < 20;) {
        if (!full) {
            // only store 2 decimal places for short amt
            if (j == num_digits - 1)
                j++;
            else if (j == num_digits - 3) {
                // stop recording comma's and instead put a period
                comma_val = 0;
                str[i++] = '.';
            }
        }

        // check if number and incr if so
        if (char_is_num(tmp[j]))
            k++;

        // copy over the character
        str[i++] = tmp[j++];

        // if we just copied the 3rd number, add a comma
        if (k == comma_val && j < last_comma) {
            str[i++] = ',';
            k = 0;
            comma_val = 3;
        }
    }

    str[20] = '\0';
}

// display's full amount in base iotas Ex. 3,040,981,551 i
void write_full_val(int64_t val, uint8_t str_defn, uint8_t num_digits)
{
    write_display(&val, TYPE_INT, str_defn);
    str_add_commas(str_defn_to_ptr(str_defn), num_digits, true);
    str_add_units(str_defn_to_ptr(str_defn), 0);
}

// displays brief amount with units Ex. 3.04 Gi
void write_readable_val(int64_t val, uint8_t str_defn, uint8_t num_digits)
{
    uint8_t base = MIN(((num_digits - 1) / 3), 4);

    int64_t new_val = val;

    for (uint8_t i = 0; i < base - 1; i++)
        new_val /= 1000;

    write_display(&new_val, TYPE_INT, str_defn);
    str_add_commas(str_defn_to_ptr(str_defn), num_digits - (3 * (base - 1)),
                   false);
    str_add_units(str_defn_to_ptr(str_defn), base);
}

// displays full/readable value based on the ui_state
bool display_value(int64_t val, uint8_t str_defn)
{
    uint8_t num_digits = get_num_digits(val);

    if (ui_state.display_full_value || num_digits <= 3)
        write_full_val(val, str_defn, num_digits);
    else
        write_readable_val(val, str_defn, num_digits);

    // return whether a shortened version is possible
    return num_digits > 3;
}

// swap between full value and readable value
void value_convert_readability()
{
    ui_state.display_full_value = !ui_state.display_full_value;
}


/* ----------- BUILDING MENU / TEXT ARRAY ------------- */
void get_init_menu(char *msg)
{
    memset(msg, '\0', MENU_INIT_LEN * 21);

    uint8_t i = 0;

    strcpy(msg + (i++ * 21), "WARNING!");
    strcpy(msg + (i++ * 21), "IOTA is not like");
    strcpy(msg + (i++ * 21), "other cryptos!");
    strcpy(msg + (i++ * 21), "Visit iota.org/sec");
    strcpy(msg + (i++ * 21), "to learn more.");
}

void get_welcome_menu(char *msg)
{
    memset(msg, '\0', MENU_WELCOME_LEN * 21);

    uint8_t i = 0;

    strcpy(msg + (i++ * 21), " Welcome to IOTA");
    strcpy(msg + (i++ * 21), "Advanced Mode");
    strcpy(msg + (i++ * 21), "Account Indexes");
    strcpy(msg + (i++ * 21), "Exit App");
}

void get_disp_idx_menu(char *msg)
{
    memset(msg, '\0', MENU_ACCOUNTS_LEN * 21);

    uint8_t i = 0;

    strcpy(msg + (i * 21), "[1]: ");
    snprintf(msg + (i++ * 21) + 5, 16, "%u", get_seed_idx(0));
    strcpy(msg + (i * 21), "[2]: ");
    snprintf(msg + (i++ * 21) + 5, 16, "%u", get_seed_idx(1));
    strcpy(msg + (i * 21), "[3]: ");
    snprintf(msg + (i++ * 21) + 5, 16, "%u", get_seed_idx(2));
    strcpy(msg + (i * 21), "[4]: ");
    snprintf(msg + (i++ * 21) + 5, 16, "%u", get_seed_idx(3));
    strcpy(msg + (i * 21), "[5]: ");
    snprintf(msg + (i++ * 21) + 5, 16, "%u", get_seed_idx(4));
    strcpy(msg + (i * 21), "Back");
}

void get_advanced_menu(char *msg)
{
    memset(msg, '\0', MENU_ADVANCED_LEN * 21);

    uint8_t i = 0;

    strcpy(msg + (i++ * 21), "Default");
    strcpy(msg + (i++ * 21), "Advanced");
}

void get_adv_warn_menu(char *msg)
{
    memset(msg, '\0', MENU_ADV_WARN_LEN * 21);

    uint8_t i = 0;

    strcpy(msg + (i++ * 21), "Are you sure?");
    strcpy(msg + (i++ * 21), "Yes");
    strcpy(msg + (i++ * 21), "No");
}

void get_address_menu(char *msg)
{
    // address is 81 characters long
    memset(msg, '\0', MENU_ADDR_LEN * 21);

    uint8_t i = 0, j = 0, c_cpy = 6;

    // 13 chunks of 6 characters
    for (; i < MENU_ADDR_LEN; i++) {
        strncpy(msg + (i * 21), ui_state.addr + (j++ * 6), c_cpy);
        msg[i * 21 + 6] = ' ';

        if (i == MENU_ADDR_LEN - 1)
            c_cpy = 3;

        strncpy(msg + (i * 21) + 7, ui_state.addr + (j++ * 6), c_cpy);
    }
}
