/**
 * DOC: xf-escg.h
 * A set of terminal escape code graphics sequences.
 *
 * http://en.wikipedia.org/wiki/ANSI_escape_code#graphics
 *
 * DOC: Version
 * Header version is accessible via %_XF_ESCG_H where the three version
 * numbers are comma-separated.
 */
#ifndef _ESCG_H
#define _ESCG_H 00,02,11

/**
 * DOC: Style sequences
 * The macro defined styles are supposed to be used as ranges: each style
 * set, must also be ended (it is also possible to end all active
 * styles with %_lCLR).
 *
 * Forgetting to end ranges will make them leak over next lines and possibly
 * even next program output.
 *
 * DOC: Example usage
 * printf("The sky is "%lB_BLUE"blue"%_lB" and the sun "%lF_WHI"white"%_lF"\n");
 *
 * DOC: Text decorations
 * %lBLD_, %_lBLD - bold style
 *
 * %lULN_, %_lULN - underline text
 *
 * DOC: Foreground colours
 * %lF_BLA, %lF_RED, %lF_GRE, %lF_YELW, %lF_BLUE, %lF_MAG, %lF_CYA
 * and %lF_WHI.
 *
 * %_lF to end coloured text (sets default colour).
 *
 * DOC: Background colours
 * %lB_BLA, %lB_RED, %lB_GRE, %lB_YELW, %lB_BLUE, %lB_MAG, %lB_CYA
 * and %lB_WHI.
 *
 * %_lB to end coloured background (sets to default colour).
 *
 * DOC: Miscellaneous
 * %_lCLR - resets all style features to defaults
 */
#define _lCLR		"\x1b[0m" /* "\x1b" == "\e" == { 27, 0 } */
#define lBLD_		"\x1b[1m" /* lBLD instead of lBOL because L""'s bad */
#define _lBLD		"\x1b[21m"
#define lULN_		"\x1b[4m"
#define _lULN		"\x1b[24m"
/* colours */
#define lF_BLA	"\x1b[30m"
#define lF_RED	"\x1b[31m"
#define lF_GRE	"\e[32m"
#define lF_YELW	"\x1b[33m" /* YELW instead of YEL because L"" is bad */
#define lF_BLUE	"\x1b[34m" /* BLUE instead of BLU because U"" is bad */
#define lF_MAG	"\x1b[35m"
#define lF_CYA	"\x1b[36m"
#define lF_WHI	"\x1b[37m"
#define _lF	"\x1b[39m"

#define lB_BLA	"\x1b[40m"
#define lB_RED	"\x1b[41m"
#define lB_GRE	"\x1b[42m"
#define lB_YELW	"\x1b[43m"
#define lB_BLUE	"\x1b[44m"
#define lB_MAG	"\x1b[45m"
#define lB_CYA	"\x1b[46m"
#define lB_WHI	"\x1b[47m"
#define _lB	"\x1b[49m"

#endif
