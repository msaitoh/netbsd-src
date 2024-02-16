/* $NetBSD: t_snprintb.c,v 1.16 2024/02/16 01:19:53 rillig Exp $ */

/*
 * Copyright (c) 2002, 2004, 2008, 2010, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Christos Zoulas and
 * Roland Illig.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2008, 2010\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_snprintb.c,v 1.16 2024/02/16 01:19:53 rillig Exp $");

#include <stdio.h>
#include <string.h>
#include <util.h>
#include <vis.h>

#include <atf-c.h>

static const char *
vis_arr(const char *arr, size_t arrsize)
{
	static char buf[6][1024];
	static size_t i;

	i = (i + 1) % (sizeof(buf) / sizeof(buf[0]));
	int rv = strnvisx(buf[i], sizeof(buf[i]), arr, arrsize,
	    VIS_WHITE | VIS_OCTAL);
	ATF_REQUIRE_MSG(rv >= 0, "strnvisx failed for size %zu", arrsize);
	return buf[i];
}

static void
check_unmodified_loc(const char *file, size_t line,
    const char *arr, size_t begin, size_t end)
{
	size_t mod_begin = begin, mod_end = end;
	while (mod_begin < mod_end && arr[mod_begin] == 'Z')
		mod_begin++;
	while (mod_begin < mod_end && arr[mod_end - 1] == 'Z')
		mod_end--;
	ATF_CHECK_MSG(
	    mod_begin == mod_end,
	    "failed:\n"
	    "\ttest case: %s:%zu\n"
	    "\tout-of-bounds write from %zu to %zu: %s\n",
	    file, line,
	    mod_begin, mod_end, vis_arr(arr + mod_begin, mod_end - mod_begin));
}

static void
h_snprintb_loc(const char *file, size_t line,
    size_t bufsize, const char *fmt, size_t fmtlen, uint64_t val,
    int exp_rv, const char *res, size_t reslen)
{
	char buf[1024];

	// Calling snprintb with bufsize == 0 invokes undefined
	// behavior due to out-of-range 'bp'.
	ATF_REQUIRE(bufsize > 0);
	ATF_REQUIRE(bufsize <= sizeof(buf));
	ATF_REQUIRE(reslen <= sizeof(buf));

	memset(buf, 'Z', sizeof(buf));
	int rv = snprintb(buf, bufsize, fmt, val);
	ATF_REQUIRE(rv >= 0);
	size_t rlen = rv;

	ATF_CHECK_MSG(
	    rv == exp_rv && memcmp(buf, res, reslen) == 0
	    && buf[rlen < bufsize ? rlen : bufsize - 1] == '\0',
	    "failed:\n"
	    "\ttest case: %s:%zu\n"
	    "\tformat: %s\n"
	    "\tvalue: %#jx\n"
	    "\twant: %d bytes %s\n"
	    "\thave: %d bytes %s\n",
	    file, line,
	    vis_arr(fmt, fmtlen),
	    (uintmax_t)val,
	    exp_rv, vis_arr(res, reslen),
	    rv, vis_arr(buf, reslen));
	check_unmodified_loc(file, line, buf, reslen, sizeof(buf));
}

#define	h_snprintb_len(bufsize, fmt, val, exp_rv, res)			\
	h_snprintb_loc(__FILE__, __LINE__,				\
	    bufsize, fmt, sizeof(fmt) - 1, val,				\
	    exp_rv, res, sizeof(res))
#define	h_snprintb(fmt, val, res)					\
	h_snprintb_len(1024, fmt, val, sizeof(res) - 1, res)

static void
h_snprintb_error_loc(const char *file, size_t line,
    const char *fmt, size_t fmtlen)
{
	char buf[1024];

	memset(buf, 'Z', sizeof(buf));
	int rv = snprintb(buf, sizeof(buf), fmt, 0);
	size_t buflen = rv;

	ATF_REQUIRE(rv >= -1);
	ATF_CHECK_MSG(rv == -1,
	    "expected error but got success:\n"
	    "\ttest case: %s:%zu\n"
	    "\tformat: %s\n"
	    "\tresult: %zu bytes %s\n",
	    file, line,
	    vis_arr(fmt, fmtlen),
	    buflen, vis_arr(buf, buflen));
}

#define	h_snprintb_error(fmt)						\
	h_snprintb_error_loc(__FILE__, __LINE__, fmt, sizeof(fmt) - 1)

ATF_TC(snprintb);
ATF_TC_HEAD(snprintb, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks snprintb(3)");
}
ATF_TC_BODY(snprintb, tc)
{

	// old-style format, octal
	h_snprintb(
	    "\010"
	    "\002BITTWO"
	    "\001BITONE",
	    3,
	    "03<BITTWO,BITONE>");

	// old-style format, decimal
	h_snprintb(
	    "\012"
	    "\0011"
	    "\0119"
	    "\02117"
	    "\04032",
	    0xffffffff,
	    "4294967295<1,9,17,32>");

	// old-style format, hexadecimal, from msb downto lsb
	h_snprintb(
	    "\020"
	    "\04032"
	    "\03024"
	    "\02016"
	    "\0108"
	    "\0077"
	    "\0066"
	    "\0055"
	    "\0044"
	    "\0033"
	    "\0022"
	    "\0011"
	    // The old-style format supports only 32 bits, interpreting the
	    // \041 as part of the text belonging to bit 1.
	    "\04133",
	    0x0000ffff00ff0f35,
	    "0xffff00ff0f35<24,6,5,3,1!33>");

	// old-style format, hexadecimal, from lsb to msb
	h_snprintb(
	    "\020"
	    "\0011"
	    "\0022"
	    "\0033"
	    "\0044"
	    "\0055"
	    "\0066"
	    "\0077"
	    "\0108"
	    "\02016"
	    "\03024"
	    "\04032"
	    // The old-style format supports only 32 bits, interpreting the
	    // \041 as part of the text belonging to bit 32.
	    "\04133",
	    0xffff0000ff00f0ca,
	    "0xffff0000ff00f0ca<2,4,7,8,16,32!33>");

	// The bits can be listed in arbitrary order, there can also be
	// duplicates. A bit's description can be empty, resulting in several
	// commas in a row.
	h_snprintb(
	    "\020"
	    "\001lsb"
	    "\040msb"
	    "\011"
	    "\012"
	    "\002above-lsb"
	    "\037below-msb"
	    "\001lsb-again"
	    "\040msb-again",
	    0xc0000303,
	    "0xc0000303<lsb,msb,,,above-lsb,below-msb,lsb-again,msb-again>");

#if 0
	// If the first bit number is 33 or more, snprintb invokes undefined
	// behavior due to an out-of-bounds bit shift, though undetected by
	// -ftrapv. Later bit numbers are properly checked.
	h_snprintb(
	    "\020"
	    "\177undefined_behavior"
	    "\001lsb",
	    0xffffffffffffffff,
	    "0xffffffffffffffff<?>");
#endif

	// old-style format, invalid number base 0
	h_snprintb_error(
	    "");

	// old-style format, invalid number base 2
	h_snprintb_error(
	    "\002");

	// old-style format, invalid number base 255 or -1
	h_snprintb_error(
	    "\377");

	// old-style format, small buffer
#if 0
	// FIXME: Calling snprintb with buffer size 0 invokes undefined
	// behavior due to out-of-bounds 'bp' pointer.
	h_snprintb_len(
	    0, "\020", 0,
	    1, "");
#endif
	h_snprintb_len(
	    1, "\020", 0,
	    1, "");
	h_snprintb_len(
	    2, "\020", 0,
	    1, "0");
	h_snprintb_len(
	    3, "\020", 0,
	    1, "0");
	h_snprintb_len(
	    3, "\020", 7,
	    3, "0x");
	h_snprintb_len(
	    4, "\020", 7,
	    3, "0x7");
	h_snprintb_len(
	    7, "\020\001lsb", 7,
	    8, "0x7<ls");
	h_snprintb_len(
	    8, "\020\001lsb", 7,
	    8, "0x7<lsb");
	h_snprintb_len(
	    9, "\020\001lsb", 7,
	    8, "0x7<lsb>");
	h_snprintb_len(
	    9, "\020\001one\002two", 7,
	    12, "0x7<one,");
	h_snprintb_len(
	    10, "\020\001one\002two", 7,
	    12, "0x7<one,t");
	h_snprintb_len(
	    12, "\020\001one\002two", 7,
	    12, "0x7<one,two");
	h_snprintb_len(
	    13, "\020\001one\002two", 7,
	    12, "0x7<one,two>");

	// new-style format, single bits, octal
	h_snprintb(
	    "\177\010"
	    "b\000bit0\0"
	    "b\037bit31\0"
	    "b\040bit32\0"
	    "b\077bit63\0",
	    0xf000000ff000000f,
	    "01700000000776000000017<bit0,bit31,bit32,bit63>");

	// new-style format, single bits, decimal
	h_snprintb(
	    "\177\012"
	    "b\000bit0\0"
	    "b\037bit31\0"
	    "b\040bit32\0"
	    "b\077bit63\0",
	    0xf000000ff000000f,
	    "17293822637553745935<bit0,bit31,bit32,bit63>");

	// new-style format, single bits, hexadecimal
	h_snprintb(
	    "\177\020"
	    "b\000bit0\0"
	    "b\037bit31\0"
	    "b\040bit32\0"
	    "b\077bit63\0",
	    0xf000000ff000000f,
	    "0xf000000ff000000f<bit0,bit31,bit32,bit63>");

	// new-style format, invalid number base 2
	h_snprintb_error(
	    "\177\002");

	// new-style format, invalid number base 255 or -1
	h_snprintb_error(
	    "\177\377");

	// new-style format, single bits, edge cases
	//
	// The bits can be listed in arbitrary order, there can also be
	// duplicates. A bit's description can be empty, resulting in several
	// commas in a row.
	h_snprintb(
	    "\177\020"
	    "b\01lsb\0"
	    "b\02\0"
	    "b\03\0"
	    "b\05NOTBOOT\0"
	    "b\06FPP\0"
	    "b\13SDVMA\0"
	    "b\15VIDEO\0"
	    "b\20LORES\0"
	    "b\21FPA\0"
	    "b\22DIAG\0"
	    "b\16CACHE\0"
	    "b\17IOCACHE\0"
	    "b\22LOOPBACK\0"
	    "b\04DBGCACHE\0",
	    0xe86f,
	    "0xe86f<lsb,,,NOTBOOT,FPP,SDVMA,VIDEO,CACHE,IOCACHE>");

	// new-style format, octal, named bit-field
	h_snprintb(
	    "\177\010"
	    "f\010\004Field\0"
		"=\001one\0"
		"=\002two\0",
	    0x100,
	    "0400<Field=01=one>");

	// new-style format, decimal, named bit-field
	h_snprintb(
	    "\177\012"
	    "f\010\004Field\0"
		"=\1one\0"
		"=\2two\0",
	    0x100,
	    "256<Field=1=one>");

	// new-style format, hexadecimal, named bit-field
	h_snprintb(
	    "\177\020"
	    "f\010\004Field\0"
		"=\1one\0"
		"=\2two\0",
	    0x100,
	    "0x100<Field=0x1=one>");

	// new-style format, octal, unnamed bit-field
	h_snprintb(
	    "\177\010"
	    "F\010\004Field\0"
		":\001one\0"
		":\002two\0",
	    0x100,
	    "0400<one>");

	// new-style format, decimal, unnamed bit-field
	h_snprintb(
	    "\177\012"
	    "F\010\004Field\0"
		":\1one\0"
		":\2two\0",
	    0x100,
	    "256<one>");

	// new-style format, hexadecimal, unnamed bit-field
	h_snprintb(
	    "\177\020"
	    "F\010\004Field\0"
		":\1one\0"
		":\2two\0",
	    0x100,
	    "0x100<one>");

	// new-style format, hexadecimal, named bit-field, edge cases
	//
	// Field values can be listed in arbitrary order, there can also be
	// duplicates. A field value's description can be empty, resulting in
	// several '=' in a row. The ':' directive can emulate the '='
	// directive, but not vice versa.
	h_snprintb(
	    "\177\20"
	    "f\0\4Field\0"
		"=\1one\0"
		"=\1one-again\0"
		"=\1\0"
		"=\1\0"
		":\1double\0"
		":\1-colon\0"
		":\1=equal\0"
		"=\2TWO\0",
	    1,
	    "0x1<Field=0x1=one=one-again==double-colon=equal>");

	// new-style format, hexadecimal, unnamed bit-field, edge cases
	//
	// Combining the 'F' and '=' directives generates output that doesn't
	// look well-formed.
	h_snprintb(
	    "\177\20"
	        "=\0all-zero\0"
	        "=\1all-one\0"
	        ":\1-continued\0"
	    "F\0\4Field\0"
		"=\1one\0"
		"=\1one-again\0"
		"=\1\0"
		"=\1\0"
		":\1double\0"
		":\1-colon\0"
		":\1=equal\0"
		"=\2TWO\0",
	    1,
	    "0x1=all-one-continued<=one=one-again==double-colon=equal>");

	// new-style format, bit-fields with fixed fallback value
	//
	// Only the first fallback value is used, all others are ignored.
	h_snprintb(
	    "\177\020"
	    "f\0\4Field\0"
		"=\1one\0"
		"=\2two\0"
		"*=other\0"
		"*=yet-another\0"
	    "b\1separator\0"
	    "F\0\4Field\0"
		":\1one\0"
		":\2two\0"
		"*other\0"
		"*yet-another\0",
	    3,
	    "0x3<Field=0x3=other,separator,other>");

	// new-style format, bit-fields with numeric fallback value
	h_snprintb(
	    "\177\020"
	    "f\010\004Field\0"
		"*=other(%04ju)\0"
	    "b\000separator\0"
	    "F\010\004Field\0"
		"*other(%04ju)\0",
	    0x301,
	    "0x301<Field=0x3=other(0003),separator,other(0003)>");

	// new-style format, bit-field with more than 8 bits
	//
	// The '=' and ':' directives can only match values from 0 to 255, so
	// the fallback value always steps in. The complete value of the
	// bit-field appears in the output, though.
	h_snprintb(
	    "\177\020"
	    "f\010\020Field\0"
		"=\377ones\0"
		"*=other(%jx)\0"
	    "F\010\020\0"
		":\377ones\0"
		"*other(%jx)\0",
	    0x77ff55,
	    "0x77ff55<Field=0x77ff=other(77ff),other(77ff)>");

	// new-style format, bit-fields with no match
	h_snprintb(
	    "\177\020"
	    "f\010\004Field\0"
		"=\1one\0"
		"=\2two\0",
	    0x301,
	    "0x301<Field=0x3>");
	h_snprintb(
	    "\177\020"
	    "F\010\004\0"
		":\1one\0"
		":\2two\0",
	    0x301,
	    "0x301<>");
	h_snprintb(
	    "\177\020"
	    "f\010\004Field\0"
		"=\1one\0"
		"=\2two\0"
	    "b\000separator\0"
	    "F\010\004\0"
		":\1one\0"
		":\2two\0",
	    0x301,
	    "0x301<Field=0x3,separator,>");

	// new-style format, two separate bit-fields
	h_snprintb(
	    "\177\20"
	    "f\0\4Field_1\0"
		"=\1ONE\0"
		"=\2TWO\0"
	    "f\4\4Field_2\0"
		"=\1ONE\0"
		"=\2TWO\0",
	    0x12,
	    "0x12<Field_1=0x2=TWO,Field_2=0x1=ONE>");

	// new-style format, mixed named and unnamed bit-fields
	h_snprintb(
	    "\177\20"
	    "f\0\4Field_1\0"
		"=\1ONE\0"
		"=\2TWO\0"
	    "F\x8\4\0"
		"*Field_3=%jd\0"
	    "f\4\4Field_2\0"
		":\1:ONE\0"
		":\2:TWO\0",
	    0xD12,
	    "0xd12<Field_1=0x2=TWO,Field_3=13,Field_2=0x1:ONE>");

	// new-style format, descriptions with spaces
	h_snprintb(
	    "\177\020"
	    "b\000has std options\0"
	    "f\010\004std options\0"
		"=\000no options\0"
		"=\017all options\0"
	    "F\020\004ext options\0"
		":\000no ext options\0"
		":\017all ext options\0",
	    0x000001,
	    "0x1<has std options,std options=0=no options,no ext options>");
	h_snprintb(
	    "\177\020"
	    "f\010\004std options\0"
		"*=other std options\0"
	    "F\020\004ext\toptions\0"
		"*other\text\toptions\0",
	    0x000001,
	    "0x1<std options=0=other std options,other\text\toptions>");

	// It is possible but cumbersome to implement a reduced variant of
	// rot13 using snprintb, shown here for lowercase letters only.
	for (char ch = 'A'; ch <= '~'; ch++) {
		char rot13 = ch >= 'a' && ch <= 'm' ? ch + 13
		    : ch >= 'n' && ch <= 'z' ? ch - 13
		    : '?';
		char expected[8];
		ATF_REQUIRE_EQ(7,
		    snprintf(expected, sizeof(expected), "%#x<%c>", ch, rot13));
		h_snprintb(
		    "\177\020"
		    "F\000\010\0"
		    ":an\0:bo\0:cp\0:dq\0:er\0:fs\0:gt\0:hu\0"
		    ":iv\0:jw\0:kx\0:ly\0:mz\0"
		    ":na\0:ob\0:pc\0:qd\0:re\0:sf\0:tg\0:uh\0"
		    ":vi\0:wj\0:xk\0:yl\0:zm\0"
		    // If snprintf accepted "%jc", it would be possible to
		    // echo the non-alphabetic characters instead of a
		    // catchall question mark.
		    "*?\0",
		    ch,
		    expected);
	}

	// new-style format, small buffer
#if 0
	// FIXME: Calling snprintb with buffer size 0 invokes undefined
	// behavior due to out-of-bounds 'bp' pointer.
	h_snprintb_len(
	    0, "\177\020", 0,
	    1, "");
#endif
	h_snprintb_len(
	    1, "\177\020", 0,
	    1, "");
	h_snprintb_len(
	    2, "\177\020", 0,
	    1, "0");
	h_snprintb_len(
	    3, "\177\020", 0,
	    1, "0");
	h_snprintb_len(
	    3, "\177\020", 7,
	    3, "0x");
	h_snprintb_len(
	    4, "\177\020", 7,
	    3, "0x7");
	h_snprintb_len(
	    7, "\177\020b\000lsb\0", 7,
	    8, "0x7<ls");
	h_snprintb_len(
	    8, "\177\020b\000lsb\0", 7,
	    8, "0x7<lsb");
	h_snprintb_len(
	    9, "\177\020b\000lsb\0", 7,
	    8, "0x7<lsb>");
	h_snprintb_len(
	    9, "\177\020b\000one\0b\001two\0", 7,
	    12, "0x7<one,");
	h_snprintb_len(
	    10, "\177\020b\000one\0b\001two\0", 7,
	    12, "0x7<one,t");
	h_snprintb_len(
	    12, "\177\020b\000one\0b\001two\0", 7,
	    12, "0x7<one,two");
	h_snprintb_len(
	    13, "\177\020b\000one\0b\001two\0", 7,
	    12, "0x7<one,two>");

}

static void
h_snprintb_m_loc(const char *file, size_t line,
    size_t bufsize, const char *fmt, size_t fmtlen, uint64_t val, size_t max,
    size_t exp_rv, const char *res, size_t reslen)
{
	char buf[1024];

	ATF_REQUIRE(bufsize > 1);
	ATF_REQUIRE(bufsize <= sizeof(buf));
	ATF_REQUIRE(reslen <= sizeof(buf));

	memset(buf, 'Z', sizeof(buf));
	int rv = snprintb_m(buf, bufsize, fmt, val, max);
	ATF_REQUIRE_MSG(rv >= 0,
	    "formatting %jx with '%s' returns error %d",
	    (uintmax_t)val, vis_arr(fmt, fmtlen), rv);

	size_t total = rv;
	ATF_CHECK_MSG(
	    total == exp_rv && memcmp(buf, res, reslen) == 0,
	    "failed:\n"
	    "\ttest case: %s:%zu\n"
	    "\tformat: %s\n"
	    "\tvalue: %#jx\n"
	    "\tmax: %zu\n"
	    "\twant: %zu bytes %s\n"
	    "\thave: %zu bytes %s\n",
	    file, line,
	    vis_arr(fmt, fmtlen),
	    (uintmax_t)val,
	    max,
	    exp_rv, vis_arr(res, reslen),
	    total, vis_arr(buf, reslen));
	check_unmodified_loc(file, line, buf, reslen, sizeof(buf));
}

#define	h_snprintb_m_len(bufsize, fmt, val, line_max, exp_rv, res)	\
	h_snprintb_m_loc(__FILE__, __LINE__,				\
	    bufsize, fmt, sizeof(fmt) - 1, val, line_max,		\
	    exp_rv, res, sizeof(res))
#define	h_snprintb_m(fmt, val, max, res)				\
	h_snprintb_m_len(1024, fmt, val, max, sizeof(res) - 1, res)

ATF_TC(snprintb_m);
ATF_TC_HEAD(snprintb_m, tc)
{
	atf_tc_set_md_var(tc, "descr", "Checks snprintb_m(3)");
}
ATF_TC_BODY(snprintb_m, tc)
{
	// old-style format, small maximum line length
	h_snprintb_m_len(
	    68,
	    "\020"
	    "\001bit1"
	    "\002bit2"
	    "\003bit3",
	    0xffff,
	    6,
	    143,
	    "0xffff>\0"
	    "0xffff<>\0"
	    "0xffffb>\0"
	    "0xffffi>\0"
	    "0xfffft>\0"
	    "0xffff1>\0"
	    "0xffff<>\0"
	    "0xff\0"
	);

	// new-style format, small maximum line length
	h_snprintb_m_len(
	    68,
	    "\177\020"
	    "b\000bit1\0"
	    "b\001bit2\0"
	    "b\002bit3\0",
	    0xffff,
	    6,
	    143,
	    "0xffff>\0"
	    "0xffff<>\0"
	    "0xffffb>\0"
	    "0xffffi>\0"
	    "0xfffft>\0"
	    "0xffff1>\0"
	    "0xffff<>\0"
	    "0xff\0"
	);

	// new-style format, buffer too small for number
	h_snprintb_m_len(
	    2,
	    "\177\020",
	    0,
	    64,
	    2,
	    "\0"
	);

	// new-style format, buffer too small for '<'
	h_snprintb_m_len(
	    6,
	    "\177\020"
	    "b\000lsb\0",
	    0xff,
	    64,
	    10,
	    "0xff\0"
	);

	// new-style format, buffer too small for description
	h_snprintb_m_len(
	    7,
	    "\177\020"
	    "b\000lsb\0",
	    0xff,
	    64,
	    10,
	    "0xff<\0"
	);

	// new-style format, buffer too small for complete description
	h_snprintb_m_len(
	    9,
	    "\177\020"
	    "b\000lsb\0",
	    0xff,
	    64,
	    10,
	    "0xff<ls\0"
	);

	// new-style format, buffer too small for '>'
	h_snprintb_m_len(
	    10,
	    "\177\020"
	    "b\000lsb\0",
	    0xff,
	    64,
	    10,
	    "0xff<lsb\0"
	);

	// new-style format, buffer too small for second line
	h_snprintb_m_len(
	    11,
	    "\177\020"
	    "b\000lsb\0"
	    "b\001two\0",
	    0xff,
	    11,
	    20,
	    "0xff<lsb>\0"
	);

	// new-style format, buffer too small for number in line 2
	h_snprintb_m_len(
	    12,
	    "\177\020"
	    "b\000lsb\0"
	    "b\001two\0",
	    0xff,
	    11,
	    20,
	    "0xff<lsb>\0"
	    "\0"
	);

	// new-style format, buffer too small for complete number in line 2
	h_snprintb_m_len(
	    15,
	    "\177\020"
	    "b\000lsb\0"
	    "b\001two\0",
	    0xff,
	    11,
	    20,
	    "0xff<lsb>\0"
	    "0xf\0"		// XXX: incomplete number may be misleading
	);

	// new-style format, buffer too small for '<' in line 2
	h_snprintb_m_len(
	    16,
	    "\177\020"
	    "b\000lsb\0"
	    "b\001two\0",
	    0xff,
	    11,
	    20,
	    "0xff<lsb>\0"
	    "0xff\0"
	);

	// new-style format, buffer too small for description in line 2
	h_snprintb_m_len(
	    17,
	    "\177\020"
	    "b\000lsb\0"
	    "b\001two\0",
	    0xff,
	    11,
	    20,
	    "0xff<lsb>\0"
	    "0xff<\0"
	);

	// new-style format, line too small for unmatched field value
	h_snprintb_m_len(
	    30,
	    "\177\020"
	    "f\000\004bits\0"
	        ":\000other\0",
	    0xff,
	    11,
	    22,
	    "0xff<bits=0xf>\0"	// XXX: line too long (14 > 11)
	    "0xff#>\0"		// XXX: why '#'? unbalanced '<>'
	);

	// new-style format, line too small for field value
	h_snprintb_m_len(
	    30,
	    "\177\020"
	    "f\000\004bits\0"
		":\017other\0",
	    0xff,
	    11,
	    27,
	    "0xff<bits=0xf>\0"	// XXX: line too long (14 > 11)
	    "0xff#other>\0"	// XXX: unbalanced '<>'
	);

	// new-style format, buffer too small for fallback
	h_snprintb_m_len(
	    20,
	    "\177\020"
	    "f\000\004bits\0"
		"*=fallback\0"
	    "b\0024\0",
	    0xff,
	    64,
	    26,
	    "0xff<bits=0xf=fall\0"
	);

	h_snprintb_m(
	    "\177\020"
	    "b\0LSB\0"
	    "b\1_BITONE\0"
	    "f\4\4NIBBLE2\0"
	    "f\x10\4BURST\0"
		"=\04FOUR\0"
		"=\17FIFTEEN\0"
	    "b\x1fMSB\0",
	    0x800f0701,
	    33,
	    "0x800f0701<LSB,NIBBLE2=0>\0"
	    "0x800f0701<BURST=0xf=FIFTEEN,MSB>\0"
	);

	h_snprintb_m(
	    "\177\020"
	    "b\0LSB\0"
	    "b\1_BITONE\0"
	    "f\4\4NIBBLE2\0"
	    "f\x10\4BURST\0"
		"=\04FOUR\0"
		"=\17FIFTEEN\0"
	    "b\x1fMSB\0",
	    0x800f0701,
	    32,
	    "0x800f0701<LSB,NIBBLE2=0>\0"
	    "0x800f0701<BURST=0xf=FIFTEEN>\0"
	    "0x800f0701<MSB>\0");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, snprintb);
	ATF_TP_ADD_TC(tp, snprintb_m);

	return atf_no_error();
}
