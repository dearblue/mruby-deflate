#include <mruby.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/value.h>
#include <mruby/data.h>
#include <mruby/error.h>
#include <stdlib.h>
#include <libdeflate.h>
#include <mruby-aux.h>
#include <mruby-aux/scanhash.h>
#include <mruby-aux/string.h>


#define CLAMP(a, min, max)  ((a) < (min) ? (min) : ((a) < (max) ? (a) : (max)))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))

#define id_deflate  mrb_intern_cstr(mrb, "deflate")
#define id_zlib     mrb_intern_cstr(mrb, "zlib")
#define id_gzip     mrb_intern_cstr(mrb, "gzip")

enum {
    FORMAT_DEFLATE = 0,
    FORMAT_ZLIB = 1,
    FORMAT_GZIP = 2,
    AUX_DEFLATE_DEFAULT_FORMAT = FORMAT_DEFLATE,

    AUX_DEFLATE_FAST_COMPRESSION = 1,
    AUX_DEFLATE_DEFAULT_COMPRESSION = 6,
#if SUPPORT_NEAR_OPTIMAL_PARSING
    AUX_DEFLATE_BEST_COMPRESSION = 12,
#else
    AUX_DEFLATE_BEST_COMPRESSION = 9,
#endif
};


static const char *
aux_error_message_cstr(enum libdeflate_result res)
{
    switch (res) {
    case LIBDEFLATE_BAD_DATA:
        return "corrupted data or unsupported";
    case LIBDEFLATE_SHORT_OUTPUT:
        return "smaller than output capacity, but output size not given";
    case LIBDEFLATE_INSUFFICIENT_SPACE:
        return "output capacity was too small";
    default:
        return "unknown status";
    }
}

static VALUE
aux_error_message(MRB, enum libdeflate_result res)
{
    return mrb_str_new_cstr(mrb, aux_error_message_cstr(res));
}

static int
check_range_by_size_t(mrb_int n)
{
    if (n < 0 || n > MRBX_STR_MAX) {
        return 0;
    } else {
        return 1;
    }
}

static size_t
convert_to_size_t(MRB, VALUE v)
{
    if (NIL_P(v)) { return (size_t)-1; }

    mrb_int n = mrb_int(mrb, v);

    if (!check_range_by_size_t(n)) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
                   "wrong negative or huge number - %S (expect zero to %S)",
                   v, mrb_fixnum_value(MRBX_STR_MAX));
    }

    return (size_t)n;
}

static int
convert_to_level(MRB, VALUE level)
{
    if (NIL_P(level)) {
        return AUX_DEFLATE_DEFAULT_COMPRESSION;
    } else if (mrb_string_p(level) || mrb_symbol_p(level)) {
        const char *p = (mrb_string_p(level) ? mrb_str_to_cstr(mrb, level) : mrb_sym2name(mrb, mrb_symbol(level)));
        if (strcasecmp(p, "default") == 0) {
            return AUX_DEFLATE_DEFAULT_COMPRESSION;
        } else if (strcasecmp(p, "best") == 0 || strcasecmp(p, "max") == 0) {
            return AUX_DEFLATE_BEST_COMPRESSION;
        } else if (strcasecmp(p, "fast") == 0 || strcasecmp(p, "min") == 0) {
            return AUX_DEFLATE_FAST_COMPRESSION;
        }

        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                   "wrong level key - %S (expect default, fast, min, best or max)",
                   level);
    }

    {
        mrb_int n = mrb_int(mrb, level);

        return CLAMP(n, AUX_DEFLATE_FAST_COMPRESSION, AUX_DEFLATE_BEST_COMPRESSION);
    }
}

static int
convert_to_format(MRB, VALUE format, int nozlib)
{
    if (NIL_P(format)) {
        return AUX_DEFLATE_DEFAULT_FORMAT;
    } else if (mrb_string_p(format) || mrb_symbol_p(format)) {
        const char *p = mrbx_get_const_cstr(mrb, format);

        if (strcasecmp(p, "deflate") == 0) {
            return FORMAT_DEFLATE;
        } else if (strcasecmp(p, "gzip") == 0) {
            return FORMAT_GZIP;
        } else if (nozlib) {
            ;
        } else if (strcasecmp(p, "zlib") == 0) {
            return FORMAT_ZLIB;
        }
    }

    if (nozlib) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                   "wrong format key - %S (expect nil, deflate or gzip)",
                   format);
    } else {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                   "wrong format key - %S (expect nil, deflate, zlib or gzip)",
                   format);
    }

    /* NOTE: not reached here */
    return -1;
}


/*
 * class Deflate::Encoder
 */

typedef LIBDEFLATEAPI size_t (libdeflate_encode_f)(struct libdeflate_compressor *, const void *, size_t, void *, size_t);
typedef LIBDEFLATEAPI size_t (libdeflate_encode_bound_f)(struct libdeflate_compressor *, size_t);

static const struct {
    const char *name;
    libdeflate_encode_f *encode;
    libdeflate_encode_bound_f *bound;
} encode_traits[] = {
    { "deflate", libdeflate_deflate_compress, libdeflate_deflate_compress_bound },
    { "zlib", libdeflate_zlib_compress, libdeflate_zlib_compress_bound },
    { "gzip", libdeflate_gzip_compress, libdeflate_gzip_compress_bound },
};

static void
enc_s_encode_args(MRB, VALUE self, const char **srcp, mrb_int *srclen, mrb_int *maxdest, struct RString **dest, int *level, int *format)
{
    mrb_int argc;
    VALUE *argv;
    mrb_get_args(mrb, "s*", srcp, srclen, &argv, &argc);

    if (argc > 0 && mrb_hash_p(argv[argc - 1])) {
        argc --;

        VALUE level_v, format_v;

        MRBX_SCANHASH(mrb, argv[argc], Qnil,
                MRBX_SCANHASH_ARGS("level", &level_v, Qnil),
                MRBX_SCANHASH_ARGS("format", &format_v, Qnil));

        *level = convert_to_level(mrb, level_v);
        *format = convert_to_format(mrb, format_v, 0);
    } else {
        *level = AUX_DEFLATE_DEFAULT_COMPRESSION;
        *format = AUX_DEFLATE_DEFAULT_FORMAT;
    }

    switch (argc) {
    case 0:
        *maxdest = -1;
        *dest = NULL;
        break;
    case 1:
        if (mrb_string_p(argv[0])) {
            *maxdest = -1;
            *dest = RSTRING(argv[0]);
        } else {
            *maxdest = convert_to_size_t(mrb, argv[0]);
            *dest = NULL;
        }
        break;
    case 2:
        *maxdest = convert_to_size_t(mrb, argv[0]);
        *dest = RString(argv[1]);
        break;
    default:
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                   "wrong number of argument (given %S, expect 1..3 + keywords)",
                   mrb_fixnum_value(argc + 1));
    }

    if (*dest) {
        mrb_str_modify(mrb, *dest);
    }
}

struct enc_s_encode_args
{
    const char *src;
    mrb_int srclen, maxdest;
    struct RString *dest;
    int level, format;
    struct libdeflate_compressor *deflate;
    libdeflate_encode_f *encode;
    libdeflate_encode_bound_f *bound;
};

static VALUE
enc_s_encode_try(MRB, VALUE args)
{
    struct enc_s_encode_args *argp = mrb_cptr(args);

    if (argp->maxdest < 0) {
        size_t boundsize = argp->bound(argp->deflate, argp->srclen);
        argp->maxdest = MIN(boundsize, MRBX_STR_MAX);
    }

    argp->dest = mrbx_str_force_recycle(mrb, argp->dest, argp->maxdest);

    size_t destlen = argp->encode(argp->deflate, argp->src, argp->srclen, RSTR_PTR(argp->dest), argp->maxdest);

    if (destlen == 0) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
                   "failed libdeflate_%S_compress (small memory allocated?)",
                   VALUE(encode_traits[argp->format].name));
    }

    mrbx_str_set_len(mrb, argp->dest, destlen);

    return Qnil;
}

static VALUE
enc_s_encode_cleanup(MRB, VALUE args)
{
    struct enc_s_encode_args *argp = mrb_cptr(args);

    libdeflate_free_compressor(argp->deflate);

    return Qnil;
}

/*
 * call-seq:
 *  encode(src, maxdest = nil, dest = nil, level: nil, format: :deflate) -> dest
 *  encode(src, dest, level: nil, format: :deflate) -> dest
 */
static VALUE
enc_s_encode(MRB, VALUE self)
{
    struct enc_s_encode_args args = { 0 };

    enc_s_encode_args(mrb, self, &args.src, &args.srclen, &args.maxdest, &args.dest, &args.level, &args.format);

    args.encode = encode_traits[args.format].encode;
    args.bound = encode_traits[args.format].bound;

    args.deflate = libdeflate_alloc_compressor(args.level);

    if (!args.deflate) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
                   "failed libdeflate_alloc_compressor - wrong level (%S), or out of memory",
                   mrb_fixnum_value(args.level));
    }

    mrb_ensure(mrb,
               enc_s_encode_try, mrb_cptr_value(mrb, &args),
               enc_s_encode_cleanup, mrb_cptr_value(mrb, &args));

    return VALUE(args.dest);
}

static void
init_encoder(MRB, struct RClass *mDeflate)
{
    struct RClass *mEncoder = mrb_define_module_under(mrb, mDeflate, "Encoder");
    mrb_define_class_method(mrb, mEncoder, "encode", enc_s_encode, MRB_ARGS_ANY());
}

/*
 * class Deflate::Decoder
 */

typedef LIBDEFLATEAPI enum libdeflate_result (libdeflate_decode_f)(struct libdeflate_decompressor *, const void *, size_t, void *, size_t, size_t *);
typedef LIBDEFLATEAPI enum libdeflate_result (libdeflate_decode_ex_f)(struct libdeflate_decompressor *, const void *, size_t, void *, size_t, size_t *, size_t *);

static const struct {
    const char *name;
    libdeflate_decode_f *decode;
    libdeflate_decode_ex_f *decode_ex;
} decode_traits[] = {
    { "deflate", libdeflate_deflate_decompress, NULL },
    { "zlib",    libdeflate_zlib_decompress,    NULL },
    { "gzip",    libdeflate_gzip_decompress,    NULL },
};

static void
dec_s_decode_args(MRB, VALUE self, const char **src, mrb_int *srclen, mrb_int *maxdest, struct RString **dest, int *format)
{
    mrb_int argc;
    VALUE *argv;
    mrb_get_args(mrb, "si*", (char **)src, srclen, maxdest, &argv, &argc);

    if (argc > 0 && mrb_hash_p(argv[argc - 1])) {
        argc --;

        VALUE format_v;
        MRBX_SCANHASH(mrb, argv[argc], Qnil,
                MRBX_SCANHASH_ARGS("format", &format_v, Qnil));

        *format = convert_to_format(mrb, format_v, 0);
    } else {
        *format = AUX_DEFLATE_DEFAULT_FORMAT;
    }

    switch (argc) {
    case 0:
        *dest = NULL;
        break;
    case 1:
        *dest = RString(argv[0]);
        break;
    default:
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                   "wrong number of arguments (given %S, expect 2..3 + keywords)",
                   mrb_fixnum_value(argc + 2));
    }

    if (*maxdest < 0 || *maxdest > MRBX_STR_MAX) {
        mrb_raise(mrb, E_RUNTIME_ERROR,
                  "wrong negative or huge number for maxdest");
    }

    *dest = mrbx_str_force_recycle(mrb, *dest, *maxdest);
    RSTR_SET_LEN((*dest), 0);
}

/*
 * call-seq:
 *  decode(src, destcapa, dest = nil, format: :deflate)
 */
static VALUE
dec_s_decode(MRB, VALUE self)
{
    const char *src;
    mrb_int srclen, maxdest;
    struct RString *dest;
    int format;
    dec_s_decode_args(mrb, self, &src, &srclen, &maxdest, &dest, &format);

    libdeflate_decode_f *decode = decode_traits[format].decode;

    struct libdeflate_decompressor *d = libdeflate_alloc_decompressor();
    if (!d) {
        mrb_raise(mrb, E_RUNTIME_ERROR,
                  "failed libdeflate_alloc_decompressor (out of memory?)");
    }

    size_t destlen;
    enum libdeflate_result res = decode(d, src, srclen, RSTR_PTR(dest), maxdest, &destlen);

    libdeflate_free_decompressor(d);

    if (res != LIBDEFLATE_SUCCESS) {
        mrb_raisef(mrb, E_RUNTIME_ERROR,
                   "failed libdeflate_%S_decompress - %S (0x%S)",
                   VALUE(decode_traits[format].name),
                   aux_error_message(mrb, res),
                   mrbx_str_new_as_hexdigest(mrb, res, 4));
    }

    mrbx_str_set_len(mrb, dest, destlen);

    return VALUE(dest);
}

static void
init_decoder(MRB, struct RClass *mDeflate)
{
    struct RClass *mDecoder = mrb_define_module_under(mrb, mDeflate, "Decoder");
    mrb_define_class_method(mrb, mDecoder, "decode", dec_s_decode, MRB_ARGS_ANY());
}

/*
 * module Deflate
 */

void
mrb_mruby_deflate_gem_init(MRB)
{
    struct RClass *mDeflate = mrb_define_module(mrb, "Deflate");
    mrb_define_const(mrb, mDeflate, "DEFAULT_COMPRESSION", mrb_fixnum_value(AUX_DEFLATE_DEFAULT_COMPRESSION));
    mrb_define_const(mrb, mDeflate, "FAST_COMPRESSION", mrb_fixnum_value(AUX_DEFLATE_FAST_COMPRESSION));
    mrb_define_const(mrb, mDeflate, "BEST_COMPRESSION", mrb_fixnum_value(AUX_DEFLATE_BEST_COMPRESSION));

    struct RClass *mConstants = mrb_define_module_under(mrb, mDeflate, "Constants");
    mrb_define_const(mrb, mConstants, "DEFLATE_DEFAULT_COMPRESSION", mrb_fixnum_value(AUX_DEFLATE_DEFAULT_COMPRESSION));
    mrb_define_const(mrb, mConstants, "DEFLATE_FAST_COMPRESSION", mrb_fixnum_value(AUX_DEFLATE_FAST_COMPRESSION));
    mrb_define_const(mrb, mConstants, "DEFLATE_BEST_COMPRESSION", mrb_fixnum_value(AUX_DEFLATE_BEST_COMPRESSION));
    mrb_include_module(mrb, mDeflate, mConstants);

    init_encoder(mrb, mDeflate);
    init_decoder(mrb, mDeflate);
}

void
mrb_mruby_deflate_gem_final(MRB)
{
}
