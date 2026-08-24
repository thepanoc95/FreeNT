/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FreeNT FreeDLL
 * FILE:            freedll/src/crt_format.c
 * PURPOSE:         Tiny C Runtime - printf-style formatting
 * PROGRAMMER:      FreeNT Team
 */

#include "freedll.h"

/* Format flags */
#define FFLAG_ALT       0x0001
#define FFLAG_ZERO      0x0002
#define FFLAG_BLANK     0x0004
#define FFLAG_SIGN      0x0008
#define FFLAG_LEFT      0x0010
#define FFLAG_SPACE     0x0020
#define FFLAG_NEGATIVE  0x0040

/* Format state */
typedef struct {
    unsigned int flags;
    int width;
    int precision;
    int length_modifier;  /* 0=none, 1=h, 2=l, 3=ll, 4=L, 5=ll */
    char specifier;
} format_spec_t;

/* Output callback context */
typedef struct {
    char *buffer;
    size_t size;
    size_t pos;
} output_ctx_t;

static void output_char(output_ctx_t *ctx, char c)
{
    if (ctx->pos < ctx->size)
        ctx->buffer[ctx->pos++] = c;
}

/* Write a number in the given base */
enum {
    BASE_DECIMAL = 10,
    BASE_HEX_LOWER = 16,
    BASE_HEX_UPPER = 16,
    BASE_OCTAL = 8,
    BASE_UNSIGNED_DECIMAL = 10
};

/* Convert a number to string */
static void format_number(output_ctx_t *ctx, unsigned long long value, int base,
                          int is_upper, int width, int precision, int pad_char)
{
    char digits[32];
    int digit_count = 0;
    unsigned long long v = value;
    int i;
    const char *digits_lower = "0123456789abcdef";
    const char *digits_upper = "0123456789ABCDEF";

    if (v == 0 && precision == 0)
        return;

    /* Generate digits in reverse */
    do {
        digits[digit_count++] = is_upper ? digits_upper[v % base] : digits_lower[v % base];
        v /= base;
    } while (v > 0);

    /* Calculate padding */
    if (precision < 0)
        precision = 1;
    if (precision < digit_count)
        precision = digit_count;

    /* Zero pad or space pad */
    while (precision > digit_count) {
        output_char(ctx, '0');
        width--;
        precision--;
    }

    width -= digit_count;
    while (width-- > 0)
        output_char(ctx, (char)pad_char);

    /* Output digits in reverse (correct order) */
    for (i = digit_count - 1; i >= 0; i--)
        output_char(ctx, digits[i]);
}

static int parse_length_modifier(const char **fmt_ptr)
{
    char c = **fmt_ptr;
    if (c == 'h') {
        (*fmt_ptr)++;
        if (**fmt_ptr == 'h') {
            (*fmt_ptr)++;
            return 1; /* hh */
        }
        return 2; /* h */
    }
    if (c == 'l') {
        (*fmt_ptr)++;
        if (**fmt_ptr == 'l') {
            (*fmt_ptr)++;
            return 3; /* ll */
        }
        return 4; /* l */
    }
    if (c == 'L') {
        (*fmt_ptr)++;
        return 5; /* L */
    }
    if (c == 'I') {
        (*fmt_ptr)++;
        if (**fmt_ptr == '6' && *((*fmt_ptr) + 1) == '4') {
            (*fmt_ptr) += 2;
        } else if (**fmt_ptr == '3' && *((*fmt_ptr) + 1) == '2') {
            (*fmt_ptr) += 2;
        }
        return 4; /* I64 treated as long long */
    }
    return 0; /* no modifier */
}

static void format_argument(output_ctx_t *ctx, format_spec_t *spec, va_list *ap)
{
    char c;
    long long signed_val = 0;
    unsigned long long unsigned_val = 0;
    int pad_char = ' ';
    int width = spec->width;
    int precision = spec->precision;
    int is_zero_padded = (spec->flags & FFLAG_ZERO) && !(spec->flags & FFLAG_LEFT);
    int is_negative = 0;
    int value_is_negative = 0;
    char sign_char = 0;

    switch (spec->specifier) {
    case 'c':
        c = (char)va_arg(*ap, int);
        output_char(ctx, c);
        width--;
        while (width-- > 0 && !(spec->flags & FFLAG_LEFT))
            output_char(ctx, ' ');
        /* Left-justify padding is already handled below for consistency */
        break;

    case 's': {
        char *str = va_arg(*ap, char *);
        int len = 0;
        if (str == NULL)
            str = "(null)";
        while (str[len] != '\0')
            len++;
        if (precision >= 0 && len > precision)
            len = precision;
        width -= len;
        if (!(spec->flags & FFLAG_LEFT)) {
            while (width-- > 0)
                output_char(ctx, (char)pad_char);
        }
        for (int i = 0; i < len; i++)
            output_char(ctx, str[i]);
        if (spec->flags & FFLAG_LEFT) {
            while (width-- > 0)
                output_char(ctx, ' ');
        }
        break;
    }

    case 'd':
    case 'i':
        switch (spec->length_modifier) {
        case 1: signed_val = (signed char)va_arg(*ap, int); break;
        case 2: signed_val = (short)va_arg(*ap, int); break;
        case 3: signed_val = va_arg(*ap, long long); break;
        default: signed_val = va_arg(*ap, int); break;
        }
        value_is_negative = (signed_val < 0);
        is_negative = value_is_negative;
        unsigned_val = value_is_negative ? -(unsigned long long)signed_val : (unsigned long long)signed_val;
        if (is_negative) {
            sign_char = '-';
            width--;
        } else if (spec->flags & FFLAG_SIGN) {
            sign_char = '+';
            width--;
        } else if (spec->flags & FFLAG_BLANK || spec->flags & FFLAG_SPACE) {
            sign_char = ' ';
            width--;
        }
        /* Fall through to unsigned formatting */
        precision = (spec->precision < 0) ? 1 : spec->precision;
        if (is_zero_padded && precision == 1) {
            /* Zero padding: adjust for sign */
            if (spec->precision < 0) {
                precision = width;
                /* Don't output zero pad separately - let format_number handle it */
            }
        }
        if (sign_char)
            output_char(ctx, sign_char);
        if (is_zero_padded)
            pad_char = '0';
        width -= (spec->width - width); /* recalculate */
        format_number(ctx, unsigned_val, BASE_DECIMAL, 0, 0, precision, pad_char);
        break;

    case 'u':
        switch (spec->length_modifier) {
        case 1: unsigned_val = (unsigned char)va_arg(*ap, unsigned int); break;
        case 2: unsigned_val = (unsigned short)va_arg(*ap, unsigned int); break;
        case 3: unsigned_val = va_arg(*ap, unsigned long long); break;
        default: unsigned_val = va_arg(*ap, unsigned int); break;
        }
        if (spec->flags & FFLAG_SIGN || spec->flags & FFLAG_BLANK) {
            width--;
            if (unsigned_val <= 0x7FFFFFFFFFFFFFFFULL)
                sign_char = (spec->flags & FFLAG_SIGN) ? '+' : ' ';
        }
        if (sign_char)
            output_char(ctx, sign_char);
        {
            int dig_count = 0;
            unsigned long long tmp = unsigned_val;
            char digs[32];
            if (precision == 0)
                precision = 1;
            do {
                digs[dig_count++] = (char)('0' + (tmp % 10));
                tmp /= 10;
            } while (tmp > 0);
            if (spec->precision > dig_count) {
                int pad = spec->precision - dig_count;
                while (pad--) {
                    output_char(ctx, '0');
                    width--;
                }
            }
            width -= dig_count;
            while (width-- > 0 && !(spec->flags & FFLAG_LEFT))
                output_char(ctx, (char)pad_char);
            for (int i = dig_count - 1; i >= 0; i--)
                output_char(ctx, digs[i]);
            if (spec->flags & FFLAG_LEFT) {
                while (width-- > 0)
                    output_char(ctx, ' ');
            }
        }
        break;

    case 'x':
    case 'X':
        switch (spec->length_modifier) {
        case 1: unsigned_val = (unsigned char)va_arg(*ap, unsigned int); break;
        case 2: unsigned_val = (unsigned short)va_arg(*ap, unsigned int); break;
        case 3: unsigned_val = va_arg(*ap, unsigned long long); break;
        default: unsigned_val = va_arg(*ap, unsigned int); break;
        }
        if (spec->flags & FFLAG_ALT && unsigned_val != 0) {
            output_char(ctx, '0');
            output_char(ctx, (spec->specifier == 'x') ? 'x' : 'X');
            width -= 2;
        }
        pad_char = (is_zero_padded) ? '0' : ' ';
        if (spec->precision < 0)
            precision = 1;
        if (spec->precision > 0)
            is_zero_padded = 0; /* If precision is specified, no zero padding */
        width -= (spec->width - width);
        format_number(ctx, unsigned_val, BASE_HEX_LOWER,
                      (spec->specifier == 'X') ? 1 : 0,
                      width, precision, pad_char);
        break;

    case 'o':
        switch (spec->length_modifier) {
        case 1: unsigned_val = (unsigned char)va_arg(*ap, unsigned int); break;
        case 2: unsigned_val = (unsigned short)va_arg(*ap, unsigned int); break;
        case 3: unsigned_val = va_arg(*ap, unsigned long long); break;
        default: unsigned_val = va_arg(*ap, unsigned int); break;
        }
        if (spec->flags & FFLAG_ALT && unsigned_val != 0) {
            /* Octal prefix handling */
        }
        pad_char = (is_zero_padded) ? '0' : ' ';
        format_number(ctx, unsigned_val, BASE_OCTAL, 0, width, precision, pad_char);
        break;

    case 'p':
        unsigned_val = (unsigned long long)va_arg(*ap, void *);
        output_char(ctx, '0');
        output_char(ctx, 'x');
        format_number(ctx, unsigned_val, BASE_HEX_LOWER, 0, 0, -1, ' ');
        break;

    case '%':
        output_char(ctx, '%');
        break;

    default:
        output_char(ctx, '%');
        break;
    }
}

static void parse_format_specifier(const char **fmt, output_ctx_t *ctx, va_list *ap)
{
    format_spec_t spec;
    spec.flags = 0;
    spec.width = 0;
    spec.precision = -1;
    spec.length_modifier = 0;
    spec.specifier = '\0';

    /* Parse flags */
    while (1) {
        char c = **fmt;
        if (c == '#') spec.flags |= FFLAG_ALT;
        else if (c == '0') spec.flags |= FFLAG_ZERO;
        else if (c == '-') spec.flags |= FFLAG_LEFT;
        else if (c == ' ') spec.flags |= FFLAG_SPACE;
        else if (c == '+') spec.flags |= FFLAG_SIGN;
        else break;
        (*fmt)++;
    }

    /* Parse width */
    if (**fmt == '*') {
        spec.width = va_arg(*ap, int);
        if (spec.width < 0) {
            spec.width = -spec.width;
            spec.flags |= FFLAG_LEFT;
        }
        (*fmt)++;
    } else {
        while (**fmt >= '0' && **fmt <= '9') {
            spec.width = spec.width * 10 + (**fmt - '0');
            (*fmt)++;
        }
    }

    /* Parse precision */
    if (**fmt == '.') {
        (*fmt)++;
        spec.precision = 0;
        if (**fmt == '*') {
            spec.precision = va_arg(*ap, int);
            if (spec.precision < 0)
                spec.precision = -1;
            (*fmt)++;
        } else {
            while (**fmt >= '0' && **fmt <= '9') {
                spec.precision = spec.precision * 10 + (**fmt - '0');
                (*fmt)++;
            }
        }
    }

    /* Parse length modifier */
    spec.length_modifier = parse_length_modifier(fmt);

    /* Parse specifier */
    spec.specifier = **fmt;
    if (**fmt != '\0')
        (*fmt)++;

    format_argument(ctx, &spec, ap);
}

int freent_vsnprintf(char *buffer, size_t size, const char *format, va_list ap)
{
    output_ctx_t ctx;
    ctx.buffer = buffer;
    ctx.size = size;
    ctx.pos = 0;

    if (size == 0)
        return 0;

    const char *fmt = format;
    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0')
                break;
            parse_format_specifier(&fmt, &ctx, &ap);
        } else {
            output_char(&ctx, *fmt);
            fmt++;
        }
    }

    /* Null-terminate if there's room */
    if (ctx.pos < ctx.size)
        buffer[ctx.pos] = '\0';
    else if (size > 0)
        buffer[size - 1] = '\0';

    return (int)ctx.pos;
}

int freent_snprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = freent_vsnprintf(buffer, size, format, ap);
    va_end(ap);

    return result;
}
