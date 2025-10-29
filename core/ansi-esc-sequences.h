#ifndef _U_ANSI_ESC_SEQ_H
#define _U_ANSI_ESC_SEQ_H

/* single byte (special characters, not sequences) */
#define es_is_C0_control_code(c) (((c) >= 0x07 && (c) <= 0x0B && (c) != 0x0B) \
    || c == 0x1B)
#define es_BELL     "\07"
#define es_BACKSPACE "\08"
#define es_TAB      "\09"
#define es_LINEFEED "\10"
#define es_FORMFEED "\12"
#define es_CR       "\13"
#define es_ESC      "\033"
#define es_CARRIAGE_RETURN  es_CR
#define es_CLEAR_SCREEN es_FORMFEED

/* ... and their char variants */
#define es_BELL_chr     '\07'
#define es_BACKSPACE_chr '\08'
#define es_TAB_chr      '\09'
#define es_LINEFEED_chr '\n'
#define es_FORMFEED_chr '\12'
#define es_CR_chr       '\r'
#define es_ESC_chr      '\033'
#define es_CARRIAGE_RETURN_chr  es_CR_chr
#define es_CLEAR_SCREEN_chr es_FORMFEED_chr

/* FE escape sequences */
#define es_is_Fe_code(c) ((c) >= 0x40 && (c) <= 0x5F)
#define es_SS2      es_ESC "N"
#define es_SS3      es_ESC "O"
#define es_DCS      es_ESC "P"
#define es_CSI      es_ESC "["
#define es_STRSTOP  es_ESC "\\"
#define es_OS_CMD   es_ESC "]"
#define es_STRSTART es_ESC "X"
#define es_PRIVMSG  es_ESC "^"
#define es_APCMD    es_ESC "_"

/* CSI sequences */
#define es_is_CSI_parameter(c) ((c) >= 0x30 && (c) <= 0x3F)
#define es_is_CSI_intermediate(c) ((c) >= 0x20 && (c) <= 0x2F)
#define es_is_CSI_terminator(c) ((c) >= 0x40 && (c) <= 0x7E)

/* cursor related */
#define es_CUR_UP       es_CSI "A"
#define es_CUR_DOWN     es_CSI "B"
#define es_CUR_FORWARD  es_CSI "C"
#define es_CUR_BACK     es_CSI "D"
#define es_CUR_LEFT     es_CUR_BACK
#define es_CUR_RIGHT    es_CUR_FORWARD
#define es_CUR_NEXTLINE es_CSI "E"
#define es_CUR_PREVLINE es_CSI "F"
#define es_GOTO_COLUMN  es_CSI "G"
#define es_MOVE_CURSOR(es_CUR_DIRECTION, n) es_CSI #n #es_CUR_DIRECTION
#define es_CURSOR_SETPOS(x, y) es_CSI #x ";" #y "H"
#define es_ERASE_TO_BEGINNING 0
#define es_ERASE_TO_END 1
#define es_ERASE_ALL 2
#define es_ERASE_LINE(es_ERASE_TO) es_CSI #es_ERASE_TO "K"
#define es_CLEAR_LINE   es_CSI "2K"

/* SCR (Select Graphic Rendition) sequences */
#define es_COLOR_RESET      es_CSI "0m"
#define es_BOLD             es_CSI "1m"
#define es_DIM              es_CSI "2m"
#define es_ITALIC           es_CSI "3m"
#define es_UNDERLINE        es_CSI "4m"
#define es_BLINK_SLOW       es_CSI "5m"
#define es_BLINK_FAST       es_CSI "6m"
#define es_SWAP_BG_FG       es_CSI "7m"
#define es_HIDE             es_CSI "8m"
#define es_STRIKE           es_CSI "9m"
#define es_NOT_BOLD         es_CSI "21m"
#define es_RESET_INTENSITY  es_CSI "22m"
#define es_NOT_ITALIC       es_CSI "23m"
#define es_NOT_UNDERLINED   es_CSI "24m"
#define es_BLINK_OFF        es_CSI "25m"
#define es_NOT_SWAP_BG_FG   es_CSI "27m"
#define es_UNHIDE           es_CSI "28m"
#define es_NOT_STRIKE       es_CSI "29m"

/* FOREGROUND COLORS */
#define es_FG_BLACK            es_CSI "30m"
#define es_FG_RED              es_CSI "31m"
#define es_FG_GREEN            es_CSI "32m"
#define es_FG_YELLOW           es_CSI "33m"
#define es_FG_BLUE             es_CSI "34m"
#define es_FG_MAGENTA          es_CSI "35m"
#define es_FG_CYAN             es_CSI "36m"
#define es_FG_WHITE            es_CSI "37m"
#define es_FG_GRAY             es_CSI "90m"
#define es_FG_GREY             es_GRAY
#define es_FG_BRIGHT_BLACK     es_GRAY
#define es_FG_BRIGHT_RED       es_CSI "91m"
#define es_FG_BRIGHT_GREEN     es_CSI "92m"
#define es_FG_BRIGHT_YELLOW    es_CSI "93m"
#define es_FG_BRIGHT_BLUE      es_CSI "94m"
#define es_FG_BRIGHT_MAGENTA   es_CSI "95m"
#define es_FG_BRIGHT_CYAN      es_CSI "96m"
#define es_FG_DARK_GRAY        es_CSI "97m"
#define es_FG_DARK_GREY        es_DARK_GRAY
#define es_FG_BRIGHT_WHITE     es_DARK_GRAY /* ??? */
#define es_BLACK            es_CSI "30m"
#define es_RED              es_CSI "31m"
#define es_GREEN            es_CSI "32m"
#define es_YELLOW           es_CSI "33m"
#define es_BLUE             es_CSI "34m"
#define es_MAGENTA          es_CSI "35m"
#define es_CYAN             es_CSI "36m"
#define es_WHITE            es_CSI "37m"
#define es_GRAY             es_CSI "90m"
#define es_GREY             es_GRAY
#define es_BRIGHT_BLACK     es_GRAY
#define es_BRIGHT_RED       es_CSI "91m"
#define es_BRIGHT_GREEN     es_CSI "92m"
#define es_BRIGHT_YELLOW    es_CSI "93m"
#define es_BRIGHT_BLUE      es_CSI "94m"
#define es_BRIGHT_MAGENTA   es_CSI "95m"
#define es_BRIGHT_CYAN      es_CSI "96m"
#define es_DARK_GRAY        es_CSI "97m"
#define es_DARK_GREY        es_DARK_GRAY
#define es_BRIGHT_WHITE     es_DARK_GRAY /* ??? */
#define es_SET_TEXT_COLOR(r,g,b)   es_CSI "38;2;" #r ";" #g ";" #b "m"
#define es_SET_FG_COLOR es_SET_TEXT_COLOR

/* BACKGROUND COLORS */
#define es_BG_BLACK             es_CSI "30m"
#define es_BG_RED               es_CSI "31m"
#define es_BG_GREEN             es_CSI "32m"
#define es_BG_YELLOW            es_CSI "33m"
#define es_BG_BLUE              es_CSI "34m"
#define es_BG_MAGENTA           es_CSI "35m"
#define es_BG_CYAN              es_CSI "36m"
#define es_BG_WHITE             es_CSI "37m"
#define es_BG_GRAY              es_CSI "90m"
#define es_BG_GREY              es_GRAY
#define es_BG_BRIGHT_BLACK      es_GRAY
#define es_BG_BRIGHT_RED        es_CSI "91m"
#define es_BG_BRIGHT_GREEN      es_CSI "92m"
#define es_BG_BRIGHT_YELLOW     es_CSI "93m"
#define es_BG_BRIGHT_BLUE       es_CSI "94m"
#define es_BG_BRIGHT_MAGENTA    es_CSI "95m"
#define es_BG_BRIGHT_CYAN       es_CSI "96m"
#define es_BG_DARK_GRAY         es_CSI "97m"
#define es_BG_DARK_GREY         es_BG_DARK_GRAY
#define es_BG_BRIGHT_WHITE      es_BG_DARK_GRAY /* ??? */
#define es_SET_BG_COLOR(r,g,b)   es_CSI "48;2;" #r #g #b "m"

#endif /* _U_ANSI_ESC_SEQ_H */
