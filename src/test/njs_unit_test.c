
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) NGINX, Inc.
 */


#include <njs_main.h>


#define NJS_HAVE_LARGE_STACK (!NJS_HAVE_ADDRESS_SANITIZER && !NJS_HAVE_MEMORY_SANITIZER)


typedef struct {
    njs_str_t  script;
    njs_str_t  ret;
} njs_unit_test_t;


static njs_unit_test_t  njs_test[] =
{
    { njs_str("@"),
      njs_str("SyntaxError: Unexpected token \"@\" in 1") },

    { njs_str("}"),
      njs_str("SyntaxError: Unexpected token \"}\" in 1") },

    { njs_str("1}"),
      njs_str("SyntaxError: Unexpected token \"}\" in 1") },

    { njs_str("/***/1/*\n**/"),
      njs_str("1") },

    /* Variable declarations. */

    { njs_str("var x"),
      njs_str("undefined") },

    { njs_str("var x;"),
      njs_str("undefined") },

    { njs_str("var x;;"),
      njs_str("undefined") },

    { njs_str("var x = 0"),
      njs_str("undefined") },

    { njs_str("var x = 0;"),
      njs_str("undefined") },

    { njs_str("var x = 0;;"),
      njs_str("undefined") },

    { njs_str("var; a"),
      njs_str("SyntaxError: Unexpected token \";\" in 1") },

    { njs_str("var + a"),
      njs_str("SyntaxError: Unexpected token \"+\" in 1") },

    { njs_str("//\r\n; var + a"),
      njs_str("SyntaxError: Unexpected token \"+\" in 2") },

    { njs_str("/*\n*/; var + a"),
      njs_str("SyntaxError: Unexpected token \"+\" in 2") },

    { njs_str("var \n a \n = 1; a"),
      njs_str("1") },

    { njs_str("var \n a, \n b; b"),
      njs_str("undefined") },

    { njs_str("var from = 2; from + 2"),
      njs_str("4") },

    { njs_str("var a / ="),
      njs_str("SyntaxError: Unexpected token \"/\" in 1") },

    { njs_str("var a = 1; var b; a"),
      njs_str("1") },

    { njs_str("a = 1;for(;a;a--)var a; a"),
      njs_str("0") },

    { njs_str("if(1)if(0){0?0:0}else\nvar a\nelse\nvar b"),
      njs_str("undefined") },

    { njs_str("var a = 1; var a; a"),
      njs_str("1") },

    { njs_str("(function (x) {if (x) { var a = 3; return a} else { var a = 4; return a}})(1)"),
      njs_str("3") },

    { njs_str("(function (x) {if (x) { var a = 3; return a} else { var a = 4; return a}})(0)"),
      njs_str("4") },

    { njs_str("function f(){return 2}; var f; f()"),
      njs_str("2") },

    { njs_str("function f(){return 2}; var f = 1; f()"),
      njs_str("TypeError: number is not a function") },

    { njs_str("function f(){return 1}; function f(){return 2}; f()"),
      njs_str("2") },

    { njs_str("var f = 1; function f() {}; f"),
      njs_str("1") },

#if 0 /* TODO */
    { njs_str("var a; Object.getOwnPropertyDescriptor(this, 'a').value"),
      njs_str("undefined") },
#endif

    { njs_str("f() = 1"),
      njs_str("ReferenceError: Invalid left-hand side in assignment in 1") },

    { njs_str("f.a() = 1"),
      njs_str("ReferenceError: Invalid left-hand side in assignment in 1") },

    { njs_str("++f()"),
      njs_str("ReferenceError: Invalid left-hand side in prefix operation in 1") },

    { njs_str("f()++"),
      njs_str("ReferenceError: Invalid left-hand side in postfix operation in 1") },

    /* Numbers. */

    { njs_str("0"),
      njs_str("0") },

    { njs_str("-0"),
      njs_str("-0") },

    { njs_str(".0"),
      njs_str("0") },

    { njs_str("0.1"),
      njs_str("0.1") },

    { njs_str(".9"),
      njs_str("0.9") },

    { njs_str("-.01"),
      njs_str("-0.01") },

    { njs_str("0.000001"),
      njs_str("0.000001") },

    { njs_str("0.00000123456"),
      njs_str("0.00000123456") },

    { njs_str("0.0000001"),
      njs_str("1e-7") },

    { njs_str("1.1000000"),
      njs_str("1.1") },

    { njs_str("99999999999999999999"),
      njs_str("100000000000000000000") },

    { njs_str("99999999999999999999.111"),
      njs_str("100000000000000000000") },

    { njs_str("999999999999999999999"),
      njs_str("1e+21") },

    { njs_str("9223372036854775808"),
      njs_str("9223372036854776000") },

    { njs_str("18446744073709551616"),
      njs_str("18446744073709552000") },

    { njs_str("1.7976931348623157E+308"),
      njs_str("1.7976931348623157e+308") },

    { njs_str("+1"),
      njs_str("1") },

    { njs_str("+1\n"),
      njs_str("1") },

    { njs_str("."),
      njs_str("SyntaxError: Unexpected token \".\" in 1") },

    /* Octal Numbers. */

    { njs_str("0o0"),
      njs_str("0") },

    { njs_str("0O10"),
      njs_str("8") },

    { njs_str("0o011"),
      njs_str("9") },

    { njs_str("-0O777"),
      njs_str("-511") },

    { njs_str("0o"),
      njs_str("SyntaxError: Unexpected token \"0o\" in 1") },

    { njs_str("0O778"),
      njs_str("SyntaxError: Unexpected token \"0O778\" in 1") },

    /* Legacy Octal Numbers are deprecated. */

    { njs_str("00"),
      njs_str("SyntaxError: Unexpected token \"00\" in 1") },

    { njs_str("08"),
      njs_str("SyntaxError: Unexpected token \"08\" in 1") },

    { njs_str("09"),
      njs_str("SyntaxError: Unexpected token \"09\" in 1") },

    { njs_str("0011"),
      njs_str("SyntaxError: Unexpected token \"00\" in 1") },

    /* Binary Numbers. */

    { njs_str("0b0"),
      njs_str("0") },

    { njs_str("0B10"),
      njs_str("2") },

    { njs_str("0b0101"),
      njs_str("5") },

    { njs_str("-0B11111111"),
      njs_str("-255") },

    { njs_str("0b"),
      njs_str("SyntaxError: Unexpected token \"0b\" in 1") },

    { njs_str("0B12"),
      njs_str("SyntaxError: Unexpected token \"0B12\" in 1") },

    /* Hex Numbers. */

    { njs_str("0x0"),
      njs_str("0") },

    { njs_str("-0x1"),
      njs_str("-1") },

    { njs_str("0xffFF"),
      njs_str("65535") },

    { njs_str("0X0000BEEF"),
      njs_str("48879") },

    { njs_str("0x"),
      njs_str("SyntaxError: Unexpected token \"0x\" in 1") },

    { njs_str("0xffff."),
      njs_str("SyntaxError: Unexpected token \"\" in 1") },

    { njs_str("0x12g"),
      njs_str("SyntaxError: Unexpected token \"g\" in 1") },

    { njs_str(""),
      njs_str("undefined") },

    { njs_str("\n"),
      njs_str("undefined") },

    { njs_str(";"),
      njs_str("undefined") },

    { njs_str("\n +1"),
      njs_str("1") },

    /* Scientific notation. */

    { njs_str("0e0"),
      njs_str("0") },

    { njs_str("0.0e0"),
      njs_str("0") },

    { njs_str("1e0"),
      njs_str("1") },

    { njs_str("1e1"),
      njs_str("10") },

    { njs_str("1.e01"),
      njs_str("10") },

    { njs_str("5.7e1"),
      njs_str("57") },

    { njs_str("5.7e-1"),
      njs_str("0.57") },

    { njs_str("-5.7e-1"),
      njs_str("-0.57") },

    { njs_str("1.1e-01"),
      njs_str("0.11") },

    { njs_str("5.7e-2"),
      njs_str("0.057") },

    { njs_str("1.1e+01"),
      njs_str("11") },

    { njs_str("-.01e-01"),
      njs_str("-0.001") },

    { njs_str("1e9"),
      njs_str("1000000000") },

    { njs_str("1.0e308"),
      njs_str("1e+308") },

    { njs_str("0e309"),
      njs_str("0") },

    { njs_str("0e-309"),
      njs_str("0") },

    { njs_str("1e309"),
      njs_str("Infinity") },

    { njs_str("-1e309"),
      njs_str("-Infinity") },

    { njs_str("1e"),
      njs_str("SyntaxError: Unexpected token \"e\" in 1") },

    { njs_str("1.e"),
      njs_str("SyntaxError: Unexpected token \"e\" in 1") },

    { njs_str("1e+"),
      njs_str("SyntaxError: Unexpected token \"e\" in 1") },

    { njs_str("1.e-"),
      njs_str("SyntaxError: Unexpected token \"e\" in 1") },

    { njs_str("1eZ"),
      njs_str("SyntaxError: Unexpected token \"eZ\" in 1") },

    { njs_str(".e1"),
      njs_str("SyntaxError: Unexpected token \".\" in 1") },

    { njs_str("Number.prototype.X = function(){return 123;};"
                 "(1).X()"),
      njs_str("123") },

    /* Indexes. */

    { njs_str("var a = []; a[-1] = 2; a[-1] == a['-1']"),
      njs_str("true") },

    { njs_str("var a = []; a[Infinity] = 2; a[Infinity] == a['Infinity']"),
      njs_str("true") },

    { njs_str("var a = []; a[NaN] = 2; a[NaN] == a['NaN']"),
      njs_str("true") },

    /* Number.toString(radix) method. */

    { njs_str("0..toString(2)"),
      njs_str("0") },

    { njs_str("(1234.567).toString(3)"),
      njs_str("1200201.120022100021001021021002202") },

    { njs_str("(1234.567).toString(5)"),
      njs_str("14414.240414141414141414") },

    { njs_str("(1234.567).toString(17)"),
      njs_str("44a.9aeb6faa0da") },

    { njs_str("(1234.567).toString(36)"),
      njs_str("ya.kety9sifl") },

    { njs_str("Number(-1.1).toString(36)"),
      njs_str("-1.3llllllllm") },

    { njs_str("Math.pow(-2, 1023).toString(2).length"),
      njs_str("1025") },

    { njs_str("8.0625.toString(2)"),
      njs_str("1000.0001") },

    { njs_str("(1/3).toString(2)"),
      njs_str("0.010101010101010101010101010101010101010101010101010101") },

    { njs_str("9999..toString(3)"),
      njs_str("111201100") },

    { njs_str("-9999..toString(3)"),
      njs_str("-111201100") },

    { njs_str("81985529216486895..toString(16)"),
      njs_str("123456789abcdf0") },

    { njs_str("0xffff.toString(16)"),
      njs_str("ffff") },

    { njs_str("30520..toString(36)"),
      njs_str("njs") },

    { njs_str("Infinity.toString()"),
      njs_str("Infinity") },

    { njs_str("Infinity.toString(2)"),
      njs_str("Infinity") },

    { njs_str("Infinity.toString(10)"),
      njs_str("Infinity") },

    { njs_str("Infinity.toString(NaN)"),
      njs_str("RangeError") },

    { njs_str("Infinity.toString({})"),
      njs_str("RangeError") },

    { njs_str("Infinity.toString(Infinity)"),
      njs_str("RangeError") },

    { njs_str("NaN.toString()"),
      njs_str("NaN") },

    { njs_str("NaN.toString(2)"),
      njs_str("NaN") },

    { njs_str("NaN.toString(10)"),
      njs_str("NaN") },

    { njs_str("NaN.toString(Infinity)"),
      njs_str("RangeError") },

    { njs_str("NaN.toString({})"),
      njs_str("RangeError") },

    { njs_str("NaN.toString(NaN)"),
      njs_str("RangeError") },

    { njs_str("1.2312313132.toString(14)"),
      njs_str("1.3346da6d5d455c") },

    { njs_str("7.799999999999999.toString(14)"),
      njs_str("7.b2b2b2b2b2b2a5") },

#ifndef NJS_SUNC
    { njs_str("1e20.toString(14)"),
      njs_str("33cb3bb449c2a92000") },

    { njs_str("1.7976931348623157E+308.toString(36) == ('1a1e4vngaiqo' + '0'.repeat(187))"),
      njs_str("true") },

    /* Largest positive double (prev_double(INFINITY)). */
    { njs_str("1.7976931348623157E+308.toString(2) == ('1'.repeat(53) + '0'.repeat(971))"),
      njs_str("true") },

    { njs_str("Array(5).fill().map((n, i) => i + 10).map((v)=>(1.2312313132).toString(v))"),
      njs_str("1.2312313132,1.25a850416057383,1.293699002749414,1.3010274cab0288,1.3346da6d5d455c") },

    { njs_str("Array(5).fill().map((n, i) => 36 - i).map((v)=>(1e23).toString(v))"),
      njs_str("ga894a06abs0000,o5hlsorok4y0000,128fpsprqld20000,1m1s0ajv6cmo0000,2kmg5hv19br00000") },
#endif

    /* Number.prototype.toFixed(frac) method. */

    { njs_str("(900.1).toFixed(1)"),
      njs_str("900.1") },

    { njs_str("(0).toFixed(0)"),
      njs_str("0") },

    { njs_str("(0).toFixed(3)"),
      njs_str("0.000") },

    { njs_str("(7).toFixed()"),
      njs_str("7") },

    { njs_str("(7).toFixed(0)"),
      njs_str("7") },

    { njs_str("(7).toFixed(2)"),
      njs_str("7.00") },

    { njs_str("(-900.1).toFixed(3.3)"),
      njs_str("-900.100") },

    { njs_str("(900.123).toFixed(5)"),
      njs_str("900.12300") },

    { njs_str("(1/3).toFixed(5)"),
      njs_str("0.33333") },

    { njs_str("(new Number(1/3)).toFixed(5)"),
      njs_str("0.33333") },

    { njs_str("(new Number(1/3)).toFixed(5)"),
      njs_str("0.33333") },

    { njs_str("(1/3).toFixed({toString(){return '5'}})"),
      njs_str("0.33333") },

    { njs_str("(1/3).toFixed(100)"),
      njs_str("0.3333333333333333148296162562473909929394721984863281250000000000000000000000000000000000000000000000") },

    { njs_str("(1.23e+20).toFixed(2)"),
      njs_str("123000000000000000000.00") },

    { njs_str("(1.23e-10).toFixed(2)"),
      njs_str("0.00") },

    { njs_str("(1.23e-10).toFixed(15)"),
      njs_str("0.000000000123000") },

    { njs_str("(1.23e-10).toFixed(100)"),
      njs_str("0.0000000001229999999999999888422768137255427361997917046210204716771841049194335937500000000000000000") },

    { njs_str("NaN.toFixed(1)"),
      njs_str("NaN") },

#if 0  /* FIXME: bignum support is requred to support frac >= 20 */
    { njs_str("(2 ** -100).toFixed(100)"),
      njs_str("0.0000000000000000000000000000007888609052210118054117285652827862296732064351090230047702789306640625") },
#endif

    /* Number.prototype.toPrecision(prec) method. */

    { njs_str("Array(4).fill().map((n, i) => i+1).map((v)=>(1/7).toPrecision(v))"),
      njs_str("0.1,0.14,0.143,0.1429") },

    { njs_str("Array(4).fill().map((n, i) => i+1).map((v)=>(0).toPrecision(v))"),
      njs_str("0,0.0,0.00,0.000") },

    { njs_str("Array(4).fill().map((n, i) => i+1).map((v)=>(1/2).toPrecision(v))"),
      njs_str("0.5,0.50,0.500,0.5000") },

    { njs_str("Array(6).fill().map((n, i) => i+2).map((v)=>(1/v).toPrecision(5))"),
      njs_str("0.50000,0.33333,0.25000,0.20000,0.16667,0.14286") },

    { njs_str("Array(6).fill().map((n, i) => i+2).map((v)=>(1/(v*100)).toPrecision(5))"),
      njs_str("0.0050000,0.0033333,0.0025000,0.0020000,0.0016667,0.0014286") },

    { njs_str("Array(6).fill().map((n, i) => i+1).map((v)=>(10*v/7).toPrecision(5))"),
      njs_str("1.4286,2.8571,4.2857,5.7143,7.1429,8.5714") },

    { njs_str("Array(6).fill().map((n, i) => i+1).map((v)=>(v/3).toPrecision(5))"),
      njs_str("0.33333,0.66667,1.0000,1.3333,1.6667,2.0000") },

    { njs_str("Array(6).fill().map((n, i) => i+1).map((v)=>((Math.pow(-1,v))*(2*v)/3).toPrecision(5))"),
      njs_str("-0.66667,1.3333,-2.0000,2.6667,-3.3333,4.0000") },

    { njs_str("Array(12).fill().map((n, i) => i-3).map((v)=>(2**v).toPrecision(6))"),
      njs_str("0.125000,0.250000,0.500000,1.00000,2.00000,4.00000,8.00000,16.0000,32.0000,64.0000,128.000,256.000") },

    { njs_str("Array(5).fill().map((n, i) => i+16).map((v)=>(4.1).toPrecision(v))"),
      njs_str("4.100000000000000,4.0999999999999996,4.09999999999999964,4.099999999999999644,4.0999999999999996447") },

    { njs_str("Array(3).fill().map((n, i) => i + 19).map((v)=>(2**(-v)).toPrecision(20))"),
      njs_str("0.0000019073486328125000000,9.5367431640625000000e-7,4.7683715820312500000e-7") },

    { njs_str("Array(3).fill().map((n, i) => i + 32).map((v)=>(2**(v)+0.1).toPrecision(10))"),
      njs_str("4294967296,8589934592,1.717986918e+10") },

#if 0  /* FIXME: bignum support is requred to support prec >= 20 */
    { njs_str("(1/7).toPrecision(100)"),
      njs_str("0.1428571428571428492126926812488818541169166564941406250000000000000000000000000000000000000000000000") },

    { njs_str("(2**128).toPrecision(40)"),
      njs_str("340282366920938463463374607431768211456.0") },
#endif

    { njs_str("(2**128).toPrecision(1)"),
      njs_str("3e+38") },

    { njs_str("(2**128).toPrecision(2)"),
      njs_str("3.4e+38") },

    { njs_str("(2**128).toPrecision(40)"),
      njs_str("340282366920938463490000000000000000000.0") },

    { njs_str("(123).toPrecision(0)"),
      njs_str("RangeError: precision argument must be between 1 and 100") },

    { njs_str("(123).toPrecision(2.4)"),
      njs_str("1.2e+2") },

    { njs_str("(123).toPrecision(101)"),
      njs_str("RangeError: precision argument must be between 1 and 100") },

    { njs_str("(2**10000).toPrecision()"),
      njs_str("Infinity") },

    { njs_str("(-(2**10000)).toPrecision()"),
      njs_str("-Infinity") },

    { njs_str("(-0).toPrecision(2)"),
      njs_str("0.0") },

    { njs_str("NaN.toPrecision()"),
      njs_str("NaN") },

    { njs_str("NaN.toPrecision(0)"),
      njs_str("NaN") },

    { njs_str("(10**22).toPrecision()"),
      njs_str("1e+22") },

    { njs_str("Number.prototype.toPrecision.call('12')"),
      njs_str("TypeError: unexpected value type:string") },

    { njs_str("(1000000000000000128).toString()"),
      njs_str("1000000000000000100") },

    { njs_str("(1000000000000000128).toFixed(0)"),
      njs_str("1000000000000000128") },

    { njs_str("(1e21).toFixed(1)"),
      njs_str("1e+21") },

    { njs_str("Number.prototype.toFixed.call({})"),
      njs_str("TypeError: unexpected value type:object") },

    { njs_str("(0).toFixed(-1)"),
      njs_str("RangeError: digits argument must be between 0 and 100") },

    { njs_str("(0).toFixed(101)"),
      njs_str("RangeError: digits argument must be between 0 and 100") },

    /* Number.prototype.toExponential(frac) method. */

    { njs_str("Array(3).fill().map((n, i) => i + 1).map((v)=>(0).toExponential(v))"),
      njs_str("0.0e+0,0.00e+0,0.000e+0") },

    { njs_str("Array(6).fill().map((n, i) => i + 1).map((v)=>((Math.pow(-1,v))*(2*v)/3).toExponential(5))"),
      njs_str("-6.66667e-1,1.33333e+0,-2.00000e+0,2.66667e+0,-3.33333e+0,4.00000e+0") },

    { njs_str("Array(5).fill().map((n, i) => i + 1).map((v)=>((Math.pow(-1,v))*(2*v)/3).toExponential())"),
      njs_str("-6.666666666666666e-1,1.3333333333333333e+0,-2e+0,2.6666666666666667e+0,-3.3333333333333337e+0") },

    { njs_str("1.7976931348623157e+308.toExponential()"),
      njs_str("1.7976931348623157e+308") },

#if 0  /* FIXME: bignum support is requred to support prec >= 20 */
    { njs_str("(1/7).toExponential(100)"),
      njs_str("1.4285714285714284921269268124888185411691665649414062500000000000000000000000000000000000000000000000e-1") },
#endif

    { njs_str("var v = 1.7976931348623157e+308; Number(v.toExponential()) == v"),
      njs_str("true") },

    { njs_str("(123).toExponential(-1)"),
      njs_str("RangeError: digits argument must be between 0 and 100") },

    { njs_str("(123).toExponential(2.4)"),
      njs_str("1.23e+2") },

    { njs_str("(123).toExponential(101)"),
      njs_str("RangeError: digits argument must be between 0 and 100") },

    { njs_str("[2**10000,-(2**10000),NaN].map((v)=>v.toExponential())"),
      njs_str("Infinity,-Infinity,NaN") },

    { njs_str("[2**10000,-(2**10000),NaN].map((v)=>v.toExponential(1000))"),
      njs_str("Infinity,-Infinity,NaN") },

    { njs_str("Number.prototype.toExponential.call('12')"),
      njs_str("TypeError: unexpected value type:string") },

    /* An object "valueOf/toString" methods. */

    { njs_str("var a = { valueOf: function() { return 1 } };    +a"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return '1' } };  +a"),
      njs_str("1") },

    { njs_str("var a = { valueOf: 2,"
                 "          toString: function() { return '1' } }; +a"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return 1 } }; ''+a"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return [] },"
                 "          toString: function() { return '1' } }; +a"),
      njs_str("1") },

    { njs_str("var a = { toString: function() { return 'a' } };"
                 "var b = { toString: function() { return a+'b' } }; '0'+b"),
      njs_str("0ab") },

    { njs_str("({valueOf:()=>'4'}) / ({valueOf:()=>2})"),
      njs_str("2") },

    { njs_str("({valueOf:()=>{throw 'x'}}) / ({valueOf:()=>{throw 'y'}});"
                 "var e; try { x/y } catch(ex) {e = ex}; ex"),
      njs_str("x") },

    { njs_str("({valueOf:()=>{ try {throw 'x'} catch (ex) {return 6} } }) / 2"),
      njs_str("3") },

    { njs_str("({valueOf:()=>2}) / ({valueOf:()=>{throw 'y'}});"
                 "var e; try { x/y } catch(ex) {e = ex}; ex"),
      njs_str("y") },

    { njs_str("({valueOf:()=>'4'}) % ({valueOf:()=>3})"),
      njs_str("1") },

    { njs_str("({valueOf:()=>9}) >>> ({valueOf:()=>2})"),
      njs_str("2") },

    { njs_str("({valueOf:()=>0x1f}) & ({valueOf:()=>0xf})"),
      njs_str("15") },

    { njs_str("({valueOf:()=>0x1f}) ^ ({valueOf:()=>0xf})"),
      njs_str("16") },

    { njs_str("({valueOf:()=>0xf}) == ({valueOf:()=>0xf})"),
      njs_str("false") },

    { njs_str("var e; try {({valueOf: String.prototype.valueOf}) == 1} "
                 "catch (ex) { e = ex}; e"),
      njs_str("TypeError: unexpected value type:object") },

    { njs_str("({valueOf:()=>0xf}) == 0xf"),
      njs_str("true") },

    { njs_str("0xf == ({valueOf:()=>0xf})"),
      njs_str("true") },

    { njs_str("({valueOf:()=>'0xf'}) == 0xf"),
      njs_str("true") },

    { njs_str("0xf == ({valueOf:()=>'0xf'})"),
      njs_str("true") },

    { njs_str("({valueOf:()=>0xf}) == '0xf'"),
      njs_str("true") },

    { njs_str("'0xf' == ({valueOf:()=>0xf})"),
      njs_str("true") },
    /**/

    { njs_str("1 + undefined"),
      njs_str("NaN") },

    { njs_str("1 + ''"),
      njs_str("1") },

    { njs_str("0xA + ''"),
      njs_str("10") },

    { njs_str("undefined + undefined"),
      njs_str("NaN") },

    { njs_str("var undefined"),
      njs_str("undefined") },

    { njs_str("1.2 + 5.7"),
      njs_str("6.9") },

    { njs_str("0xf + 1"),
      njs_str("16") },

    { njs_str("1 + 1 + '2' + 1 + 1"),
      njs_str("2211") },

    { njs_str("'gg' + -0"),
      njs_str("gg0") },

    { njs_str("1.2 - '5.7'"),
      njs_str("-4.5") },

    { njs_str("1.2 + -'5.7'"),
      njs_str("-4.5") },

    { njs_str("1.2 - '-5.7'"),
      njs_str("6.9") },

    { njs_str("5 - ' \t 12  \t'"),
      njs_str("-7") },

    { njs_str("5 - '12zz'"),
      njs_str("NaN") },

    { njs_str("5 - '0x2'"),
      njs_str("3") },

    { njs_str("5 - '-0x2'"),
      njs_str("7") },

    { njs_str("5 - '\t 0x2 \t'"),
      njs_str("3") },

    { njs_str("5 - '\t\\u000c0x2 \t'"),
      njs_str("3") },

    { njs_str("5 - '0x2 z'"),
      njs_str("NaN") },

    { njs_str("12 - '5.7e1'"),
      njs_str("-45") },

    { njs_str("12 - '5.e1'"),
      njs_str("-38") },

    { njs_str("12 - '5.7e+01'"),
      njs_str("-45") },

    { njs_str("12 - '5.7e-01'"),
      njs_str("11.43") },

    { njs_str("12 - ' 5.7e1 '"),
      njs_str("-45") },

    { njs_str("12 - '5.7e'"),
      njs_str("NaN") },

    { njs_str("12 - '5.7e+'"),
      njs_str("NaN") },

    { njs_str("12 - '5.7e-'"),
      njs_str("NaN") },

    { njs_str("12 - ' 5.7e1 z'"),
      njs_str("NaN") },

    { njs_str("5 - '0x'"),
      njs_str("NaN") },

    { njs_str("1 + +'3'"),
      njs_str("4") },

    { njs_str("1 - undefined"),
      njs_str("NaN") },

    { njs_str("1 - ''"),
      njs_str("1") },

    { njs_str("undefined - undefined"),
      njs_str("NaN") },

    /* String.toString() method. */

    { njs_str("'A'.toString()"),
      njs_str("A") },

    { njs_str("'A'.toString('hex')"),
      njs_str("TypeError: argument must be a byte string") },

    { njs_str("'A'.toBytes().toString('latin1')"),
      njs_str("TypeError: Unknown encoding: \"latin1\"") },

    { njs_str("'ABCD'.toBytes().toString('hex')"),
      njs_str("41424344") },

    { njs_str("'\\x00\\xAA\\xBB\\xFF'.toBytes().toString('hex')"),
      njs_str("00aabbff") },

    { njs_str("'\\x00\\xAA\\xBB\\xFF'.toBytes().toString('base64')"),
      njs_str("AKq7/w==") },

    { njs_str("'ABCD'.toBytes().toString('base64')"),
      njs_str("QUJDRA==") },

    { njs_str("'ABC'.toBytes().toString('base64')"),
      njs_str("QUJD") },

    { njs_str("'AB'.toBytes().toString('base64')"),
      njs_str("QUI=") },

    { njs_str("'A'.toBytes().toString('base64')"),
      njs_str("QQ==") },

    { njs_str("''.toBytes().toString('base64')"),
      njs_str("") },

    { njs_str("'\\x00\\xAA\\xBB\\xFF'.toBytes().toString('base64url')"),
      njs_str("AKq7_w") },

    { njs_str("'ABCD'.toBytes().toString('base64url')"),
      njs_str("QUJDRA") },

    { njs_str("'ABC'.toBytes().toString('base64url')"),
      njs_str("QUJD") },

    { njs_str("'AB'.toBytes().toString('base64url')"),
      njs_str("QUI") },

    { njs_str("'A'.toBytes().toString('base64url')"),
      njs_str("QQ") },

    { njs_str("''.toBytes().toString('base64url')"),
      njs_str("") },

    /* Assignment. */

    { njs_str("var a, b = (a = [2]) * (3 * 4); a +' '+ b"),
      njs_str("2 24") },

    { njs_str("var a = 1; var b = a += 1; b"),
      njs_str("2") },

    { njs_str("var a = 1; var b = a -= 1; b"),
      njs_str("0") },

    { njs_str("var a = 1; var b = a <<= 1; b"),
      njs_str("2") },

    /* 3 address operation and side effect. */

    { njs_str("var a = 1; function f(x) { a = x; return 2 }; a+f(5)+' '+a"),
      njs_str("3 5") },

    { njs_str("var a = 1; function f(x) { a = x; return 2 }; a += f(5)"),
      njs_str("3") },

    { njs_str("var x; x in (x = 1, [1, 2, 3])"),
      njs_str("false") },

    /* Exponentiation. */

    { njs_str("2 ** 3 ** 2"),
      njs_str("512") },

    { njs_str("2 ** (3 ** 2)"),
      njs_str("512") },

    { njs_str("(2 ** 3) ** 2"),
      njs_str("64") },

    { njs_str("3 ** 2 - 9"),
      njs_str("0") },

    { njs_str("-9 + 3 ** 2"),
      njs_str("0") },

    { njs_str("-3 ** 2"),
      njs_str("SyntaxError: Either left-hand side or entire exponentiation "
                 "must be parenthesized in 1") },

    { njs_str("-(3) ** 2"),
      njs_str("SyntaxError: Either left-hand side or entire exponentiation "
                 "must be parenthesized in 1") },

    { njs_str("-(3 ** 2)"),
      njs_str("-9") },

    { njs_str("(-3) ** 2"),
      njs_str("9") },

    { njs_str("1 ** NaN"),
      njs_str("NaN") },

    { njs_str("'a' ** -0"),
      njs_str("1") },

    { njs_str("1.1 ** Infinity"),
      njs_str("Infinity") },

    { njs_str("(-1.1) ** -Infinity"),
      njs_str("0") },

    { njs_str("(-1) ** Infinity"),
      njs_str("NaN") },

    { njs_str("1 ** -Infinity"),
      njs_str("NaN") },

    { njs_str("(-0.9) ** Infinity"),
      njs_str("0") },

    { njs_str("0.9 ** -Infinity"),
      njs_str("Infinity") },

    { njs_str("'Infinity' ** 0.1"),
      njs_str("Infinity") },

    { njs_str("Infinity ** '-0.1'"),
      njs_str("0") },

    { njs_str("(-Infinity) ** 3"),
      njs_str("-Infinity") },

    { njs_str("'-Infinity' ** '3.1'"),
      njs_str("Infinity") },

    { njs_str("(-Infinity) ** '-3'"),
      njs_str("-0") },

    { njs_str("'-Infinity' ** -2"),
      njs_str("0") },

    { njs_str("'0' ** 0.1"),
      njs_str("0") },

#ifndef __NetBSD__  /* NetBSD 7: pow(0, negative) == -Infinity. */
    { njs_str("0 ** '-0.1'"),
      njs_str("Infinity") },
#endif

    { njs_str("(-0) ** 3"),
      njs_str("-0") },

    { njs_str("'-0' ** '3.1'"),
      njs_str("0") },

    { njs_str("(-0) ** '-3'"),
      njs_str("-Infinity") },

#ifndef __NetBSD__  /* NetBSD 7: pow(0, negative) == -Infinity. */
    { njs_str("'-0' ** -2"),
      njs_str("Infinity") },
#endif

    { njs_str("(-3) ** 0.1"),
      njs_str("NaN") },

    { njs_str("var a = 0.1; a **= -2"),
      njs_str("99.99999999999999") },

    { njs_str("var a = 1; a **= NaN"),
      njs_str("NaN") },

    { njs_str("var a = 'a'; a **= -0"),
      njs_str("1") },

    { njs_str("var a = 1.1; a **= Infinity"),
      njs_str("Infinity") },

    { njs_str("var a = -1.1; a **= -Infinity"),
      njs_str("0") },

    { njs_str("var a = -1; a **= Infinity"),
      njs_str("NaN") },

    { njs_str("var a = 1; a **= -Infinity"),
      njs_str("NaN") },

    { njs_str("var a = -0.9; a **= Infinity"),
      njs_str("0") },

    { njs_str("var a = 0.9; a **= -Infinity"),
      njs_str("Infinity") },

    { njs_str("var a = 'Infinity'; a **= 0.1"),
      njs_str("Infinity") },

    { njs_str("var a = Infinity; a **= '-0.1'"),
      njs_str("0") },

    { njs_str("var a = -Infinity; a **= 3"),
      njs_str("-Infinity") },

    { njs_str("var a = '-Infinity'; a **= '3.1'"),
      njs_str("Infinity") },

    { njs_str("var a = -Infinity; a **= '-3'"),
      njs_str("-0") },

    { njs_str("var a = '-Infinity'; a **= -2"),
      njs_str("0") },

    { njs_str("var a = '0'; a **= 0.1"),
      njs_str("0") },

#ifndef __NetBSD__  /* NetBSD 7: pow(0, negative) == -Infinity. */
    { njs_str("var a = 0; a **= '-0.1'"),
      njs_str("Infinity") },
#endif

    { njs_str("var a = -0; a **= 3"),
      njs_str("-0") },

    { njs_str("var a = '-0'; a **= '3.1'"),
      njs_str("0") },

    { njs_str("var a = -0; a **= '-3'"),
      njs_str("-Infinity") },

#ifndef __NetBSD__  /* NetBSD 7: pow(0, negative) == -Infinity. */
    { njs_str("var a = '-0'; a **= -2"),
      njs_str("Infinity") },
#endif

    { njs_str("var a = -3; a **= 0.1"),
      njs_str("NaN") },

    /**/

    { njs_str("12 | 6"),
      njs_str("14") },

    { njs_str("12 | 'abc'"),
      njs_str("12") },

    { njs_str("-1 | 0"),
      njs_str("-1") },

    { njs_str("-2147483648 | 0"),
      njs_str("-2147483648") },

    { njs_str("1024.9 | 0"),
      njs_str("1024") },

    { njs_str("-1024.9 | 0"),
      njs_str("-1024") },

    { njs_str("9007199254740991 | 0"),
      njs_str("-1") },

    { njs_str("9007199254740992 | 0"),
      njs_str("0") },

    { njs_str("9007199254740993 | 0"),
      njs_str("0") },

#if 0
    { njs_str("9223372036854775808 | 0"),
      njs_str("0") },
#endif

    { njs_str("9223372036854777856 | 0"),
      njs_str("2048") },

    { njs_str("-9223372036854777856 | 0"),
      njs_str("-2048") },

    { njs_str("NaN | 0"),
      njs_str("0") },

    { njs_str("-NaN | 0"),
      njs_str("0") },

    { njs_str("Infinity | 0"),
      njs_str("0") },

    { njs_str("-Infinity | 0"),
      njs_str("0") },

    { njs_str("+0 | 0"),
      njs_str("0") },

    { njs_str("-0 | 0"),
      njs_str("0") },

    { njs_str("32.5 << 2.4"),
      njs_str("128") },

    { njs_str("32.5 << 'abc'"),
      njs_str("32") },

    { njs_str("'abc' << 2"),
      njs_str("0") },

    { njs_str("-1 << 0"),
      njs_str("-1") },

    { njs_str("-1 << -1"),
      njs_str("-2147483648") },

    { njs_str("-2147483648 << 0"),
      njs_str("-2147483648") },

#if 0
    { njs_str("9223372036854775808 << 0"),
      njs_str("0") },
#endif

    { njs_str("9223372036854777856 << 0"),
      njs_str("2048") },

    { njs_str("-9223372036854777856 << 0"),
      njs_str("-2048") },

    { njs_str("NaN << 0"),
      njs_str("0") },

    { njs_str("32.5 >> 2.4"),
      njs_str("8") },

    { njs_str("-1 >> 30"),
      njs_str("-1") },

    { njs_str("'abc' >> 2"),
      njs_str("0") },

    { njs_str("-1 >> 0"),
      njs_str("-1") },

    { njs_str("-1 >> -1"),
      njs_str("-1") },

    { njs_str("-2147483648 >> 0"),
      njs_str("-2147483648") },

    { njs_str("-2147483648 >> -1"),
      njs_str("-1") },

#if 0
    { njs_str("9223372036854775808 >> 0"),
      njs_str("0") },
#endif

    { njs_str("9223372036854777856 >> 0"),
      njs_str("2048") },

    { njs_str("-9223372036854777856 >> 0"),
      njs_str("-2048") },

    { njs_str("NaN >> 0"),
      njs_str("0") },

    { njs_str("-1 >>> 30"),
      njs_str("3") },

    { njs_str("NaN >>> 1"),
      njs_str("0") },

#if 0
    { njs_str("9223372036854775808 >>> 1"),
      njs_str("0") },
#endif

    { njs_str("-1 >>> 0"),
      njs_str("4294967295") },

    { njs_str("-1 >>> -1"),
      njs_str("1") },

    { njs_str("-2147483648 >>> 0"),
      njs_str("2147483648") },

    { njs_str("-2147483648 >>> -1"),
      njs_str("1") },

#if 0
    { njs_str("9223372036854775808 >>> 0"),
      njs_str("0") },
#endif

    { njs_str("9223372036854777856 >>> 0"),
      njs_str("2048") },

    { njs_str("-9223372036854777856 >>> 0"),
      njs_str("4294965248") },

    { njs_str("NaN >>> 0"),
      njs_str("0") },

    { njs_str("!2"),
      njs_str("false") },

    /**/

    { njs_str("var a = { valueOf: function() { return 1 } };   ~a"),
      njs_str("-2") },

    { njs_str("var a = { valueOf: function() { return '1' } }; ~a"),
      njs_str("-2") },

    /**/

    { njs_str("1 || 2"),
      njs_str("1") },

    { njs_str("var a = 1; 1 || (a = 2); a"),
      njs_str("1") },

    { njs_str("var x; x = 0 || x; x"),
      njs_str("undefined") },

    { njs_str("var x; x = 1 && x; x"),
      njs_str("undefined") },

    { njs_str("1 || 2 || 3"),
      njs_str("1") },

    { njs_str("1 || (2 + 2) || 3"),
      njs_str("1") },

    { njs_str("1 && 2"),
      njs_str("2") },

    { njs_str("1 && 2 && 3"),
      njs_str("3") },

    { njs_str("var a = 1; 0 && (a = 2); a"),
      njs_str("1") },

    { njs_str("false && true || true"),
      njs_str("true") },

    { njs_str("false && (true || true)"),
      njs_str("false") },

    { njs_str("var a = true; a = -~!a"),
      njs_str("1") },

    { njs_str("12 & 6"),
      njs_str("4") },

    { njs_str("-1 & 65536"),
      njs_str("65536") },

    { njs_str("-2147483648 & 65536"),
      njs_str("0") },

#if 0
    { njs_str("9223372036854775808 & 65536"),
      njs_str("0") },
#endif

    { njs_str("NaN & 65536"),
      njs_str("0") },

    { njs_str("12 ^ 6"),
      njs_str("10") },

    { njs_str("-1 ^ 65536"),
      njs_str("-65537") },

    { njs_str("-2147483648 ^ 65536"),
      njs_str("-2147418112") },

#if 0
    { njs_str("9223372036854775808 ^ 65536"),
      njs_str("65536") },
#endif

    { njs_str("NaN ^ 65536"),
      njs_str("65536") },

    { njs_str("var x = '1'; +x + 2"),
      njs_str("3") },

    /* Weird things. */

    { njs_str("'3' -+-+-+ '1' + '1' / '3' * '6' + '2'"),
      njs_str("42") },

    { njs_str("((+!![])+(+!![])+(+!![])+(+!![])+[])+((+!![])+(+!![])+[])"),
      njs_str("42") },

    { njs_str("1+[[]+[]]-[]+[[]-[]]-1"),
      njs_str("9") },

    { njs_str("[[]+[]]-[]+[[]-[]]"),
      njs_str("00") },

    { njs_str("!--[][1]"),
      njs_str("true") },

    { njs_str("[].concat[1,2,3]"),
      njs_str("undefined") },

    /**/

    { njs_str("'true' == true"),
      njs_str("false") },

    { njs_str("null == false"),
      njs_str("false") },

    { njs_str("0 == null"),
      njs_str("false") },

    { njs_str("!null"),
      njs_str("true") },

    { njs_str("0 === -0"),
      njs_str("true") },

    { njs_str("1/-0"),
      njs_str("-Infinity") },

    { njs_str("1/0 === 1/-0"),
      njs_str("false") },

    { njs_str("1 == true"),
      njs_str("true") },

    { njs_str("NaN === NaN"),
      njs_str("false") },

    { njs_str("NaN !== NaN"),
      njs_str("true") },

    { njs_str("NaN == NaN"),
      njs_str("false") },

    { njs_str("NaN != NaN"),
      njs_str("true") },

    { njs_str("NaN == false"),
      njs_str("false") },

    { njs_str("Infinity == Infinity"),
      njs_str("true") },

    { njs_str("-Infinity == -Infinity"),
      njs_str("true") },

    { njs_str("-Infinity < Infinity"),
      njs_str("true") },

    { njs_str("Infinity - Infinity"),
      njs_str("NaN") },

    { njs_str("Infinity - -Infinity"),
      njs_str("Infinity") },

    { njs_str("undefined == 0"),
      njs_str("false") },

    { njs_str("undefined == null"),
      njs_str("true") },

    { njs_str("'1' == 1"),
      njs_str("true") },

    { njs_str("'1a' == '1'"),
      njs_str("false") },

    { njs_str("'abc' == 'abc'"),
      njs_str("true") },

    { njs_str("'abc' < 'abcde'"),
      njs_str("true") },

    { njs_str("0 == ''"),
      njs_str("true") },

    { njs_str("0 == ' '"),
      njs_str("true") },

    { njs_str("0 == '  '"),
      njs_str("true") },

    { njs_str("0 == '0'"),
      njs_str("true") },

    { njs_str("0 == ' 0 '"),
      njs_str("true") },

    { njs_str("0 == '000'"),
      njs_str("true") },

    { njs_str("'0' == ''"),
      njs_str("false") },

    { njs_str("1 < 2"),
      njs_str("true") },

    { njs_str("NaN < NaN"),
      njs_str("false") },

    { njs_str("NaN > NaN"),
      njs_str("false") },

    { njs_str("undefined < 1"),
      njs_str("false") },

    { njs_str("[] == false"),
      njs_str("true") },

    { njs_str("[0] == false"),
      njs_str("true") },

    { njs_str("[0,0] == false"),
      njs_str("false") },

    { njs_str("({}) == false"),
      njs_str("false") },

    { njs_str("new Number(1) == new String('1')"),
      njs_str("false") },

    { njs_str("var a = Object; a == Object"),
      njs_str("true") },

    { njs_str("'1' == new Number(1)"),
      njs_str("true") },

    { njs_str("new Number(null) + ''"),
      njs_str("0") },

    { njs_str("new String('abc') == 'abc'"),
      njs_str("true") },

    { njs_str("false == new String('0')"),
      njs_str("true") },

    { njs_str("var a = { valueOf: function() { return 5 } };   a == 5"),
      njs_str("true") },

    { njs_str("var a = { valueOf: function() { return '5' } }; a == 5"),
      njs_str("true") },

    { njs_str("var a = { valueOf: function() { return '5' } }; a == '5'"),
      njs_str("true") },

    { njs_str("var a = { valueOf: function() { return 5 } }; a == '5'"),
      njs_str("true") },

    { njs_str("var a = { toString: function() { return true } }; '1' == a"),
      njs_str("true") },

    { njs_str("var a = { valueOf: function() { return 'b' },"
                 "          toString: function() { return 'a' } }; a == 'a'"),
      njs_str("false") },

    /* Comparisions. */

    { njs_str("1 < 2"),
      njs_str("true") },

    { njs_str("1 < 1"),
      njs_str("false") },

    { njs_str("1 <= 1"),
      njs_str("true") },

    { njs_str("1 <= 2"),
      njs_str("true") },

    { njs_str("2 > 1"),
      njs_str("true") },

    { njs_str("1 > 2"),
      njs_str("false") },

    { njs_str("1 > 1"),
      njs_str("false") },

    { njs_str("1 >= 1"),
      njs_str("true") },

    { njs_str("2 >= 1"),
      njs_str("true") },

    { njs_str("1 >= 2"),
      njs_str("false") },

    /**/

    { njs_str("null === null"),
      njs_str("true") },

    { njs_str("null !== null"),
      njs_str("false") },

    { njs_str("null == null"),
      njs_str("true") },

    { njs_str("null != null"),
      njs_str("false") },

    { njs_str("null < null"),
      njs_str("false") },

    { njs_str("null > null"),
      njs_str("false") },

    { njs_str("null <= null"),
      njs_str("true") },

    { njs_str("null >= null"),
      njs_str("true") },

    /**/

    { njs_str("null === undefined"),
      njs_str("false") },

    { njs_str("null !== undefined"),
      njs_str("true") },

    { njs_str("null == undefined"),
      njs_str("true") },

    { njs_str("null != undefined"),
      njs_str("false") },

    { njs_str("null < undefined"),
      njs_str("false") },

    { njs_str("null > undefined"),
      njs_str("false") },

    { njs_str("null <= undefined"),
      njs_str("false") },

    { njs_str("null >= undefined"),
      njs_str("false") },

    /**/

    { njs_str("null === false"),
      njs_str("false") },

    { njs_str("null !== false"),
      njs_str("true") },

    { njs_str("null == false"),
      njs_str("false") },

    { njs_str("null != false"),
      njs_str("true") },

    { njs_str("null < false"),
      njs_str("false") },

    { njs_str("null > false"),
      njs_str("false") },

    { njs_str("null <= false"),
      njs_str("true") },

    { njs_str("null >= false"),
      njs_str("true") },

    /**/

    { njs_str("null === true"),
      njs_str("false") },

    { njs_str("null !== true"),
      njs_str("true") },

    { njs_str("null == true"),
      njs_str("false") },

    { njs_str("null != true"),
      njs_str("true") },

    { njs_str("null < true"),
      njs_str("true") },

    { njs_str("null > true"),
      njs_str("false") },

    { njs_str("null <= true"),
      njs_str("true") },

    { njs_str("null >= true"),
      njs_str("false") },

    /**/

    { njs_str("Infinity === Infinity"),
      njs_str("true") },

    { njs_str("Infinity !== Infinity"),
      njs_str("false") },

    { njs_str("Infinity == Infinity"),
      njs_str("true") },

    { njs_str("Infinity != Infinity"),
      njs_str("false") },

    { njs_str("Infinity < Infinity"),
      njs_str("false") },

    { njs_str("Infinity > Infinity"),
      njs_str("false") },

    { njs_str("Infinity <= Infinity"),
      njs_str("true") },

    { njs_str("Infinity >= Infinity"),
      njs_str("true") },

    /**/

    { njs_str("-Infinity === Infinity"),
      njs_str("false") },

    { njs_str("-Infinity !== Infinity"),
      njs_str("true") },

    { njs_str("-Infinity == Infinity"),
      njs_str("false") },

    { njs_str("-Infinity != Infinity"),
      njs_str("true") },

    { njs_str("-Infinity < Infinity"),
      njs_str("true") },

    { njs_str("-Infinity > Infinity"),
      njs_str("false") },

    { njs_str("-Infinity <= Infinity"),
      njs_str("true") },

    { njs_str("-Infinity >= Infinity"),
      njs_str("false") },

    { njs_str("Boolean(Infinity)"),
      njs_str("true") },

    { njs_str("!Infinity === false"),
      njs_str("true") },

    { njs_str("var Infinity"),
      njs_str("undefined") },

#if 0 /* ES5FIX */
    { njs_str("Infinity = 1"),
      njs_str("TypeError: Cannot assign to read-only property "Infinity" of object") },
#endif

    /**/

    { njs_str("NaN === NaN"),
      njs_str("false") },

    { njs_str("NaN !== NaN"),
      njs_str("true") },

    { njs_str("NaN == NaN"),
      njs_str("false") },

    { njs_str("NaN != NaN"),
      njs_str("true") },

    { njs_str("NaN < NaN"),
      njs_str("false") },

    { njs_str("NaN > NaN"),
      njs_str("false") },

    { njs_str("NaN >= NaN"),
      njs_str("false") },

    { njs_str("NaN <= NaN"),
      njs_str("false") },

    { njs_str("var NaN"),
      njs_str("undefined") },

#if 0 /* ES5FIX */
    { njs_str("NaN = 1"),
      njs_str("TypeError: Cannot assign to read-only property "NaN" of object") },
#endif

    /**/

    { njs_str("null < 0"),
      njs_str("false") },

    { njs_str("null < 1"),
      njs_str("true") },

    { njs_str("null < NaN"),
      njs_str("false") },

    { njs_str("null < -Infinity"),
      njs_str("false") },

    { njs_str("null < Infinity"),
      njs_str("true") },

    { njs_str("null < 'null'"),
      njs_str("false") },

    { njs_str("null < '1'"),
      njs_str("true") },

    { njs_str("null < [1]"),
      njs_str("true") },

    { njs_str("null < ({})"),
      njs_str("false") },

    { njs_str("var a = { valueOf: function() { return 1 } };     null < a"),
      njs_str("true") },

    { njs_str("var a = { valueOf: function() { return 'null' } };null < a"),
      njs_str("false") },

    { njs_str("var a = { valueOf: function() { return '1' } };   null < a"),
      njs_str("true") },

    { njs_str("1 < {valueOf: ()=>{throw 'x'}}"),
      njs_str("x") },

    { njs_str("({valueOf: ()=>{throw 'x'}}) <= ({valueOf:()=>{throw 'y'}})"),
      njs_str("x") },

    { njs_str("({valueOf:()=>{throw 'x'}}) > ({valueOf:()=>{throw 'y'}})"),
      njs_str("x") },

    /**/

    { njs_str("undefined == undefined"),
      njs_str("true") },

    { njs_str("undefined != undefined"),
      njs_str("false") },

    { njs_str("undefined === undefined"),
      njs_str("true") },

    { njs_str("undefined !== undefined"),
      njs_str("false") },

    { njs_str("undefined < undefined"),
      njs_str("false") },

    { njs_str("undefined < null"),
      njs_str("false") },

    { njs_str("undefined < false"),
      njs_str("false") },

    { njs_str("undefined < true"),
      njs_str("false") },

    { njs_str("undefined < 0"),
      njs_str("false") },

    { njs_str("undefined < 1"),
      njs_str("false") },

    { njs_str("undefined < NaN"),
      njs_str("false") },

    { njs_str("undefined < -Infinity"),
      njs_str("false") },

    { njs_str("undefined < Infinity"),
      njs_str("false") },

    { njs_str("undefined < 'undefined'"),
      njs_str("false") },

    { njs_str("undefined < '1'"),
      njs_str("false") },

    { njs_str("undefined < [1]"),
      njs_str("false") },

    { njs_str("undefined < ({})"),
      njs_str("false") },

    { njs_str("var a = { valueOf: function() { return 1 } }; undefined < a"),
      njs_str("false") },

    { njs_str("var a = { valueOf: function() { return 'undefined' } };"
                 "undefined < a"),
      njs_str("false") },

    { njs_str("var a = { valueOf: function() { return '1' } };"
                 "undefined < a"),
      njs_str("false") },

    /**/

    { njs_str("false < 1"),
      njs_str("true") },

    { njs_str("true < 1"),
      njs_str("false") },

    { njs_str("-1 < 1"),
      njs_str("true") },

    { njs_str("-1 < '1'"),
      njs_str("true") },

    { njs_str("NaN < NaN"),
      njs_str("false") },

    { njs_str("-Infinity < Infinity"),
      njs_str("true") },

    { njs_str("Infinity < -Infinity"),
      njs_str("false") },

    { njs_str("1 < 'abc'"),
      njs_str("false") },

    /**/

    { njs_str("[] === []"),
      njs_str("false") },

    { njs_str("[] !== []"),
      njs_str("true") },

    { njs_str("[] == []"),
      njs_str("false") },

    { njs_str("[] != []"),
      njs_str("true") },

    { njs_str("[] < []"),
      njs_str("false") },

    { njs_str("[] > []"),
      njs_str("false") },

    { njs_str("[] >= []"),
      njs_str("true") },

    { njs_str("[] <= []"),
      njs_str("true") },

    /**/

    { njs_str("({}) === ({})"),
      njs_str("false") },

    { njs_str("({}) !== ({})"),
      njs_str("true") },

    { njs_str("({}) == ({})"),
      njs_str("false") },

    { njs_str("({}) != ({})"),
      njs_str("true") },

    { njs_str("({}) > ({})"),
      njs_str("false") },

    { njs_str("({}) <= ({})"),
      njs_str("true") },

    { njs_str("({}) >= ({})"),
      njs_str("true") },

    /**/

    { njs_str("[0] == ({})"),
      njs_str("false") },

    { njs_str("[0] != ({})"),
      njs_str("true") },

    { njs_str("[0] <= ({})"),
      njs_str("true") },

    { njs_str("[0] >= ({})"),
      njs_str("false") },

    /**/
    { njs_str("new String('1') > new Number(1)"),
      njs_str("false") },

    { njs_str("new Boolean(true) > '1'"),
      njs_str("false") },

    { njs_str("'0' >= new Number(1)"),
      njs_str("false") },

    { njs_str("'1' >= new Number(1)"),
      njs_str("true") },

    { njs_str("new String('1') < new Number(1)"),
      njs_str("false") },

    { njs_str("new Boolean(true) < '1'"),
      njs_str("false") },

    { njs_str("new String('1') <= new Number(1)"),
      njs_str("true") },

    { njs_str("new Boolean(true) <= '1'"),
      njs_str("true") },

    { njs_str("'-1' < {valueOf: function() {return -2}}"),
      njs_str("false") },

    { njs_str("new 0[isNaN]"),
      njs_str("TypeError: (intermediate value)[\"[object Function]\"] is not a function") },

    { njs_str("new 0[undefined]"),
      njs_str("TypeError: (intermediate value)[\"undefined\"] is not a function") },

    /**/

    { njs_str("var a; a = 1 ? 2 : 3"),
      njs_str("2") },

    { njs_str("var a; a = 1 ? 2 : 3 ? 4 : 5"),
      njs_str("2") },

    { njs_str("var a; a = 0 ? 2 : 3 ? 4 : 5"),
      njs_str("4") },

    { njs_str("0 ? 2 ? 3 : 4 : 5"),
      njs_str("5") },

    { njs_str("1 ? 2 ? 3 : 4 : 5"),
      njs_str("3") },

    { njs_str("1 ? 0 ? 3 : 4 : 5"),
      njs_str("4") },

    { njs_str("(1 ? 0 : 3) ? 4 : 5"),
      njs_str("5") },

    { njs_str("var a; a = (1 + 2) ? 2 ? 3 + 4 : 5 : 6"),
      njs_str("7") },

    { njs_str("var a; a = (1 ? 2 : 3) + 4"),
      njs_str("6") },

    { njs_str("var a, b; a = 1 ? b = 2 + 4 : b = 3"),
      njs_str("6") },

    { njs_str("var a; a = 1 ? [1,2] : []"),
      njs_str("1,2") },

    /**/

    { njs_str("var a = { valueOf: function() { return 1 } };   +a"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return '1' } }; +a"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return 1 } };   -a"),
      njs_str("-1") },

    { njs_str("var a = { valueOf: function() { return '1' } }; -a"),
      njs_str("-1") },

    /* Increment. */

    { njs_str("var a = 1;   ++a"),
      njs_str("2") },

    { njs_str("var a = '1'; ++a"),
      njs_str("2") },

    { njs_str("var a = [1]; ++a"),
      njs_str("2") },

    { njs_str("var a = {};  ++a"),
      njs_str("NaN") },

    { njs_str("var a = [1,2,3]; var b = 1; b = ++a[b]; b + ' '+ a"),
      njs_str("3 1,3,3") },

    { njs_str("var a = { valueOf: function() { return 1 } };"
                 "++a +' '+ a +' '+ typeof a"),
      njs_str("2 2 number") },

    { njs_str("var a = { valueOf: function() { return '1' } };"
                 "++a +' '+ a +' '+ typeof a"),
      njs_str("2 2 number") },

    { njs_str("var a = { valueOf: function() { return [1] } };"
                 "++a +' '+ a +' '+ typeof a"),
      njs_str("NaN NaN number") },

    { njs_str("var a = { valueOf: function() { return {} } };"
                 "++a +' '+ a +' '+ typeof a"),
      njs_str("NaN NaN number") },

    /**/

    { njs_str("var a = 1;   a = ++a"),
      njs_str("2") },

    { njs_str("var a = '1'; a = ++a"),
      njs_str("2") },

    { njs_str("var a = [1]; a = ++a"),
      njs_str("2") },

    { njs_str("var a = {};  a = ++a"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };   a = ++a"),
      njs_str("2") },

    { njs_str("var a = { valueOf: function() { return '1' } }; a = ++a"),
      njs_str("2") },

    { njs_str("var a = { valueOf: function() { return [1] } }; a = ++a"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };  a = ++a"),
      njs_str("NaN") },

    /**/

    { njs_str("var a = 1;   var b = ++a; a +' '+ b"),
      njs_str("2 2") },

    { njs_str("var a = '1'; var b = ++a; a +' '+ b"),
      njs_str("2 2") },

    { njs_str("var a = [1]; var b = ++a; a +' '+ b"),
      njs_str("2 2") },

    { njs_str("var a = {};  var b = ++a; a +' '+ b"),
      njs_str("NaN NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };"
                 "var b = ++a; a +' '+ b"),
      njs_str("2 2") },

    { njs_str("var a = { valueOf: function() { return '1' } };"
                 "var b = ++a; a +' '+ b"),
      njs_str("2 2") },

    { njs_str("var a = { valueOf: function() { return [1] } };"
                 "var b = ++a; a +' '+ b"),
      njs_str("NaN NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };"
                 "var b = ++a; a +' '+ b"),
      njs_str("NaN NaN") },

    /* Post increment. */

    { njs_str("var a = 1;   a++"),
      njs_str("1") },

    { njs_str("var a = '1'; a++"),
      njs_str("1") },

    { njs_str("var a = [1]; a++"),
      njs_str("1") },

    { njs_str("var a = {};  a++"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };   a++"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return '1' } }; a++"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return [1] } }; a++"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };  a++"),
      njs_str("NaN") },

    /**/

    { njs_str("var a = 1;   a = a++"),
      njs_str("1") },

    { njs_str("var a = '1'; a = a++"),
      njs_str("1") },

    { njs_str("var a = [1]; a = a++"),
      njs_str("1") },

    { njs_str("var a = {};  a = a++"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };   a = a++"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return '1' } }; a = a++"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return [1] } }; a = a++"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };  a = a++"),
      njs_str("NaN") },

    /**/

    { njs_str("var a = 1;   var b = a++; a +' '+ b"),
      njs_str("2 1") },

    { njs_str("var a = '1'; var b = a++; a +' '+ b"),
      njs_str("2 1") },

    { njs_str("var a = [1]; var b = a++; a +' '+ b"),
      njs_str("2 1") },

    { njs_str("var a = {};  var b = a++; a +' '+ b"),
      njs_str("NaN NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };"
                 "var b = a++; a +' '+ b"),
      njs_str("2 1") },

    { njs_str("var a = { valueOf: function() { return '1' } };"
                 "var b = a++; a +' '+ b"),
      njs_str("2 1") },

    { njs_str("var a = { valueOf: function() { return [1] } };"
                 "var b = a++; a +' '+ b"),
      njs_str("NaN NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };"
                 "var b = a++; a +' '+ b"),
      njs_str("NaN NaN") },

    /* Decrement. */

    { njs_str("var a = 1;   --a"),
      njs_str("0") },

    { njs_str("var a = '1'; --a"),
      njs_str("0") },

    { njs_str("var a = [1]; --a"),
      njs_str("0") },

    { njs_str("var a = {};  --a"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return 1} };   --a"),
      njs_str("0") },

    { njs_str("var a = { valueOf: function() { return '1'} }; --a"),
      njs_str("0") },

    { njs_str("var a = { valueOf: function() { return [1]} }; --a"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return {} } }; --a"),
      njs_str("NaN") },

    /**/

    { njs_str("var a = 1;   a = --a"),
      njs_str("0") },

    { njs_str("var a = '1'; a = --a"),
      njs_str("0") },

    { njs_str("var a = [1]; a = --a"),
      njs_str("0") },

    { njs_str("var a = {};  a = --a"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return 1} };   a = --a"),
      njs_str("0") },

    { njs_str("var a = { valueOf: function() { return '1'} }; a = --a"),
      njs_str("0") },

    { njs_str("var a = { valueOf: function() { return [1]} }; a = --a"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return {} } }; a = --a"),
      njs_str("NaN") },

    /**/

    { njs_str("var a = 1;   var b = --a; a +' '+ b"),
      njs_str("0 0") },

    { njs_str("var a = '1'; var b = --a; a +' '+ b"),
      njs_str("0 0") },

    { njs_str("var a = [1]; var b = --a; a +' '+ b"),
      njs_str("0 0") },

    { njs_str("var a = {};  var b = --a; a +' '+ b"),
      njs_str("NaN NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };"
                 "var b = --a; a +' '+ b"),
      njs_str("0 0") },

    { njs_str("var a = { valueOf: function() { return '1' } };"
                 "var b = --a; a +' '+ b"),
      njs_str("0 0") },

    { njs_str("var a = { valueOf: function() { return [1] } };"
                 "var b = --a; a +' '+ b"),
      njs_str("NaN NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };"
                 "var b = --a; a +' '+ b"),
      njs_str("NaN NaN") },

    /* Post decrement. */

    { njs_str("var a = 1;   a--"),
      njs_str("1") },

    { njs_str("var a = '1'; a--"),
      njs_str("1") },

    { njs_str("var a = [1]; a--"),
      njs_str("1") },

    { njs_str("var a = {};  a--"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };   a--"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return '1' } }; a--"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return [1] } }; a--"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };  a--"),
      njs_str("NaN") },

    /**/

    { njs_str("var a = 1;   a = a--"),
      njs_str("1") },

    { njs_str("var a = '1'; a = a--"),
      njs_str("1") },

    { njs_str("var a = [1]; a = a--"),
      njs_str("1") },

    { njs_str("var a = {};  a = a--"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };   a = a--"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return '1' } }; a = a--"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return [1] } }; a = a--"),
      njs_str("NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };  a = a--"),
      njs_str("NaN") },

    /**/

    { njs_str("var a = 1;   var b = a--; a +' '+ b"),
      njs_str("0 1") },

    { njs_str("var a = '1'; var b = a--; a +' '+ b"),
      njs_str("0 1") },

    { njs_str("var a = [1]; var b = a--; a +' '+ b"),
      njs_str("0 1") },

    { njs_str("var a = {};  var b = a--; a +' '+ b"),
      njs_str("NaN NaN") },

    { njs_str("var a = { valueOf: function() { return 1 } };"
                 "var b = a--; a +' '+ b"),
      njs_str("0 1") },

    { njs_str("var a = { valueOf: function() { return '1' } };"
                 "var b = a--; a +' '+ b"),
      njs_str("0 1") },

    { njs_str("var a = { valueOf: function() { return [1] } };"
                 "var b = a--; a +' '+ b"),
      njs_str("NaN NaN") },

    { njs_str("var a = { valueOf: function() { return {} } };"
                 "var b = a--; a +' '+ b"),
      njs_str("NaN NaN") },

    /**/

    { njs_str("var a, b; a = 2; b = ++a + ++a; a + ' ' + b"),
      njs_str("4 7") },

    { njs_str("var a, b; a = 2; b = a++ + a++; a + ' ' + b"),
      njs_str("4 5") },

    { njs_str("var a, b; a = b = 7; a +' '+ b"),
      njs_str("7 7") },

    { njs_str("var a, b, c; a = b = c = 5; a +' '+ b +' '+ c"),
      njs_str("5 5 5") },

    { njs_str("var a, b, c; a = b = (c = 5) + 2; a +' '+ b +' '+ c"),
      njs_str("7 7 5") },

    { njs_str("1, 2 + 5, 3"),
      njs_str("3") },

    { njs_str("var a, b; a = 1 /* YES */\n b = a + 2 \n \n + 1 \n + 3"),
      njs_str("7") },

    { njs_str("var a, b; a = 1 // YES \n b = a + 2 \n \n + 1 \n + 3"),
      njs_str("7") },

    { njs_str("var a; a = 0; ++ \n a"),
      njs_str("1") },

    { njs_str("var a; a = 0\n ++a"),
      njs_str("1") },

    { njs_str("a = 0; a \n ++"),
      njs_str("SyntaxError: Unexpected end of input in 2") },

    { njs_str("a = 0; a \n --"),
      njs_str("SyntaxError: Unexpected end of input in 2") },

    { njs_str("var a = 0; a \n + 1"),
      njs_str("1") },

    { njs_str("var a = 0; a \n +\n 1"),
      njs_str("1") },

    { njs_str("var a; a = 1 ? 2 \n : 3"),
      njs_str("2") },

    { njs_str("var a, b, c;"
                 "a = 0 / 0; b = 1 / 0; c = -1 / 0; a +' '+ b +' '+ c"),
      njs_str("NaN Infinity -Infinity") },

    { njs_str("var a, b; a = (b = 7) + 5; var c; a +' '+ b +' '+ c"),
      njs_str("12 7 undefined") },

    { njs_str("var a, b = 1, c; a +' '+ b +' '+ c"),
      njs_str("undefined 1 undefined") },

    { njs_str("var a = 1, b = a + 1; a +' '+ b"),
      njs_str("1 2") },

    { njs_str("var a; a = a = 1"),
      njs_str("1") },

    { njs_str("var a = 1, \n b; a +' '+ b"),
      njs_str("1 undefined") },

    { njs_str("var a; a = b + 1; var b; a +' '+ b"),
      njs_str("NaN undefined") },

    { njs_str("var a += 1"),
      njs_str("SyntaxError: Unexpected token \"+=\" in 1") },

    { njs_str("var a = a + 1"),
      njs_str("undefined") },

    { njs_str("var a; a = b + 1; var b = 1; a +' '+ b"),
      njs_str("NaN 1") },

    { njs_str("var a; (a) = 1"),
      njs_str("1") },

    { njs_str("a"),
      njs_str("ReferenceError: \"a\" is not defined in 1") },

    { njs_str("\na"),
      njs_str("ReferenceError: \"a\" is not defined in 2") },

    { njs_str("\n\na"),
      njs_str("ReferenceError: \"a\" is not defined in 3") },

    { njs_str("a + a"),
      njs_str("ReferenceError: \"a\" is not defined in 1") },

    { njs_str("a = b + 1"),
      njs_str("ReferenceError: \"a\" is not defined in 1") },

    { njs_str("a = a + 1"),
      njs_str("ReferenceError: \"a\" is not defined in 1") },

    { njs_str("a += 1"),
      njs_str("ReferenceError: \"a\" is not defined in 1") },

    { njs_str("a += 1; var a = 2"),
      njs_str("undefined") },

    { njs_str("var a = 1"),
      njs_str("undefined") },

    { njs_str("var a = 1; a = (a = 2) + a"),
      njs_str("4") },

    { njs_str("var a = 1; a = a + (a = 2)"),
      njs_str("3") },

    { njs_str("var a = 1; a += (a = 2)"),
      njs_str("3") },

    { njs_str("var a = b = 1; var b; a +' '+ b"),
      njs_str("1 1") },

    { njs_str("var a \n if (!a) a = 3; a"),
      njs_str("3") },

    /* automatic semicolon insertion. */

    { njs_str("(a\n--"),
      njs_str("SyntaxError: Unexpected token \"--\" in 2") },

    { njs_str("(a\n++"),
      njs_str("SyntaxError: Unexpected token \"++\" in 2") },

    { njs_str("var x = 0, y = 2; x\n--\ny; [x,y]"),
      njs_str("0,1") },

    /* if. */

    { njs_str("if (0);"),
      njs_str("undefined") },

    { njs_str("if (0) {}"),
      njs_str("undefined") },

    { njs_str("if (0);else;"),
      njs_str("undefined") },

    { njs_str("var a = 1; if (true); else a = 2; a"),
      njs_str("1") },

    { njs_str("var a = 1; if (false); else a = 2; a"),
      njs_str("2") },

    { njs_str("var a = 1; if (true) a = 2; else a = 3; a"),
      njs_str("2") },

    { njs_str("var a = 3; if (true) if (false); else a = 2; a"),
      njs_str("2") },

    { njs_str("var a = 3; if (true) if (false); else; a = 2; a"),
      njs_str("2") },

    { njs_str("var a = [3], b; if (1==1||2==2) { b = '1'+'2'+a[0] }; b"),
      njs_str("123") },

    { njs_str("(function(){ if(true) return 1 else return 0; })()"),
      njs_str("SyntaxError: Unexpected token \"else\" in 1") },

    { njs_str("(function(){ if(true) return 1; else return 0; })()"),
      njs_str("1") },

    { njs_str("(function(){ if(true) return 1;; else return 0; })()"),
      njs_str("SyntaxError: Unexpected token \"else\" in 1") },

    { njs_str("(function(){ if(true) return 1\n else return 0; })()"),
      njs_str("1") },

    { njs_str("(function(){ if(true) return 1\n;\n else return 0; })()"),
      njs_str("1") },

    { njs_str("function f(n) {if (n) throw 'foo' else return 1}; f(0)"),
      njs_str("SyntaxError: Unexpected token \"else\" in 1") },

    { njs_str("function f(n) {if (n)\n throw 'foo'\nelse return 1}; f(0)"),
      njs_str("1") },

    { njs_str("function f(n) {if (n)\n throw 'foo'\nelse return 1}; f(1)"),
      njs_str("foo") },

    { njs_str("function f(n) {if (n == 1) throw 'foo'\nelse if (n == 2) return 1}; f(2)"),
      njs_str("1") },

    { njs_str("(function(){ for (var p in [1] ){ if (1) break else return 0; }})()"),
      njs_str("SyntaxError: Unexpected token \"else\" in 1") },

    { njs_str("(function(){ for (var p in [1] ){ if (1) break\n else return 0; }})()"),
      njs_str("undefined") },

    { njs_str("(function(){ for (var p in [1] ){ if (1) break; else return 0; }})()"),
      njs_str("undefined") },

    { njs_str("(function(){ for (var p in [1] ){ if (1) continue else return 0; }})()"),
      njs_str("SyntaxError: Unexpected token \"else\" in 1") },

    { njs_str("(function(){ for (var p in [1] ){ if (1) continue\n else return 0; }})()"),
      njs_str("undefined") },

    { njs_str("(function(){ for (var p in [1] ){ if (1) continue; else return 0; }})()"),
      njs_str("undefined") },

    { njs_str("(function(x){ if\n(x) return -1; else return 0; })(0)"),
      njs_str("0") },

    { njs_str("(function(x){ if\n(\nx) return -1; else return 0; })(0)"),
      njs_str("0") },

    { njs_str("(function(x){ if\n(\nx)\nreturn -1; else return 0; })(0)"),
      njs_str("0") },

    { njs_str("(function(x){ if\n(\nx)\nreturn -1\n else return 0; })(0)"),
      njs_str("0") },

    { njs_str("(function(x){ if\n(\nx)\nreturn -1\n else\nreturn 0; })(0)"),
      njs_str("0") },

    /* do while. */

    { njs_str("do { break } if (false)"),
      njs_str("SyntaxError: Unexpected token \"if\" in 1") },

    /* for in. */

    { njs_str("for (null in undefined);"),
      njs_str("ReferenceError: Invalid left-hand side \"null\" in for-in statement in 1") },

    { njs_str("for (var a, b in []);"),
      njs_str("SyntaxError: Unexpected token \"in\" in 1") },

    { njs_str("var s = ''; for (var p in [1,2]) {s += p}; s"),
      njs_str("01") },

    { njs_str("var s; for (var p in [1]) {s = typeof(p)}; s"),
      njs_str("string") },

    { njs_str("var s = ''; for (var p in {a:1, b:2}) {s += p}; s"),
      njs_str("ab") },

    { njs_str("var s = '';"
                 "var o = Object.defineProperty({}, 'x', {value:1});"
                 "Object.defineProperty(o, 'y', {value:2, enumerable:true});"
                 "for (var p in o) {s += p}; s"),
      njs_str("y") },

    { njs_str("var o = {a:1, b:2}; var arr = []; "
                 "for (var a in o) {arr.push(a)}; arr"),
      njs_str("a,b") },

    { njs_str("var o = {a:1, b:2}; var arr = []; delete o.a; "
                 "for (var a in o) {arr.push(a)}; arr"),
      njs_str("b") },

    /* switch. */

    { njs_str("switch"),
      njs_str("SyntaxError: Unexpected end of input in 1") },

    { njs_str("switch (1);"),
      njs_str("SyntaxError: Unexpected token \";\" in 1") },

    { njs_str("switch (1) { do { } while (1) }"),
      njs_str("SyntaxError: Unexpected token \"do\" in 1") },

    { njs_str("switch (1) {}"),
      njs_str("undefined") },

    { njs_str("switch (1) {default:}"),
      njs_str("undefined") },

    { njs_str("switch (1) {case 0:}"),
      njs_str("undefined") },

    { njs_str("switch (1) {default:;}"),
      njs_str("undefined") },

    { njs_str("switch (1) {default:; default:}"),
      njs_str("SyntaxError: More than one default clause in switch statement in 1") },

    { njs_str("switch (1) {case 0:;}"),
      njs_str("undefined") },

    { njs_str("var a = 'A'; switch (a) {"
                 "case 0: a += '0';"
                 "case 1: a += '1';"
                 "}; a"),
      njs_str("A") },

    { njs_str("var a = 'A'; switch (0) {"
                 "case 0: a += '0';"
                 "case 1: a += '1';"
                 "}; a"),
      njs_str("A01") },

    { njs_str("var a = 'A'; switch (0) {"
                 "case 0: a += '0'; break;"
                 "case 1: a += '1';"
                 "}; a"),
      njs_str("A0") },

    { njs_str("var a = 'A'; switch (1) {"
                 "case 0: a += '0';"
                 "case 1: a += '1';"
                 "}; a"),
      njs_str("A1") },

    { njs_str("var a = 'A'; switch (2) {"
                 "case 0: a += '0';"
                 "case 1: a += '1';"
                 "default: a += 'D';"
                 "}; a"),
      njs_str("AD") },

    { njs_str("var a = 'A'; switch (2) {"
                 "case 0: a += '0';"
                 "default: a += 'D';"
                 "case 1: a += '1';"
                 "}; a"),
      njs_str("AD1") },

    { njs_str("var a = 'A'; function f(x) { a += x; return 0 }"
                 "switch (a) {"
                 "case f(1):"
                 "default:"
                 "case f(2): a += 'D';"
                 "case f(3): a += 'T';"
                 "} a"),
      njs_str("A123DT") },

    { njs_str("var t; "
                 "switch ($r3.uri) {"
                 "case 'abc': "
                 "  t='A'; "
                 "  break; "
                 "default: "
                 "  t='F'; "
                 "}; t"),
      njs_str("A") },

    { njs_str("[isNaN, undefined, isFinite]."
              "map((v)=>{switch(v) { case isNaN: return 1; default: return 0;}})"),
      njs_str("1,0,0") },

    /* continue. */

    { njs_str("continue"),
      njs_str("SyntaxError: Illegal continue statement in 1") },

    { njs_str("\n{\ncontinue;\n}"),
      njs_str("SyntaxError: Illegal continue statement in 3") },

    { njs_str("do continue while (false)"),
      njs_str("SyntaxError: Unexpected token \"while\" in 1") },

    { njs_str("do continue; while (false)"),
      njs_str("undefined") },

    { njs_str("do { continue } while (false)"),
      njs_str("undefined") },

    { njs_str("var i = 0; do if (i++ > 9) continue; while (i < 100); i"),
      njs_str("100") },

    { njs_str("while (false) continue"),
      njs_str("undefined") },

    { njs_str("while (false) continue;"),
      njs_str("undefined") },

    { njs_str("while (false) { continue }"),
      njs_str("undefined") },

    { njs_str("var i = 0; while (i < 100) if (i++ > 9) continue; i"),
      njs_str("100") },

    { njs_str("for ( ;null; ) continue"),
      njs_str("undefined") },

    { njs_str("for ( ;null; ) continue;"),
      njs_str("undefined") },

    { njs_str("for ( ;null; ) { continue }"),
      njs_str("undefined") },

    { njs_str("var i; for (i = 0; i < 100; i++) if (i > 9) continue; i"),
      njs_str("100") },

    { njs_str("var a = [], i; for (i in a) continue"),
      njs_str("undefined") },

    { njs_str("var a = [], i; for (i in a) continue;"),
      njs_str("undefined") },

    { njs_str("var a = [], i; for (i in a) { continue }"),
      njs_str("undefined") },

    { njs_str("var a = [1,2,3,4,5]; var s = 0, i;"
                 "for (i in a) { if (a[i] > 4) continue; else s += a[i] } s"),
      njs_str("10") },

    { njs_str("var a = [1,2,3,4,5]; var s = 0, i;"
                 "for (i in a) { if (a[i] > 4) continue; s += a[i] } s"),
      njs_str("10") },

    { njs_str("var a; for (a = 1; a; a--) switch (a) { case 0: continue }"),
      njs_str("undefined") },

    { njs_str("var a = [1,2,3], i; for (i in a) {Object.seal({})}"),
      njs_str("undefined") },

    { njs_str("var i; for (i in [1,2,3]) {Object.seal({});}"),
      njs_str("undefined") },

    /* break. */

    { njs_str("break"),
      njs_str("SyntaxError: Illegal break statement in 1") },

    { njs_str("{break}"),
      njs_str("SyntaxError: Illegal break statement in 1") },

    { njs_str("\nbreak"),
      njs_str("SyntaxError: Illegal break statement in 2") },

    { njs_str("do break while (true)"),
      njs_str("SyntaxError: Unexpected token \"while\" in 1") },

    { njs_str("do break; while (true)"),
      njs_str("undefined") },

    { njs_str("do { break } while (true)"),
      njs_str("undefined") },

    { njs_str("var i = 0; do if (i++ > 9) break; while (i < 100); i"),
      njs_str("11") },

    { njs_str("while (true) break"),
      njs_str("undefined") },

    { njs_str("while (true) break;"),
      njs_str("undefined") },

    { njs_str("while (true) { break }"),
      njs_str("undefined") },

    { njs_str("var i = 0; while (i < 100) if (i++ > 9) break; i"),
      njs_str("11") },

    { njs_str("for ( ;; ) break"),
      njs_str("undefined") },

    { njs_str("for ( ;; ) break;"),
      njs_str("undefined") },

    { njs_str("for ( ;; ) { break }"),
      njs_str("undefined") },

    { njs_str("var i; for (i = 0; i < 100; i++) if (i > 9) break; i"),
      njs_str("10") },

    { njs_str("var a = [], i; for (i in a) break"),
      njs_str("undefined") },

    { njs_str("var a = [], i; for (i in a) break;"),
      njs_str("undefined") },

    { njs_str("var a = [], i; for (i in a) { break }"),
      njs_str("undefined") },

    { njs_str("var a = [1,2,3,4,5]; var s = 0, i;"
                 "for (i in a) { if (a[i] > 4) break; else s += a[i] } s"),
      njs_str("10") },

    { njs_str("var a = [1,2,3,4,5]; var s = 0, i;"
                 "for (i in a) { if (a[i] > 4) break; s += a[i] } s"),
      njs_str("10") },

    { njs_str("var a = [1,2,3,4,5]; var s = 0, i;"
                 "for (i in a) if (a[i] > 4) break; s += a[i]; s"),
      njs_str("5") },

    /* Labels. */

    { njs_str("var n = 0; a:{n++}; a:{n++}; n"),
      njs_str("2") },

    { njs_str("a: throw 'a'"),
      njs_str("a") },

    { njs_str("a\n:\n1"),
      njs_str("1") },

    { njs_str("a\n\n:1"),
      njs_str("1") },

    { njs_str("a:\n\n1"),
      njs_str("1") },

    { njs_str("a:\n\n"),
      njs_str("SyntaxError: Unexpected end of input in 3") },

    { njs_str("a : var n = 0; b :++n"),
      njs_str("1") },

    { njs_str("a:{a:1}"),
      njs_str("SyntaxError: Label \"a\" has already been declared in 1") },

    { njs_str("for (var i in [1]) {break b}"),
      njs_str("SyntaxError: Undefined label \"b\" in 1") },

    { njs_str("for (var i in [1]) {continue b}"),
      njs_str("SyntaxError: Undefined label \"b\" in 1") },

    { njs_str("a:{break b}"),
      njs_str("SyntaxError: Undefined label \"b\" in 1") },

    { njs_str("a:{continue b}"),
      njs_str("SyntaxError: Undefined label \"b\" in 1") },

#if 0 /* TODO */
    { njs_str("a:{1; break a}"),
      njs_str("1") },
#endif

    { njs_str("var a = 0; a:{a++}; a"),
      njs_str("1") },

    { njs_str("var a = 0; a:{break a; a++}; a"),
      njs_str("0") },

    { njs_str("var r = 0; "
                 "out: for (var i in [1,2,3]) { if (i == 2) {break out;}; r++}; r"),
      njs_str("2") },

    { njs_str("var r = 0; "
                 "out: for (var i = 0; i < 5; i++) { if (i == 2) {break out;}; r++}; r"),
      njs_str("2") },

    { njs_str("var l1 = 0, l2 = 0; "
                 "out: "
                 "for (var i in [1,2,3]) { "
                 "  for (var j in [1,2,3]) { "
                 "    if (i == 1 && j == 1) {break;}"
                 "    l2++;"
                 "  }"
                 "  l1++;"
                 "}; [l1, l2]"),
      njs_str("3,7") },

    { njs_str("var l1 = 0, l2 = 0; "
                 "out: "
                 "for (var i in [1,2,3]) { "
                 "  for (var j in [1,2,3]) { "
                 "    if (i == 1 && j == 1) {break out;}"
                 "    l2++;"
                 "  }"
                 "  l1++;"
                 "}; [l1, l2]"),
      njs_str("1,4") },

    { njs_str("var l1 = 0, l2 = 0; "
                 "out: "
                 "for (var i in [1,2,3]) { "
                 "  for (var j in [1,2,3]) { "
                 "    if (i == 1 && j == 1) {continue out;}"
                 "    l2++;"
                 "  }"
                 "  l1++;"
                 "}; [l1, l2]"),
      njs_str("2,7") },

    { njs_str("var l1 = 0, l2 = 0; "
                 "out: "
                 "for (var i in [1,2,3]) { "
                 "  l1++;"
                 "  switch (i) { "
                 "    case '1':"
                 "      break out;"
                 "    default:"
                 "  }"
                 "  l2++;"
                 "}; [l1, l2]"),
      njs_str("2,1") },

    { njs_str("var l1 = 0, l2 = 0; "
                 "out: "
                 "for (var i in [1,2,3]) { "
                 "  l1++;"
                 "  switch (i) { "
                 "    case '1':"
                 "      continue out;"
                 "    default:"
                 "  }"
                 "  l2++;"
                 "}; [l1, l2]"),
      njs_str("3,2") },

    { njs_str("var l1 = 0, l2 = 0, i = 0, j; "
                 "out: "
                 "while (i < 3) { "
                 "  j = 0;"
                 "  while (j < 3) { "
                 "    if (i == 1 && j == 1) {break out;}"
                 "    l2++;"
                 "    j++;"
                 "  }"
                 "  l1++;"
                 "  i++;"
                 "}; [l1, l2]"),
      njs_str("1,4") },

    { njs_str("var l1 = 0, l2 = 0, i = 0, j; "
                 "out: "
                 "while (i < 3) { "
                 "  j = 0;"
                 "  while (j < 3) { "
                 "    if (i == 1 && j == 1) {i++; continue out;}"
                 "    l2++;"
                 "    j++;"
                 "  }"
                 "  l1++;"
                 "  i++;"
                 "}; [l1, l2]"),
      njs_str("2,7") },

    { njs_str("var l1 = 0, l2 = 0, i = 0, j; "
                 "out: "
                 "do { "
                 "  j = 0;"
                 "  do { "
                 "    if (i == 1 && j == 1) {break out;}"
                 "    l2++;"
                 "    j++;"
                 "  } while (j < 3)"
                 "  l1++;"
                 "  i++;"
                 "} while (i < 3); [l1, l2]"),
      njs_str("1,4") },

    { njs_str("var l1 = 0, l2 = 0, i = 0, j; "
                 "out: "
                 "do { "
                 "  j = 0;"
                 "  do { "
                 "    if (i == 1 && j == 1) {i++; continue out;}"
                 "    l2++;"
                 "    j++;"
                 "  } while (j < 3)"
                 "  l1++;"
                 "  i++;"
                 "} while (i < 3); [l1, l2]"),
      njs_str("2,7") },

    { njs_str("out1: while (1) { out2: while (1) { "
                 "  try { break out1; break out2; } catch (e) {}"
                 "}}"),
      njs_str("InternalError: break/return instructions with different labels "
                 "(\"out1\" vs \"out2\") from try-catch block are not supported") },

    { njs_str("out1: while (1) { out2: while (1) { "
                 "  try { } catch (e) {break out1; break out2;} finally {}"
                 "}}"),
      njs_str("InternalError: break/return instructions with different labels "
                 "(\"out1\" vs \"out2\") from try-catch block are not supported") },

    { njs_str("out1: while (1) { out2: while (1) { "
                 "  try { break out1; } catch (e) {break out2;} finally {}"
                 "}}"),
      njs_str("InternalError: try break/return instructions with different labels "
                 "(\"out1\" vs \"out2\") from try-catch block are not supported") },

    { njs_str("out1: while (1) { out2: while (1) { "
                 "  try { break out1; break out2; } finally {}"
                 "}}"),
      njs_str("InternalError: break/return instructions with different labels "
                 "(\"out1\" vs \"out2\") from try-catch block are not supported") },

    { njs_str("out1: while (1) { out2: while (1) { "
                 "  try { continue out1; continue out2; } catch (e) {}"
                 "}}"),
      njs_str("InternalError: continue instructions with different labels "
                 "(\"out1\" vs \"out2\") from try-catch block are not supported") },

    { njs_str("out1: while (1) { out2: while (1) { "
                 "  try { continue out1; } catch (e) {continue out2;} finally {}"
                 "}}"),
      njs_str("InternalError: try continue instructions with different labels "
                 "(\"out1\" vs \"out2\") from try-catch block are not supported") },

    { njs_str("function f() {"
                 "  a:{ try { try { return 'a'; } catch (e) {break a;} finally {} } "
                 "      catch (e) {} finally {}; }"
                 "}"),
      njs_str("InternalError: try break/return instructions with different labels "
                 "(\"@return\" vs \"a\") from try-catch block are not supported") },

    { njs_str("a:{ try { try { continue a; } catch (e) {} finally {} } "
                 "    catch (e) {} finally {}; "
                 "}"),
      njs_str("SyntaxError: Illegal continue statement in 1") },

    { njs_str("var i = 0, j = 0, r = 0;"
                 "out1: while (i < 3) "
                 "{ "
                 "  i++;"
                 "  out2: while (j < 3) { "
                 "      j++; try { break out1; } catch (e) {} finally {r++}"
                 "  }"
                 "}; [i, j, r]"),
      njs_str("1,1,1") },

    { njs_str("var i = 0, j = 0, r = 0;"
                 "out1: while (i < 3) "
                 "{ "
                 "  i++;"
                 "  out2: while (j < 3) { "
                 "      j++; try { continue out1; } catch (e) {} finally {r++}"
                 "  }"
                 "}; [i, j, r]"),
      njs_str("3,3,3") },

    { njs_str("var c=0,fin=0;"
                 "try {"
                 " while (c < 2) {"
                 "    try { c += 1; throw 'e';}"
                 "    finally { fin = 1; break;}"
                 "    fin = -1;"
                 "    c += 2;"
                 " }"
                 "} catch(e) {c = 10;}; [c, fin]"),
      njs_str("1,1") },

    /* jumping out of a nested try-catch block. */

    { njs_str("var r = 0; "
                 "function f () { try { try {return 'a';} finally { r++; }} "
                 "                finally { r++; } }; "
                 "[f(), r]"),
      njs_str("a,2") },

    { njs_str("function f(n) { "
                 "  var r1 = 0, r2 = 0, r3 = 0;"
                 "  a:{ try { try { "
                 "              if (n == 0) { break a; } "
                 "              if (n == 1) { throw 'a'; } "
                 "            } "
                 "            catch (e) { break a; } finally { r1++; } } "
                 "      catch (e) {} "
                 "      finally { r2++; } "
                 "      r3++;  "
                 "  }; "
                 "return [r1, r2, r3]"
                 "}; njs.dump([f(0), f(1), f(3)])"),
      njs_str("[[1,1,0],[1,1,0],[1,1,1]]") },

    /**/

    { njs_str("var i; for (i = 0; i < 10; i++) { i += 1 } i"),
      njs_str("10") },

    /* Factorial. */

    { njs_str("var n = 5, f = 1; while (n--) f *= n + 1; f"),
      njs_str("120") },

    { njs_str("var n = 5, f = 1; while (n) { f *= n; n-- } f"),
      njs_str("120") },

    /* Fibonacci. */

    { njs_str("var n = 50, x, i, j, k;"
                 "for(i=0,j=1,k=0; k<n; i=j,j=x,k++ ){ x=i+j } x"),
      njs_str("20365011074") },

    { njs_str("3 + 'abc' + 'def' + null + true + false + undefined"),
      njs_str("3abcdefnulltruefalseundefined") },

    { njs_str("var a = 0; do a++; while (a < 5) if (a == 5) a = 7.33 \n"
                 "else a = 8; while (a < 10) a++; a"),
      njs_str("10.33") },

    /* typeof. */

    { njs_str("typeof null"),
      njs_str("object") },

    { njs_str("typeof undefined"),
      njs_str("undefined") },

    { njs_str("typeof false"),
      njs_str("boolean") },

    { njs_str("typeof true"),
      njs_str("boolean") },

    { njs_str("typeof 0"),
      njs_str("number") },

    { njs_str("typeof -1"),
      njs_str("number") },

    { njs_str("typeof Infinity"),
      njs_str("number") },

    { njs_str("typeof NaN"),
      njs_str("number") },

    { njs_str("typeof 'a'"),
      njs_str("string") },

    { njs_str("typeof {}"),
      njs_str("object") },

    { njs_str("typeof Object()"),
      njs_str("object") },

    { njs_str("typeof []"),
      njs_str("object") },

    { njs_str("typeof function(){}"),
      njs_str("function") },

    { njs_str("typeof Object"),
      njs_str("function") },

    { njs_str("typeof /./i"),
      njs_str("object") },

    { njs_str("typeof Date.prototype"),
      njs_str("object") },

    { njs_str("typeof a"),
      njs_str("undefined") },

    { njs_str("typeof a; var a"),
      njs_str("undefined") },

    { njs_str("typeof a; var a; a"),
      njs_str("undefined") },

    { njs_str("var a = 5; typeof a"),
      njs_str("number") },

    { njs_str("function f() { return typeof a } ; f()"),
      njs_str("undefined") },

    { njs_str("typeof a; a"),
      njs_str("ReferenceError: \"a\" is not defined in 1") },

    { njs_str("typeof a; a = 1"),
      njs_str("ReferenceError: \"a\" is not defined in 1") },

    /**/

    { njs_str("void 0"),
      njs_str("undefined") },

    { njs_str("null = 1"),
      njs_str("ReferenceError: Invalid left-hand side in assignment in 1") },

#if 0 /* ES5FIX */
    { njs_str("undefined = 1"),
      njs_str("TypeError: Cannot assign to read-only property "undefined" of object") },
#endif

    { njs_str("null++"),
      njs_str("ReferenceError: Invalid left-hand side in postfix operation in 1") },

    { njs_str("++null"),
      njs_str("ReferenceError: Invalid left-hand side in prefix operation in 1") },

    { njs_str("var a, b; b = a; a = 1; a +' '+ b"),
      njs_str("1 undefined") },

    { njs_str("a = 1"),
      njs_str("ReferenceError: \"a\" is not defined in 1") },

    { njs_str("var a; a = 1; a"),
      njs_str("1") },

    { njs_str("var a = {}; typeof a +' '+ a"),
      njs_str("object [object Object]") },

    { njs_str("var a = {}; a.b"),
      njs_str("undefined") },

    { njs_str("var a = {}; a.b = 1 + 2; a.b"),
      njs_str("3") },

    { njs_str("var a = {}; a['b']"),
      njs_str("undefined") },

    { njs_str("var a = {}; a.b.c"),
      njs_str("TypeError: cannot get property \"c\" of undefined") },

    { njs_str("'a'[0]"),
      njs_str("a") },

    { njs_str("'a'[undefined]"),
      njs_str("undefined") },

    { njs_str("'a'[null]"),
      njs_str("undefined") },

    { njs_str("'a'[false]"),
      njs_str("undefined") },

    { njs_str("'a'.b = 1"),
      njs_str("TypeError: property set on primitive string type") },

    { njs_str("'a'[2] = 1"),
      njs_str("TypeError: property set on primitive string type") },

    { njs_str("var a = {}; a.b = 1; a.b"),
      njs_str("1") },

    { njs_str("var a = {}; a.b = 1; a.b += 2"),
      njs_str("3") },

    { njs_str("var a = {}; a.b = 1; a.b += a.b"),
      njs_str("2") },

    { njs_str("var a = {}; a.b = 1; var x = {}; x.b = 3; a.b += (x.b = 2)"),
      njs_str("3") },

    { njs_str("var a = {}; a.b = 1; a.b += (a.b = 2)"),
      njs_str("3") },

    { njs_str("var a = {}; a.b += 1"),
      njs_str("NaN") },

    { njs_str("var a = 1; var b = 2; a = b += 1"),
      njs_str("3") },

    { njs_str("var a = 1; var b = { x:2 }; a = b.x += 1"),
      njs_str("3") },

    { njs_str("var a = 1; var b = { x:2 }; a = b.x += (a = 1)"),
      njs_str("3") },

    { njs_str("var a = undefined; a.b++; a.b"),
      njs_str("TypeError: cannot get property \"b\" of undefined") },

    { njs_str("var a = null; a.b++; a.b"),
      njs_str("TypeError: cannot get property \"b\" of undefined") },

    { njs_str("var a = true; a.b++; a.b"),
      njs_str("TypeError: property set on primitive boolean type") },

    { njs_str("var a = 1; a.b++; a.b"),
      njs_str("TypeError: property set on primitive number type") },

    { njs_str("var n = 1, o = { p: n += 1 }; o.p"),
      njs_str("2") },

    { njs_str("var a = {}; a.b = {}; a.b.c = 1; a.b['c']"),
      njs_str("1") },

    { njs_str("var a = {}; a.b = {}; a.b.c = 1; a['b']['c']"),
      njs_str("1") },

    { njs_str("var a = {}; a.b = {}; var c = 'd'; a.b.d = 1; a['b'][c]"),
      njs_str("1") },

    { njs_str("var a = {}; a.b = 1; var c = a.b++; a.b +' '+ c"),
      njs_str("2 1") },

    { njs_str("var a = 2; a.b = 1; var c = a.b++; a +' '+ a.b +' '+ c"),
      njs_str("TypeError: property set on primitive number type") },

    { njs_str("var x = { a: 1 }; x.a"),
      njs_str("1") },

    { njs_str("var a = { x:1 }; var b = { y:2 }; a.x = b.y; a.x"),
      njs_str("2") },

    { njs_str("var a = { x:1 }; var b = { y:2 }; var c; c = a.x = b.y"),
      njs_str("2") },

    { njs_str("var a = { x:1 }; var b = { y:2 }; var c = a.x = b.y; c"),
      njs_str("2") },

    { njs_str("var a = { x:1 }; var b = { y:2 }; a.x = b.y"),
      njs_str("2") },

    { njs_str("var a = { x:1 }; var b = a.x = 1 + 2; a.x +' '+ b"),
      njs_str("3 3") },

    { njs_str("var a = { x:1 }; var b = { y:2 }; var c = {};"
                 "c.x = a.x = b.y; c.x"),
      njs_str("2") },

    { njs_str("var y = 2, x = { a:1, b: y + 5, c:3 };"
                 "x.a +' '+ x.b +' '+ x.c"),
      njs_str("1 7 3") },

    { njs_str("var x = { a: 1, b: { a:2, c:5 } };"
                 "x.a +' '+ x.b.a +' '+ x.b.c"),
      njs_str("1 2 5") },

    { njs_str("var y = 5, x = { a:y }; x.a"),
      njs_str("5") },

    { njs_str("var x = { a: 1; b: 2 }"),
      njs_str("SyntaxError: Unexpected token \";\" in 1") },

    { njs_str("var x = { a: 1, b: x.a }"),
      njs_str("TypeError: cannot get property \"a\" of undefined") },

    { njs_str("var a = { b: 2 }; a.b += 1"),
      njs_str("3") },

    { njs_str("var o = {a:1}, c = o; o.a = o = {b:5};"
                 "o.a +' '+ o.b +' '+ c.a.b"),
      njs_str("undefined 5 5") },

    { njs_str("var y = { a: 2 }, x = { a: 1, b: y.a }; x.a +' '+ x.b"),
      njs_str("1 2") },

    { njs_str("var y = { a: 1 }, x = { a: y.a++, b: y.a++ }\n"
                 "x.a +' '+ x.b +' '+ y.a"),
      njs_str("1 2 3") },

    { njs_str("var a='', o = {a:1, b:2}, p;"
                 "for (p in o) { a += p +':'+ o[p] +',' } a"),
      njs_str("a:1,b:2,") },

    { njs_str("var x = { a: 1 }, b = delete x.a; x.a +' '+ b"),
      njs_str("undefined true") },

    /* Object shorthand property. */

    { njs_str("var a = 1; njs.dump({a})"),
      njs_str("{a:1}") },

    { njs_str("var a = 1, b; njs.dump({a,b})"),
      njs_str("{a:1,b:undefined}") },

    { njs_str("var a = 1, b = 2; ({a,b,c})"),
      njs_str("ReferenceError: \"c\" is not defined in 1") },

    { njs_str("var a = 1, b = 2; njs.dump({a,b,c:3})"),
      njs_str("{a:1,b:2,c:3}") },

    { njs_str("var b = 2, c = 3; njs.dump({a:1,b,c})"),
      njs_str("{a:1,b:2,c:3}") },

    { njs_str("({1})"),
      njs_str("SyntaxError: Unexpected token \"}\" in 1") },

    { njs_str("({default})"),
      njs_str("SyntaxError: Unexpected token \"}\" in 1") },

    { njs_str("({var})"),
      njs_str("SyntaxError: Unexpected token \"}\" in 1") },

    { njs_str("({this})"),
        njs_str("SyntaxError: Unexpected token \"}\" in 1") },

    { njs_str("typeof ({Number}).Number"),
      njs_str("function") },

    { njs_str("typeof ({eval}).eval"),
      njs_str("function") },

    { njs_str("typeof ({Math}).Math.sin"),
      njs_str("function") },

    { njs_str("delete null"),
      njs_str("true") },

    { njs_str("var a; delete a"),
      njs_str("SyntaxError: Delete of an unqualified identifier in 1") },

    { njs_str("delete undefined"),
      njs_str("SyntaxError: Delete of an unqualified identifier in 1") },

    /* Object shorthand methods. */

    { njs_str("var o = {m(){}}; new o.m();"),
      njs_str("TypeError: function is not a constructor") },

    { njs_str("var o = {sum(a, b){return a + b;}}; o.sum(1, 2)"),
      njs_str("3") },

    /* Object computed property. */

    { njs_str("var o = { [0]: 1, [-0]: 2 }; o[0];"),
      njs_str("2") },

    { njs_str("var k = 'abc'.split('');var o = {[k[0]]: 'baz'}; o.a"),
      njs_str("baz") },

    { njs_str("var k = {}; var o = {[k]() {return 'baz'}}; o[k]()"),
      njs_str("baz") },

    { njs_str("njs.dump({[{toString(){return 'xx'}}]:1})"),
      njs_str("{xx:1}") },

    { njs_str("var o = {}; Object.defineProperty(o, 'toString', {value:()=>'xx'});"
                 "njs.dump({[o]:1})"),
      njs_str("{xx:1}") },

    { njs_str("({[{toString(){return {}}}]:1})"),
      njs_str("TypeError: Cannot convert object to primitive value") },

    { njs_str("var o = { [new Number(12345)]: 1000 }; o[12345]"),
      njs_str("1000") },

    { njs_str("delete NaN"),
      njs_str("SyntaxError: Delete of an unqualified identifier in 1") },

    { njs_str("delete Infinity"),
      njs_str("SyntaxError: Delete of an unqualified identifier in 1") },

    { njs_str("delete -Infinity"),
      njs_str("true") },

    { njs_str("delete (1/0)"),
      njs_str("true") },

    { njs_str("delete 1"),
      njs_str("true") },

    { njs_str("var a = []; delete a[1]"),
      njs_str("true") },

    { njs_str("var o = {}; [delete o.m, delete o.m]"),
      njs_str("true,true") },

    { njs_str("[delete Array.nonexistent, delete Array.Array]"),
      njs_str("true,true") },

    { njs_str("var a; delete (a = 1); a"),
      njs_str("1") },

    { njs_str("delete a"),
      njs_str("SyntaxError: Delete of an unqualified identifier in 1") },

    { njs_str("var a = 1; delete a"),
      njs_str("SyntaxError: Delete of an unqualified identifier in 1") },

    { njs_str("function f(){} delete f"),
      njs_str("SyntaxError: Delete of an unqualified identifier in 1") },

    { njs_str("var a = { x:1 }; ('x' in a) +' '+ (1 in a)"),
      njs_str("true false") },

    { njs_str("delete --[][1]"),
      njs_str("true") },

    { njs_str("var a = [1,2]; delete a.length"),
      njs_str("TypeError: Cannot delete property \"length\" of array") },

    { njs_str("var a = [1,2,3]; a.x = 10;  delete a[1]"),
      njs_str("true") },

    { njs_str("var o = Object.create({a:1}); o.a = 2; delete o.a; o.a"),
      njs_str("1") },

    { njs_str("delete Array.name"),
      njs_str("true") },

    { njs_str("delete Math.max"),
      njs_str("true") },

    { njs_str("delete Math.max.length"),
      njs_str("true") },

    { njs_str("function f(a,b) {} "
                 "[f.length, delete f.length, f.length, delete f.length]"),
      njs_str("2,true,0,true") },

    { njs_str("njs.dump({break:1,3:2,'a':4,\"b\":2,true:1,null:0})"),
      njs_str("{break:1,3:2,a:4,b:2,true:1,null:0}") },

    { njs_str("var o1 = {a:1,b:2}, o2 = {c:3}; o1.a + o2.c"),
      njs_str("4") },

    { njs_str("({[]:1})"),
      njs_str("SyntaxError: Unexpected token \"]\" in 1") },

    { njs_str("({'AB\\ncd':1})['AB\\ncd']"),
      njs_str("1") },

    /* Inheritance. */

    { njs_str("function Foo() {this.bar = 10;}; Foo.prototype.bar = 42; "
                 "var v = new Foo(); delete v.bar; v.bar"),
      njs_str("42") },

    { njs_str("function Cl(x,y) {this.x = x; this.y = y}; "
                 "var c = new Cl('a', 'b'); Cl.prototype.z = 1; c.z"),
      njs_str("1") },

    /**/

    { njs_str("delete Math.E"),
      njs_str("TypeError: Cannot delete property \"E\" of object") },

    { njs_str("Math.E = 1"),
      njs_str("TypeError: Cannot assign to read-only property \"E\" of object") },

    /* "in" operation. */

    { njs_str("var o = { 'a': 1, 'b': 2 }; var i; "
                 "for (i in o) { delete o.a; delete o.b; }; njs.dump(o)"),
      njs_str("{}") },

    { njs_str("var o  = {}; Object.defineProperty(o, 'a', {value:1, configurable:1}); "
                 "delete o.a; o.a=2; o.a"),
      njs_str("2") },

    { njs_str("var a = {}; 1 in a"),
      njs_str("false") },

    { njs_str("'a' in {a:1}"),
      njs_str("true") },

    { njs_str("Symbol.unscopables in { [Symbol.unscopables]: 1 }"),
      njs_str("true") },

    { njs_str("Object(Symbol.toStringTag) in Math"),
      njs_str("true") },

    { njs_str("'1' in [0,,2]"),
      njs_str("false") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {get:()=>und}); 'a' in o"),
      njs_str("true") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {value:1}); ({toString(){return 'a'}}) in o"),
      njs_str("true") },

    { njs_str("'a' in Object.create({a:1})"),
      njs_str("true") },

    { njs_str("[false, NaN, '', Symbol()]"
              ".map((x) => { "
              "    try { 'toString' in x; } "
              "    catch (e) { return e instanceof TypeError ? e.message : ''; } "
              "})"
              ".every((x) => x.startsWith('property \"in\" on primitive'))"),
      njs_str("true") },

    { njs_str("var p = new String('test');"
              "p.toString = () => { throw new Error('failed') };"
              "p in 1"),
      njs_str("TypeError: property \"in\" on primitive number type") },

    { njs_str("var n = { toString: function() { return 'a' } };"
                 "var o = { a: 5 }; o[n]"),
      njs_str("5") },

    { njs_str("var n = { valueOf: function() { return 'a' } };"
                 "var o = { a: 5, '[object Object]': 7 }; o[n]"),
      njs_str("7") },

    { njs_str("var o = {}; o.new = 'OK'; o.new"),
      njs_str("OK") },

    { njs_str("var o = { new: 'OK'}; o.new"),
      njs_str("OK") },

    /* Arrays */

    /* Empty array to primitive. */

    { njs_str("3 + []"),
      njs_str("3") },

    { njs_str("3 * []"),
      njs_str("0") },

    /* Single element array to primitive. */

    { njs_str("3 + [5]"),
      njs_str("35") },

    { njs_str("3 * [5]"),
      njs_str("15") },

    /* Array to primitive. */

    { njs_str("3 + [5,7]"),
      njs_str("35,7") },

    { njs_str("3 * [5,7]"),
      njs_str("NaN") },

    { njs_str("var a = [ 1, 2, 3 ]; a[0] + a[1] + a[2]"),
      njs_str("6") },

    { njs_str("var a = [ 1, 2, 3 ]; a[-1] = 4; a + a[-1]"),
      njs_str("1,2,34") },

    { njs_str("var a = [ 1, 2, 3 ]; a[4294967295] = 4; a + a[4294967295]"),
      njs_str("1,2,34") },

    { njs_str("var a = [ 1, 2, 3 ]; a[4294967296] = 4; a + a[4294967296]"),
      njs_str("1,2,34") },

    { njs_str("delete[]['4e9']"),
      njs_str("true") },

    { njs_str("var n = 1, a = [ n += 1 ]; a"),
      njs_str("2") },

    { njs_str("var a = [ 1, 2; 3 ]; a[0] + a[1] + a[2]"),
      njs_str("SyntaxError: Unexpected token \";\" in 1") },

    { njs_str("var a = [ 1, 2, 3 ]; a[0] +' '+ a[1] +' '+ a[2] +' '+ a[3]"),
      njs_str("1 2 3 undefined") },

    { njs_str("var a = [ 5, 6, 7 ]; a['1']"),
      njs_str("6") },

    { njs_str("var a = [ 5, 6, 7 ]; a['01']"),
      njs_str("undefined") },

    { njs_str("var a = [ 5, 6, 7 ]; a[0x1]"),
      njs_str("6") },

    { njs_str("var a = [ 5, 6, 7 ]; a['0x1']"),
      njs_str("undefined") },

    { njs_str("[] - 2"),
      njs_str("-2") },

    { njs_str("[1] - 2"),
      njs_str("-1") },

    { njs_str("[[1]] - 2"),
      njs_str("-1") },

    { njs_str("[[[1]]] - 2"),
      njs_str("-1") },

    { njs_str("var a = []; a - 2"),
      njs_str("-2") },

    { njs_str("var a = [1]; a - 2"),
      njs_str("-1") },

    { njs_str("var a = []; a[0] = 1; a - 2"),
      njs_str("-1") },

    { njs_str("[] + 2 + 3"),
      njs_str("23") },

    { njs_str("[1] + 2 + 3"),
      njs_str("123") },

    { njs_str("var a = []; a + 2 + 3"),
      njs_str("23") },

    { njs_str("var a = [1]; a + 2 + 3"),
      njs_str("123") },

    { njs_str("var a = [1,2], i = 0; a[i++] += a[0] = 5 + i;"
                 "a[0] +' '+ a[1]"),
      njs_str("7 2") },

    { njs_str("var a = []; a[0] = 1; a + 2 + 3"),
      njs_str("123") },

    { njs_str("var a = []; a['0'] = 1; a + 2 + 3"),
      njs_str("123") },

    { njs_str("var a = []; a[2] = 1; a[2]"),
      njs_str("1") },

    { njs_str("var a = [1, 2]; 1 in a"),
      njs_str("true") },

    { njs_str("var a = [1, 2]; 2 in a"),
      njs_str("false") },

    { njs_str("var a = [1, 2]; delete a[0]; 0 in a"),
      njs_str("false") },

    { njs_str("var a = [ function(a) {return a + 1} ]; a[0](5)"),
      njs_str("6") },

    { njs_str("var s = '', a = [5,1,2], i;"
                 "a[null] = null;"
                 "a[undefined] = 'defined';"
                 "a[false] = false;"
                 "a[true] = true;"
                 "a[-0] = 0;"
                 "a[Infinity] = Infinity;"
                 "a[-Infinity] = -Infinity;"
                 "a[NaN] = NaN;"
                 "a[-NaN] = -NaN;"
                 "for (i in a) { s += i +':'+ a[i] +',' } s"),
      njs_str("0:0,1:1,2:2,null:null,undefined:defined,false:false,"
                 "true:true,Infinity:Infinity,-Infinity:-Infinity,NaN:NaN,") },

    { njs_str("--[][3e9]"),
      njs_str("MemoryError") },

    { njs_str("[].length"),
      njs_str("0") },

    { njs_str("[1,2].length"),
      njs_str("2") },

    { njs_str("var a = [1,2]; a.length"),
      njs_str("2") },

    { njs_str("[\n1]"),
      njs_str("1") },

    { njs_str("\n[\n1\n]"),
      njs_str("1") },

    { njs_str("\n[\n1\n,\n2]\n[\n0]"),
      njs_str("1") },

    { njs_str("Object.create([1,2]).length"),
      njs_str("2") },

    { njs_str("Object.create(['α','β'])[1]"),
      njs_str("β") },

    { njs_str("Object.create(['α','β'])[false]"),
      njs_str("undefined") },

    { njs_str("var a = ['abc']; var o = Object.create(a); o[0] = 32;"
                 "[a,o[0]]"),
      njs_str("abc,32") },

    /* Array.length setter */

    { njs_str("[].length = {}"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("[].length = 2**32 - 1"),
      njs_str("MemoryError") },

    { njs_str("[].length = 3e9"),
      njs_str("MemoryError") },

    { njs_str("Object.defineProperty([], 'length',{value: 2**32 - 1})"),
      njs_str("MemoryError") },

    { njs_str("[].length = 2**32"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("[].length = 2**32 + 1"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("[].length = -1"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("var a = [1];"
                 "typeof (a.length = '') == 'string' && a.length == 0"),
      njs_str("true") },

    { njs_str("var a = [1]; "
                 "typeof (a.length = Object(2)) == 'object' && a.length == 2"),
      njs_str("true") },

    { njs_str("var a = [1]; "
                 "typeof (a.length = Object('2')) == 'object'"),
      njs_str("true") },

    { njs_str("var a = [1]; "
                 "a.length = { valueOf: () => 2 }; a.length == 2"),
      njs_str("true") },

    { njs_str("var a = [1]; "
                 "a.length = { toString: () => '2' }; a.length == 2"),
      njs_str("true") },

    { njs_str("var a = []; a.length = 0; JSON.stringify(a)"),
      njs_str("[]") },

    { njs_str("var a = []; a.length = 1; JSON.stringify(a)"),
      njs_str("[null]") },

    { njs_str("var a = [1]; a.length = 1; JSON.stringify(a)"),
      njs_str("[1]") },

    { njs_str("var a = [1]; a.length = 2; JSON.stringify(a)"),
      njs_str("[1,null]") },

    { njs_str("var a = [1]; a.length = 4; a.length = 0; JSON.stringify(a)"),
      njs_str("[]") },

    { njs_str("var a = [1,2,3]; a.length = 2; JSON.stringify(a)"),
      njs_str("[1,2]") },

    { njs_str("var a = [1,2,3]; a.length = 3; a"),
      njs_str("1,2,3") },

    { njs_str("var a = [1,2,3]; a.length = 16; a"),
      njs_str("1,2,3,,,,,,,,,,,,,") },

    { njs_str("var a = [1,2,3]; a.join()"),
      njs_str("1,2,3") },

    { njs_str("var a = [1,2,3]; a.join(':')"),
      njs_str("1:2:3") },

    { njs_str("var a = ['#', '@']; var out= a.join('α'.repeat(33)); [out, out.length]"),
      njs_str("#ααααααααααααααααααααααααααααααααα@,35") },

    { njs_str("var a = ['β', 'γ']; var out= a.join('α'); [out, out.length]"),
      njs_str("βαγ,3") },

    { njs_str("var a = []; a[5] = 5; a.join()"),
      njs_str(",,,,,5") },

    { njs_str("var a = [,null,undefined,false,true,0,1]; a.join()"),
      njs_str(",,,false,true,0,1") },

    { njs_str("var o = { toString: function() { return null } };"
                 "[o].join()"),
      njs_str("null") },

    { njs_str("var o = { toString: function() { return undefined } };"
                 "[o].join()"),
      njs_str("undefined") },

    { njs_str("var a = []; a[5] = 5; a"),
      njs_str(",,,,,5") },

    { njs_str("var a = []; a.concat([])"),
      njs_str("") },

    { njs_str("var s = { toString: function() { return 'S' } };"
                 "var v = { toString: 8, valueOf: function() { return 'V' } };"
                 "var o = [9]; o.join = function() { return 'O' };"
                 "var a = [1,2,3,[4,5,6],s,v,o]; a.join('')"),
      njs_str("1234,5,6SVO") },

    { njs_str("var s = { toString: function() { return 'S' } };"
                 "var v = { toString: 8, valueOf: function() { return 'V' } };"
                 "var o = [9]; o.join = function() { return 'O' };"
                 "var a = [1,2,3,[4,5,6],s,v,o]; a"),
      njs_str("1,2,3,4,5,6,S,V,O") },

    /* Array.toString(). */

    { njs_str("var a = [1,2,3]; a.join = 'NO';"
                 "Object.prototype.toString = function () { return 'A' }; a"),
      njs_str("[object Array]") },

    { njs_str("Array.prototype.toString.call(1)"),
      njs_str("[object Number]") },

    { njs_str("Array.prototype.toString.call('abc')"),
      njs_str("[object String]") },

    /* Empty array elements. */

    { njs_str("[,,]"),
      njs_str(",") },

    { njs_str("[,,,]"),
      njs_str(",,") },

    { njs_str("[1,2,]"),
      njs_str("1,2") },

    { njs_str("[1,2,,3]"),
      njs_str("1,2,,3") },

    { njs_str("[,,].length"),
      njs_str("2") },

    { njs_str("[,,,].length"),
      njs_str("3") },

    { njs_str("[1,2,,3].length"),
      njs_str("4") },

    /**/

    { njs_str("var n = { toString: function() { return 1 } };   [1,2][n]"),
      njs_str("2") },

    { njs_str("var n = { toString: function() { return '1' } }; [1,2][n]"),
      njs_str("2") },

    { njs_str("var n = { toString: function() { return 1 },"
                          " valueOf:  function() { return 0 } };   [1,2][n]"),
      njs_str("2") },

    { njs_str("var n = { toString: function() { return 1.5 } };"
                 "var a = [1,2]; a[1.5] = 5; a[n]"),
      njs_str("5") },

    { njs_str("var n = { toString: function() { return 1.5 } };"
                 "var a = [1,2]; a[n] = 5; a[1.5]"),
      njs_str("5") },

    { njs_str("var n = { toString: function() { return '1.5' } };"
                 "var a = [1,2]; a[1.5] = 5; a[n]"),
      njs_str("5") },

    { njs_str("var n = { toString: function() { return '1.5' } };"
                 "var a = [1,2]; a[n] = 5; a[1.5]"),
      njs_str("5") },

    { njs_str("var n = { toString: function() { return 1.5 } };"
                 "var a = [1,2]; a[1.5] = 5; n in a"),
      njs_str("true") },

    { njs_str("var n = { toString: function() { return '1.5' } };"
                 "var a = [1,2]; a[1.5] = 5; '' + (n in a) + (delete a[n])"),
      njs_str("truetrue") },

    /**/

    { njs_str("Array.isArray()"),
      njs_str("false") },

    { njs_str("Array.isArray(1)"),
      njs_str("false") },

    { njs_str("Array.isArray(1) ? 'true' : 'false'"),
      njs_str("false") },

    { njs_str("Array.isArray([])"),
      njs_str("true") },

    { njs_str("Array.isArray([]) ? 'true' : 'false'"),
      njs_str("true") },

    { njs_str("Array.of()"),
      njs_str("") },

    { njs_str("Array.of(1,2,3)"),
      njs_str("1,2,3") },

    { njs_str("Array.of(undefined,1)"),
      njs_str(",1") },

    { njs_str("Array.of(NaN,-1,{})"),
      njs_str("NaN,-1,[object Object]") },

    { njs_str("var a = [1,2,3]; a.concat(4, [5, 6, 7], 8)"),
      njs_str("1,2,3,4,5,6,7,8") },

    { njs_str("var a = []; a[100] = a.length; a[100] +' '+ a.length"),
      njs_str("0 101") },

    { njs_str("var a = [1,2]; a[100] = 100; a[100] +' '+ a.length"),
      njs_str("100 101") },

    { njs_str("var a = []; Object.defineProperty(a, 'length', {writable:0});"
              "Object.getOwnPropertyDescriptor(a, 'length').writable"),
      njs_str("false") },

    { njs_str("var a = []; Object.defineProperty(a, 'length', {writable:0});"
              "Object.defineProperty(a, 'length', {writable:true})"),
      njs_str("TypeError: Cannot redefine property: \"length\"") },

    { njs_str("[1, 2, 3, 4, 5].copyWithin(0, 3)"),
      njs_str("4,5,3,4,5") },

    { njs_str("[1, 2, 3, 4, 5].copyWithin(0, 3, 4)"),
      njs_str("4,2,3,4,5") },

    { njs_str("[1, 2, 3, 4, 5].copyWithin(0, -2, -1)"),
      njs_str("4,2,3,4,5") },

    { njs_str("[1, 2, 3, 4, 5].copyWithin(100, 200, 500)"),
      njs_str("1,2,3,4,5") },

    { njs_str("[0, 1, , , 1].copyWithin(0, 1, 4)"),
      njs_str("1,,,,1") },

    { njs_str("var o = [0, 1, , , 1].copyWithin(0, 1, 4); typeof o"),
      njs_str("object") },

    { njs_str("[].copyWithin.call({length: 5, 3: 1}, 0, 3)"),
      njs_str("[object Object]") },

    { njs_str("var o = [1, 2, 3, 4]; Object.defineProperties(o, { 5: {value: 'abc'}});"
              "[].copyWithin.call(o, 0, 3, 4);"),
      njs_str("4,2,3,4,,abc") },

    { njs_str("var obj = {length: 5, 3: 1}; [].copyWithin.call(obj, 0, 3);"
              "Object.keys(obj)"),
      njs_str("length,3,0") },

    { njs_str("var obj = {length: 5, 1: 'a', 2: 'b', 3: 'c', 4: 'd', 5: 'e'};"
              "[].copyWithin.call(obj, 0, -2, -1);"
              "Object.keys(obj) + '|' + Object.values(obj)"),
      njs_str("length,1,2,3,4,5,0|5,a,b,c,d,e,c") },

    { njs_str("Array.prototype.slice(1)"),
      njs_str("") },

    { njs_str("Array.prototype.slice(1,2)"),
      njs_str("") },

    { njs_str("Array.prototype.slice.call(undefined)"),
      njs_str("TypeError: cannot convert null or undefined to object") },

    { njs_str("Array.prototype.slice.call(1)"),
      njs_str("") },

    { njs_str("Array.prototype.slice.call(false)"),
      njs_str("") },

    { njs_str("Array.prototype.slice.call({'0':'a', '1':'b', length:1})"),
      njs_str("a") },

    { njs_str("Array.prototype.slice.call({'0':'a', '1':'b', length:2})"),
      njs_str("a,b") },

    { njs_str("Array.prototype.slice.call({'0':'a', '1':'b', length:4})"),
      njs_str("a,b,,") },

    { njs_str("Array.prototype.slice.call({'0':'a', '1':'b', length:2}, 1)"),
      njs_str("b") },

    { njs_str("Array.prototype.slice.call({'0':'a', '1':'b', length:2}, 1, 2)"),
      njs_str("b") },

    { njs_str("Array.prototype.slice.call({length:'2'})"),
      njs_str(",") },

    { njs_str("njs.dump(Array.prototype.slice.call({length: 3, 1: undefined }))"),
      njs_str("[<empty>,undefined,<empty>]") },

    { njs_str("Array.prototype.slice.call({length:new Number(3)})"),
      njs_str(",,") },

    { njs_str("Array.prototype.slice.call({length: { valueOf: function() { return 2; } }})"),
      njs_str(",") },

    { njs_str("Array.prototype.slice.call({ length: Object.create(null) })"),
      njs_str("TypeError: Cannot convert object to primitive value") },

    { njs_str("Array.prototype.slice.call({length:-1})"),
      njs_str("") },

    { njs_str("Array.prototype.slice.call('αβZγ')"),
      njs_str("α,β,Z,γ") },

    { njs_str("Array.prototype.slice.call(String.bytesFrom(Array(16).fill(0x9d)))[0].charCodeAt(0)"),
      njs_str("157") },

    { njs_str("Array.prototype.slice.call('αβZγ', 1)"),
      njs_str("β,Z,γ") },

    { njs_str("Array.prototype.slice.call('αβZγ', 2)"),
      njs_str("Z,γ") },

    { njs_str("Array.prototype.slice.call('αβZγ', 3)"),
      njs_str("γ") },

    { njs_str("Array.prototype.slice.call('αβZγ', 4)"),
      njs_str("") },

    { njs_str("Array.prototype.slice.call('αβZγ', 0, 1)"),
      njs_str("α") },

    { njs_str("Array.prototype.slice.call('αβZγ', 1, 2)"),
      njs_str("β") },

    { njs_str("Array.prototype.slice.call('αβZγ').length"),
      njs_str("4") },

    { njs_str("Array.prototype.slice.call('αβZγ')[1].length"),
      njs_str("1") },

    { njs_str("Array.prototype.slice.call(new String('αβZγ'))"),
      njs_str("α,β,Z,γ") },

    { njs_str("1..__proto__.length = '2';"
                 "Array.prototype.slice.call(1, 0, 2)"),
      njs_str(",") },

    { njs_str("Array.prototype.pop()"),
      njs_str("undefined") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c', 'length': 3};"
              "Array.prototype.pop.call(o); var res = o.length;"
              "Array.prototype.forEach.call(o, (v) => {res += ', ' + v}); res"),
      njs_str("2, a, b") },

    { njs_str("var obj = {}; obj.pop = Array.prototype.pop; obj.pop(); obj.length === 0"),
      njs_str("true") },

    { njs_str("var o = Object.freeze({0: 0, 1: 1, length: 2}); Array.prototype.pop.call(o)"),
      njs_str("TypeError: Cannot delete property \"1\" of object") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.pop.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: Cannot set property \"length\" of object which has only a getter") },

    { njs_str("Array.prototype.pop.call({ length: 3 })"),
      njs_str("undefined") },

    { njs_str("var o = { length: 3 }; Array.prototype.pop.call(o); o.length"),
      njs_str("2") },

    { njs_str("Array.prototype.shift()"),
      njs_str("undefined") },

    { njs_str("[0,1].slice()"),
      njs_str("0,1") },

    { njs_str("[0,1].slice(undefined)"),
      njs_str("0,1") },

    { njs_str("[0,1].slice(undefined, undefined)"),
      njs_str("0,1") },

    { njs_str("[0,1,2,3,4].slice(1,4)"),
      njs_str("1,2,3") },

    { njs_str("[0,1,2,3,4].slice(6,7)"),
      njs_str("") },

    { njs_str("var a = [1,2,3,4,5], b = a.slice(3);"
                 "b[0] +' '+ b[1] +' '+ b[2]"),
      njs_str("4 5 undefined") },

    { njs_str("var a = [1,2]; a.pop() +' '+ a.length +' '+ a"),
      njs_str("2 1 1") },

    { njs_str("var a = [1,2], len = a.push(3); len +' '+ a.pop() +' '+ a"),
      njs_str("3 3 1,2") },

    { njs_str("var a = [1,2], len = a.push(3,4,5);"
                 "len +' '+ a.pop() +' '+ a"),
      njs_str("5 5 1,2,3,4") },

    { njs_str("var x = {'0': 'a', '1': 'b', '2': 'c', 'length': 3};"
              "Array.prototype.push.call(x, 'x', 'y', 'z', 123) +' '+ x[0] +' '+ x.length"),
      njs_str("7 a 7") },

    { njs_str("var x = {'0': 'a', '1': 'b', '2': 'c', 'length': 3}; var a = [];"
              "Array.prototype.push.call(x, 'x', 'y', 'z', 123);"
              "Array.prototype.forEach.call(x, (v) => a.push(v)); a"),
      njs_str("a,b,c,x,y,z,123") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.push.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: Cannot set property \"length\" of object which has only a getter") },

    { njs_str("var a = [1,2,3]; a.shift() +' '+ a[0] +' '+ a.length"),
      njs_str("1 2 2") },

    { njs_str("var x = {'0': 'x', '1': 'y', '2': 'z', 'length': 3};"
              "Array.prototype.shift.call(x) +' '+ x[0] +' '+ x.length"),
      njs_str("x y 2") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.shift.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: Cannot set property \"length\" of object which has only a getter") },

    { njs_str("var a = [1,2], len = a.unshift(3);"
                 "len +' '+ a +' '+ a.shift()"),
      njs_str("3 3,1,2 3") },

    { njs_str("var a = [1,2], len = a.unshift(3,4,5);"
                 "len +' '+ a +' '+ a.shift()"),
      njs_str("5 3,4,5,1,2 3") },

    { njs_str("var x = {'0': 'x', '1': 'y', '2': 'z', 'length': 3}; var a = [];"
              "Array.prototype.unshift.call(x, 'a', 'b', 'c');"
              "Array.prototype.forEach.call(x, (v) => a.push(v)); a + ', ' + x.length"),
      njs_str("a,b,c,x,y,z, 6") },

    { njs_str("var x = {}; x.length = 1; var a = [];"
              "Array.prototype.unshift.call(x, 'a', 'b', 1025, 'c', 'oh my');"
              "Array.prototype.forEach.call(x, (v) => a.push(v)); a + ', ' + x.length"),
      njs_str("a,b,1025,c,oh my, 6") },

    { njs_str("var x = {5: 1, 6: 2, length: 7}; var a = [];"
              "Array.prototype.unshift.call(x, '0');"
              "Array.prototype.forEach.call(x, (v) => a.push(v)); a + ', ' + x.length"),
      njs_str("0,1,2, 8") },

    { njs_str("var x = {5: 2, 10: 3, 11: 4, 12: 5, 20: 6, length: 21}; var a = [];"
              "Array.prototype.unshift.call(x, '0', '1');"
              "Array.prototype.forEach.call(x, (v, k) => a.push(k + ':' + v)); a + ', ' + x.length"),
      njs_str("0:0,1:1,7:2,12:3,13:4,14:5,22:6, 23") },

    { njs_str("var x = {0: 0, length: 4294967294};"
              "Array.prototype.unshift.call(x, '0', '1');"),
      njs_str("TypeError: Invalid length") },

    { njs_str("var x = {0: 0}; Array.prototype.unshift.call(x); x.length"),
      njs_str("0") },

    { njs_str("var obj = {'10000000': 'x', '10000001': 'y', '10000002': 'z'}; var a = [];"
              "obj.length = 90000000;"
              "Array.prototype.unshift.call(obj, 'a', 'b', 'c');"
              "Array.prototype.forEach.call(obj, (v) => a.push(v)); a"),
      njs_str("a,b,c,x,y,z")},

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.unshift.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: Cannot set property \"length\" of object which has only a getter") },

    { njs_str("var a=[0], n = 64; while(--n) {a.push(n); a.shift()}; a"),
      njs_str("1") },

    { njs_str("Array.prototype.shift.call({ length: 3 })"),
      njs_str("undefined") },

    { njs_str("var o = { length: 3 }; Array.prototype.shift.call(o); o.length"),
      njs_str("2") },

    { njs_str("var a = []; a.splice()"),
      njs_str("") },

    { njs_str("[].splice(0,5,0)"),
      njs_str("") },

    { njs_str("[1,2,3,4,5].splice(-2,3,0)"),
      njs_str("4,5") },

    { njs_str("[].__proto__.splice(0,1,0)"),
      njs_str("") },

    { njs_str("var a = [];"
                 "a.splice(9,0,1,2).join(':') + '|' + a"),
      njs_str("|1,2") },

    { njs_str("var a = [0,1,2,3,4,5,6,7];"
                 "a.splice(3).join(':') + '|' + a"),
      njs_str("3:4:5:6:7|0,1,2") },

    { njs_str("var a = [0,1,2,3,4,5,6,7];"
                 "a.splice(3, 2).join(':') + '|' + a"),
      njs_str("3:4|0,1,2,5,6,7") },

    { njs_str("var a = [0,1,2,3,4,5,6,7];"
                 "a.splice(3, 2, 8, 9, 10, 11 ).join(':') + '|' + a"),
      njs_str("3:4|0,1,2,8,9,10,11,5,6,7") },

    { njs_str("var a = []; a.reverse()"),
      njs_str("") },

    { njs_str("var a = [1]; a.reverse()"),
      njs_str("1") },

    { njs_str("var a = [1,2]; a.reverse()"),
      njs_str("2,1") },

    { njs_str("var a = [1,2,3]; a.reverse()"),
      njs_str("3,2,1") },

    { njs_str("var a = [1,2,3,4]; a.reverse()"),
      njs_str("4,3,2,1") },

    { njs_str("var a = [1,2,3,4]; a.indexOf()"),
      njs_str("-1") },

    { njs_str("var a = [1,2,3,4]; a.indexOf(5)"),
      njs_str("-1") },

    { njs_str("var a = [1,2,3,4]; a.indexOf(4, 3)"),
      njs_str("3") },

    { njs_str("var a = [1,2,3,4]; a.indexOf(4, 4)"),
      njs_str("-1") },

    { njs_str("var a = [1,2,3,4,3,4]; a.indexOf(3, '2')"),
      njs_str("2") },

    { njs_str("var a = [1,2,3,4,3,4]; a.indexOf(4, -1)"),
      njs_str("5") },

    { njs_str("var a = [1,2,3,4,3,4]; a.indexOf(3, -10)"),
      njs_str("2") },

    { njs_str("[].indexOf.bind(0)(0, 0)"),
      njs_str("-1") },

    { njs_str("var o = 'abcd';"
              "Array.prototype.indexOf.call(o, 'c')"),
      njs_str("2") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "Array.prototype.indexOf.call(o, 'd')"),
      njs_str("3") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "Array.prototype.indexOf.call(o); i"),
      njs_str("1") },

    { njs_str("[].lastIndexOf(1, -1)"),
      njs_str("-1") },

    { njs_str("[undefined].lastIndexOf()"),
      njs_str("0") },

    { njs_str("[undefined].lastIndexOf(undefined)"),
      njs_str("0") },

    { njs_str("var a = [1,2,3,4]; a.lastIndexOf()"),
      njs_str("-1") },

    { njs_str("var a = [1,2,3,4]; a.lastIndexOf(5)"),
      njs_str("-1") },

    { njs_str("var a = [1,2,3,4,3,4]; a.lastIndexOf(1, 0)"),
      njs_str("0") },

    { njs_str("var a = [1,2,3,4,3,4]; a.lastIndexOf(3, '2')"),
      njs_str("2") },

    { njs_str("var a = [1,2,3,4,3,4]; a.lastIndexOf(1, 6)"),
      njs_str("0") },

    { njs_str("var a = [1,2,3,4,3,4]; a.lastIndexOf(2, 6)"),
      njs_str("1") },

    { njs_str("var a = [1,2,3,4,3,4]; a.lastIndexOf(4, -1)"),
      njs_str("5") },

    { njs_str("var a = [1,2,3,4,3,4]; a.lastIndexOf(4, -6)"),
      njs_str("-1") },

    { njs_str("var a = [1,2,3,4,3,4]; a.lastIndexOf(3, -10)"),
      njs_str("-1") },

    { njs_str("[1,2,1].lastIndexOf(2,undefined)"),
      njs_str("-1") },

    { njs_str("[1,2,1].lastIndexOf(1,undefined)"),
      njs_str("0") },

    { njs_str("[1,2,1].lastIndexOf(1)"),
      njs_str("2") },

    { njs_str("var o = 'addc';"
              "Array.prototype.lastIndexOf.call(o, 'd')"),
      njs_str("2") },

    { njs_str("var o = 'dddd';"
              "Array.prototype.lastIndexOf.call(o, 'd')"),
      njs_str("3") },

    { njs_str("var o = 'dabc';"
              "Array.prototype.lastIndexOf.call(o, 'd')"),
      njs_str("0") },

    { njs_str("var o = 'АБВГ';"
              "Array.prototype.lastIndexOf.call(o, 'Г')"),
      njs_str("3") },

    { njs_str("var o = 'ГВБА';"
              "Array.prototype.lastIndexOf.call(o, 'Г')"),
      njs_str("0") },

    { njs_str("var o = 'ВГБА';"
              "Array.prototype.lastIndexOf.call(o, 'Г')"),
      njs_str("1") },

    { njs_str("var o = {0: 'a', 1: 'd', 2: 'd'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "Array.prototype.lastIndexOf.call(o, 'd')"),
      njs_str("3") },

    { njs_str("var obj = {'10000000': 'x', '10000001': 'y', '10000002': 'z'}; var a = [];"
              "obj.length = 90000000;"
              "Array.prototype.lastIndexOf.call(obj, 'y');"),
      njs_str("10000001")},

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "Array.prototype.lastIndexOf.call(o); i"),
      njs_str("1") },

    { njs_str("[''].lastIndexOf.call('00000000000000000000000000000а00')"),
      njs_str("-1") },

    { njs_str("var o = 'ГВБА';"
              "Array.prototype.lastIndexOf.call(o, 'Г', 0)"),
      njs_str("0") },

    { njs_str("var o = 'ГВБА';"
              "Array.prototype.lastIndexOf.call(o, 'Г', 4)"),
      njs_str("0") },

    { njs_str("[1,2,3,4].includes()"),
      njs_str("false") },

    { njs_str("[1,2,3,4].includes(5)"),
      njs_str("false") },

    { njs_str("[1,2,3,4].includes(4, 3)"),
      njs_str("true") },

    { njs_str("[1,2,3,4].includes(4, 4)"),
      njs_str("false") },

    { njs_str("[1,2,3,4,3,4].includes(3, '2')"),
      njs_str("true") },

    { njs_str("[1,2,3,4,3,4].includes(4, -1)"),
      njs_str("true") },

    { njs_str("[1,2,3,4,3,4].includes(3, -10)"),
      njs_str("true") },

    { njs_str("[1,2,3,NaN,3,4].includes(NaN)"),
      njs_str("true") },

    { njs_str("[1,2,3,4,5].includes(NaN)"),
      njs_str("false") },

    { njs_str("[].includes.bind(0)(0, 0)"),
      njs_str("false") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "Array.prototype.includes.call(o, 'd')"),
      njs_str("true") },

    { njs_str("var obj = {'0': 'a', '1': 'b', '10000000': 'c', '10000001': 'd', '10000002': 'e'};"
              "var fromIndex = 1;"
              "obj.length = 90000000;"
              "Array.prototype.includes.call(obj, 'c', fromIndex);"),
      njs_str("true") },

    { njs_str("var obj = {'0': 'a', '1': 'b', '10000000': 'c', '10000001': 'd', '10000002': 'e'};"
              "var fromIndex = 1;"
              "obj.length = 90000000;"
              "Array.prototype.includes.call(obj, 'a', fromIndex);"),
      njs_str("false") },

    { njs_str("var a = []; var s = { sum: 0 };"
                 "a.forEach(function(v, i, a) { this.sum += v }, s); s.sum"),
      njs_str("0") },

    { njs_str("var a = new Array(3); var s = { sum: 0 };"
                 "a.forEach(function(v, i, a) { this.sum += v }, s); s.sum"),
      njs_str("0") },

    { njs_str("var a = [,,,]; var s = { sum: 0 };"
                 "a.forEach(function(v, i, a) { this.sum += v }, s); s.sum"),
      njs_str("0") },

    { njs_str("var a = [1,2,3]; var s = { sum: 0 };"
                 "a.forEach(function(v, i, a) { this.sum += v }, s); s.sum"),
      njs_str("6") },

    { njs_str("var a = [1,2,3];"
                 "a.forEach(function(v, i, a) { a[i+3] = a.length }); a"),
      njs_str("1,2,3,3,4,5") },

    { njs_str("function f() { var c; [1].forEach(function(v) { c })}; f()"),
      njs_str("undefined") },

    { njs_str("var a = [1,2,3]; var s = { sum: 0 };"
                 "[].forEach.call(a, function(v, i, a) { this.sum += v }, s);"
                 "s.sum"),
      njs_str("6") },

    { njs_str("var a = [1,2,3]; var s = { sum: 0 };"
                 "[].forEach.apply(a,"
                                  "[ function(v, i, a) { this.sum += v }, s ]);"
                 "s.sum"),
      njs_str("6") },

    { njs_str("var a = []; var c = 0;"
                 "a.forEach(function(v, i, a) { c++ }); c"),
      njs_str("0") },

    { njs_str("var a = [,,,,]; var c = 0;"
                 "a.forEach(function(v, i, a) { c++ }); c"),
      njs_str("0") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "var r = ''; Array.prototype.forEach.call(o, function(v, i, a) { r += v }); r"),
      njs_str("abcd") },

    { njs_str("var s = 't'; var t = '';"
              "Array.prototype.forEach.call(s, function (a, b, c) {t = typeof c;}); [t, typeof s];"),
      njs_str("object,string") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.forEach.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var a = [];"
                 "a.some(function(v, i, a) { return v > 1 })"),
      njs_str("false") },

    { njs_str("var a = [1,2,3];"
                 "a.some(function(v, i, a) { return v > 1 })"),
      njs_str("true") },

    { njs_str("var a = [1,2,3];"
                 "a.some(function(v, i, a) { return v > 2 })"),
      njs_str("true") },

    { njs_str("var a = [1,2,3];"
                 "a.some(function(v, i, a) { return v > 3 })"),
      njs_str("false") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c', 'length': { valueOf: function() { return 3 }}};"
              "var r = Array.prototype.some.call(o, function(el, i, arr) {return el == 'c'}); r"),
      njs_str("true") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'd', 'length': { valueOf: function() { return 3 }}};"
              "var r = Array.prototype.some.call(o, function(el, i, arr) {return el == 'c'}); r"),
      njs_str("false") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "var r = Array.prototype.some.call(o, function(v, i, a) { return v === 'd' }); r"),
      njs_str("true") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.some.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var a = [];"
                 "a.every(function(v, i, a) { return v > 1 })"),
      njs_str("true") },

    { njs_str("var a = [3,2,1];"
                 "a.every(function(v, i, a) { return v > 3 })"),
      njs_str("false") },

    { njs_str("var a = [3,2,1];"
                 "a.every(function(v, i, a) { return v > 2 })"),
      njs_str("false") },

    { njs_str("var a = [3,2,1];"
                 "a.every(function(v, i, a) { return v > 0 })"),
      njs_str("true") },

    { njs_str("var o = {0: 'c', 1: 'b', 2: 'c', 'length': { valueOf() { return 3 }}};"
              "var r = Array.prototype.every.call(o, function(el, i, arr) {return el == 'c'}); r"),
      njs_str("false") },

    { njs_str("var o = {0: 'c', 1: 'c', 2: 'c', 'length': { valueOf() { return 3 }}};"
              "var r = Array.prototype.every.call(o, function(el, i, arr) {return el == 'c'}); r"),
      njs_str("true") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.every.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var o = {0: 'x', 1: 'y', 2: 'z'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'a'});"
              "var r = Array.prototype.some.call(o, function(v, i, a) { return v === 'a' }); r"),
      njs_str("true") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'b'});"
              "var r = Array.prototype.some.call(o, function(v, i, a) { return v === 'y' }); r"),
      njs_str("false") },

    { njs_str("[].fill(1);"),
      njs_str("") },

    { njs_str("[1,2,3].fill(5);"),
      njs_str("5,5,5") },

    { njs_str("[1,2,3].fill(5, 0);"),
      njs_str("5,5,5") },

    { njs_str("[1,2,3].fill(5, 1);"),
      njs_str("1,5,5") },

    { njs_str("[1,2,3].fill(5, 4);"),
      njs_str("1,2,3") },

    { njs_str("[1,2,3].fill(5, -2);"),
      njs_str("1,5,5") },

    { njs_str("[1,2,3].fill(5, -3);"),
      njs_str("5,5,5") },

    { njs_str("[1,2,3].fill(5, -4);"),
      njs_str("5,5,5") },

    { njs_str("[1,2,3].fill(5, 1, 0);"),
      njs_str("1,2,3") },

    { njs_str("[1,2,3].fill(5, 1, 1);"),
      njs_str("1,2,3") },

    { njs_str("[1,2,3].fill(5, 1, 2);"),
      njs_str("1,5,3") },

    { njs_str("[1,2,3].fill(5, 1, 3);"),
      njs_str("1,5,5") },

    { njs_str("[1,2,3].fill(5, 1, 4);"),
      njs_str("1,5,5") },

    { njs_str("[1,2,3].fill(5, 1, -1);"),
      njs_str("1,5,3") },

    { njs_str("[1,2,3].fill(5, 1, -3);"),
      njs_str("1,2,3") },

    { njs_str("[1,2,3].fill(5, 1, -4);"),
      njs_str("1,2,3") },

    { njs_str("[1,2,3].fill(\"a\", 1, 2);"),
      njs_str("1,a,3") },

    { njs_str("[1,2,3].fill({a:\"b\"}, 1, 2);"),
      njs_str("1,[object Object],3") },

    { njs_str("Array(3).fill().reduce(function(a, x)"
                                 "{ return a + (x === undefined); }, 0)"),
      njs_str("3") },

    { njs_str("var a = Array.prototype.fill.apply("
                 "Object({length: 40}), [\"a\", 1, 20]); Object.values(a)"),
      njs_str("a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,40,a,a,a,a") },

    { njs_str("var a = Array.prototype.fill.apply({length: "
                 "{ valueOf: function() { return 40 }}}, [\"a\", 1, 20]);"
                 "Object.values(a)"),
      njs_str("a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,[object Object],a,a,a,a") },

    { njs_str("[NaN, false, ''].map("
                 "(x) => Array.prototype.fill.call(x)"
                 ").every((x) => typeof x == 'object')"),
      njs_str("true") },

    { njs_str("var o = {}; Object.defineProperty(o, 'length', {get:()=>2}); "
                 "Array.prototype.slice.call(Array.prototype.fill.call(o, 1))"),
      njs_str("1,1") },

    { njs_str("var o = {}; Object.defineProperty(o, 'length', {get:()=>'0x0002'}); "
                 "Array.prototype.slice.call(Array.prototype.fill.call(o, 1))"),
      njs_str("1,1") },

    { njs_str("var o = {}; Object.defineProperty(o, 'length', {get:()=> {throw TypeError('Boom')}}); "
                 "Array.prototype.fill.call(o, 1)"),
      njs_str("TypeError: Boom") },

    { njs_str("var o = Object({length: 3});"
                 "Object.defineProperty(o, '0', {set: ()=>{throw TypeError('Boom')}});"
                 "Array.prototype.fill.call(o, 1)"),
      njs_str("TypeError: Boom") },

    { njs_str("var o = Object({length: 3});"
                 "Object.defineProperty(o, '0', {set: function(v){this.a = 2 * v}});"
                 "Array.prototype.fill.call(o, 2).a"),
      njs_str("4") },

    { njs_str("ArrayBuffer()"),
      njs_str("TypeError: Constructor ArrayBuffer requires 'new'") },

    { njs_str("new ArrayBuffer()"),
      njs_str("[object ArrayBuffer]") },

    { njs_str("ArrayBuffer.prototype.constructor.name === 'ArrayBuffer'"),
      njs_str("true") },

    { njs_str("ArrayBuffer.prototype.constructor()"),
      njs_str("TypeError: Constructor ArrayBuffer requires 'new'") },

    { njs_str("ArrayBuffer.name"),
      njs_str("ArrayBuffer") },

    { njs_str("ArrayBuffer[Symbol.species]"),
      njs_str("[object Function]") },

    { njs_str("ArrayBuffer.prototype[Symbol.toStringTag]"),
      njs_str("ArrayBuffer") },

    { njs_str("var desc = Object.getOwnPropertyDescriptor(ArrayBuffer,"
              "Symbol.species); desc.get"),
      njs_str("[object Function]") },

    { njs_str("var ctor = ArrayBuffer[Symbol.species]; var a = new ctor(100);"
              "a.byteLength;"),
      njs_str("100") },

    { njs_str("var a = new ArrayBuffer(); a.byteLength"),
      njs_str("0") },

    { njs_str("var a = new ArrayBuffer.prototype.constructor(10); a.byteLength"),
      njs_str("10") },

    { njs_str("var get = Object.getOwnPropertyDescriptor(ArrayBuffer.prototype, 'byteLength').get;"
              "get.call([])"),
      njs_str("TypeError: Method ArrayBuffer.prototype.byteLength called on incompatible receiver") },

    { njs_str("[undefined, 1, 10, 1000, null, NaN, false, {}, [1,2,3], Object(1),'10',"
              " -1, -Infinity, Infinity, 2**50]"
              ".map(v=>{ var a; try { a = new ArrayBuffer(v) } catch (e) {return e.name} return a.byteLength})"),
      njs_str("0,1,10,1000,0,0,0,0,0,1,10,RangeError,RangeError,RangeError,RangeError") },

    { njs_str("var buffer = new ArrayBuffer(16);"
              "[[4,12], [-1,-1], [-1,10], [0, -1], [0, -16], [0,-17]]"
              ".map(pr=>buffer.slice(pr[0], pr[1]).byteLength)"),
      njs_str("8,0,0,15,0,0") },

#if NJS_HAVE_LARGE_STACK
    { njs_str("var o = Object({length: 3});"
                 "Object.defineProperty(o, '0', {set: function(v){this[0] = 2 * v}});"
                 "Array.prototype.fill.call(o, 2)"),
      njs_str("RangeError: Maximum call stack size exceeded") },
#endif

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "Array.prototype.fill.call(o); i"),
      njs_str("1") },

    { njs_str("var a = [];"
                 "a.filter(function(v, i, a) { return v > 1 })"),
      njs_str("") },

    { njs_str("var a = [1,2,3,-1,5];"
                 "a.filter(function(v, i, a) { return v > 1 })"),
      njs_str("2,3,5") },

    { njs_str("var a = [1,2,3,4,5,6,7,8];"
                 "a.filter(function(v, i, a) { a.pop(); return v > 1 })"),
      njs_str("2,3,4") },

    { njs_str("var a = [1,2,3,4,5,6,7,8];"
                 "a.filter(function(v, i, a) { a.shift(); return v > 1 })"),
      njs_str("3,5,7") },

    { njs_str("var a = [1,2,3,4,5,6,7];"
                 "a.filter(function(v, i, a) { a.pop(); return v > 1 })"),
      njs_str("2,3,4") },

    { njs_str("var a = [1,2,3,4,5,6,7];"
                 "a.filter(function(v, i, a) { a.shift(); return v > 1 })"),
      njs_str("3,5,7") },

    { njs_str("var a = [1,2,3,4,5,6,7];"
                 "a.filter(function(v, i, a) { a[i] = v + 1; return true })"),
      njs_str("1,2,3,4,5,6,7") },

    { njs_str("var a = [1,2,3,4,5,6,7];"
                 "a.filter(function(v, i, a) { a[i+1] = v+10; return true })"),
      njs_str("1,11,21,31,41,51,61") },

    { njs_str("var o = {0: 'c', 1: 'b', 2: 'c', 'length': { valueOf() { return 3 }}};"
              "var r = Array.prototype.filter.call(o, function(el, i, arr) {return el == 'c'}); r"),
      njs_str("c,c") },

    { njs_str("var o = {0: 'c', 1: 'a', 2: 'b'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'c'});"
              "var r = Array.prototype.filter.call(o, function(el, i, arr) { return el == 'c' }); r"),
      njs_str("c,c") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.filter.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var a = [];"
                 "a.find(function(v, i, a) { return v > 1 })"),
      njs_str("undefined") },

    { njs_str("var a = [,NaN,0,-1];"
                 "a.find(function(v, i, a) { return v > 1 })"),
      njs_str("undefined") },

    { njs_str("var a = [,NaN,0,-1,2];"
                 "a.find(function(v, i, a) { return v > 1 })"),
      njs_str("2") },

    { njs_str("var a = [1,2,3,-1,5];"
                 "a.find(function(v, i, a) { return v > 1 })"),
      njs_str("2") },

    { njs_str("var a = [,1,,-1,5];"
                 "a.find(function(v, i, a) { return v > 1 })"),
      njs_str("5") },

    { njs_str("var a = [,1,,-1,5,6];"
                 "a.find(function(v, i, a) { return v > 1 })"),
      njs_str("5") },

    { njs_str("[].find(function(v) { return (v === undefined) })"),
      njs_str("undefined") },

    { njs_str("var a = [,3];"
                 "a.find(function(v) { return (v === 3 || v === undefined) })"),
      njs_str("undefined") },

    { njs_str("var a = [1,,3];"
                 "a.find(function(v) { return (v === 3 || v === undefined) })"),
      njs_str("undefined") },

    { njs_str("var a = [1,2,3,4,5,6];"
                 "a.find(function(v, i, a) { a.shift(); return v == 3 })"),
      njs_str("3") },

    { njs_str("var a = [1,2,3,4,5,6];"
              "a.find(function(v, i, a) { a.shift(); return v == 4 })"),
      njs_str("undefined") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c', 'length': { valueOf() { return 3 }}};"
              "var r = Array.prototype.find.call(o, function(el, i, arr) {return el == 'b'}); r"),
      njs_str("b") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c', 'length': { valueOf() { return 3 }}};"
              "var r = Array.prototype.find.call(o, function(el, i, arr) {delete o['1']; return el == 'c'}); r"),
      njs_str("c") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "var r = Array.prototype.find.call(o, function(el, i, arr) { return el == 'd' }); r"),
      njs_str("d") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.find.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var callz = 0, res = [], arr = 'abc'.split('');"
              "void arr.find((k) => { if (0 == callz++) { arr.splice(1,1); } res.push(k) });"
              "res.join(',')"),
      njs_str("a,c,") },

    { njs_str("var a = [];"
                 "a.findIndex(function(v, i, a) { return v > 1 })"),
      njs_str("-1") },

    { njs_str("var a = [,NaN,0,-1];"
                 "a.findIndex(function(v, i, a) { return v > 1 })"),
      njs_str("-1") },

    { njs_str("var a = [,NaN,0,-1,2];"
                 "a.findIndex(function(v, i, a) { return v > 1 })"),
     njs_str("4") },

    { njs_str("var a = [1,2,3,-1,5];"
                 "a.findIndex(function(v, i, a) { return v > 1 })"),
      njs_str("1") },

    { njs_str("var a = [,1,,-1,5];"
                 "a.findIndex(function(v, i, a) { return v > 1 })"),
      njs_str("4") },

    { njs_str("var a = [,1,,-1,5,6];"
                 "a.findIndex(function(v, i, a) { return v > 1 })"),
      njs_str("4") },

    { njs_str("[].findIndex(function(v) { return (v === undefined) })"),
      njs_str("-1") },

    { njs_str("[,].findIndex(function(v) { return (v === undefined) })"),
      njs_str("0") },

    { njs_str("[1,2,,3].findIndex(function(el){return el === undefined})"),
      njs_str("2") },

    { njs_str("[,2,,3].findIndex(function(el){return el === undefined})"),
      njs_str("0") },

    { njs_str("var a = [1,2,3,4,5,6];"
                 "a.findIndex(function(v, i, a) { a.shift(); return v == 3 })"),
      njs_str("1") },

    { njs_str("var a = [1,2,3,4,5,6];"
                 "a.findIndex(function(v, i, a) { a.shift(); return v == 4 })"),
      njs_str("-1") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c', 'length': { valueOf() { return 3 }}};"
              "var r = Array.prototype.findIndex.call(o, function(el, i, arr) {return el == 'b'}); r"),
      njs_str("1") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "var r = Array.prototype.findIndex.call(o, function(el, i, arr) { return el == 'd' }); r"),
      njs_str("3") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.findIndex.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var callz = 0, res = [], arr = 'abc'.split('');"
              "void arr.findIndex((k) => { if (0 == callz++) { arr.splice(1,1); } res.push(k) });"
              "res.join(',')"),
      njs_str("a,c,") },

    { njs_str("var a = [];"
                 "a.map(function(v, i, a) { return v + 1 })"),
      njs_str("") },

    { njs_str("var a = [,,,];"
                 "a.map(function(v, i, a) { return v + 1 })"),
      njs_str(",,") },

    { njs_str("var a = [,,,1];"
                 "a.map(function(v, i, a) { return v + 1 })"),
      njs_str(",,,2") },

    { njs_str("var a = [1,2,3];"
                 "a.map(function(v, i, a) { return v + 1 })"),
      njs_str("2,3,4") },

    { njs_str("var a = [1,2,3,4,5,6];"
                 "a.map(function(v, i, a) { a.pop(); return v + 1 })"),
      njs_str("2,3,4,,,") },

    { njs_str("var a = [1,2,3,4,5,6];"
                 "a.map(function(v, i, a) { a.shift(); return v + 1 })"),
      njs_str("2,4,6,,,") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c', 'length': { valueOf() { return 3 }}};"
              "var r = Array.prototype.map.call(o, num => num + '1'); r"),
      njs_str("a1,b1,c1") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "var r = Array.prototype.map.call(o, function(el, i, arr) { return el + '1' }); r"),
      njs_str("a1,b1,c1,d1") },

    { njs_str("Array.prototype.map.call(new String('abc'),"
             "                          (val, idx, obj) => {return obj instanceof String})"
              ".every(x => x === true)"),
      njs_str("true") },

    { njs_str("Array.prototype.map.call('abcdef', (val, idx, obj) => {return val === 100})"),
      njs_str("false,false,false,false,false,false") },

    { njs_str("function callbackfn(val, idx, obj) {return idx === 1 && typeof val === 'undefined';}"
              "var obj = {2: 2, length: 10};"
              "var res = Array.prototype.map.call(obj, callbackfn); typeof res[7]"),
      njs_str("undefined") },

    { njs_str("function callbackfn(val, idx, obj) {return idx === 1 && typeof val === 'undefined';}"
              "var obj = {2: 2, length: 9000};"
              "var res = Array.prototype.map.call(obj, callbackfn); typeof res[8000]"),
      njs_str("undefined") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.map.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var a = [];"
                 "a.reduce(function(p, v, i, a) { return p + v })"),
      njs_str("TypeError: Reduce of empty object with no initial value") },

    { njs_str("var a = [];"
                 "a.reduce(function(p, v, i, a) { return p + v }, 10)"),
      njs_str("10") },

    { njs_str("var a = [,,];"
                 "a.reduce(function(p, v, i, a) { return p + v })"),
      njs_str("TypeError: Reduce of empty object with no initial value") },

    { njs_str("var a = [,,];"
                 "a.reduce(function(p, v, i, a) { return p + v }, 10)"),
      njs_str("10") },

    { njs_str("var a = [1];"
                 "a.reduce(function(p, v, i, a) { return p + v })"),
      njs_str("1") },

    { njs_str("var a = [1];"
                 "a.reduce(function(p, v, i, a) { return p + v }, 10)"),
      njs_str("11") },

    { njs_str("var a = [1,2,3];"
                 "a.reduce(function(p, v, i, a) { return p + v })"),
      njs_str("6") },

    { njs_str("var a = [1,2,3];"
                 "a.reduce(function(p, v, i, a) { return p + v }, 10)"),
      njs_str("16") },

    { njs_str("[[0, 1], [2, 3], [4, 5]].reduce(function(a, b)"
                 "                         { return a.concat(b) }, [])"),
      njs_str("0,1,2,3,4,5") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c', 'length': { valueOf() { return 3 }}};"
                 "var reducer = (a, b) => a + b;"
                 "var a = Array.prototype.reduce.call(o, reducer); a"),
      njs_str("abc") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "var r = Array.prototype.reduce.call(o, (a, b) => a + b); r"),
      njs_str("abcd") },

    { njs_str("var o = {1: 'b', 2: 'c', 3: 'd'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '0', {get: () => 'a'});"
              "var r = Array.prototype.reduce.call(o, (a, b) => a + b); r"),
      njs_str("abcd") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.reduce.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var a = [];"
                 "a.reduceRight(function(p, v, i, a) { return p + v })"),
      njs_str("TypeError: Reduce of empty object with no initial value") },

    { njs_str("var a = [];"
                 "a.reduceRight(function(p, v, i, a) { return p + v }, 10)"),
      njs_str("10") },

    { njs_str("var a = [,,];"
                 "a.reduceRight(function(p, v, i, a) { return p + v })"),
      njs_str("TypeError: Reduce of empty object with no initial value") },

    { njs_str("var a = [,,];"
                 "a.reduceRight(function(p, v, i, a) { return p + v }, 10)"),
      njs_str("10") },

    { njs_str("var a = [1];"
                 "a.reduceRight(function(p, v, i, a) { return p + v })"),
      njs_str("1") },

    { njs_str("var a = [1];"
                 "a.reduceRight(function(p, v, i, a) { return p + v }, 10)"),
      njs_str("11") },

    { njs_str("var a = [1,2,3];"
                 "a.reduceRight(function(p, v, i, a) { return p + v })"),
      njs_str("6") },

    { njs_str("var a = [1,2,3];"
                 "a.reduceRight(function(p, v, i, a) { return p + v }, 10)"),
      njs_str("16") },

    { njs_str("var a = [1,2,3];"
                 "a.reduceRight(function(p, v, i, a)"
                 "              { a.shift(); return p + v })"),
      njs_str("7") },

    { njs_str("var a = [1,2,3];"
                 "a.reduceRight(function(p, v, i, a)"
                 "              { a.shift(); return p + v }, 10)"),
      njs_str("19") },

    { njs_str("var o = {0: 'a', 1: 'b', 2: 'c'};"
              "Object.defineProperty(o, 'length', {get: () => 4});"
              "Object.defineProperty(o, '3', {get: () => 'd'});"
              "Array.prototype.reduceRight.call(o, (p, v) => p + v)"),
      njs_str("dcba") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "try {Array.prototype.reduceRight.call(o);}"
              "catch (e) {i += '; ' + e} i"),
      njs_str("1; TypeError: unexpected iterator arguments") },

    { njs_str("var m = [];"
              "[''].reduceRight.call('00000000000000000000000000000а00', (p, v, i, a) => {m.push(v)});"
              "m.join('')"),
      njs_str("0а00000000000000000000000000000") },

    { njs_str("var a = ['1','2','3','4','5','6']; a.sort()"),
      njs_str("1,2,3,4,5,6") },

    { njs_str("var o = { toString: function() { return 5 } };"
                 "var a = [6,o,4,3,2,1]; a.sort()"),
      njs_str("1,2,3,4,5,6") },

    { njs_str("var a = [1,2,3,4,5,6];"
                 "a.sort(function(x, y) { return x - y })"),
      njs_str("1,2,3,4,5,6") },

    { njs_str("var a = [6,5,4,3,2,1];"
                 "a.sort(function(x, y) { return x - y })"),
      njs_str("1,2,3,4,5,6") },

    { njs_str("var a = [2,2,2,1,1,1];"
                 "a.sort(function(x, y) { return x - y })"),
      njs_str("1,1,1,2,2,2") },

    { njs_str("var a = [,,,2,2,2,1,1,1];"
                 "a.sort(function(x, y) { return x - y })"),
      njs_str("1,1,1,2,2,2,,,") },

    { njs_str("var a = [,,,,];"
                 "a.sort(function(x, y) { return x - y })"),
      njs_str(",,,") },

    { njs_str("var a = [1,,];"
                 "a.sort(function(x, y) { return x - y })"),
      njs_str("1,") },

    /* Template literal. */

    { njs_str("`"),
      njs_str("SyntaxError: Unterminated template literal in 1") },

    { njs_str("`$"),
      njs_str("SyntaxError: Unterminated template literal in 1") },

    { njs_str("`${"),
      njs_str("SyntaxError: Unexpected end of input in 1") },

    { njs_str("`${a"),
      njs_str("SyntaxError: Missing \"}\" in template expression in 1") },

    { njs_str("`${}"),
      njs_str("SyntaxError: Unexpected token \"}\" in 1") },

    { njs_str("`${a}"),
      njs_str("SyntaxError: Unterminated template literal in 1") },

    { njs_str("`${a}bc"),
      njs_str("SyntaxError: Unterminated template literal in 1") },

    { njs_str("`\\"),
      njs_str("SyntaxError: Unterminated template literal in 1") },

    { njs_str("`\\${a}bc"),
      njs_str("SyntaxError: Unterminated template literal in 1") },

    { njs_str("`text1\ntext2`;"),
      njs_str("text1\ntext2") },

    { njs_str("var o = 1; `o = \\`${o}\\``"),
      njs_str("o = `1`") },

    { njs_str("`\\unicode`"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\unicode\" in 1") },

    { njs_str("var a = 5; var b = 10;"
                 "`Fifteen is ${a + b} and \nnot ${2 * a + b}.`;"),
      njs_str("Fifteen is 15 and \nnot 20.") },

    { njs_str("var s = `1undefined`; s;"),
      njs_str("1undefined") },

    { njs_str("var s = '0'; s = `x${s += '1'}`;"),
      njs_str("x01") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                 "var something = 'test'; var one = 1; var two = 2;"
                 "`[${d.toISOString()}] the message contents ${something} ${one + two}`"),
      njs_str("[2011-06-24T18:45:12.625Z] the message contents test 3") },

    { njs_str("function isLargeScreen() { return false; }"
                 "var item = { isCollapsed: true };"
                 "`header ${ isLargeScreen() ? '' : `icon-${item.isCollapsed ? 'expander' : 'collapser'}` }`;"),
      njs_str("header icon-expander") },

    { njs_str("function foo(strings, person, age) { return `${strings[0]}${strings[1]}${person}${age}` };"
                 "var person = 'Mike'; var age = 21;"
                 "foo`That ${person} is a ${age}`;"),
      njs_str("That  is a Mike21") },

    /* Strings. */

    { njs_str("var a = '0123456789' + '012345';"
                 "var b = 'abcdefghij' + 'klmnop';"
                 "    a = b"),
      njs_str("abcdefghijklmnop") },

    { njs_str("String.prototype.my = function f() {return 7}; 'a'.my()"),
      njs_str("7") },

    { njs_str("'a'.my"),
      njs_str("undefined") },

    { njs_str("var a = '123'\n[2].toString();a"),
      njs_str("3") },

    { njs_str("'\xE5\x96\x9C\xE3\x81\xB6'"),
      njs_str("喜ぶ") },

    /* Broken UTF-8 literals.*/

    { njs_str("'\x96\xE5\x9C\xE3\x81\xB6'"),
      njs_str("��ぶ") },

    { njs_str("'\x96\xE5\x9C'"),
      njs_str("��") },

    { njs_str("'\x96\xE5'"),
      njs_str("��") },

    { njs_str("'\x96'"),
      njs_str("�") },

    { njs_str("'\xF3'"),
      njs_str("�") },

    { njs_str("'\xF3\xFF'"),
      njs_str("��") },

    { njs_str("'\x96\x96\xE5\x9C\xE3\x81\xB6'"),
      njs_str("���ぶ") },

    { njs_str("'\x9C\x96\xE5\xE3\x81\xB6'"),
      njs_str("���ぶ") },

    { njs_str("'\xE5\x9C\xE3\x81\xB6'"),
      njs_str("�ぶ") },

    { njs_str("'\xEF\xBF\xBD\xE3\x81\xB6'"),
      njs_str("�ぶ") },

    { njs_str("'\xE5\xF6\x9C\xE3\x81\xB6'"),
      njs_str("���ぶ") },

    { njs_str("var a = 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\xF3'; "
                 "[a.length, a[33], a[34]]"),
      njs_str("35,a,�") },

    /* Escape strings. */

    { njs_str("'\\a \\' \\\" \\\\ \\0 \\b \\f \\n \\r \\t \\v'"),
      njs_str("a ' \" \\ \0 \b \f \n \r \t \v") },

    { njs_str("'\\\n'"),
      njs_str("") },

    { njs_str("'\\\r'"),
      njs_str("") },

    { njs_str("'\\\r\n'"),
      njs_str("") },

    { njs_str("'a\\\nb'"),
      njs_str("ab") },

    { njs_str("'a\\\rb'"),
      njs_str("ab") },

    { njs_str("'a\\\r\nb'"),
      njs_str("ab") },

    { njs_str("'a\\\n\rb'"),
      njs_str("SyntaxError: Unterminated string \"'a\\\n\r\" in 1") },

    { njs_str("'a\\\nb\nc'"),
      njs_str("SyntaxError: Unterminated string \"'a\\\nb\n\" in 1") },

    { njs_str("'abcde"),
      njs_str("SyntaxError: Unterminated string \"'abcde\" in 1") },

    { njs_str("'\\"),
      njs_str("SyntaxError: Unterminated string \"'\\\" in 1") },

    { njs_str("'\\\r\n"),
      njs_str("SyntaxError: Unterminated string \"'\\\r\n\" in 1") },

    { njs_str("'\\'"),
      njs_str("SyntaxError: Unterminated string \"'\\'\" in 1") },

    { njs_str("'a\n"),
      njs_str("SyntaxError: Unterminated string \"'a\n\" in 1") },

    { njs_str("'a\r"),
      njs_str("SyntaxError: Unterminated string \"'a\r\" in 1") },

    { njs_str("\"a\n"),
      njs_str("SyntaxError: Unterminated string \"\"a\n\" in 1") },

    { njs_str("'\\u03B1'"),
      njs_str("α") },

    { njs_str("'\\u'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u\" in 1") },

    { njs_str("'\\uzzzz'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\uzzzz\" in 1") },

    { njs_str("'\\u03B'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u03B\" in 1") },

    { njs_str("'\\u03BG'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u03BG\" in 1") },

    { njs_str("'\\u03B '"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u03B \" in 1") },

    { njs_str("'\\u{61}\\u{3B1}\\u{20AC}'"),
      njs_str("aα€") },

    { njs_str("'\\u'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u\" in 1") },

    { njs_str("'\\u{'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u{\" in 1") },

    { njs_str("'\\u{}'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u{}\" in 1") },

    { njs_str("'\\u{1234567}'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u{1234567}\" in 1") },

    { njs_str("'\\u{a00000}'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u{a00000}\" in 1") },

    { njs_str("'\\x61'"),
      njs_str("a") },

    { njs_str("''.length"),
      njs_str("0") },

    { njs_str("'abc'.length"),
      njs_str("3") },

    { njs_str("'привет\\n'.length"),
      njs_str("7") },

    { njs_str("'привет\\n\\u{61}\\u{3B1}\\u{20AC}'.length"),
      njs_str("10") },

    { njs_str("'\\ud83d\\udc4d'"),
      njs_str("\xf0\x9f\x91\x8d") },

    { njs_str("'\\ud83d\\udc4d'.length"),
      njs_str("1") },

    { njs_str("'\\ud83d abc \\udc4d'"),
      njs_str("� abc �") },

    { njs_str("'\\ud83d'"),
      njs_str("�") },

    { njs_str("'\\ud83d\\uabcd'"),
      njs_str("�ꯍ") },

    { njs_str("'\\u{d800}\\u{dB00}'"),
      njs_str("��") },

    { njs_str("'\\u{d800}\\u{d7ff}'"),
      njs_str("�퟿") },

    { njs_str("'\\u{d800}['"),
      njs_str("�[") },

    { njs_str("'\\u{D800}\\u{'"),
      njs_str("SyntaxError: Invalid Unicode code point \"\\u{D800}\\u{\" in 1") },

    { njs_str("'α' !== '\\α'"),
      njs_str("false") },

    { njs_str("'r' !== '\\r'"),
      njs_str("true") },

    /* Broken UTF-8 literals.*/

    { njs_str("'\\a\x96\xE5\x9C\xE3\x81\xB6'"),
      njs_str("a��ぶ") },

    { njs_str("'\x96\\a\xE5\x9C'"),
      njs_str("�a�") },

    { njs_str("'\x96\xE5\\a'"),
      njs_str("��a") },

    { njs_str("'\\a\x96\\a'"),
      njs_str("a�a") },

    { njs_str("'\xF3\\a'"),
      njs_str("�a") },

    { njs_str("'\xF3\\a\xFF'"),
      njs_str("�a�") },

    { njs_str("'\\a\x96\x96\xE5\x9C\xE3\x81\xB6'"),
      njs_str("a���ぶ") },

    { njs_str("'\\a\x9C\x96\xE5\xE3\x81\xB6'"),
      njs_str("a���ぶ") },

    { njs_str("'\\a\xE5\x9C\xE3\x81\xB6'"),
      njs_str("a�ぶ") },

    { njs_str("'\\a\xEF\xBF\xBD\xE3\x81\xB6'"),
      njs_str("a�ぶ") },

    { njs_str("'\\a\xE5\xF6\x9C\xE3\x81\xB6'"),
      njs_str("a���ぶ") },

    { njs_str("var a = '\\aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\xF3'; "
                 "[a.length, a[34], a[35]]"),
      njs_str("36,a,�") },

    { njs_str("''.hasOwnProperty('length')"),
      njs_str("true") },

    { njs_str("'abc'.hasOwnProperty('length')"),
      njs_str("true") },

    { njs_str("(new String('abc')).hasOwnProperty('length')"),
      njs_str("true") },

    { njs_str("'abc'.toUTF8().length"),
      njs_str("3") },

    { njs_str("'абв'.length"),
      njs_str("3") },

    { njs_str("'абв'.toUTF8().length"),
      njs_str("6") },

    { njs_str("'αβγ'.length"),
      njs_str("3") },

    { njs_str("'αβγ'.toUTF8().length"),
      njs_str("6") },

    { njs_str("'絵文字'.length"),
      njs_str("3") },

    { njs_str("'絵文字'.toUTF8().length"),
      njs_str("9") },

    { njs_str("'えもじ'.length"),
      njs_str("3") },

    { njs_str("'えもじ'.toUTF8().length"),
      njs_str("9") },

    { njs_str("'囲碁織'.length"),
      njs_str("3") },

    { njs_str("'囲碁織'.toUTF8().length"),
      njs_str("9") },

    { njs_str("var a = 'abc'; a.length"),
      njs_str("3") },

    { njs_str("var a = 'abc'; a['length']"),
      njs_str("3") },

    { njs_str("var a = 'абв'; a.length"),
      njs_str("3") },

    { njs_str("var a = 'abc' + 'абв'; a.length"),
      njs_str("6") },

    { njs_str("var a = 'abc' + 1 + 'абв'; a +' '+ a.length"),
      njs_str("abc1абв 7") },

    { njs_str("var a = 1; a.length"),
      njs_str("undefined") },

    { njs_str("var a = 'abc'; a.concat('абв', 123)"),
      njs_str("abcабв123") },

    { njs_str("''.concat.call(0, 1, 2, 3, 4, 5, 6, 7, 8, 9)"),
      njs_str("0123456789") },

    { njs_str("''.concat.apply(0, [1, 2, 3, 4, 5, 6, 7, 8, 9])"),
      njs_str("0123456789") },

    { njs_str("var f = ''.concat.bind(0, 1, 2, 3, 4); f(5, 6, 7, 8, 9)"),
      njs_str("0123456789") },

    { njs_str("var f = ''.concat.bind(0, 1, 2, 3, 4); f(Math.sqrt(25))"),
      njs_str("012345") },

    { njs_str("var f = String.prototype.concat.bind(0, 1); f(2)"),
      njs_str("012") },

    { njs_str("var f = Function.prototype.call.bind"
                 "                            (String.prototype.concat, 0, 1);"
                 "f(2)"),
      njs_str("012") },

    { njs_str("var f = String.prototype.concat.bind(0, 1);"
                 "var o = { toString: f }; o"),
      njs_str("01") },

    { njs_str("''.concat.bind(0, 1, 2, 3, 4).call(5, 6, 7, 8, 9)"),
      njs_str("012346789") },

    { njs_str("''.concat.bind(0, 1, 2, 3, 4).apply(5,[6, 7, 8, 9])"),
      njs_str("012346789") },

    { njs_str("var f = Array.prototype.join.bind([0, 1, 2]); f()"),
      njs_str("0,1,2") },

    { njs_str("var f = Array.prototype.join.bind([0, 1, 2]);"
                 "var o = { toString: f }; o"),
      njs_str("0,1,2") },

    { njs_str("var f = Array.prototype.join.bind([0, 1, 2]); f('.')"),
      njs_str("0.1.2") },

    { njs_str("var f = Array.prototype.join.bind([0, 1, 2], '.');"
                 "var o = { toString: f }; o"),
      njs_str("0.1.2") },

    { njs_str("var f = Array.prototype.toString.bind([0, 1, 2]); f()"),
      njs_str("0,1,2") },

    { njs_str("var f = Array.prototype.toString.bind([0, 1, 2]);"
                 "var o = { toString: f }; o"),
      njs_str("0,1,2") },

    { njs_str("var f = Object.defineProperty(function() {}, 'name', {value: 'F'});"
              "[f.name, f.bind().name, f.bind().bind().name]"),
      njs_str("F,bound F,bound bound F") },

    { njs_str("var f = Object.defineProperty(function() {}, 'name', {value: undefined});"
              "[f.name, f.bind().name, f.bind().bind().name]"),
      njs_str(",bound ,bound bound ") },

    { njs_str("var f = Object.defineProperty(function() {}, 'name', {get:()=>'F'});"
              "[f.name, f.bind().name]"),
      njs_str("F,bound F") },

    { njs_str("var f = Object.defineProperty(function() {}, 'name', {get:()=>{throw Error('Oops')}});"
              "f.bind().name"),
      njs_str("Error: Oops") },

    { njs_str("var f = function() {}; f.a = 'a'; [f.bind().a, f.a]"),
      njs_str(",a") },

    { njs_str("var f = function() {}; var bf = f.bind(); bf.b = 'b'; "
              "[f.b, bf.b]"),
      njs_str(",b") },

    { njs_str("function f(x,y) {return {args:arguments,length:arguments.length}};"
              "var nf = Function.prototype.bind.call(f, {}, 'a', 'b');"
              "var o = new nf();[o.length, o.args[0]]"),
      njs_str("2,a") },

    { njs_str("function f(x,y) {return {args:arguments,length:arguments.length}};"
              "var nf = Function.prototype.bind.call(f, {});"
              "var o = new nf('a', 'b');[o.length, o.args[0]]"),
      njs_str("2,a") },

    { njs_str("var f = function(a,b) { this.a = a; this.b = b; };"
              "f.prototype.X = 'X';"
              "var bf = f.bind(null, 1,2);"
              "var o = new bf(); "
              "[Object.keys(o), o.X,(o instanceof f) && (o instanceof bf),bf.prototype]"),
      njs_str("a,b,X,true,") },

    { njs_str("var bArray = Array.bind(null, 10,16); bArray()"),
      njs_str("10,16") },

    { njs_str("var bArray = Array.bind(null, 10,16); new bArray()"),
      njs_str("10,16") },

    { njs_str("var bArray = Array.bind(null, 10); new bArray(16)"),
      njs_str("10,16") },

#if 0 /* FIXME: refactor Bound calls (9.4.1.1[[Call]]). */
    { njs_str("function f(x,y) {return {args:arguments,length:arguments.length}};"
              "var bf = f.bind({}, 'a'); var bbf = bf.bind({},'b'); var o = bbf('c');"),
              "[o.args[0], o.args[2], o.length]"
      njs_str("a,c,3") },
#endif

    { njs_str("var s = { toString: function() { return '123' } };"
                 "var a = 'abc'; a.concat('абв', s)"),
      njs_str("abcабв123") },

    { njs_str("'\\u00CE\\u00B1'.toBytes() == 'α'"),
      njs_str("true") },

    { njs_str("'\\u00CE\\u00B1'.toBytes() === 'α'"),
      njs_str("true") },

    { njs_str("var b = '\\u00C2\\u00B6'.toBytes(), u = b.fromUTF8();"
                 "b.length +' '+ b +' '+ u.length +' '+ u"),
      njs_str("2 ¶ 1 ¶") },

    { njs_str("'α'.toBytes()"),
      njs_str("null") },

    { njs_str("'α'.toUTF8()[0]"),
      njs_str("\xCE") },

    { njs_str("var r = /^\\x80$/; r.source + r.source.length"),
      njs_str("^\\x80$6") },

    { njs_str("var r = /^\\\\x80$/; r.source + r.source.length"),
      njs_str("^\\\\x80$7") },

    { njs_str("/^\\x80$/.test('\\x80'.toBytes())"),
      njs_str("true") },

    { njs_str("/^\\xC2\\x80$/.test('\\x80'.toUTF8())"),
      njs_str("true") },

    { njs_str("'α'.toUTF8().toBytes()"),
      njs_str("α") },

    { njs_str("var a = 'a'.toBytes() + 'α'; a + a.length"),
      njs_str("aα3") },

    { njs_str("var a = 'µ§±®'.toBytes(); a"),
      njs_str("\xB5\xA7\xB1\xAE") },

    { njs_str("var a = 'µ§±®'.toBytes(2); a"),
      njs_str("\xB1\xAE") },

    { njs_str("var a = 'µ§±®'.toBytes(1,3); a"),
      njs_str("\xA7\xB1") },

    { njs_str("var a = '\\xB5\\xA7\\xB1\\xAE'.toBytes(); a.fromBytes()"),
      njs_str("µ§±®") },

    { njs_str("var a = '\\xB5\\xA7\\xB1\\xAE'.toBytes(); a.fromBytes(2)"),
      njs_str("±®") },

    { njs_str("var a = '\\xB5\\xA7\\xB1\\xAE'.toBytes(); a.fromBytes(1, 3)"),
      njs_str("§±") },

    { njs_str("'A'.repeat(8).toBytes() === 'A'.repeat(8)"),
      njs_str("true") },

    { njs_str("'A'.repeat(16).toBytes() === 'A'.repeat(16)"),
      njs_str("true") },

    { njs_str("'A'.repeat(38).toBytes(-5) === 'AAAAA'"),
      njs_str("true") },

    { njs_str("('α' + 'A'.repeat(32)).toBytes()"),
      njs_str("null") },

    { njs_str("('α' + 'A'.repeat(32)).toBytes(1) === 'A'.repeat(32)"),
      njs_str("true") },

    { njs_str("('α' + 'A'.repeat(40)).toBytes(-3,-1)"),
      njs_str("AA") },

    { njs_str("var s = 'x'.repeat(2**10).repeat(2**14);"
                 "var a = Array(200).fill(s);"
                 "String.prototype.concat.apply(s, a.slice(1))"),
      njs_str("RangeError: invalid string length") },

    { njs_str("var a = 'abcdefgh'; a.substr(3, 15)"),
      njs_str("defgh") },

    { njs_str("'abcdefgh'.substr(3, 15)"),
      njs_str("defgh") },

    { njs_str("'abcdefghijklmno'.substr(3, 4)"),
      njs_str("defg") },

    { njs_str("'abcdefghijklmno'.substr(-3, 2)"),
      njs_str("mn") },

    { njs_str("'abcdefgh'.substr(100, 120)"),
      njs_str("") },

    { njs_str("('abc' + 'defgh').substr(1, 4)"),
      njs_str("bcde") },

    { njs_str("'abcdefghijklmno'.substring(3, 5)"),
      njs_str("de") },

    { njs_str("'abcdefgh'.substring(3)"),
      njs_str("defgh") },

    { njs_str("'abcdefgh'.substring(5, 3)"),
      njs_str("de") },

    { njs_str("'abcdefgh'.substring(100, 120)"),
      njs_str("") },

    { njs_str("'α'.repeat(32).substring(32)"),
      njs_str("") },

    { njs_str("'α'.repeat(32).substring(32,32)"),
      njs_str("") },

    { njs_str("'abcdefghijklmno'.slice(NaN, 5)"),
      njs_str("abcde") },

    { njs_str("'abcdefghijklmno'.slice(NaN, Infinity)"),
      njs_str("abcdefghijklmno") },

    { njs_str("'abcdefghijklmno'.slice(-Infinity, Infinity)"),
      njs_str("abcdefghijklmno") },

    { njs_str("'abcdefghijklmno'.slice('0', '5')"),
      njs_str("abcde") },

    { njs_str("'abcdefghijklmno'.slice(3, 5)"),
      njs_str("de") },

    { njs_str("'abcdefgh'.slice(3)"),
      njs_str("defgh") },

    { njs_str("'abcdefgh'.slice(undefined, undefined)"),
      njs_str("abcdefgh") },

    { njs_str("'abcdefgh'.slice(undefined)"),
      njs_str("abcdefgh") },

    { njs_str("'abcdefgh'.slice(undefined, 1)"),
      njs_str("a") },

    { njs_str("'abcdefgh'.slice(3, undefined)"),
      njs_str("defgh") },

    { njs_str("'abcde'.slice(50)"),
      njs_str("") },

    { njs_str("'abcde'.slice(1, 50)"),
      njs_str("bcde") },

    { njs_str("'abcdefgh'.slice(3, -2)"),
      njs_str("def") },

    { njs_str("'abcdefgh'.slice(5, 3)"),
      njs_str("") },

    { njs_str("'abcdefgh'.slice(100, 120)"),
      njs_str("") },

    { njs_str("String.prototype.substring(1, 5)"),
      njs_str("") },

    { njs_str("String.prototype.substr.call({toString:()=>{throw new Error('Oops')}})"),
      njs_str("Error: Oops") },

    { njs_str("String.prototype.slice(1, 5)"),
      njs_str("") },

    { njs_str("String.prototype.toBytes(1, 5)"),
      njs_str("") },

    { njs_str("'abc'.charAt(1 + 1)"),
      njs_str("c") },

    { njs_str("'abc'.charAt(3)"),
      njs_str("") },

    { njs_str("'abc'.charAt(undefined)"),
      njs_str("a") },

    { njs_str("'abc'.charAt(null)"),
      njs_str("a") },

    { njs_str("'abc'.charAt(false)"),
      njs_str("a") },

    { njs_str("'abc'.charAt(true)"),
      njs_str("b") },

    { njs_str("'abc'.charAt(NaN)"),
      njs_str("a") },

    { njs_str("'abc'.charAt(Infinity)"),
      njs_str("") },

    { njs_str("'abc'.charAt(-Infinity)"),
      njs_str("") },

    { njs_str("var o = { valueOf: function() {return 2} };"
                 "'abc'.charAt(o)"),
      njs_str("c") },

    { njs_str("var o = { toString: function() {return '2'} };"
                 "'abc'.charAt(o)"),
      njs_str("c") },

    { njs_str("'abc'.charCodeAt(1 + 1)"),
      njs_str("99") },

    { njs_str("'abc'.charCodeAt(3)"),
      njs_str("NaN") },

    { njs_str("'abc'.charCodeAt(undefined)"),
      njs_str("97") },

    { njs_str("var a = 'abcdef'; a.3"),
      njs_str("SyntaxError: Unexpected token \".3\" in 1") },

    { njs_str("'abcdef'[3]"),
      njs_str("d") },

    { njs_str("'abcdef'[0]"),
      njs_str("a") },

    { njs_str("'abcdef'[-1]"),
      njs_str("undefined") },

    { njs_str("'abcdef'[NaN]"),
      njs_str("undefined") },

    { njs_str("'abcdef'[3.5]"),
      njs_str("undefined") },

    { njs_str("'abcdef'[8]"),
      njs_str("undefined") },

    { njs_str("'abcdef'['1']"),
      njs_str("b") },

    { njs_str("'abcdef'[' 1']"),
      njs_str("undefined") },

    { njs_str("'abcdef'['1 ']"),
      njs_str("undefined") },

    { njs_str("'abcdef'['']"),
      njs_str("undefined") },

    { njs_str("'abcdef'['-']"),
      njs_str("undefined") },

    { njs_str("'abcdef'['-1']"),
      njs_str("undefined") },

    { njs_str("'abcdef'['01']"),
      njs_str("undefined") },

    { njs_str("'abcdef'['0x01']"),
      njs_str("undefined") },

    { njs_str("var a = 'abcdef', b = 1 + 2; a[b]"),
      njs_str("d") },

    /**/

    { njs_str("'abc'.toString()"),
      njs_str("abc") },

    { njs_str("''.toString.call('abc')"),
      njs_str("abc") },

    { njs_str("String.prototype.toString.call('abc')"),
      njs_str("abc") },

    { njs_str("String.prototype.toString.call(new String('abc'))"),
      njs_str("abc") },

    { njs_str("String.prototype.toString.call(1)"),
      njs_str("TypeError: unexpected value type:number") },

    { njs_str("'abc'.valueOf()"),
      njs_str("abc") },

    /**/

    { njs_str("var n = { toString: function() { return 1 } };   '12'[n]"),
      njs_str("2") },

    { njs_str("var n = { toString: function() { return '1' } }; '12'[n]"),
      njs_str("2") },

    { njs_str("var n = { toString: function() { return 1 },"
                          " valueOf:  function() { return 0 } };   '12'[n]"),
      njs_str("2") },

    /* Externals. */

    { njs_str("typeof $r"),
      njs_str("external") },

    { njs_str("var a = $r.uri, s = a.fromUTF8(); s.length +' '+ s"),
      njs_str("3 АБВ") },

    { njs_str("var a = $r.uri, b = $r2.uri, c = $r3.uri; a+b+c"),
      njs_str("АБВαβγabc") },

    { njs_str("var a = $r.uri; $r.uri = $r2.uri; $r2.uri = a; $r2.uri+$r.uri"),
      njs_str("АБВαβγ") },

    { njs_str("var a = $r.uri, s = a.fromUTF8(2); s.length +' '+ s"),
      njs_str("2 БВ") },

    { njs_str("var a = $r.uri, s = a.fromUTF8(2, 4); s.length +' '+ s"),
      njs_str("1 Б") },

    { njs_str("var a = $r.uri; a +' '+ a.length +' '+ a"),
      njs_str("АБВ 6 АБВ") },

    { njs_str("$r.uri = 'αβγ'; var a = $r.uri; a.length +' '+ a"),
      njs_str("6 αβγ") },

    { njs_str("$r.uri.length +' '+ $r.uri"),
      njs_str("6 АБВ") },

    { njs_str("$r.uri = $r.uri.substr(2); $r.uri.length +' '+ $r.uri"),
      njs_str("4 БВ") },

    { njs_str("'' + $r.props.a + $r2.props.a + $r.props.a"),
      njs_str("121") },

    { njs_str("var p1 = $r.props, p2 = $r2.props; '' + p2.a + p1.a"),
      njs_str("21") },

    { njs_str("var p1 = $r.props, p2 = $r2.props; '' + p1.a + p2.a"),
      njs_str("12") },

    { njs_str("var p = $r3.props; p.a = 1"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of external") },
    { njs_str("var p = $r3.props; delete p.a"),
      njs_str("TypeError: Cannot delete property \"a\" of external") },

    { njs_str("$r.vars.p + $r2.vars.q + $r3.vars.k"),
      njs_str("pvalqvalkval") },

    { njs_str("$r.vars.unset"),
      njs_str("undefined") },

    { njs_str("var v = $r3.vars; v.k"),
      njs_str("kval") },

    { njs_str("var v = $r3.vars; v.unset = 1; v.unset"),
      njs_str("1") },

    { njs_str("$r.vars.unset = 'a'; $r2.vars.unset = 'b';"
                 "$r.vars.unset + $r2.vars.unset"),
      njs_str("ab") },

    { njs_str("$r.vars.unset = 1; $r2.vars.unset = 2;"
                 "$r.vars.unset + $r2.vars.unset"),
      njs_str("12") },

    { njs_str("$r3.vars.p = 'a'; $r3.vars.p2 = 'b';"
                 "$r3.vars.p + $r3.vars.p2"),
      njs_str("ab") },

    { njs_str("$r3.vars.p = 'a'; delete $r3.vars.p; $r3.vars.p"),
      njs_str("undefined") },

    { njs_str("$r3.vars.p = 'a'; delete $r3.vars.p; $r3.vars.p = 'b'; $r3.vars.p"),
      njs_str("b") },

    { njs_str("$r3.vars.error = 1"),
      njs_str("Error: cannot set \"error\" prop") },

    { njs_str("delete $r3.vars.error"),
      njs_str("Error: cannot delete \"error\" prop") },

    { njs_str("delete $r3.vars.e"),
      njs_str("true") },

    { njs_str("$r3.consts.k"),
      njs_str("kval") },

    { njs_str("$r3.consts.k = 1"),
      njs_str("TypeError: Cannot assign to read-only property \"k\" of external") },

    { njs_str("delete $r3.consts.k"),
      njs_str("TypeError: Cannot delete property \"k\" of external") },

    { njs_str("delete $r3.vars.p; $r3.vars.p"),
      njs_str("undefined") },

    { njs_str("var a = $r.host; a +' '+ a.length +' '+ a"),
      njs_str("АБВГДЕЁЖЗИЙ 22 АБВГДЕЁЖЗИЙ") },

    { njs_str("var a = $r.host; a.substr(2, 2)"),
      njs_str("Б") },

    { njs_str("var a = $r.header['User-Agent']; a +' '+ a.length +' '+ a"),
      njs_str("User-Agent|АБВ 17 User-Agent|АБВ") },

    { njs_str("var a='', p;"
                 "for (p in $r.header) { a += p +':'+ $r.header[p] +',' }"
                 "a"),
      njs_str("01:01|АБВ,02:02|АБВ,03:03|АБВ,") },

    { njs_str("$r.some_method('YES')"),
      njs_str("АБВ") },

    { njs_str("$r.create('XXX').uri"),
      njs_str("XXX") },

    { njs_str("var sr = $r.create('XXX'); sr.uri = 'YYY'; sr.uri"),
      njs_str("YYY") },

    { njs_str("var sr = $r.create('XXX'), sr2 = $r.create('YYY');"
                 "sr.uri = 'ZZZ'; "
                 "sr.uri + sr2.uri"),
      njs_str("ZZZYYY") },

    { njs_str("var sr = $r.create('XXX'); sr.vars.p = 'a'; sr.vars.p"),
      njs_str("a") },

    { njs_str("var p; for (p in $r.some_method);"),
      njs_str("undefined") },

    { njs_str("'uri' in $r"),
      njs_str("true") },

    { njs_str("'one' in $r"),
      njs_str("false") },

    { njs_str("'a' in $r.props"),
      njs_str("true") },

    { njs_str("delete $r.uri"),
      njs_str("TypeError: Cannot delete property \"uri\" of external") },

    { njs_str("delete $r.one"),
      njs_str("TypeError: Cannot delete property \"one\" of external") },

    { njs_str("$r.some_method.call($r, 'YES')"),
      njs_str("АБВ") },

    { njs_str("var f = $r.some_method.bind($r); f('YES')"),
      njs_str("АБВ") },

    { njs_str("function f(fn, arg) {return fn(arg);}; f($r.some_method.bind($r), 'YES')"),
      njs_str("АБВ") },

    { njs_str("$r.some_method.apply($r, ['YES'])"),
      njs_str("АБВ") },

    { njs_str("$r.some_method.call([], 'YES')"),
      njs_str("TypeError: external value is expected") },

    { njs_str("$r.nonexistent"),
      njs_str("undefined") },

    { njs_str("$r.error = 'OK'"),
      njs_str("TypeError: Cannot assign to read-only property \"error\" of external") },

    { njs_str("var a = { toString: function() { return 1 } }; a"),
      njs_str("1") },

    { njs_str("var a = { valueOf: function() { return 1 } };  a"),
      njs_str("[object Object]") },

    { njs_str("var a = { toString: 2,"
                 "          valueOf: function() { return 1 } };  a"),
      njs_str("1") },

    { njs_str("var a = { toString: function() { return [] },"
                 "          valueOf: function() { return 1 } };  a"),
      njs_str("1") },

    { njs_str("var a = { toString: function() { return [] },"
                 "          valueOf: function() { return 1 } };"
                 "var o = {}; o[a] = 'test'"),
      njs_str("test") },

    { njs_str("({})[{}] = 'test'"),
      njs_str("test") },

    { njs_str("var o = {b:$r.props.b}; o.b"),
      njs_str("42") },

    { njs_str("$r2.uri == 'αβγ' && $r2.uri === 'αβγ'"),
      njs_str("true") },

    /**/

    { njs_str("'абвгдеёжзийклмнопрстуфхцчшщъыьэюя'.charCodeAt(5)"),
      njs_str("1077") },

    { njs_str("'12345абвгдеёжзийклмнопрстуфхцчшщъыьэюя'.charCodeAt(35)"),
      njs_str("1101") },

    { njs_str("'12345абвгдеёжзийклмнопрстуфхцчшщъыьэюя'.substring(35)"),
      njs_str("эюя") },

    { njs_str("'abcdef'.substr(-5, 4).substring(3, 1).charAt(1)"),
      njs_str("d") },

    { njs_str("'abcdef'.substr(2, 4).charAt(2)"),
      njs_str("e") },

    { njs_str("var a = 'abcdef'.substr(2, 4).charAt(2).length; a"),
      njs_str("1") },

    { njs_str("var a = 'abcdef'.substr(2, 4).charAt(2) + '1234'; a"),
      njs_str("e1234") },

    { njs_str("var a = ('abcdef'.substr(2, 5 * 2 - 6).charAt(2) + '1234')"
                 "         .length; a"),
      njs_str("5") },

    { njs_str("String.fromCharCode('_').charCodeAt(0)"),
      njs_str("0") },

    { njs_str("String.fromCodePoint('_')"),
      njs_str("RangeError") },

    { njs_str("String.fromCharCode(65.14)"),
      njs_str("A") },

    { njs_str("String.fromCodePoint(3.14)"),
      njs_str("RangeError") },

    { njs_str("String.fromCharCode(65.14 + 65536)"),
      njs_str("A") },

    { njs_str("String.fromCodePoint(65 + 65536)"),
      njs_str("𐁁") },

    { njs_str("String.fromCharCode(2**53 + 10)"),
      njs_str("\n") },

    { njs_str("String.fromCodePoint(1114111 + 1)"),
      njs_str("RangeError") },

    { njs_str("String.fromCharCode(65, 90) + String.fromCodePoint(65, 90)"),
      njs_str("AZAZ") },

    { njs_str("String.fromCharCode(945, 946, 947) + String.fromCodePoint(945, 946, 947)"),
      njs_str("αβγαβγ") },

    { njs_str("(function() {"
                 "    var n;"
                 "    for (n = 0; n <= 65536; n++) {"
                 "        if (String.fromCharCode(n).charCodeAt(0) !== n)"
                 "            return n;"
                 "    }"
                 "    return -1"
                 "})()"),
      njs_str("65536") },

#if (!NJS_HAVE_MEMORY_SANITIZER) /* very long test under MSAN */
    { njs_str("(function() {"
                 "    var n;"
                 "    for (n = 0; n <= 1114111; n++) {"
                 "        if (String.fromCodePoint(n).codePointAt(0) !== n)"
                 "            return n;"
                 "    }"
                 "    return -1"
                 "})()"),
      njs_str("-1") },
#endif

    { njs_str("var a = 'abcdef'; function f(a) {"
                 "return a.slice(a.indexOf('cd')) } f(a)"),
      njs_str("cdef") },

    { njs_str("var a = 'abcdef'; a.slice(a.indexOf('cd'))"),
      njs_str("cdef") },

    { njs_str("'abcdef'.indexOf('de', 2)"),
      njs_str("3") },

    { njs_str("'абвгдеёжзийклмнопрстуфхцчшщъыьэюя'.indexOf('лмно', 10)"),
      njs_str("12") },

    { njs_str("'abcdef'.indexOf('a', 10)"),
      njs_str("-1") },

    { njs_str("'abcdef'.indexOf('q', 0)"),
      njs_str("-1") },

    { njs_str("'abcdef'.indexOf('', 10)"),
      njs_str("6") },

    { njs_str("'abcdef'.indexOf('', 3)"),
      njs_str("3") },

    { njs_str("'12345'.indexOf()"),
      njs_str("-1") },

    { njs_str("''.indexOf('')"),
      njs_str("0") },

    { njs_str("'12345'.indexOf(45, '0')"),
      njs_str("3") },

    { njs_str("'12'.indexOf('12345')"),
      njs_str("-1") },

    { njs_str("''.indexOf.call(12345, 45, '0')"),
      njs_str("3") },

    { njs_str("'abc'.lastIndexOf('abcdef')"),
      njs_str("-1") },

    { njs_str("'abc abc abc abc'.lastIndexOf('abc')"),
      njs_str("12") },

    { njs_str("'abc abc abc abc'.lastIndexOf('abc', 11)"),
      njs_str("8") },

    { njs_str("'abc abc abc abc'.lastIndexOf('abc', 0)"),
      njs_str("0") },

    { njs_str("'abc abc abc abc'.lastIndexOf('', 0)"),
      njs_str("0") },

    { njs_str("'abc abc abc abc'.lastIndexOf('', 5)"),
      njs_str("5") },

    { njs_str("'abc abc абвгд abc'.lastIndexOf('абвгд')"),
      njs_str("8") },

    { njs_str("'abc abc абвгд abc'.lastIndexOf('абвгд', undefined)"),
      njs_str("8") },

    { njs_str("'abc abc абвгд abc'.lastIndexOf('абвгд', NaN)"),
      njs_str("8") },

    { njs_str("'abc abc абвгд abc'.lastIndexOf('абвгд', {})"),
      njs_str("8") },

    { njs_str("String.prototype.lastIndexOf.call({toString:()=>'abc abc абвгд abc'}, 'абвгд')"),
      njs_str("8") },

    { njs_str("'abc abc абвгдежз'.lastIndexOf('абвгд')"),
      njs_str("8") },

    { njs_str("'abc abc абвгдежз'.lastIndexOf('абвгд', 11)"),
      njs_str("8") },

    { njs_str("'abc abc абвгдежз'.lastIndexOf('абвгд', 12)"),
      njs_str("8") },

    { njs_str("'abc abc абвгдежз'.lastIndexOf('абвгд', 13)"),
      njs_str("8") },

    { njs_str("'abc abc абвгдежз'.lastIndexOf('абвгд', 14)"),
      njs_str("8") },

    { njs_str("'abc abc абвгдежз'.lastIndexOf('абвгд', 15)"),
      njs_str("8") },

    { njs_str("'abc abc абвгдежз'.lastIndexOf('')"),
      njs_str("16") },

    { njs_str("'abc abc абвгдежз'.lastIndexOf('', 12)"),
      njs_str("12") },

    { njs_str("''.lastIndexOf('')"),
      njs_str("0") },

    { njs_str("''.lastIndexOf()"),
      njs_str("-1") },

    { njs_str("''.lastIndexOf(undefined)"),
      njs_str("-1") },

    { njs_str("'β'.repeat(32).lastIndexOf('β')"),
      njs_str("31") },

    { njs_str("'β'.repeat(32).lastIndexOf``"),
      njs_str("32") },

    { njs_str("JSON.stringify(Array(24).fill(true).map((v,i) => 'abc abc ab abc абвгдежзab'.lastIndexOf('abc', i)))"
                 "== JSON.stringify([].concat(Array(4).fill(0), Array(7).fill(4), Array(13).fill(11)))"),
      njs_str("true") },

    { njs_str("''.includes('')"),
      njs_str("true") },

    { njs_str("'12345'.includes()"),
      njs_str("false") },

    { njs_str("'абвгдеёжзийклмнопрстуфхцчшщъыьэюя'.includes('лмно', 10)"),
      njs_str("true") },

    { njs_str("'абв абв абвгдежз'.includes('абвгд', 7)"),
      njs_str("true") },

    { njs_str("'абв абв абвгдежз'.includes('абвгд', 8)"),
      njs_str("true") },

    { njs_str("'абв абв абвгдежз'.includes('абвгд', 9)"),
      njs_str("false") },

    { njs_str("var i = 0; var o = {get length() {i++}};"
              "Array.prototype.includes.call(o); i"),
      njs_str("1") },

    { njs_str("[,,,].includes(undefined)"),
      njs_str("true") },

    { njs_str("''.startsWith('')"),
      njs_str("true") },

    { njs_str("'12345'.startsWith()"),
      njs_str("false") },

    { njs_str("'abc'.startsWith('abc')"),
      njs_str("true") },

    { njs_str("'abc'.startsWith('abc', 1)"),
      njs_str("false") },

    { njs_str("'abc'.startsWith('abc', -1)"),
      njs_str("true") },

    { njs_str("'абв абв абвгдежз'.startsWith('абвгд', 8)"),
      njs_str("true") },

    { njs_str("'абв абв абвгдежз'.startsWith('абвгд', 9)"),
      njs_str("false") },

    { njs_str("''.endsWith('')"),
      njs_str("true") },

    { njs_str("'12345'.endsWith()"),
      njs_str("false") },

    { njs_str("'abc'.endsWith('abc')"),
      njs_str("true") },

    { njs_str("'abc'.endsWith('abc', 4)"),
      njs_str("true") },

    { njs_str("'abc'.endsWith('abc', 1)"),
      njs_str("false") },

    { njs_str("'abc'.endsWith('abc', -1)"),
      njs_str("true") },

    { njs_str("'абв абв абвгдежз'.endsWith('абвгд', 13)"),
      njs_str("true") },

    { njs_str("'абв абв абвгдежз'.endsWith('абвгд', 14)"),
      njs_str("false") },

    { njs_str("'\x00АБВГДЕЁЖЗ'.toLowerCase().length"),
      njs_str("10") },

    { njs_str("'ΑΒΓ'.toLowerCase()"),
      njs_str("αβγ") },

    { njs_str("'АБВ'.toLowerCase()"),
      njs_str("абв") },

    { njs_str("'Ȿ'.repeat(256).toLowerCase() === 'ȿ'.repeat(256)"),
      njs_str("true") },

    { njs_str("'abc'.toUpperCase()"),
      njs_str("ABC") },

    { njs_str("'αβγ'.toUpperCase()"),
      njs_str("ΑΒΓ") },

    { njs_str("'ȿ'.repeat(256).toUpperCase() === 'Ȿ'.repeat(256)"),
      njs_str("true") },

    { njs_str("'\x00абвгдеёжз'.toUpperCase().length"),
      njs_str("10") },

    { njs_str("['ȿ', 'Ȿ', 'ȿ'.toUpperCase(), 'Ȿ'.toLowerCase()].map((v)=>v.toUTF8().length)"),
      njs_str("2,3,3,2") },

#if (!NJS_HAVE_MEMORY_SANITIZER) /* very long tests under MSAN */
    { njs_str("var a = [], code;"
                 "for (code = 0; code <= 1114111; code++) {"
                 "    var s = String.fromCodePoint(code);"
                 "    var n = s.toUpperCase();"
                 "    if (s != n && s != n.toLowerCase())"
                 "        a.push(code);"
                 "} a"),
      njs_str("181,305,383,453,456,459,498,837,962,976,977,981,982,1008,1009,1013,7296,7297,7298,7299,7300,7301,7302,7303,7304,7835,8126") },

    { njs_str("var a = [], code;"
                 "for (code = 0; code <= 1114111; code++) {"
                 "    var s = String.fromCodePoint(code);"
                 "    var n = s.toLowerCase();"
                 "    if (s != n && s != n.toUpperCase())"
                 "        a.push(code);"
                 "} a"),
      njs_str("304,453,456,459,498,1012,7838,8486,8490,8491") },
#endif

    { njs_str("'abc'.trimStart().trim().trimEnd()"),
      njs_str("abc") },

    { njs_str("''.trim()"),
      njs_str("") },

    { njs_str("'    '.trim()"),
      njs_str("") },

    { njs_str("'abc  '.trimEnd()"),
      njs_str("abc") },

    { njs_str("'   abc'.trimStart()"),
      njs_str("abc") },

    { njs_str("'   abc  '.trim()"),
      njs_str("abc") },

    { njs_str("'абв  '.trimEnd()"),
      njs_str("абв") },

    { njs_str("'   абв'.trimStart()"),
      njs_str("абв") },

    { njs_str("'   абв  '.trimStart().trimEnd()"),
      njs_str("абв") },

    { njs_str("'\\u2029abc\\uFEFF\\u2028'.trim()"),
      njs_str("abc") },

#if (!NJS_HAVE_MEMORY_SANITIZER) /* very long test under MSAN */
    { njs_str("var a = [], code;"
                 "for (code = 0; code <= 1114111; code++) {"
                 "    var ws = String.fromCodePoint(code);"
                 "    if ((ws + '-' + ws).trim() === '-')"
                 "        a.push(code);"
                 "} a"),
      njs_str("9,10,11,12,13,32,160,5760,8192,8193,8194,8195,8196,8197,8198,8199,8200,8201,8202,8232,8233,8239,8287,12288,65279") },
#endif

    { njs_str("'abcdefgh'.search()"),
      njs_str("0") },

    { njs_str("'abcdefgh'.search('')"),
      njs_str("0") },

    { njs_str("'abcdefgh'.search(undefined)"),
      njs_str("0") },

    { njs_str("'abcdefgh'.search(/def/)"),
      njs_str("3") },

    { njs_str("'abcdefgh'.search('def')"),
      njs_str("3") },

    { njs_str("'123456'.search('45')"),
      njs_str("3") },

    { njs_str("'123456'.search(45)"),
      njs_str("3") },

    { njs_str("'123456'.search(String(45))"),
      njs_str("3") },

    { njs_str("'123456'.search(Number('45'))"),
      njs_str("3") },

    { njs_str("var r = { toString: function() { return '45' } };"
                 "'123456'.search(r)"),
      njs_str("3") },

    { njs_str("var r = { toString: function() { return 45 } };"
                 "'123456'.search(r)"),
      njs_str("3") },

    { njs_str("var r = { toString: function() { return /45/ } };"
                 "'123456'.search(r)"),
      njs_str("TypeError: Cannot convert object to primitive value") },

    { njs_str("var r = { toString: function() { return /34/ },"
                 "          valueOf:  function() { return 45 } };"
                 "'123456'.search(r)"),
      njs_str("3") },

    { njs_str("'abcdefgh'.replace()"),
      njs_str("abcdefgh") },

    { njs_str("'abcdefgh'.replace('d')"),
      njs_str("abcundefinedefgh") },

    { njs_str("'abcdefgh'.replace('d', undefined)"),
      njs_str("abcundefinedefgh") },

    { njs_str("'a'.repeat(16).replace('a'.repeat(17)) === 'a'.repeat(16)"),
      njs_str("true") },

    { njs_str("'abcdefgh'.replace('d', null)"),
      njs_str("abcnullefgh") },

    { njs_str("'abcdefgh'.replace('d', 1)"),
      njs_str("abc1efgh") },

    { njs_str("'abcdefghdijklm'.replace('d', 'X')"),
      njs_str("abcXefghdijklm") },

    { njs_str("'абвгдежгийклм'.replace('г', 'Г')"),
      njs_str("абвГдежгийклм") },

    { njs_str("'abcdefghdijklm'.replace('d',"
                 "   function(m, o, s) { return '|'+s+'|'+o+'|'+m+'|' })"),
      njs_str("abc|abcdefghdijklm|3|d|efghdijklm") },

    { njs_str("'abcdefgh'.replace('', 'X')"),
      njs_str("Xabcdefgh") },

    { njs_str("'abcdefghdijklm'.replace(/d/, 'X')"),
      njs_str("abcXefghdijklm") },

    { njs_str("'abcdefghdijklm'.replace(/d/,"
                 "   function(m, o, s) { return '|'+s+'|'+o+'|'+m+'|' })"),
      njs_str("abc|abcdefghdijklm|3|d|efghdijklm") },

    { njs_str("'abcdefghdijklm'.replace(/(d)/,"
                 "   function(m, p, o, s)"
                       "{ return '|'+s+'|'+o+'|'+m+'|'+p+'|' })"),
      njs_str("abc|abcdefghdijklm|3|d|d|efghdijklm") },

    { njs_str("'abcdefghdijklm'.replace(/x/, 'X')"),
      njs_str("abcdefghdijklm") },

    { njs_str("'abcdefghdijklm'.replace(/x/,"
                 "   function(m, o, s) { return '|'+s+'|'+o+'|'+m+'|' })"),
      njs_str("abcdefghdijklm") },

    { njs_str("'абвгдежгийклм'.replace(/г/, 'Г')"),
      njs_str("абвГдежгийклм") },

    { njs_str("'abcdefghdijklm'.replace(/d/g, 'X')"),
      njs_str("abcXefghXijklm") },

    { njs_str("'абвгдежгийклм'.replace(/г/g, 'Г')"),
      njs_str("абвГдежГийклм") },

    { njs_str("'abc12345#$*%'.replace(/([^\\d]*)(\\d*)([^\\w]*)/,"
                 "   function(match, p1, p2, p3) {"
                 "     return [p1, p2, p3].join('-')})"),
      njs_str("abc-12345-#$*%") },

    { njs_str("'ABCDEFGHDIJKLM'.replace(/[A-Z]/g,"
                 "   function(match) { return '-' + match.toLowerCase() })"),
      njs_str("-a-b-c-d-e-f-g-h-d-i-j-k-l-m") },

    { njs_str("'abcdbe'.replace(/(b)/g, '$')"),
      njs_str("a$cd$e") },

    { njs_str("'abcdbe'.replace(/(b)/g, '$2$23')"),
      njs_str("a$2$23cd$2$23e") },

    { njs_str("'abcdbe'.replace(/(b)/g, '$2$23X$$Y')"),
      njs_str("a$2$23X$Ycd$2$23X$Ye") },

    { njs_str("'abcdbe'.replace('b', '|$`X$\\'|')"),
      njs_str("a|aXcdbe|cdbe") },

    { njs_str("'abcdbe'.replace(/b/, '|$`X$\\'|')"),
      njs_str("a|aXcdbe|cdbe") },

    { njs_str("'abcdbefbgh'.replace(/b/g, '|$`X$\\'|')"),
      njs_str("a|aXcdbefbgh|cd|abcdXefbgh|ef|abcdbefXgh|gh") },

    { njs_str("'abc12345#$*%'.replace(/([^\\d]*)(\\d*)([^\\w]*)/,"
                 "                       '$1-$2-$3')"),
      njs_str("abc-12345-#$*%") },

    { njs_str("'$1,$2'.replace(/(\\$(\\d))/g, '$$1-$1$2')"),
      njs_str("$1-$11,$1-$22") },

    { njs_str("('β' + 'α'.repeat(33)+'β').replace(/(α+)(β+)/, function(m, p1) { return p1[32]; })"),
      njs_str("βα") },

    { njs_str("'abc'.replace(/(h*)(z*)(g*)/g, '$1nn$2zz$3')"),
      njs_str("nnzzannzzbnnzzcnnzz") },

    { njs_str("'abc'.replace(/(h*)(z*)/g, '$1nn$2zz$3yy')"),
      njs_str("nnzz$3yyannzz$3yybnnzz$3yycnnzz$3yy") },

    { njs_str("'ъ'.replace(/(h*)/g, '$1ЮЙ')"),
      njs_str("ЮЙъЮЙ") },

    { njs_str("'ъg'.replace(/(h*)/g, '$1ЮЙ')"),
      njs_str("ЮЙъЮЙgЮЙ") },

    { njs_str("'ъg'.replace(/(ъ*)/g, '$1ЮЙ')"),
      njs_str("ъЮЙЮЙgЮЙ") },

    { njs_str("'ъg'.replace(/(h*)/g, 'fg$1ЮЙ')"),
      njs_str("fgЮЙъfgЮЙgfgЮЙ") },

    { njs_str("'юgёfя'.replace(/(gё)/g, 'n$1i')"),
      njs_str("юngёifя") },

    { njs_str("'aabbccaa'.replace(/a*/g, '')"),
      njs_str("bbcc") },

    { njs_str("'aabbccaab'.replace(/z*/g, '')"),
      njs_str("aabbccaab") },

    { njs_str("'αβγ'.replace(/z*/g, '|')"),
      njs_str("|α|β|γ|") },

    { njs_str("''.replace(/a*/g, '')"),
      njs_str("") },

    { njs_str("'12345'.replace(3, () => 0)"),
      njs_str("12045") },

    { njs_str("'123'.replace(3, function() { return {toString: ()=>({})}; })"),
      njs_str("TypeError: Cannot convert object to primitive value") },

    { njs_str("'12345'.replace(3, () => ({toString: () => 'aaaa'}))"),
      njs_str("12aaaa45") },

    { njs_str("'abc'.replace(/(z*)/g, function v0() {return '124'})"),
      njs_str("124a124b124c124") },

    { njs_str("'abc'.replace(/(a*)/g, function v0() {return '124'})"),
      njs_str("124124b124c124") },

    { njs_str("'abc'.replace(/b/g, '$0')"),
      njs_str("a$0c") },

    { njs_str("typeof String.bytesFrom(Array(15).fill(0xE3)).replace(/^/g, 1)"),
      njs_str("string") },

#if 0 /* FIXME: PCRE limitation */
    { njs_str("'abc'.replace(/^/g, '|$&|')"),
      njs_str("||abc") },
#endif

    { njs_str("'abc'.replace(/b/g, '|$&|')"),
      njs_str("a|b|c") },

    { njs_str("'ABC'.replace(/((A)B)/g, '($1|$&|$2)')"),
      njs_str("(AB|AB|A)C") },

    { njs_str("'undefined'.replace(void 0, 'x')"),
      njs_str("x") },

    { njs_str("/]/"),
      njs_str("/\\]/") },

    { njs_str("/["),
      njs_str("SyntaxError: Unterminated RegExp \"/[\" in 1") },

    { njs_str("/[\\"),
      njs_str("SyntaxError: Unterminated RegExp \"/[\\\" in 1") },

    { njs_str("RegExp(']')"),
      njs_str("/\\]/") },

    { njs_str("RegExp('[\\\\')"),
      njs_str("SyntaxError: pcre_compile(\"[\\\") failed: \\ at end of pattern") },

    { njs_str("RegExp('[\\\\\\\\]]')"),
      njs_str("/[\\\\]\\]/") },

    { njs_str("/[\\\\]]/"),
      njs_str("/[\\\\]\\]/") },

    { njs_str("/\\]/"),
      njs_str("/\\]/") },

    { njs_str("RegExp('\\]')"),
      njs_str("/\\]/") },

    { njs_str("/ab]cd/"),
      njs_str("/ab\\]cd/") },

    { njs_str("/ab]/"),
      njs_str("/ab\\]/") },

    { njs_str("/]cd/"),
      njs_str("/\\]cd/") },

    { njs_str("RegExp('\\\\0').source[1]"),
      njs_str("0") },

    { njs_str("']'.match(/]/)"),
      njs_str("]") },

    { njs_str("'ab]cd'.match(/]/)"),
      njs_str("]") },

    { njs_str("'ab]'.match(/]/)"),
      njs_str("]") },

    { njs_str("']cd'.match(/]/)"),
      njs_str("]") },

    { njs_str("'ab]cd'.match(/\\]/)"),
      njs_str("]") },

    { njs_str("'abc'.match(/a*/g)"),
      njs_str("a,,,") },

    { njs_str("'abc'.match(/z*/g)"),
      njs_str(",,,") },

    { njs_str("'abc'.match(/.?/g)"),
      njs_str("a,b,c,") },

    { njs_str("''.match(/a*/g)"),
      njs_str("") },

    { njs_str("''.match(/.?/g)"),
      njs_str("") },

    { njs_str("'абв'.match(/я?/g)"),
      njs_str(",,,") },

    { njs_str("'αβγ'.match(/z*/g)"),
      njs_str(",,,") },

    { njs_str("'囲碁織'.match(/z*/g)"),
      njs_str(",,,") },

    { njs_str("'𝟘𝟙𝟚𝟛'.match(/z*/g)"),
      njs_str(",,,,") },

    { njs_str("'abcdefgh'.match()"),
      njs_str("") },

    { njs_str("'abcdefgh'.match('')"),
      njs_str("") },

    { njs_str("'abcdefgh'.match(undefined)"),
      njs_str("") },

    { njs_str("'abcdefgh'.match(/def/)"),
      njs_str("def") },

    { njs_str("'abcdefgh'.match('def')"),
      njs_str("def") },

    { njs_str("'123456'.match('45')"),
      njs_str("45") },

    { njs_str("'123456'.match(45)"),
      njs_str("45") },

    { njs_str("'123456'.match(String(45))"),
      njs_str("45") },

    { njs_str("'123456'.match(Number('45'))"),
      njs_str("45") },

    { njs_str("var r = { toString: function() { return '45' } };"
                 "'123456'.match(r)"),
      njs_str("45") },

    { njs_str("var r = { toString: function() { return 45 } };"
                 "'123456'.match(r)"),
      njs_str("45") },

    { njs_str("var r = { toString: function() { return /45/ } };"
                 "'123456'.match(r)"),
      njs_str("TypeError: Cannot convert object to primitive value") },

    { njs_str("var r = { toString: function() { return /34/ },"
                 "          valueOf:  function() { return 45 } };"
                 "'123456'.match(r)"),
      njs_str("45") },

    { njs_str("''.match(/^$/)"),
      njs_str("") },

    { njs_str("''.match(/^$/g)"),
      njs_str("") },

    { njs_str("'abcdefgh'.match(/def/)"),
      njs_str("def") },

    { njs_str("'abc abc abc'.match('abc')"),
      njs_str("abc") },

    { njs_str("'abc abc abc'.match(/abc/)"),
      njs_str("abc") },

    { njs_str("'abc abc abc'.match(/abc/g)"),
      njs_str("abc,abc,abc") },

    { njs_str("'abc ABC aBc'.match(/abc/ig)"),
      njs_str("abc,ABC,aBc") },

    { njs_str("var a = 'α'.match(/α/g)[0] + 'α';"
                 "a +' '+ a.length"),
      njs_str("αα 2") },

    { njs_str("('β' + 'α'.repeat(33) +'β').match(/α+/g)[0][32]"),
      njs_str("α") },

    { njs_str("var a = '\\u00CE\\u00B1'.toBytes().match(/α/g)[0] + 'α';"
                 "a +' '+ a.length"),
      njs_str("αα 4") },

    { njs_str("typeof String.bytesFrom(Array(15).fill(0xE3)).match(/^/g)"),
      njs_str("object") },

    { njs_str("'abc'.split()"),
      njs_str("abc") },

    { njs_str("'abc'.split(undefined)"),
      njs_str("abc") },

    { njs_str("''.split('').length"),
      njs_str("1") },

    { njs_str("'abc'.split('')"),
      njs_str("a,b,c") },

    { njs_str("'αβγ'.split('')"),
      njs_str("α,β,γ") },

    { njs_str("'囲碁織'.split('')"),
      njs_str("囲,碁,織") },

    { njs_str("'𝟘𝟙𝟚𝟛'.split('')"),
      njs_str("𝟘,𝟙,𝟚,𝟛") },

    { njs_str("'囲α碁α織'.split('α')"),
      njs_str("囲,碁,織") },

    { njs_str("'a'.repeat(16).split('a'.repeat(15))"),
      njs_str(",a") },

    { njs_str("('α'+'β'.repeat(33)).repeat(2).split('α')[1][32]"),
      njs_str("β") },

    { njs_str("'abc'.split('abc')"),
      njs_str(",") },

    { njs_str("'a bc def'.split(' ')"),
      njs_str("a,bc,def") },

    { njs_str("'a bc  def'.split(' ')"),
      njs_str("a,bc,,def") },

    { njs_str("'a bc  def'.split(' ', 3)"),
      njs_str("a,bc,") },

    { njs_str("'abc'.split('abc')"),
      njs_str(",") },

    { njs_str("'ab'.split('123')"),
      njs_str("ab") },

    { njs_str("''.split(/0/).length"),
      njs_str("1") },

    { njs_str("'abc'.split(/(?:)/)"),
      njs_str("a,b,c") },

    { njs_str("'a bc def'.split(/ /)"),
      njs_str("a,bc,def") },

    { njs_str("'a bc  def'.split(/ /)"),
      njs_str("a,bc,,def") },

    { njs_str("'abc'.split(/abc/)"),
      njs_str(",") },

    { njs_str("'0123456789'.split('').reverse().join('')"),
      njs_str("9876543210") },

    { njs_str("'abc'.repeat(3)"),
      njs_str("abcabcabc") },

    { njs_str("'абв'.repeat(3)"),
      njs_str("абвабвабв") },

    { njs_str("''.repeat(3)"),
      njs_str("") },

    { njs_str("'abc'.repeat(0)"),
      njs_str("") },

    { njs_str("'abc'.repeat(NaN)"),
      njs_str("") },

    { njs_str("'abc'.repeat(Infinity)"),
      njs_str("RangeError") },

    { njs_str("'abc'.repeat(-1)"),
      njs_str("RangeError") },

    { njs_str("''.repeat(-1)"),
      njs_str("RangeError") },

    { njs_str("'a'.repeat(2147483647)"),
      njs_str("RangeError") },

    { njs_str("'a'.repeat(2147483648)"),
      njs_str("RangeError") },

    { njs_str("'a'.repeat(Infinity)"),
      njs_str("RangeError") },

    { njs_str("'a'.repeat(NaN)"),
      njs_str("") },

    { njs_str("''.repeat(2147483646)"),
      njs_str("") },

    /* ES6: "". */
    { njs_str("''.repeat(2147483647)"),
      njs_str("RangeError") },

    { njs_str("''.repeat(2147483648)"),
      njs_str("RangeError") },

    { njs_str("''.repeat(Infinity)"),
      njs_str("RangeError") },

    { njs_str("''.repeat(NaN)"),
      njs_str("") },

    { njs_str("'abc'.padStart(7)"),
      njs_str("    abc") },

    { njs_str("'абв'.padStart(7)"),
      njs_str("    абв") },

    { njs_str("'abc'.padStart(3)"),
      njs_str("abc") },

    { njs_str("'абв'.padStart(0)"),
      njs_str("абв") },

    { njs_str("'abc'.padStart(NaN)"),
      njs_str("abc") },

    { njs_str("'abc'.padStart(2147483647)"),
      njs_str("RangeError") },

    { njs_str("'abc'.padStart(2147483646, '')"),
      njs_str("abc") },

    { njs_str("''.padStart(0, '')"),
      njs_str("") },

    { njs_str("'1'.padStart(5, 0)"),
      njs_str("00001") },

    { njs_str("''.padStart(1, 'я')"),
      njs_str("я") },

    { njs_str("'abc'.padStart(6, NaN)"),
      njs_str("NaNabc") },

    { njs_str("'abc'.padStart(11, 123)"),
      njs_str("12312312abc") },

    { njs_str("'abc'.padStart(6, 12345)"),
      njs_str("123abc") },

    { njs_str("'абв'.padStart(6, 'эюя')"),
      njs_str("эюяабв") },

    { njs_str("'абв'.padStart(4, 'эюя')"),
      njs_str("эабв") },

    { njs_str("'абв'.padStart(7, 'эюя')"),
      njs_str("эюяэабв") },

    { njs_str("'абв'.padStart(10, 'эю')"),
      njs_str("эюэюэюэабв") },

    { njs_str("'abc'.padStart(10, Symbol())"),
      njs_str("TypeError: Cannot convert a Symbol value to a string") },

    { njs_str("'1234'.padEnd(4)"),
      njs_str("1234") },

    { njs_str("'1234'.padEnd(-1)"),
      njs_str("1234") },

    { njs_str("'я'.padEnd(1)"),
      njs_str("я") },

    { njs_str("'1234'.padEnd(5)"),
      njs_str("1234 ") },

    { njs_str("'я'.padEnd(6)"),
      njs_str("я     ") },

    { njs_str("'я'.padEnd(2147483647)"),
      njs_str("RangeError") },

    { njs_str("'я'.padEnd(2147483646, '')"),
      njs_str("я") },

    { njs_str("''.padEnd(0, '')"),
      njs_str("") },

    { njs_str("'эю'.padEnd(3, 'я')"),
      njs_str("эюя") },

    { njs_str("''.padEnd(1, 0)"),
      njs_str("0") },

    { njs_str("'1234'.padEnd(8, 'abcd')"),
      njs_str("1234abcd") },

    { njs_str("'1234'.padEnd(10, 'abcd')"),
      njs_str("1234abcdab") },

    { njs_str("'1234'.padEnd(7, 'abcd')"),
      njs_str("1234abc") },

    { njs_str("'абв'.padEnd(5, 'ГД')"),
      njs_str("абвГД") },

    { njs_str("'абв'.padEnd(4, 'ГДУ')"),
      njs_str("абвГ") },

    { njs_str("'абвг'.padEnd(10, 'ДЕЖЗ')"),
      njs_str("абвгДЕЖЗДЕ") },

    { njs_str("'abc'.padEnd(10, Symbol())"),
      njs_str("TypeError: Cannot convert a Symbol value to a string") },

    { njs_str("String.bytesFrom({})"),
      njs_str("TypeError: value must be a string or array") },

    { njs_str("String.bytesFrom([1, 2, 0.23, '5', 'A']).toString('hex')"),
      njs_str("0102000500") },

    { njs_str("String.bytesFrom([NaN, Infinity]).toString('hex')"),
      njs_str("0000") },

    { njs_str("String.bytesFrom('', 'hex')"),
      njs_str("") },

    { njs_str("String.bytesFrom('00aabbcc', 'hex').toString('hex')"),
      njs_str("00aabbcc") },

    { njs_str("String.bytesFrom('deadBEEF##', 'hex').toString('hex')"),
      njs_str("deadbeef") },

    { njs_str("String.bytesFrom('aa0', 'hex').toString('hex')"),
      njs_str("aa") },

    { njs_str("String.bytesFrom('', 'base64')"),
      njs_str("") },

    { njs_str("String.bytesFrom('#', 'base64')"),
      njs_str("") },

    { njs_str("String.bytesFrom('QQ==', 'base64')"),
      njs_str("A") },

    { njs_str("String.bytesFrom('QQ', 'base64')"),
      njs_str("A") },

    { njs_str("String.bytesFrom('QUI=', 'base64')"),
      njs_str("AB") },

    { njs_str("String.bytesFrom('QUI', 'base64')"),
      njs_str("AB") },

    { njs_str("String.bytesFrom('QUJD', 'base64')"),
      njs_str("ABC") },

    { njs_str("String.bytesFrom('QUJDRA==', 'base64')"),
      njs_str("ABCD") },

    { njs_str("String.bytesFrom('', 'base64url')"),
      njs_str("") },

    { njs_str("String.bytesFrom('QQ', 'base64url')"),
      njs_str("A") },

    { njs_str("String.bytesFrom('QUI', 'base64url')"),
      njs_str("AB") },

    { njs_str("String.bytesFrom('QUJD', 'base64url')"),
      njs_str("ABC") },

    { njs_str("String.bytesFrom('QUJDRA', 'base64url')"),
      njs_str("ABCD") },

    { njs_str("String.bytesFrom('QUJDRA#', 'base64url')"),
      njs_str("ABCD") },

    { njs_str("encodeURI.name"),
      njs_str("encodeURI")},

    { njs_str("encodeURI.length"),
      njs_str("1")},

    { njs_str("encodeURI()"),
      njs_str("undefined")},

    { njs_str("encodeURI('012абв')"),
      njs_str("012%D0%B0%D0%B1%D0%B2")},

    { njs_str("encodeURI('~}|{`_^]\\\\[@?>=<;:/.-,+*)(\\\'&%$#\"! ')"),
      njs_str("~%7D%7C%7B%60_%5E%5D%5C%5B@?%3E=%3C;:/.-,+*)('&%25$#%22!%20")},

    { njs_str("encodeURIComponent.name"),
      njs_str("encodeURIComponent")},

    { njs_str("encodeURIComponent.length"),
      njs_str("1")},

    { njs_str("encodeURIComponent('~}|{`_^]\\\\[@?>=<;:/.-,+*)(\\\'&%$#\"! ')"),
      njs_str("~%7D%7C%7B%60_%5E%5D%5C%5B%40%3F%3E%3D%3C%3B%3A%2F.-%2C%2B*)('%26%25%24%23%22!%20")},

    { njs_str("decodeURI.name"),
      njs_str("decodeURI")},

    { njs_str("decodeURI.length"),
      njs_str("1")},

    { njs_str("decodeURI()"),
      njs_str("undefined")},

    { njs_str("decodeURI('%QQ')"),
      njs_str("URIError")},

    { njs_str("decodeURI('%')"),
      njs_str("URIError")},

    { njs_str("decodeURI('%0')"),
      njs_str("URIError")},

    { njs_str("decodeURI('%00')"),
      njs_str("\0")},

    { njs_str("decodeURI('%3012%D0%B0%D0%B1%D0%B2')"),
      njs_str("012абв")},

    { njs_str("decodeURI('%7e%7d%7c%7b%60%5f%5e%5d%5c%5b%40%3f%3e%3d%3c%3b%3a%2f%2e%2c%2b%2a%29%28%27%26%25%24%23%22%21%20')"),
      njs_str("~}|{`_^]\\[%40%3f>%3d<%3b%3a%2f.%2c%2b*)('%26%%24%23\"! ")},

    { njs_str("decodeURIComponent.name"),
      njs_str("decodeURIComponent")},

    { njs_str("decodeURIComponent.length"),
      njs_str("1")},

    { njs_str("decodeURIComponent('%7e%7d%7c%7b%60%5f%5e%5d%5c%5b%40%3f%3e%3d%3c%3b%3a%2f%2e%2c%2b%2a%29%28%27%26%25%24%23%22%21%20')"),
      njs_str("~}|{`_^]\\[@?>=<;:/.,+*)('&%$#\"! ")},

    { njs_str("decodeURI('%41%42%43').length"),
      njs_str("3")},

    { njs_str("decodeURI('%D0%B0%D0%B1%D0%B2').length"),
      njs_str("3")},

    { njs_str("decodeURI('%80%81%82').length"),
      njs_str("3")},

    /* Functions. */

    { njs_str("return"),
      njs_str("SyntaxError: Illegal return statement in 1") },

    { njs_str("{return}"),
      njs_str("SyntaxError: Illegal return statement in 1") },

    { njs_str("\n{\nreturn;\n}"),
      njs_str("SyntaxError: Illegal return statement in 3") },

    { njs_str("if (1) function f(){}"),
      njs_str("SyntaxError: Functions can only be declared at top level or inside a block in 1") },

    { njs_str("if (1) { function f(){}}"),
      njs_str("undefined") },

    { njs_str("while (1) function f() { }"),
      njs_str("SyntaxError: Functions can only be declared at top level or inside a block in 1") },

    { njs_str("while (1) { break; function f(){}}"),
      njs_str("undefined") },

    { njs_str("for (;;) function f() { }"),
      njs_str("SyntaxError: Functions can only be declared at top level or inside a block in 1") },

    { njs_str("for (;;) { break; function f(){}}"),
      njs_str("undefined") },

    { njs_str("do function f() { } while (0)"),
      njs_str("SyntaxError: Functions can only be declared at top level or inside a block in 1") },

    { njs_str("function f() { return 1; } { function f() { return 2; } } f()"),
      njs_str("1") },

    { njs_str("function f() { return 1; } { function f() { return 2; } { function f() { return 3; } }} f()"),
      njs_str("1") },

    { njs_str("{function f() {} {} f() }"),
      njs_str("undefined") },

    { njs_str("{ var f; function f() {} }"),
      njs_str("SyntaxError: \"f\" has already been declared in 1") },

    { njs_str("{ function f() {} var f; }"),
      njs_str("SyntaxError: \"f\" has already been declared in 1") },

    { njs_str("{ function f() {} { var f }}"),
      njs_str("SyntaxError: \"f\" has already been declared in 1") },

#if NJS_HAVE_LARGE_STACK
    { njs_str("function f() { return f() } f()"),
      njs_str("RangeError: Maximum call stack size exceeded") },
#endif

    { njs_str("function () { } f()"),
      njs_str("SyntaxError: Unexpected token \"(\" in 1") },

    { njs_str("function f() { }"),
      njs_str("undefined") },

    { njs_str("function f() { }; f.length"),
      njs_str("0") },

    { njs_str("function f() { }; f.length = 1"),
      njs_str("TypeError: Cannot assign to read-only property \"length\" of function") },

    { njs_str("function f(...rest) { }; f.length"),
      njs_str("0") },

    { njs_str("function f(...rest) { }; var binded = f.bind(this, [1,2]);"
                 "binded.length"),
      njs_str("0") },

    { njs_str("function f(a,a) { };"),
      njs_str("SyntaxError: Duplicate parameter names in 1") },

    { njs_str("function f(a,b,a) { };"),
      njs_str("SyntaxError: Duplicate parameter names in 1") },

    { njs_str("function f(a, ...a) { };"),
      njs_str("SyntaxError: Duplicate parameter names in 1") },

    { njs_str("(function(a,a) { })"),
      njs_str("SyntaxError: Duplicate parameter names in 1") },

    { njs_str("(function(a,...a) { })"),
      njs_str("SyntaxError: Duplicate parameter names in 1") },

    { njs_str("(function f(a,a) { })"),
      njs_str("SyntaxError: Duplicate parameter names in 1") },

    { njs_str("(function f(a,...a) { })"),
      njs_str("SyntaxError: Duplicate parameter names in 1") },

    { njs_str("function f(a,b) { }; f.length"),
      njs_str("2") },

    { njs_str("function f(a,...rest) { }; f.length"),
      njs_str("1") },

    { njs_str("function f(a,b) { }; var ff = f.bind(f, 1); ff.length"),
      njs_str("1") },

    { njs_str("Object((new Date(0)).toJSON())+0"),
      njs_str("1970-01-01T00:00:00.000Z0") },

    { njs_str("Object((new Array(0)).toString())+0"),
      njs_str("0") },

    { njs_str("JSON.parse.length"),
      njs_str("2") },

    { njs_str("JSON.parse.bind(JSON, '[]').length"),
      njs_str("1") },

    { njs_str("var o = {}; o.hasOwnProperty.length"),
      njs_str("1") },

    { njs_str("var x; function f() { }"),
      njs_str("undefined") },

    { njs_str("function f() { } f()"),
      njs_str("undefined") },

    { njs_str("function f() { ; } f()"),
      njs_str("undefined") },

    { njs_str("function f() { ;; } f()"),
      njs_str("undefined") },

    { njs_str("function f() { return } f()"),
      njs_str("undefined") },

    { njs_str("function f() { return; } f()"),
      njs_str("undefined") },

    { njs_str("function f() { return;; } f()"),
      njs_str("undefined") },

    { njs_str("function f() { return 1 } f()"),
      njs_str("1") },

    { njs_str("function f() { return 1; } f()"),
      njs_str("1") },

    { njs_str("function f() { return 1;; } f()"),
      njs_str("1") },

    { njs_str("function f() { return 1\n 2 } f()"),
      njs_str("1") },

    { njs_str("function f() { return 1\n 2 } f()"),
      njs_str("1") },

    { njs_str("(function f() { return 2.toString(); })()"),
      njs_str("SyntaxError: Unexpected token \"toString\" in 1") },

    { njs_str("(function f() { return 2..toString(); })()"),
      njs_str("2") },

    { njs_str("function f(a) { if (a) return 'OK' } f(1)+f(0)"),
      njs_str("OKundefined") },

    { njs_str("function f(a) { if (a) return 'OK'; } f(1)+f(0)"),
      njs_str("OKundefined") },

    { njs_str("function f(a) { if (a) return 'OK';; } f(1)+f(0)"),
      njs_str("OKundefined") },

    { njs_str("var a = 1; a()"),
      njs_str("TypeError: number is not a function") },

    { njs_str("var o = {a:1}; o.a()"),
      njs_str("TypeError: (intermediate value)[\"a\"] is not a function") },

    { njs_str("(function(){})()"),
      njs_str("undefined") },

    { njs_str("var q = 1; function x(a, b, c) { q = a } x(5); q"),
      njs_str("5") },

    { njs_str("function x(a) { while (a < 2) a++; return a + 1 } x(1) "),
      njs_str("3") },

    { njs_str("(function(){"
              "(function(){"
              "(function(){"
              "(function(){"
              "(function(){"
              "(function(){"
              "(function(){"
              "(function(){"
              "(function(){})"
              "})"
              "})"
              "})"
              "})"
              "})"
              "})"
              "})"
              "})"),
      njs_str("SyntaxError: The maximum function nesting level is \"8\" in 1") },

    { njs_str("Function.prototype.toString = function () {return 'X'};"
                 "eval"),
      njs_str("X") },

    { njs_str("var o = {f:function(x){ return x**2}}; o.f\n(2)"),
      njs_str("4") },

    { njs_str("var o = {f:function(x){ return x**2}}; o\n.f\n(2)"),
      njs_str("4") },

    { njs_str("var o = {f:function(x){ return x**2}}; o\n.\nf\n(2)"),
      njs_str("4") },

    { njs_str("function f(x){ return x**2}; [f(2)\n, f\n(2),\nf\n(\n2),\nf\n(\n2\n)]"),
      njs_str("4,4,4,4") },

    { njs_str("function f (x){ return x**2}; f\n(2)"),
      njs_str("4") },

    { njs_str("function f (x){ return x**2}; f\n(\n2)"),
      njs_str("4") },

    { njs_str("function f (x){ return x**2}; f\n(\n2\n)"),
      njs_str("4") },

    { njs_str("function f (x){ return x**2}; f\n(2\n)"),
      njs_str("4") },

    { njs_str("function f (x){ return x**2}; f(2\n)"),
      njs_str("4") },

    { njs_str("var fn = Function.prototype.call; fn.call(() => 1)"),
      njs_str("1") },

    { njs_str("var fn = Function.prototype.call; fn.call(fn, () => 1)"),
      njs_str("1") },

    { njs_str("var fn = Function.prototype.call; fn.call(fn, fn, () => 1)"),
      njs_str("1") },

    { njs_str("eval.call.call(Number)"),
      njs_str("0") },

    { njs_str("URIError.apply.apply(RegExp)"),
      njs_str("/(?:)/") },

    { njs_str("[0].some(function(){return Array.call.bind(isNaN)}())"),
      njs_str("false") },

    { njs_str("(function (undefined, NaN, Infinity){ return undefined + NaN + Infinity})('x', 'y', 'z')"),
      njs_str("xyz") },

    { njs_str("function f(undefined,NaN, Infinity){ return undefined + NaN + Infinity}; f('x', 'y', 'z')"),
      njs_str("xyz") },

    { njs_str("(function (Object, Array, Boolean){ return Object + Array + Boolean})('x', 'y', 'z')"),
      njs_str("xyz") },

    /* Recursive factorial. */

    { njs_str("function f(a) {"
                 "    if (a > 1)"
                 "        return a * f(a - 1)\n"
                 "    return 1"
                 "}"
                 "f(10)"),
      njs_str("3628800") },

    /* Recursive factorial. */

    { njs_str("function f(a) { return (a > 1) ? a * f(a - 1) : 1 } f(10)"),
      njs_str("3628800") },

    { njs_str("var g = function f(a) { return (a > 1) ? a * f(a - 1) : 1 };"
                 "g(10)"),
      njs_str("3628800") },

    { njs_str("(function f(a) { return (a > 1) ? a * f(a - 1) : 1 })(10)"),
      njs_str("3628800") },

    /* Nested functions and closures. */

    { njs_str("function f() { var x = 4; "
                 "function g() { return x }; return g(); } f()"),
      njs_str("4") },

    { njs_str("function f(a) { function g(b) { return a + b } return g }"
                 "var k = f('a'); k('b')"),
      njs_str("ab") },

    { njs_str("function f(a) { return function(b) { return a + b } }"
                 "var k = f('a'); k('b')"),
      njs_str("ab") },

    { njs_str("function f(a) { return function(b) { return a + b } }"
                 "var k = f('a'), m = f('b'); k('c') + m('d')"),
      njs_str("acbd") },

    { njs_str("function f(a) { return "
                 "function(b) { return function(c) { return a + b + c } } }"
                 "var g = f('a'), k = g('b'), m = g('c'); k('d') + m('e')"),
      njs_str("abdace") },

    { njs_str("function f(a) {"
                 "function g() { return a }; return g; }"
                 "var y = f(4); y()"),
      njs_str("4") },

    { njs_str("function f() { var x = 4; "
                 "return function() { return x } }"
                 "var y = f(); y()"),
      njs_str("4") },

    { njs_str("function f() { var x = 4;"
                 "function g() { if (1) { return 2 + x; } }; return g }"
                 "var y = f(); y()"),
      njs_str("6") },

    { njs_str("var x; var y = 4;"
                 "function f() { function h() { x = 3; return y; } }"),
      njs_str("undefined") },

    { njs_str("function f() {"
                 "    var a = 'a';"
                 "    if (0) { a = 'b' };"
                 "    function f2() { return a };"
                 "    return f2"
                 "};"
                 "f()()"),
      njs_str("a") },

    { njs_str("function f() {"
                 "    var a = 'a'; "
                 "    if (0) { if (0) {a = 'b'} };"
                 "    function f2() { return a };"
                 "    return f2"
                 "};"
                 "f()()"),
      njs_str("a") },

    { njs_str("function f() { var a = f2(); }"),
      njs_str("undefined") },

    { njs_str("function f() { var a = f2(); } f();"),
      njs_str("ReferenceError: \"f2\" is not defined in 1") },

    { njs_str("typeof Buffer !== 'undefined' ? Buffer : function Buffer(){}"),
      njs_str("[object Function]") },

    { njs_str("1 == 2 ? func() : '123'"),
      njs_str("123") },

    { njs_str("1 == 1 ? func() : '123'"),
      njs_str("ReferenceError: \"func\" is not defined in 1") },

    { njs_str("function f(){ if (1 == 1) { 1 == 2 ? some_var : '123' } }; f()"),
      njs_str("undefined") },

    { njs_str("function f(){ if (1 == 1) { 1 == 1 ? some_var : '123' } }; f()"),
      njs_str("ReferenceError: \"some_var\" is not defined in 1") },

    { njs_str("function f(){ if (1 == 1) { 1 == 2 ? some_func() : '123' } }; f()"),
      njs_str("undefined") },

    { njs_str("function f(){ if (1 == 1) { 1 == 1 ? some_func() : '123' } }; f()"),
      njs_str("ReferenceError: \"some_func\" is not defined in 1") },

    { njs_str("(function(){ function f() {return f}; return f()})()"),
      njs_str("[object Function]") },

    { njs_str("function  f() {}; f.toString = ()=> 'F'; ({'F':1})[f]"),
      njs_str("1") },

    { njs_str("var a = ''; "
                 "function f(list) {"
                 "    function add(v) {a+=v};"
                 "    list.forEach(function(v) {add(v)});"
                 "};"
                 "f(['a', 'b', 'c']); a"),
      njs_str("abc") },

    { njs_str("var l = [];"
                 "var f = function() { "
                 "    function f2() { l.push(f); l.push(f2); }; "
                 "    l.push(f); l.push(f2); "
                 "    f2(); "
                 "}; "
                 "f(); "
                 "l.every(function(v) {return typeof v == 'function'})"),
      njs_str("true") },

    { njs_str("var l = [];"
                 "function baz() {"
                 "  function foo(v) {"
                 "     function bar() { foo(0); }"
                 "     l.push(v);"
                 "     if (v === 1) { bar(); }"
                 "  }"
                 "  foo(1);"
                 "}; baz(); l"),
      njs_str("1,0") },

    { njs_str("var gen = (function(){  "
                 "           var s = 0; "
                 "           return { inc: function() {s++}, "
                 "                    s: function() {return s} };});"
                 "var o1 = gen(); var o2 = gen();"
                 "[o1.s(),o2.s(),o1.inc(),o1.s(),o2.s(),o2.inc(),o1.s(),o2.s()]"),
      njs_str("0,0,,1,0,,1,1") },

    /* Recursive fibonacci. */

    { njs_str("function fibo(n) {"
                 "    if (n > 1)"
                 "        return fibo(n-1) + fibo(n-2)\n"
                 "     return 1"
                 "}"
                 "fibo(10)"),
      njs_str("89") },

    { njs_str("function fibo(n) {"
                 "    if (n > 1)"
                 "        return fibo(n-1) + fibo(n-2)\n"
                 "     return '.'"
                 "}"
                 "fibo(10).length"),
      njs_str("89") },

    { njs_str("function fibo(n) {"
                 "    if (n > 1)"
                 "        return fibo(n-1) + fibo(n-2)\n"
                 "     return 1"
                 "}"
                 "fibo('10')"),
      njs_str("89") },

    { njs_str("function add(a, b) { return a + b }"
                 "function mul(a, b) { return a * b }"
                 "function f(a, b) {"
                 "    return a + mul(add(1, 2), add(2, 3)) + b"
                 "}"
                 "f(30, 70)"),
      njs_str("115") },

    { njs_str("function a(x, y) { return x + y }"
                 "function b(x, y) { return x * y }"
                 "a(3, b(4, 5))"),
      njs_str("23") },

    { njs_str("function x(n) { return n }; x('12'.substr(1))"),
      njs_str("2") },

    { njs_str("function f(a) { a *= 2 } f(10)"),
      njs_str("undefined") },

    { njs_str("function f() { return 5 } f()"),
      njs_str("5") },

    { njs_str("function g(x) { return x + 1 }"
                 "function f(x) { return x } f(g)(2)"),
      njs_str("3") },

    { njs_str("function f() { return 5 } f(1)"),
      njs_str("5") },

    { njs_str("function f() {} f()"),
      njs_str("undefined") },

    { njs_str("function f() {;} f()"),
      njs_str("undefined") },

    { njs_str("function f(){return} f()"),
      njs_str("undefined") },

    { njs_str("function f(){return;} f()"),
      njs_str("undefined") },

    { njs_str("function f(){return\n1} f()"),
      njs_str("undefined") },

    { njs_str("function f(a) { return a + 1 } var b = f(2); b"),
      njs_str("3") },

    { njs_str("var f = function(a) { a *= 2; return a }; f(10)"),
      njs_str("20") },

    { njs_str("var f = function b(a) { a *= 2; return a }; f(10)"),
      njs_str("20") },

    { njs_str("var f = function b(a) { a *= 2; return a }; b(10)"),
      njs_str("ReferenceError: \"b\" is not defined in 1") },

    { njs_str("var f; f = function(a) { a *= 2; return a }; f(10)"),
      njs_str("20") },

    { njs_str("var f; f = function b(a) { a *= 2; return a }; f(10)"),
      njs_str("20") },

    { njs_str("var a, f = a = function(a) { a *= 2; return a }; f(10)"),
      njs_str("20") },

    { njs_str("var a, f = a = function(a) { a *= 2; return a }; a(10)"),
      njs_str("20") },

    { njs_str("var f = function b(a) { a *= 2; return a } = 5"),
      njs_str("ReferenceError: Invalid left-hand side in assignment in 1") },

    { njs_str("function a() { return { x:2} }; var b = a(); b.x"),
      njs_str("2") },

    { njs_str("var a = {}; function f(a) { return a + 1 } a.b = f(2); a.b"),
      njs_str("3") },

    { njs_str("(function(x) { return x + 1 })(2)"),
      njs_str("3") },

    { njs_str("(function(x) { return x + 1 }(2))"),
      njs_str("3") },

    { njs_str("var a = function() { return 1 }(); a"),
      njs_str("1") },

    { njs_str("var a = (function() { return 1 })(); a"),
      njs_str("1") },

    { njs_str("var a = (function(a) { return a + 1 })(2); a"),
      njs_str("3") },

    { njs_str("var a = (function(a) { return a + 1 }(2)); a"),
      njs_str("3") },

    { njs_str("var a = +function(a) { return a + 1 }(2); a"),
      njs_str("3") },

    { njs_str("var a = -function(a) { return a + 1 }(2); a"),
      njs_str("-3") },

    { njs_str("var a = !function(a) { return a + 1 }(2); a"),
      njs_str("false") },

    { njs_str("var a = ~function(a) { return a + 1 }(2); a"),
      njs_str("-4") },

    { njs_str("var a = void function(a) { return a + 1 }(2); a"),
      njs_str("undefined") },

    { njs_str("var a = true && function(a) { return a + 1 }(2); a"),
      njs_str("3") },

    { njs_str("var a; a = 0, function(a) { return a + 1 }(2); a"),
      njs_str("0") },

    { njs_str("var a = (0, function(a) { return a + 1 }(2)); a"),
      njs_str("3") },

    { njs_str("var a = 0, function(a) { return a + 1 }(2); a"),
      njs_str("SyntaxError: Unexpected token \"function\" in 1") },

    { njs_str("var a = (0, function(a) { return a + 1 }(2)); a"),
      njs_str("3") },

    { njs_str("var a = +function f(a) { return a + 1 }(2);"
                 "var b = f(5); a"),
      njs_str("ReferenceError: \"f\" is not defined in 1") },

    { njs_str("var o = { f: function(a) { return a * 2 } }; o.f(5)"),
      njs_str("10") },

    { njs_str("var o = {}; o.f = function(a) { return a * 2 }; o.f(5)"),
      njs_str("10") },

    { njs_str("var o = { x: 1, f: function() { return this.x } }; o.f()"),
      njs_str("1") },

    { njs_str("var o = { x: 1, f: function(a) { return this.x += a } };"
                 "o.f(5) +' '+ o.x"),
      njs_str("6 6") },

    { njs_str("var f = function(a) { return 3 }; f.call()"),
      njs_str("3") },

    { njs_str("var f = function(a) { return this }; f.call(5)"),
      njs_str("5") },

    { njs_str("var f = function(a, b) { return this + a }; f.call(5, 1)"),
      njs_str("6") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "f.call(5, 1, 2)"),
      njs_str("8") },

    { njs_str("var f = function(a) { return 3 }; f.apply()"),
      njs_str("3") },

    { njs_str("var f = function(a) { return this }; f.apply(5)"),
      njs_str("5") },

    { njs_str("var f = function(a) { return this + a }; f.apply(5, 1)"),
      njs_str("TypeError: second argument is not an array-like object") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "f.apply(5, [1, 2])"),
      njs_str("8") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "f.apply(5, [1, 2], 3)"),
      njs_str("8") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "f.apply(5, {'length':2, '0':1, '1':2})"),
      njs_str("8") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "f.apply(5, {'length':2, '0':1, '1':2, '2':3})"),
      njs_str("8") },

    { njs_str("var f = function(a, b, c) { return this + a + b + c};"
                 "f.apply(\"a\", {'length':2, '0':1, '1':2, '2':3})"),
      njs_str("a12undefined") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "f.apply(5, {'length':3, '0':1, '1':2, '2':3})"),
      njs_str("8") },

    { njs_str("var f = function(a) { return this + a };"
                 "f.apply(5, {'nolength':3, '0':1, '1':2})"),
      njs_str("NaN") },

    { njs_str("var f = function(a) { return this };"
                 "f.apply(5, {'nolength':3, '0':1, '1':2})"),
      njs_str("5") },

    { njs_str("var f = function(a, b, c) { return this + a + b + c };"
                 "f.apply(\"a\", {'length':3, '0':1, '1':2})"),
      njs_str("a12undefined") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "f.apply(\"a\", {'length':2, '0':undefined, '1':null})"),
      njs_str("aundefinednull") },

    { njs_str("var f = function() { return this };"
                 "f.apply(123, {})"),
      njs_str("123") },

    { njs_str("String.prototype.concat.apply('a', "
                 "{length:2, 0:{toString:function() {return 'b'}}, 1:'c'})"),
      njs_str("abc") },

    { njs_str("String.prototype.concat.apply('a',"
                 "{length: {valueOf: () => 2}, 0:'b', 1:'c'})"),
      njs_str("abc") },

    { njs_str("var o = {0:'b', 1:'c'}; Object.defineProperty(o, 'length', {get: () => 2});"
              "String.prototype.concat.apply('a', o)"),
      njs_str("abc") },

    { njs_str("var a = function() { return 1 } + ''; a"),
      njs_str("[object Function]") },

    { njs_str("''.concat.call()"),
      njs_str("TypeError: \"this\" argument is null or undefined") },

    { njs_str("''.concat.call('a', 'b', 'c')"),
      njs_str("abc") },

    { njs_str("''.concat.call('a')"),
      njs_str("a") },

    { njs_str("''.concat.call('a', [ 'b', 'c' ])"),
      njs_str("ab,c") },

    { njs_str("''.concat.call('a', [ 'b', 'c' ], 'd')"),
      njs_str("ab,cd") },

    { njs_str("''.concat.apply()"),
      njs_str("TypeError: \"this\" argument is null or undefined") },

    { njs_str("''.concat.apply('a')"),
      njs_str("a") },

    { njs_str("''.concat.apply('a', 'b')"),
      njs_str("TypeError: second argument is not an array-like object") },

    { njs_str("''.concat.apply('a', [ 'b', 'c' ])"),
      njs_str("abc") },

    { njs_str("''.concat.apply('a', [ 'b', 'c' ], 'd')"),
      njs_str("abc") },

    { njs_str("[].join.call([1,2,3])"),
      njs_str("1,2,3") },

    { njs_str("[].join.call([1,2,3], ':')"),
      njs_str("1:2:3") },

    { njs_str("[].join.call([1,2,3], 55)"),
      njs_str("1552553") },

    { njs_str("[].join.call()"),
      njs_str("TypeError: cannot convert null or undefined to object") },

    { njs_str("[1,2,3].join(undefined)"),
      njs_str("1,2,3") },

    { njs_str("[].slice.call()"),
      njs_str("TypeError: cannot convert null or undefined to object") },

    { njs_str("function f(a) {} ; var a = f; var b = f; a === b"),
      njs_str("true") },

    { njs_str("function f() {} ; f.toString()"),
      njs_str("[object Function]") },

    { njs_str("function f() {}; f"),
      njs_str("[object Function]") },

    { njs_str("function f() {}; f = f + 1; f"),
      njs_str("[object Function]1") },

    { njs_str("function a() { return 1 }"
                 "function b() { return a }"
                 "function c() { return b }"
                 "c()()()"),
      njs_str("1") },

#if 0
    { njs_str("function f() {}; f += 1; f"),
      njs_str("[object Function]1") },
#endif

    { njs_str("function f() {}; function g() { return f }; g()"),
      njs_str("[object Function]") },

    { njs_str("function f(a) { return this+a }; var a = f; a.call('0', 1)"),
      njs_str("01") },

    { njs_str("function f(a) { return this+a }; f.call('0', 1)"),
      njs_str("01") },

    { njs_str("function f(a) { return this+a };"
                 "function g(f, a, b) { return f.call(a, b) }; g(f, '0', 1)"),
      njs_str("01") },

    { njs_str("function f(a) { return this+a };"
                 "var o = { g: function (f, a, b) { return f.call(a, b) } };"
                 "o.g(f, '0', 1)"),
      njs_str("01") },

    { njs_str("var concat = ''.concat; concat(1,2,3)"),
      njs_str("TypeError: \"this\" argument is null or undefined") },

    { njs_str("var concat = ''.concat; concat.call(1,2,3)"),
      njs_str("123") },

    { njs_str("var concat = ''.concat; concat.yes = 'OK';"
                 "concat.call(1,2,3, concat.yes)"),
      njs_str("123OK") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1'); b('2', '3')"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1', '2'); b('3')"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1', 2, '3'); b()"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1'); b.call('0', '2', '3')"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1', '2'); b.call('0', '3')"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1', '2', '3'); b.call('0')"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1', '2', '3'); b.call()"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1'); b.apply('0', ['2', '3'])"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1', '2'); b.apply('0', ['3'])"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1', '2', '3'); b.apply('0')"),
      njs_str("123") },

    { njs_str("var f = function(a, b) { return this + a + b };"
                 "var b = f.bind('1', '2', '3'); b.apply()"),
      njs_str("123") },

    { njs_str("function f(a, b) { return this + a + b }"
                 "var b = f.bind('1', '2', '3'); b.apply()"),
      njs_str("123") },

    { njs_str("function f() { var a; return (function() { a = 1; return a; }).bind()() } f()"),
      njs_str("1") },

    { njs_str("function f() { var a; function baz() { a = 1; return a; } return baz.bind()(); } f()"),
      njs_str("1") },

    { njs_str("(function(){ var a = 1; return (function() { return a; })})().bind()()"),
      njs_str("1") },

    { njs_str("var r = (function(){ var a = 1; return (function() { return {a,args:arguments}; })})().bind()('b');"
              "njs.dump(r)"),
      njs_str("{a:1,args:{0:'b'}}") },

    { njs_str("function f() { var a = 1; function baz() { return a; } return baz; } f().bind()()"),
      njs_str("1") },

    { njs_str("function f() { var t = 1; function baz() { return t; } return baz; }"
                 "f().bind()();"),
      njs_str("1") },

    { njs_str("(function(a) { var s = typeof g, q = g; var g = 1; s += typeof g; function g(b) { return a + b }; return q; })(1)(2)"),
      njs_str("3")},

    { njs_str("(function(a) { var g = f; var f = 1; function f() { return a; } return g; })(42)()"),
      njs_str("42") },

    { njs_str("function f(a, b) { return a + b }"
                 "f(3,4) === f.bind()(3,4)"),
      njs_str("true") },

    { njs_str("var obj = {prop:'abc'}; "
                 "var func = function(x) { "
                 "    return this === obj && x === 1 && arguments[0] === 1 "
                 "           && arguments.length === 1 && this.prop === 'abc';"
                 "};"
                 "Function.prototype.bind.call(func, obj, 1)()"),
      njs_str("true") },

    { njs_str("function F(a, b) { this.a = a + b }"
                 "var o = new F(1, 2);"
                 "o.a"),
      njs_str("3") },

    { njs_str("function F(a, b) { this.a = a + b; return { a: 7 } }"
                 "var o = new F(1, 2);"
                 "o.a"),
      njs_str("7") },

    { njs_str("function F(a, b) { return }"
                 "F.prototype.constructor === F"),
      njs_str("true") },

    { njs_str("function F() { return }"
                 "F.prototype.ok = 'OK';"
                 "var o = new F(); o.ok"),
      njs_str("OK") },

    { njs_str("function F() { return }"
                 "var o = new F();"
                 "o.constructor === F"),
      njs_str("true") },

    { njs_str("function F() { return }"
                 "var o = new F();"
                 "o.__proto__ === F.prototype"),
      njs_str("true") },

    { njs_str("var f = { F: function(){} }; var o = new f.F();"
                 "o.__proto__ === f.F.prototype"),
      njs_str("true") },

    { njs_str("function F(){}; typeof F.prototype"),
      njs_str("object") },

    { njs_str("var F = function (){}; typeof F.prototype"),
      njs_str("object") },

    { njs_str("var F = function (){}; F.prototype = NaN; ({}) instanceof F"),
      njs_str("TypeError: Function has non-object prototype in instanceof") },

    { njs_str("var F = function() {};"
              "[F, F].map((x)=>Object.getOwnPropertyDescriptor(x, 'prototype').writable)"
              ".every((x)=> x === true)"),
      njs_str("true") },

    { njs_str("var F = function() {}, a = {t: 1}, b = {t: 2}, x, y; "
              "F.prototype = a; x = new F();"
              "F.prototype = b; y = new F();"
              "x.t == 1 && y.t == 2"),
      njs_str("true") },

    { njs_str("var x = {}, y = function() {}, z; y.prototype = x; z = new y();"
              "(z instanceof y) && (z.__proto__ == y.prototype) && (x.isPrototypeOf(z))"),
      njs_str("true") },

    { njs_str("var x = {}, y = function() {}, z; y.prototype = x; z = new y();"
              "(z instanceof y) && (z.__proto__ == y.prototype) && (x.isPrototypeOf(z))"),
      njs_str("true") },

    { njs_str("[undefined, null, false, NaN, '']"
              ".map((x) => { var f = function() {}; f.prototype = x; "
              "              return Object.getPrototypeOf(new f()); })"
              ".every((x) => x == Object.prototype)"),
      njs_str("true") },

    { njs_str("[undefined, null, false, NaN, '']"
              ".map((x) => { var f = function() {}; f.prototype = x; return f; })"
              ".map((x) => { try { return ({} instanceof x) ? 1 : 2; } "
              "              catch (e) { return (e instanceof TypeError) ? 3 : 4; } })"
              ".every((x) => x == 3)"),
      njs_str("true")},

    { njs_str("new decodeURI('%00')"),
      njs_str("TypeError: function is not a constructor")},

    { njs_str("new ''.toString"),
      njs_str("TypeError: function is not a constructor")},

    { njs_str("function F() { return Number }"
                 "var o = new (F())(5);"
                 "typeof o +' '+ o"),
      njs_str("object 5") },

    { njs_str("function F() { return Number }"
                 "var o = new (F());"
                 "typeof o +' '+ o"),
      njs_str("object 0") },

    { njs_str("var o = new function F() { return Number }()(5);"
                 "typeof o +' '+ o"),
      njs_str("number 5") },

    { njs_str("var o = new (function F() { return Number }())(5);"
                 "typeof o +' '+ o"),
      njs_str("object 5") },

    { njs_str("var o = new (new function F() { return Number }())(5);"
                 "typeof o +' '+ o"),
      njs_str("object 5") },

    { njs_str("var o = new new function F() { return Number }()(5);"
                 "typeof o +' '+ o"),
      njs_str("object 5") },

    { njs_str("var b; function F(x) { return {a:x} }"
                 "function G(y) { b = y; return F }"
                 "var o = new G(3)(5);"
                 "b + ' ' + o.a"),
      njs_str("3 5") },

    { njs_str("var b; function F(x) { return {a:x} }"
                 "function G(y) { b = y; return F }"
                 "var o = new (new G(3))(5);"
                 "b + ' ' + o.a"),
      njs_str("3 5") },

    { njs_str("var b; function F(x) { return {a:x} }"
                 "function G(y) { b = y; return F }"
                 "var o = new new G(3)(5);"
                 "b + ' ' + o.a"),
      njs_str("3 5") },

    { njs_str("var b; function F(x) { return {a:x} }"
                 "var g = { G: function (y) { b = y; return F } };"
                 "var o = new new g.G(3)(5);"
                 "b + ' ' + o.a"),
      njs_str("3 5") },

    { njs_str("function a() { return function(x) { return x + 1 } }"
                 "var b = a(); b(2)"),
      njs_str("3") },

    /* arguments object. */

    { njs_str("arguments"),
      njs_str("SyntaxError: \"arguments\" object in global scope in 1") },

    { njs_str("{arguments}"),
      njs_str("SyntaxError: \"arguments\" object in global scope in 1") },

    { njs_str("var arguments"),
      njs_str("SyntaxError: Identifier \"arguments\" is forbidden in var declaration in 1") },

    { njs_str("for (var arguments in []) {}"),
      njs_str("SyntaxError: Identifier \"arguments\" is forbidden in var declaration in 1") },

    { njs_str("function arguments(){}"),
      njs_str("SyntaxError: Identifier \"arguments\" is forbidden in function declaration in 1") },

    { njs_str("(function () {arguments = [];})"),
      njs_str("SyntaxError: Identifier \"arguments\" is forbidden as left-hand in assignment in 1") },

    { njs_str("(function(){return arguments;})()"),
      njs_str("[object Arguments]") },

    { njs_str("(function(){return arguments[0];})(1,2,3)"),
      njs_str("1") },

    { njs_str("(function(){return arguments[2];})(1,2,3)"),
      njs_str("3") },

    { njs_str("(function(){return arguments[3];})(1,2,3)"),
      njs_str("undefined") },

    { njs_str("(function(a,b,c){return a;})(1,2,3)"),
      njs_str("1") },

    { njs_str("(function(a,b,c){arguments[0] = 4; return a;})(1,2,3)"),
      njs_str("1") },

    { njs_str("(function(a,b,c){a = 4; return arguments[0];})(1,2,3)"),
      njs_str("1") },

    { njs_str("function check(v) {if (v == false) {throw TypeError('Too few arguments')}}; "
                 "function f() {check(arguments.length > 1); return 1}; f()"),
      njs_str("TypeError: Too few arguments") },

    { njs_str("function check(v) {if (v == false) {throw TypeError('Too few arguments')}}; "
                 "function f() {check(arguments.length > 1); return 1}; f(1,2)"),
      njs_str("1") },

    { njs_str("(function(a,b){delete arguments[0]; return arguments[0]})(1,1)"),
      njs_str("undefined") },

    { njs_str("(function(){return arguments.length;})()"),
      njs_str("0") },

    { njs_str("(function(){return arguments.length;})(1,2,3)"),
      njs_str("3") },

    { njs_str("(function(){arguments.length = 1; return arguments.length;})(1,2,3)"),
      njs_str("1") },

     { njs_str("(function(){return arguments[3];}).bind(null, 0)('a','b','c')"),
       njs_str("c") },

    { njs_str("function sum() { var args = Array.prototype.slice.call(arguments); "
                 "return args.reduce(function(prev, curr) {return prev + curr})};"
                 "[sum(1), sum(1,2), sum(1,2,3), sum(1,2,3,4)]"),
      njs_str("1,3,6,10") },

    { njs_str("function concat(sep) { var args = Array.prototype.slice.call(arguments, 1); "
                 "return args.join(sep)};"
                 "[concat('.',1,2,3), concat('+',1,2,3,4)]"),
      njs_str("1.2.3,1+2+3+4") },

    /* strict mode restrictions */

    { njs_str("(function() {}).caller"),
      njs_str("TypeError: \"caller\", \"callee\", \"arguments\" properties may not be accessed") },

    { njs_str("(function() {}).arguments"),
      njs_str("TypeError: \"caller\", \"callee\", \"arguments\" properties may not be accessed") },

    { njs_str("var desc = Object.getOwnPropertyDescriptor(Object.getPrototypeOf(Math.min), 'caller');"
              "desc.get === desc.set"),
      njs_str("true") },

    { njs_str("var desc = Object.getOwnPropertyDescriptor(Object.getPrototypeOf(Math.min), 'caller');"
              "1/desc.get"),
      njs_str("NaN") },

    { njs_str("var p = Object.getPrototypeOf(function() {});"
              "var d = Object.getOwnPropertyDescriptor(p, 'caller');"
              "typeof d.get == 'function' && typeof d.get == typeof d.set"
              "                           && d.configurable && !d.enumerable"),
      njs_str("true") },

    { njs_str("var p = Object.getPrototypeOf(function() {});"
              "var d = Object.getOwnPropertyDescriptor(p, 'arguments');"
              "typeof d.get == 'function' && typeof d.get == typeof d.set"
              "                           && d.configurable && !d.enumerable"),
      njs_str("true") },

    { njs_str("(function(){return arguments.callee;})()"),
      njs_str("TypeError: \"caller\", \"callee\", \"arguments\" properties may not be accessed") },

    { njs_str("var f = function() { return arguments; };"
              "Object.getOwnPropertyDescriptor(f(), 'caller')"),
      njs_str("undefined") },

    { njs_str("var f = function() { return arguments; };"
              "var d = Object.getOwnPropertyDescriptor(f(), 'callee');"
              "typeof d.get == 'function' && typeof d.get == typeof d.set"
              "                           && !d.configurable && !d.enumerable"),
      njs_str("true") },

    /* rest parameters. */

    { njs_str("function myFoo(a,b,...other) { return other };"
                 "myFoo(1,2,3,4,5);" ),
      njs_str("3,4,5") },

    { njs_str("function myFoo(a,b,...other, c) { return other };"),
      njs_str("SyntaxError: Unexpected token \"c\" in 1") },

    { njs_str("function sum(a, b, c, ...other) { return a+b+c+other[2] };"
                 "sum(\"one \",2,\" three \",\"four \",\"five \",\"the long enough sixth argument \");"),
      njs_str("one 2 three the long enough sixth argument ") },

    { njs_str("function myFoo1(a,...other) { return other };"
                 "function myFoo2(a,b,...other) { return other };"
                 "myFoo1(1,2,3,4,5,myFoo2(1,2,3,4));"),
      njs_str("2,3,4,5,3,4") },

    { njs_str("function myFoo(...other) { return (other instanceof Array) };"
                 "myFoo(1);" ),
      njs_str("true") },

    { njs_str("function myFoo(a,...other) { return other.length };"
                 "myFoo(1,2,3,4,5);" ),
      njs_str("4") },

    { njs_str("function myFoo(a,b,...other) { return other };"
                 "myFoo(1,2);" ),
      njs_str("") },

    /* arrow functions. */

    { njs_str("()"),
      njs_str("SyntaxError: Unexpected token \")\" in 1") },

    { njs_str("() => "),
      njs_str("SyntaxError: Unexpected end of input in 1") },

    { njs_str("() => {"),
      njs_str("SyntaxError: Unexpected end of input in 1") },

    { njs_str("a\n => 1"),
      njs_str("SyntaxError: Unexpected token \"=>\" in 2") },

    { njs_str("new (()=>1)"),
      njs_str("TypeError: function is not a constructor")},

    { njs_str("(\n) => {}"),
      njs_str("[object Function]") },

    { njs_str("a => 1"),
      njs_str("[object Function]") },

    { njs_str("({f:()=>1, g:()=>2}).f()"),
      njs_str("1") },

    { njs_str("var f = f => {return 1;}; f()"),
      njs_str("1") },

    { njs_str("var f = (f) => {return 1;}; f()"),
      njs_str("1") },

    { njs_str("var f = (f, a, b) => {return 1;}; f()"),
      njs_str("1") },

    { njs_str("var f = () => {return 1;}; f()"),
      njs_str("1") },

    { njs_str("(f => {return 1;})()"),
      njs_str("1") },

    { njs_str("((f) => {return 1;})()"),
      njs_str("1") },

    { njs_str("(((f) => {return 1;}))()"),
      njs_str("1") },

    { njs_str("var f = f => 1; f()"),
      njs_str("1") },

    { njs_str("() => 1"),
      njs_str("[object Function]") },

    { njs_str("var f = ()=>{}; f()"),
      njs_str("undefined") },

    { njs_str("var f = ()=>({}); f()"),
      njs_str("[object Object]") },

    { njs_str("var materials = ['Hydrogen', 'Helium', 'Lithium', 'Beryllium'];"
                 "materials.map(material => { return material.length; });"),
      njs_str("8,6,7,9") },

    { njs_str("var materials = ['Hydrogen', 'Helium', 'Lithium', 'Beryllium'];"
                 "materials.map(material => material.length);"),
      njs_str("8,6,7,9") },

    { njs_str("var materials = ['Hydrogen', 'Helium', 'Lithium', 'Beryllium'];"
                 "materials.map(material => { material.length });"),
      njs_str(",,,") },

    { njs_str("function f(a, b, c) {a = 1; return () => { return arguments[1]; };};"
                 "f(1, 2, 3)('a', 'b');"),
      njs_str("2") },

    { njs_str("var f = (...c) => { return (function() { return arguments.length; }).bind(null, c); };"
                 "var x = f(1,'a',false, {}); x()"),
      njs_str("1") },

    { njs_str("var f = (...c) => { return (function() { return arguments.length; }).bind(null, c); };"
                 "var x = f(1,'a',false, {}); x(1,2,3)"),
      njs_str("4") },

    { njs_str("function Car(){ this.age = 0; (() => { this.age++;})();}"
                 "(new Car()).age"),
      njs_str("1") },

    { njs_str("function Car(){ this.age = 0; (function(){ this.age++;})();}"
                 "(new Car()).age"),
      njs_str("TypeError: cannot get property \"age\" of undefined") },

    /* arrow functions + global this. */

    { njs_str("(() => this)()"),
      njs_str("[object global]") },

    { njs_str("(() => this).call('abc')"),
      njs_str("[object global]") },

    { njs_str("(() => this).apply('abc')"),
      njs_str("[object global]") },

    { njs_str("(() => this).bind('abc')()"),
      njs_str("[object global]") },

    { njs_str("(function() { return (() => this); })()()"),
      njs_str("undefined") },

    { njs_str("(function() { return (() => this); }).call('abc')()"),
      njs_str("abc") },

    { njs_str("(function() { return (() => this); }).bind('abc')()()"),
      njs_str("abc") },

    { njs_str("(function() { return (() => this); })"
                 ".call('abc').call('bca')"),
      njs_str("abc") },

    { njs_str("(function() { return (() => this); })"
                 ".call('abc').bind('bca')()"),
      njs_str("abc") },

    { njs_str("(function() { return function() { return () => this; }; })"
                 ".call('bca').call('abc')()"),
      njs_str("abc") },

     { njs_str("var f = () => 1; f.prototype"),
       njs_str("undefined") },

     { njs_str("var f = (a,b) => 0; f.length"),
       njs_str("2") },

    { njs_str("var o = Object.create(f => 1); o.length = 3"),
      njs_str("TypeError: Cannot assign to read-only property \"length\" of object") },

    /* Scopes. */

    { njs_str("function f(x) { a = x } var a; f(5); a"),
      njs_str("5") },

    { njs_str("function f(x) { var a = x } var a = 2; f(5); a"),
      njs_str("2") },

    { njs_str("function f(a) { return a } var a = '2'; a + f(5)"),
      njs_str("25") },

    { njs_str("for (var i = 0; i < 5; i++); i"),
      njs_str("5") },

    { njs_str("for (var i = 0, j; i < 5; i++); j"),
      njs_str("undefined") },

    { njs_str("for (var i = 0, j, k = 3; i < 5; i++); k"),
      njs_str("3") },

    { njs_str("var o = { a: 1, b: 2, c: 3 }, s = ''; "
                 "for (var i in o) { s += i }; s"),
      njs_str("abc") },

    { njs_str("var o = { a: 1, b: 2, c: 3 }; for (var i in o); i"),
      njs_str("c") },

    { njs_str("var o = {}; i = 7; for (var i in o); i"),
      njs_str("7") },

    { njs_str("var a = [1,2,3]; for (var i in a); i"),
      njs_str("2") },

    /* RegExp. */

    { njs_str("/./x"),
      njs_str("SyntaxError: Invalid RegExp flags \"x\" in 1") },

    { njs_str("/"),
      njs_str("SyntaxError: Unterminated RegExp \"/\" in 1") },

    { njs_str("/a\n/"),
      njs_str("SyntaxError: Unterminated RegExp \"/a\" in 1") },

    { njs_str("/a\r/"),
      njs_str("SyntaxError: Unterminated RegExp \"/a\" in 1") },

    { njs_str("/a\\q/"),
      njs_str("/a\\q/") },

    { njs_str("/\\\\/"),
      njs_str("/\\\\/") },

    { njs_str("/\\\\\\/"),
      njs_str("SyntaxError: Unterminated RegExp \"/\\\\\\/\" in 1") },

    { njs_str("/\\\\\\\\/"),
      njs_str("/\\\\\\\\/") },

    { njs_str("/\\\\\\//"),
      njs_str("/\\\\\\//") },

    { njs_str("/[A-Z/]/"),
      njs_str("/[A-Z/]/") },

    { njs_str("/[A-Z\n]/"),
      njs_str("SyntaxError: Unterminated RegExp \"/[A-Z\" in 1") },

    { njs_str("/[A-Z\\\n]/"),
      njs_str("SyntaxError: Unterminated RegExp \"/[A-Z\\\" in 1") },

    { njs_str("/\\\n/"),
      njs_str("SyntaxError: Unterminated RegExp \"/\\\" in 1") },

    { njs_str("/^[A-Za-z0-9+/]{4}$/.test('////')"),
      njs_str("true") },

    { njs_str("'[]!\"#$%&\\'()*+,.\\/:;<=>?@\\^_`{|}-'.split('')"
                 ".every(ch=>/[\\]\\[!\"#$%&'()*+,.\\/:;<=>?@\\^_`{|}-]/.test(ch))"),
      njs_str("true") },

    { njs_str("/a\\q/.test('a\\q')"),
      njs_str("true") },

    { njs_str("/(\\.(?!com|org)|\\/)/.test('ah.info')"),
      njs_str("true") },

    { njs_str("/(/.test('')"),
      njs_str("SyntaxError: pcre_compile(\"(\") failed: missing ) in 1") },

    { njs_str("/+/.test('')"),
      njs_str("SyntaxError: pcre_compile(\"+\") failed: nothing to repeat at \"+\" in 1") },

    { njs_str("/^$/.test('')"),
      njs_str("true") },

    { njs_str("var a = /\\d/; a.test('123')"),
      njs_str("true") },

    { njs_str("var a = /\\d/; a.test('abc')"),
      njs_str("false") },

    { njs_str("/\\d/.test('123')"),
      njs_str("true") },

    { njs_str("/\\d/.test(123)"),
      njs_str("true") },

    { njs_str("/undef/.test()"),
      njs_str("true") },

    { njs_str("var s = { toString: function() { return 123 } };"
                 "/\\d/.test(s)"),
      njs_str("true") },

    { njs_str("/\\d/.test('abc')"),
      njs_str("false") },

    { njs_str("/abc/i.test('ABC')"),
      njs_str("true") },

#if (!NJS_HAVE_MEMORY_SANITIZER) /* FIXME */
    { njs_str("/абв/i.test('АБВ')"),
      njs_str("true") },
#endif

    { njs_str("var re = /<(?<key>[\\w\\-\\.\\:]+)>(?<body>.*?)<\\/\\1>/g;"
              "['<A>XXX</A>', '<A>XX</B>'].map(s=>re.test(s))"),
      njs_str("true,false") },

    { njs_str("/\\x80/.test('\\u0080')"),
      njs_str("true") },

    { njs_str("/\\x80/.test('\\u0080'.toBytes())"),
      njs_str("true") },

    { njs_str("/α/.test('\\u03B1')"),
      njs_str("true") },

    { njs_str("/α/.test('\\u00CE\\u00B1'.toBytes())"),
      njs_str("true") },

    { njs_str("/\\d/.exec('123')"),
      njs_str("1") },

    { njs_str("/\\d/.exec(123)"),
      njs_str("1") },

    { njs_str("/undef/.exec()"),
      njs_str("undef") },

    { njs_str("var s = { toString: function() { return 123 } };"
                 "/\\d/.exec(s)"),
      njs_str("1") },

    { njs_str("var a = /^$/.exec(''); a.length +' '+ a"),
      njs_str("1 ") },

    { njs_str("var r = /3/g; r.exec('123') +' '+ r.exec('3')"),
      njs_str("3 null") },

#if (!NJS_HAVE_MEMORY_SANITIZER) /* FIXME */
    { njs_str("var r = /бв/ig;"
                 "var a = r.exec('АБВ');"
                 "r.lastIndex +' '+ a +' '+ "
                 "r.source +' '+ r.source.length +' '+ r"),
      njs_str("3 БВ бв 2 /бв/gi") },
#endif

    { njs_str("var r = /\\x80/g; r.exec('\\u0081\\u0080'.toBytes());"
                 "r.lastIndex +' '+ r.source +' '+ r.source.length +' '+ r"),
      njs_str("1 \\x80 4 /\\x80/g") },

    { njs_str("var descs = Object.getOwnPropertyDescriptors(RegExp('a'));"
              "Object.keys(descs)"),
      njs_str("lastIndex") },

    { njs_str("var props = Object.getOwnPropertyDescriptor(RegExp('a'), 'lastIndex');"
              "props.writable && !props.enumerable && !props.configurable"),
      njs_str("true") },

    { njs_str("var re = /a/; re.lastIndex"),
      njs_str("0") },

    { njs_str("var re = /aα/g; re.exec('aα'.repeat(32)); re.lastIndex"),
      njs_str("2") },

    { njs_str("var re = new RegExp('α'.repeat(33), 'g'); re.exec('α'.repeat(33)); re.lastIndex"),
      njs_str("33") },

    { njs_str("var re = new RegExp('α'.repeat(33), 'g'); re.exec('α'.repeat(33)); "
              "re.lastIndex = 67; re.lastIndex"),
      njs_str("67") },

    { njs_str("var re = /a/; re.lastIndex = 4; Object.create(re).lastIndex"),
      njs_str("4") },

    { njs_str("var re = /a/g; re.lastIndex = {valueOf(){throw 'Oops'}}; typeof re.lastIndex"),
      njs_str("object") },

    { njs_str("var re = /a/g; re.lastIndex = {valueOf(){throw 'Oops'}}; re.exec('a')"),
      njs_str("Oops") },

    { njs_str("var re = /a/; Object.defineProperty(re, 'lastIndex', {value:'qq'}); re.lastIndex"),
      njs_str("qq") },

    { njs_str("var re = /a/; re.lastIndex = 'qq'; Object.create(re).lastIndex"),
      njs_str("qq") },

    { njs_str("var re = /(?:ab|cd)\\d?/g; re.lastIndex=-1; re.test('aacd22 '); re.lastIndex"),
      njs_str("5") },

    { njs_str("var re = /(?:ab|cd)\\d?/g; re.lastIndex=-1; re.test('@@'); re.lastIndex"),
      njs_str("0") },

    /*
     * It seems that "/стоп/ig" fails on early PCRE versions.
     * It fails at least in 8.1 and works at least in 8.31.
     */

#if (!NJS_HAVE_MEMORY_SANITIZER) /* FIXME */
    { njs_str("var r = /Стоп/ig;"
                 "var a = r.exec('АБВДЕЁЖЗИКЛМНОПРСТУФХЦЧШЩЬЫЪЭЮЯСТОП');"
                 "r.lastIndex +' '+ a +' '+ r.source +' '+ r"),
      njs_str("35 СТОП Стоп /Стоп/gi") },
#endif

    { njs_str("var r = /quick\\s(brown).+?(jumps)/ig;"
                "var a = r.exec('The Quick Brown Fox Jumps Over The Lazy Dog');"
                "a[0] +' '+ a[1] +' '+ a[2] +' '+ a[3] +' '+ "
                "a.index +' '+ r.lastIndex +' '+ a.input"),
      njs_str("Quick Brown Fox Jumps Brown Jumps undefined "
                 "4 25 The Quick Brown Fox Jumps Over The Lazy Dog") },

    { njs_str("var r = /a/.exec('a'); ['groups' in r, typeof r.groups]"),
      njs_str("true,undefined") },

#if (!NJS_HAVE_MEMORY_SANITIZER) /* PCRE bug in groups code */
    { njs_str("var r = /(?<m>[0-9]{2})\\/(?<d>[0-9]{2})\\/(?<y>[0-9]{4})/;"
                 "var g = r.exec('12/31/1986').groups;"
                 "g.d + '.' + g.m + '.' + g.y"),
      njs_str("31.12.1986") },

    { njs_str("var g = /(?<r>(?<no>no)?(?<yes>yes)?)/.exec('yes').groups;"
                 "[Object.keys(g).length,'no' in g, typeof g.no, g.yes, g.r]"),
      njs_str("3,true,undefined,yes,yes") },
#endif

    { njs_str("var s; var r = /./g; while (s = r.exec('abc')); s"),
      njs_str("null") },

    { njs_str("var r = /LS/i.exec(false); r[0]"),
      njs_str("ls") },

    { njs_str("var r = (/^.+$/mg), s = 'ab\\nc'; [r.exec(s), r.exec(s)]"),
      njs_str("ab,c") },

    { njs_str("var r = (/^.+$/mg); [r.global, r.multiline, r.ignoreCase]"),
      njs_str("true,true,false") },

    { njs_str("var r = /./; r"),
      njs_str("/./") },

    { njs_str("var r = new RegExp(); r"),
      njs_str("/(?:)/") },

    { njs_str("var r = new RegExp('.'); r"),
      njs_str("/./") },

    { njs_str("var r = new RegExp('.', 'ig'); r"),
      njs_str("/./gi") },

    { njs_str("var r = new RegExp('abc'); r.test('00abc11')"),
      njs_str("true") },

    { njs_str("var r = new RegExp('abc', 'i'); r.test('00ABC11')"),
      njs_str("true") },

    { njs_str("RegExp('α'.repeat(33)).toString()[32]"),
      njs_str("α") },

    { njs_str("new RegExp('', 'x')"),
      njs_str("SyntaxError: Invalid RegExp flags \"x\"") },

    { njs_str("RegExp({})"),
      njs_str("/[object Object]/") },

    { njs_str("RegExp(true)"),
      njs_str("/true/") },

    { njs_str("RegExp(undefined)"),
      njs_str("/(?:)/") },

    { njs_str("RegExp('abc', undefined)"),
      njs_str("/abc/") },

    { njs_str("RegExp('abc', {})"),
      njs_str("SyntaxError: Invalid RegExp flags \"[object Object]\"") },

    { njs_str("RegExp(/expr/)"),
      njs_str("/expr/") },

    { njs_str("RegExp(/expr/i).ignoreCase"),
      njs_str("true") },

    { njs_str("RegExp(/expr/, 'x')"),
      njs_str("SyntaxError: Invalid RegExp flags \"x\"") },

    { njs_str("RegExp(new RegExp('expr'))"),
      njs_str("/expr/") },

    { njs_str("RegExp(new RegExp('expr')).multiline"),
      njs_str("false") },

    { njs_str("RegExp(new RegExp('expr'), 'm').multiline"),
      njs_str("true") },

    { njs_str("new RegExp('[')"),
      njs_str("SyntaxError: pcre_compile(\"[\") failed: missing terminating ] for character class") },

    { njs_str("new RegExp('['.repeat(16))"),
      njs_str("SyntaxError: pcre_compile(\"[[[[[[[[[[[[[[[[\") failed: missing terminating ] for character class") },

    { njs_str("new RegExp('\\\\')"),
      njs_str("SyntaxError: pcre_compile(\"\\\") failed: \\ at end of pattern") },

    { njs_str("[0].map(RegExp().toString)"),
      njs_str("TypeError: \"this\" argument is not a regexp") },

    /* Non-standard ECMA-262 features. */

    /* 0x10400 is not a surrogate pair of 0xD801 and 0xDC00. */

    { njs_str("var chars = '𐐀'; chars.length +' '+ chars.charCodeAt(0)"),
      njs_str("1 66560") },

    /* es5id: 6.1, 0x104A0 is not a surrogate pair of 0xD801 and 0xDCA0. */

    { njs_str("var chars = '𐒠'; chars.length +' '+ chars.charCodeAt(0)"),
      njs_str("1 66720") },

    /* Error object. */

    { njs_str("Error()"),
      njs_str("Error") },

    { njs_str("new Error()"),
      njs_str("Error") },

    { njs_str("Error('e')"),
      njs_str("Error: e") },

    { njs_str("Error(123)"),
      njs_str("Error: 123") },

    { njs_str("Error({toString(){return 'e'}})"),
      njs_str("Error: e") },

    { njs_str("Error([1,'α'])"),
      njs_str("Error: 1,α") },

    { njs_str("var e = TypeError(Error('e')); e"),
      njs_str("TypeError: Error: e") },

    { njs_str("Error('α'.repeat(33)).toString().length"),
      njs_str("40") },

    { njs_str("var e = Error('e'); e.name = {toString(){return 'E'}}; e"),
      njs_str("E: e") },

    { njs_str("var e = Error('e'); Object.defineProperty(e, 'name', {get(){return 'E'}}); e"),
      njs_str("E: e") },

    { njs_str("var e = Error('e'); e.name = ''; e"),
      njs_str("e") },

    { njs_str("var e = Error(); e.name = ''; e"),
      njs_str("") },

    { njs_str("var e = Error(); e.name = ''; e.message = 'e'; e"),
      njs_str("e") },

    { njs_str("Error('e').name + ': ' + Error('e').message"),
      njs_str("Error: e") },

    { njs_str("Error(String.bytesFrom(Array(1).fill(0x9d))).toString().length"),
      njs_str("8") },

    { njs_str("var e = Error('α'); e.name = String.bytesFrom(Array(1).fill(0x9d)); "
              "e.toString().length"),
      njs_str("5") },

    { njs_str("Error(1)"),
      njs_str("Error: 1") },

    { njs_str("Error.__proto__ == Function.prototype"),
      njs_str("true") },

    { njs_str("Error.prototype.name"),
      njs_str("Error") },

    { njs_str("Error.prototype.message"),
      njs_str("") },

    { njs_str("Error.prototype.constructor == Error"),
      njs_str("true") },

    { njs_str("Error.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("Error().__proto__ == Error.prototype"),
      njs_str("true") },

    { njs_str("Error().__proto__.__proto__ == Object.prototype"),
      njs_str("true") },

    { njs_str("Error.prototype.message = 'm';"
              "Error.prototype.name = 'n';"
              "new Error()"),
      njs_str("n: m") },

    { njs_str("var e = RangeError('e'); Object.preventExtensions(e);e"),
      njs_str("RangeError: e") },

    /* Memory object is immutable. */

    { njs_str("var e = MemoryError('e'); e.name = 'E'"),
      njs_str("TypeError: Cannot add property \"name\", object is not extensible") },

    { njs_str("EvalError.prototype.name"),
      njs_str("EvalError") },

    { njs_str("InternalError.prototype.name"),
      njs_str("InternalError") },

    { njs_str("RangeError.prototype.name"),
      njs_str("RangeError") },

    { njs_str("ReferenceError.prototype.name"),
      njs_str("ReferenceError") },

    { njs_str("SyntaxError.prototype.name"),
      njs_str("SyntaxError") },

    { njs_str("TypeError.prototype.name"),
      njs_str("TypeError") },

    { njs_str("URIError.prototype.name"),
      njs_str("URIError") },

    { njs_str("MemoryError.prototype.name"),
      njs_str("InternalError") },


    /* NativeErrors. */

    { njs_str(
        "var global = this;"
        "function isValidNativeError(e) {"
        "   var inst;"
        "   var proto = Object.getPrototypeOf(e) === Error;"
        "   var proto2 = e.__proto__ === Error;"
        "   var iproto = e().__proto__ === e.prototype;"
        "   var iproto2 = e().__proto__.__proto__ === Error.prototype;"
        "   var tpof = typeof e() === 'object';"
        "   var ctor = e.prototype.constructor === e;"
        "   var msg = e.prototype.message === '';"
        "   var name = e('e').toString() === `${e.prototype.name}: e`;"
        "   var name2 = (inst = e('e'), inst.name = 'E', inst.toString() === 'E: e');"
        "   var name3 = (inst = e('e'), inst.name = '', inst.toString() === 'e');"
        "   var name4 = e().toString() === `${e.prototype.name}`;"
        "   var name_prop = Object.getOwnPropertyDescriptor(e.prototype, 'message');"
        "   name_prop = name_prop.writable && !name_prop.enumerable && name_prop.configurable;"
        "   var own_proto_ctor = e.prototype.hasOwnProperty('constructor');"
        "   var props = Object.getOwnPropertyDescriptor(global, e.prototype.name);"
        "   props = props.writable && !props.enumerable && props.configurable;"
        "   var same = e === global[e.prototype.name];"
        ""
        "   return proto && proto2 && iproto && iproto2 "
        "          && tpof && ctor && msg && name && name2 && name3 && name4 "
        "          && name_prop && own_proto_ctor && props && same;"
        "};"
        "["
        "  EvalError,"
        "  InternalError,"
        "  RangeError,"
        "  ReferenceError,"
        "  SyntaxError,"
        "  TypeError,"
        "  URIError,"
        "].every(e => isValidNativeError(e))"),
      njs_str("true") },

    /* Exceptions. */

    { njs_str("throw null"),
      njs_str("null") },

    { njs_str("var a; try { throw null } catch (e) { a = e } a"),
      njs_str("null") },

    { njs_str("var a; try { throw Error('e') } catch (e) { a = e.message } a"),
      njs_str("e") },

    { njs_str("var a; try { NaN.toString(NaN) } catch (e) { a = e.name } a"),
      njs_str("RangeError") },

    { njs_str("try { throw null } catch (e) { throw e }"),
      njs_str("null") },

    { njs_str("try { throw Error('e') } catch (e) { throw Error(e.message + '2') }"),
      njs_str("Error: e2") },

    { njs_str("try { throw null } catch (null) { throw e }"),
      njs_str("SyntaxError: Unexpected token \"null\" in 1") },

    { njs_str("'a'.f()"),
      njs_str("TypeError: (intermediate value)[\"f\"] is not a function") },

    { njs_str("1..f()"),
      njs_str("TypeError: (intermediate value)[\"f\"] is not a function") },

    { njs_str("try {}"),
      njs_str("SyntaxError: Missing catch or finally after try in 1") },

    { njs_str("try{}catch(a[]"),
      njs_str("SyntaxError: Unexpected token \"[\" in 1") },

    { njs_str("function f(a) {return a;}; "
                 "function thrower() {throw TypeError('Oops')}; "
                 "f(thrower())"),
      njs_str("TypeError: Oops") },

    { njs_str("var a = 0; try { a = 5 }"
                 "catch (e) { a = 9 } finally { a++ } a"),
      njs_str("6") },

    { njs_str("var a = 0; try { throw 3 }"
                 "catch (e) { a = e } finally { a++ } a"),
      njs_str("4") },

    { njs_str("var a = 0; try { throw 3 }"
                 "catch (e) { throw e + 1 } finally { a++ }"),
      njs_str("4") },

    { njs_str("var a = 0; try { throw 3 }"
                 "catch (e) { a = e } finally { throw a }"),
      njs_str("3") },

    { njs_str("try { throw null } catch (e) { } finally { }"),
      njs_str("undefined") },

    { njs_str("var a = 0; try { throw 3 }"
                 "catch (e) { throw 4 } finally { throw a }"),
      njs_str("0") },

    { njs_str("var a = 0; try { a = 5 } finally { a++ } a"),
      njs_str("6") },

    { njs_str("var a = 0; try { throw 5 } finally { a++ }"),
      njs_str("5") },

    { njs_str("var a = 0; try { a = 5 } finally { throw 7 }"),
      njs_str("7") },

    { njs_str("function f(a) {"
                 "   if (a > 1) return f(a - 1);"
                 "   throw 9; return a }"
                 "var a = 0; try { a = f(5); a++ } catch(e) { a = e } a"),
      njs_str("9") },

    { njs_str("var a; try { try { throw 5 } catch (e) { a = e } throw 3 }"
                 "       catch(x) { a += x } a"),
      njs_str("8") },

    { njs_str("throw\nnull"),
      njs_str("SyntaxError: Illegal newline after throw in 2") },

    { njs_str("for (var x in [1,2]) { try{ continue; } catch(e) {} } throw 1"),
      njs_str("1") },

    { njs_str("for (var x in [1,2]) { try{ break; } catch(e) {} } throw 1"),
      njs_str("1") },

    { njs_str("try\n {\n continue; } catch(e) {}"),
      njs_str("SyntaxError: Illegal continue statement in 3") },

    { njs_str("var a = 1; "
                 "switch (a) {"
                 "default:"
                 "  try\n {\n continue; } "
                 "  catch(e) {}"
                 "}"),
      njs_str("SyntaxError: Illegal continue statement in 3") },

    { njs_str("try\n {\n break; } catch(e) {}"),
      njs_str("SyntaxError: Illegal break statement in 3") },

    { njs_str("try\n { }\n catch(e) {continue;}"),
      njs_str("SyntaxError: Illegal continue statement in 3") },

    { njs_str("try { } catch(e) {break;}"),
      njs_str("SyntaxError: Illegal break statement in 1") },

    { njs_str("try { continue; } finally {}"),
      njs_str("SyntaxError: Illegal continue statement in 1") },

    { njs_str("try { break; } finally {}"),
      njs_str("SyntaxError: Illegal break statement in 1") },

    { njs_str("try\n {\n try\n {\n continue; } finally {} } finally {}"),
      njs_str("SyntaxError: Illegal continue statement in 5") },

    /* break from try in try/catch. */

    { njs_str("function f(n) {"
                 "    var pre = 0; var post = 0;"
                 "    for (var x in [1, 2, 3]) {"
                 "        pre++;"
                 "        try { "
                 "            if (n == 'b') {break;}"
                 "        }"
                 "        catch (e) {};"
                 "        post++"
                 "    }"
                 "    return [pre, post];"
                 "}; njs.dump([f(),f('b')])"),
      njs_str("[[3,3],"
                  "[1,0]]") },

    { njs_str("function f(v, n) {"
                 "    var pre = 0; var post = 0; var case2 = 0;"
                 "    switch (v) {"
                 "    case 1: "
                 "        pre++;"
                 "        try { "
                 "            if (n == 'b') {break;}"
                 "        }"
                 "        catch (e) {};"
                 "        post++;"
                 "        break;"
                 "    default:"
                 "        case2++;"
                 "    }"
                 "    return [pre, post, case2];"
                 "}; njs.dump([f(),f(1)])"),
      njs_str("[[0,0,1],"
                  "[1,1,0]]") },

    /* continue from try in try/catch. */

    { njs_str("function f(n) {"
                 "    var pre = 0; var post = 0;"
                 "    for (var x in [1, 2, 3]) {"
                 "        pre++;"
                 "        try { "
                 "            if (n == 'c') {continue;}"
                 "        }"
                 "        catch (e) {};"
                 "        post++"
                 "    }"
                 "    return [pre, post];"
                 "}; njs.dump([f(),f('c')])"),
      njs_str("[[3,3],"
                  "[3,0]]") },

    /* Multiple break/continue from try in try/catch. */

    { njs_str("function f(n) {"
                 "    var pre = 0; var mid = 0; var post = 0;"
                 "    for (var x in [1, 2, 3]) {"
                 "        pre++;"
                 "        try { "
                 "            if (n == 'c') {continue;}"
                 "            if (n == 'b') {break;}"
                 "            mid++;"
                 "            if (n == 'c2') {continue;}"
                 "            if (n == 'b2') {break;}"
                 "        }"
                 "        catch (e) {};"
                 "        post++"
                 "    }"
                 "    return [pre, mid, post];"
                 "}; njs.dump([f(),f('c'),f('b'),f('c2'),f('b2')])"),
      njs_str("[[3,3,3],"
                  "[3,0,0],"
                  "[1,0,0],"
                  "[3,3,0],"
                  "[1,1,0]]") },

    /* Multiple break/continue from catch in try/catch. */

    { njs_str("function f(t, n) {"
                 "    var pre = 0; var mid = 0; var post = 0;"
                 "    for (var x in [1, 2, 3]) {"
                 "        pre++;"
                 "        try { "
                 "            if (t) {throw 'a'}"
                 "        }"
                 "        catch (e) {"
                 "            if (n == 'c') {continue;}"
                 "            if (n == 'b') {break;}"
                 "            mid++;"
                 "            if (n == 'c2') {continue;}"
                 "            if (n == 'b2') {break;}"
                 "        };"
                 "        post++"
                 "    }"
                 "    return [pre, mid, post];"
                 "}; njs.dump([f(), f(1), f(1, 'c'), f(1, 'b'), f(1, 'c2'), f(1, 'b2')])"),
      njs_str("[[3,0,3],"
                  "[3,3,3],"
                  "[3,0,0],"
                  "[1,0,0],"
                  "[3,3,0],"
                  "[1,1,0]]") },

    /* break from try in try/finally. */

    { njs_str("function f(n) {"
                 "    var pre = 0; var mid = 0; var fin = 0; var post = 0;"
                 "    for (var x in [1, 2, 3]) {"
                 "        pre++;"
                 "        try { "
                 "            if (n == 'c') {continue;}"
                 "            if (n == 'b') {break;}"
                 "            mid++;"
                 "            if (n == 'c2') {continue;}"
                 "            if (n == 'b2') {break;}"
                 "        }"
                 "        finally {fin++};"
                 "        post++"
                 "    }"
                 "    return [pre, mid, fin, post];"
                 "}; njs.dump([f(),f('c'),f('b'),f('c2'),f('b2')])"),
      njs_str("[[3,3,3,3],"
                  "[3,0,3,0],"
                  "[1,0,1,0],"
                  "[3,3,3,0],"
                  "[1,1,1,0]]") },

    /* Multiple break/continue from try in try/catch/finally. */

    { njs_str("function f(n) {"
                 "    var pre = 0; var mid = 0; var fin = 0; var post = 0;"
                 "    for (var x in [1, 2, 3]) {"
                 "        pre++;"
                 "        try { "
                 "            if (n == 'c') {continue;}"
                 "            if (n == 'b') {break;}"
                 "            mid++;"
                 "            if (n == 'c2') {continue;}"
                 "            if (n == 'b2') {break;}"
                 "        }"
                 "        catch (e) {}"
                 "        finally {fin++};"
                 "        post++"
                 "    }"
                 "    return [pre, mid, fin, post];"
                 "}; njs.dump([f(),f('c'),f('b'),f('c2'),f('b2')])"),
      njs_str("[[3,3,3,3],"
                  "[3,0,3,0],"
                  "[1,0,1,0],"
                  "[3,3,3,0],"
                  "[1,1,1,0]]") },

    /* Multiple break/continue from catch in try/catch/finally. */

    { njs_str("function f(t, n) {"
                 "    var pre = 0; var mid = 0; var fin = 0; var post = 0;"
                 "    for (var x in [1, 2, 3]) {"
                 "        pre++;"
                 "        try {if (t) {throw 'a'}}"
                 "        catch (e) { "
                 "            if (n == 'c') {continue;}"
                 "            if (n == 'b') {break;}"
                 "            mid++;"
                 "            if (n == 'c2') {continue;}"
                 "            if (n == 'b2') {break;}"
                 "        }"
                 "        finally {fin++};"
                 "        post++"
                 "    }"
                 "    return [pre, mid, fin, post];"
                 "}; njs.dump([f(), f(1), f(1, 'c'), f(1, 'b'), f(1, 'c2'), f(1, 'b2')])"),
      njs_str("[[3,0,3,3],"
                  "[3,3,3,3],"
                  "[3,0,3,0],"
                  "[1,0,1,0],"
                  "[3,3,3,0],"
                  "[1,1,1,0]]") },

    /* Multiple return from try. */

    { njs_str("var r = 0; "
                 "function f(i, n) {"
                 "   try { "
                 "      var a = 'x'; "
                 "      if (i != 0) {"
                 "         return a.repeat(n);"
                 "      } else {"
                 "         return;"
                 "      }"
                 "   }"
                 "   catch (e) {  } "
                 "   finally { r++; }};"
                 "[f(1,1), f(1,2), f(0), r]"),
      njs_str("x,xx,,3") },

    { njs_str("var r = 0; "
                 "function f(i) {"
                 "   try { "
                 "      return i;"
                 "   }"
                 "   catch (e) {  } "
                 "   finally { r++; }};"
                 "[f(true), f(false), r]"),
      njs_str("true,false,2") },

    /* Multiple return from catch. */

    { njs_str("var r = 0; "
                 "function f(i, n) {"
                 "   try { "
                 "      throw 1;"
                 "   }"
                 "   catch (e) {  "
                 "      var a = 'x'; "
                 "      if (i != 0) {"
                 "         return a.repeat(n);"
                 "      } else {"
                 "         return;"
                 "      }"
                 "   } "
                 "   finally { r++; }};"
                 "[f(1,1), f(1,2), f(0), r]"),
      njs_str("x,xx,,3") },

    /* return overrun by finally. */

    { njs_str("function f() {"
                 "   try { "
                 "      return 'a';"
                 "   }"
                 "   catch (e) {  "
                 "   } "
                 "   finally { "
                 "      return 'b'; "
                 "   }}; "
                 "f()"),
      njs_str("b") },

    { njs_str("(function (f, val) { "
                 "  try { return f(val); } "
                 "  finally { return val; }"
                 "})(function () {throw 'a'}, 'v')"),
      njs_str("v") },

    { njs_str("(function() { try { return ['a'];} finally {} } )()"),
      njs_str("a") },

    { njs_str("function f(){}; try {f((new RegExp('a**')))} catch (e) { }"),
      njs_str("undefined") },

    { njs_str("function f(){}; try {f(f((new RegExp('a**'))))} catch (e) { }"),
      njs_str("undefined") },

    { njs_str("function f(){}; (function(){try {f(f((new RegExp('a**'))))} catch (e) { return 1}})()"),
      njs_str("1") },

    { njs_str("var o = { valueOf: function() { return '3' } }; --o"),
      njs_str("2") },

    { njs_str("var o = { valueOf: function() { return [3] } }; --o"),
      njs_str("NaN") },

    { njs_str("var o = { valueOf: function() { return '3' } }; 10 - o"),
      njs_str("7") },

    { njs_str("var o = { valueOf: function() { return [3] } }; 10 - o"),
      njs_str("NaN") },

    { njs_str("var o = { toString: function() { return 'OK' } }; 'o:' + o"),
      njs_str("o:OK") },

    { njs_str("var o = { toString: function() { return [1] } }; o"),
      njs_str("TypeError: Cannot convert object to primitive value") },

    { njs_str("var o = { toString: function() { return [1] } }; 'o:' + o"),
      njs_str("TypeError: Cannot convert object to primitive value") },

    { njs_str("var a = { valueOf: function() { return '3' } };"
                 "var b = { toString: function() { return 10 - a + 'OK' } };"
                 "var c = { toString: function() { return b + 'YES' } };"
                 "'c:' + c"),
      njs_str("c:7OKYES") },

    { njs_str("[1,2,3].valueOf()"),
      njs_str("1,2,3") },

    { njs_str("var o = { valueOf: function() { return 'OK' } };"
                 "o.valueOf()"),
      njs_str("OK") },

    { njs_str("false.__proto__ === true.__proto__"),
      njs_str("true") },

    { njs_str("0..__proto__ === 1..__proto__"),
      njs_str("true") },

    { njs_str("[].__proto__ === [1,2].__proto__"),
      njs_str("true") },

    { njs_str("/./.__proto__ === /a/.__proto__"),
      njs_str("true") },

    { njs_str("''.__proto__ === 'abc'.__proto__"),
      njs_str("true") },

    { njs_str("[].__proto__.join.call([1,2,3], ':')"),
      njs_str("1:2:3") },

    { njs_str("''.__proto__.concat.call('a', 'b', 'c')"),
      njs_str("abc") },

    { njs_str("/./.__proto__.test.call(/a{2}/, 'aaa')"),
      njs_str("true") },

    { njs_str("true instanceof Boolean"),
      njs_str("false") },

    { njs_str("1 instanceof Number"),
      njs_str("false") },

    { njs_str("'' instanceof String"),
      njs_str("false") },

    { njs_str("({}) instanceof Object"),
      njs_str("true") },

    { njs_str("[] instanceof []"),
      njs_str("TypeError: right argument is not callable") },

    { njs_str("[] instanceof Array"),
      njs_str("true") },

    { njs_str("[] instanceof Object"),
      njs_str("true") },

    { njs_str("/./ instanceof RegExp"),
      njs_str("true") },

    { njs_str("/./ instanceof Object"),
      njs_str("true") },

    { njs_str("Object.defineProperty(Function.prototype, \"prototype\", {get: ()=>Array.prototype});"
                 "[] instanceof Function.prototype"),
      njs_str("true") },

     { njs_str("Object.defineProperty(Function.prototype, \"prototype\", {get: ()=>{throw Error('Oops')}});"
                 "[] instanceof Function.prototype"),
      njs_str("Error: Oops") },

     { njs_str("var o = {};"
                 "Object.defineProperty(o, 'foo', { get: function() {return () => 1} });"
                 "o.foo()"),
      njs_str("1") },

    /* global this. */

    { njs_str("this"),
      njs_str("[object global]") },

    { njs_str("Object.getOwnPropertyDescriptor(this, 'NaN').value"),
      njs_str("NaN") },

    { njs_str("Object.getOwnPropertyDescriptors(this).NaN.value"),
      njs_str("NaN") },

    { njs_str("Object.getOwnPropertyNames(this).includes('NaN')"),
      njs_str("true") },

    { njs_str("Object.keys(this).sort()"),
      njs_str("$r,$r2,$r3,njs,process") },

    { njs_str("var r = njs.dump(this); "
              "['$r', 'global', njs.version].every(v=>r.includes(v))"),
      njs_str("true") },

    { njs_str("var r = JSON.stringify(this); "
              "['$r', njs.version].every(v=>r.includes(v))"),
      njs_str("true") },


    { njs_str("this.a = 1; this.a"),
      njs_str("1") },

    { njs_str("this.a = 1; a"),
      njs_str("1") },

    { njs_str("this.a = 2; this.b = 3; a * b - a"),
      njs_str("4") },

    { njs_str("this.a = 1; a()"),
      njs_str("TypeError: number is not a function") },

    { njs_str("this.a = ()=>1; a()"),
      njs_str("1") },

    { njs_str("var global = this;"
              "function isImmutableConstant(v) {"
              "    var d = Object.getOwnPropertyDescriptor(global, v);"
              "    return !d.writable && !d.enumerable && !d.configurable;"
              "};"
              "['undefined', 'NaN', 'Infinity'].every((v)=>isImmutableConstant(v))"),
      njs_str("true") },

    { njs_str("this.undefined = 42"),
      njs_str("TypeError: Cannot assign to read-only property \"undefined\" of object") },

    { njs_str("this.Infinity = 42"),
      njs_str("TypeError: Cannot assign to read-only property \"Infinity\" of object") },

    { njs_str("this.NaN = 42"),
      njs_str("TypeError: Cannot assign to read-only property \"NaN\" of object") },

    { njs_str("typeof this.undefined"),
      njs_str("undefined") },

    { njs_str("typeof this.Infinity"),
      njs_str("number") },

    { njs_str("this.Infinity + 1"),
      njs_str("Infinity") },

    { njs_str("typeof this.NaN"),
      njs_str("number") },

    { njs_str("this.NaN + 1"),
      njs_str("NaN") },

    { njs_str("{this}"),
      njs_str("undefined") },

    { njs_str("if (1) {new this}"),
      njs_str("TypeError: object is not a function") },

    { njs_str("if (1) {this()}"),
      njs_str("TypeError: object is not a function") },

    { njs_str("var ex; try {new this} catch (e) {ex = e}; ex"),
      njs_str("TypeError: object is not a function") },

    { njs_str("var ex; try {({}) instanceof this} catch (e) {ex = e}; ex"),
      njs_str("TypeError: right argument is not callable") },

    { njs_str("njs"),
      njs_str("[object njs]") },

    { njs_str("var o = Object(); o"),
      njs_str("[object Object]") },

    { njs_str("var o = new Object(); o"),
      njs_str("[object Object]") },

    { njs_str("var o = new Object(1); o"),
      njs_str("1") },

    { njs_str("var o = {}; o === Object(o)"),
      njs_str("true") },

    { njs_str("var o = {}; o === new Object(o)"),
      njs_str("true") },

    { njs_str("var o = Object([]); Object.prototype.toString.call(o)"),
      njs_str("[object Array]") },

    { njs_str("Object.name"),
      njs_str("Object") },

    { njs_str("Object.length"),
      njs_str("1") },

    { njs_str("Object.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("Object.prototype.constructor === Object"),
      njs_str("true") },

    { njs_str("Object.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("Object.prototype.__proto__ === null"),
      njs_str("true") },

    { njs_str("Object.prototype.__proto__ = {}"),
      njs_str("TypeError: Cyclic __proto__ value") },

    { njs_str("var o = {}; var o2 = Object.create(o); o.__proto__ = o2"),
      njs_str("TypeError: Cyclic __proto__ value") },

    { njs_str("Object.prototype.__proto__.f()"),
      njs_str("TypeError: cannot get property \"f\" of undefined") },

    { njs_str("var obj = Object.create(null); obj.one = 1;"
                 "var res = [];"
                 "for (var val in obj) res.push(val); res"),
      njs_str("one") },

    { njs_str("var o1 = Object.create(null); o1.one = 1;"
                 "var o2 = Object.create(o1); o2.two = 2;"
                 "var o3 = Object.create(o2); o3.three = 3;"
                 "var res = [];"
                 "for (var val in o3) res.push(val); res"),
      njs_str("three,two,one") },

    { njs_str("var o1 = Object.create(null); o1.one = 1;"
                 "var o2 = Object.create(o1);"
                 "var o3 = Object.create(o2); o3.three = 3;"
                 "var res = [];"
                 "for (var val in o3) res.push(val); res"),
      njs_str("three,one") },

    { njs_str("var o1 = Object.create(null); o1.one = 1;"
                 "var o2 = Object.create(o1);"
                 "var o3 = Object.create(o2);"
                 "var res = [];"
                 "for (var val in o3) res.push(val); res"),
      njs_str("one") },

    { njs_str("var o1 = Object.create(null); o1.one = 1;"
                 "var o2 = Object.create(o1); o2.two = 2;"
                 "var o3 = Object.create(o2); o3.three = 3;"
                 "o3.two = -2; o3.one = -1;"
                 "var res = [];"
                 "for (var val in o3) res.push(val); res"),
      njs_str("three,two,one") },

    { njs_str("var a = []; for(var p in 'abc') a.push(p); a"),
      njs_str("0,1,2") },

    { njs_str("var a = []; for(var p in Object('abc')) a.push(p); a"),
      njs_str("0,1,2") },

    { njs_str("var o = Object('abc'); var x = Object.create(o);"
                 "x.a = 1; x.b = 2;"
                 "var a = []; for(var p in x) a.push(p); a"),
      njs_str("a,b,0,1,2") },

    { njs_str("var o = Object.create(Math); o.abs = -5; Math.abs(o.abs)"),
      njs_str("5") },

    { njs_str("Object.prototype.toString.call(Object.prototype)"),
      njs_str("[object Object]") },

    { njs_str("Object.prototype.toString.call(new Array)"),
      njs_str("[object Array]") },

    { njs_str("Object.prototype.toString.call(new URIError)"),
      njs_str("[object Error]") },

    { njs_str("Object.prototype.toString.call(URIError.prototype)"),
      njs_str("[object Object]") },

    { njs_str("Object.prototype"),
      njs_str("[object Object]") },

    { njs_str("Object.prototype.valueOf.prototype"),
      njs_str("undefined") },

    { njs_str("Object.prototype.valueOf.call()"),
      njs_str("TypeError: cannot convert null or undefined to object") },

    { njs_str("Object.prototype.valueOf.call(null)"),
      njs_str("TypeError: cannot convert null or undefined to object") },

    { njs_str("[false, NaN, Symbol(), '']"
              ".map((x) => Object.prototype.valueOf.call(x))"
              ".map((x) => Object.prototype.toString.call(x))"),
      njs_str("[object Boolean],[object Number],[object Symbol],[object String]") },

    { njs_str("Object.constructor === Function"),
      njs_str("true") },

    { njs_str("({}).__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("({}).__proto__ = 1"),
      njs_str("1") },

    { njs_str("({}).__proto__ = null"),
      njs_str("null") },

    { njs_str("({__proto__:null}).__proto__"),
      njs_str("undefined") },

    { njs_str("({__proto__:null, a:1}).a"),
      njs_str("1") },

    { njs_str("Object.getPrototypeOf({__proto__:null})"),
      njs_str("null") },

    { njs_str("Object.getPrototypeOf({__proto__:1}) === Object.prototype"),
      njs_str("true") },

    { njs_str("Object.getPrototypeOf({__proto__:Array.prototype}) === Array.prototype"),
      njs_str("true") },

    { njs_str("({__proto__: []}) instanceof Array"),
      njs_str("true") },

    { njs_str("({__proto__: Array.prototype}) instanceof Array"),
      njs_str("true") },

    { njs_str("var o = {};"
              "o.__proto__ = Array.prototype;"
              "Object.getPrototypeOf(o) === Array.prototype"),
      njs_str("true") },

    { njs_str("var o = Object.preventExtensions({});"
              "o.__proto__ = Array.prototype;"
              "Object.getPrototypeOf(o) === Object.prototype"),
      njs_str("true") },

    { njs_str("var o = {__proto__: Array.prototype, length:3}; o.fill('a')[2]"),
      njs_str("a") },

    { njs_str("({__proto__:null, __proto__: null})"),
      njs_str("SyntaxError: Duplicate __proto__ fields are not allowed in object literals in 1") },

    { njs_str("({__proto__:null, '__proto__': null})"),
      njs_str("SyntaxError: Duplicate __proto__ fields are not allowed in object literals in 1") },

    { njs_str("({__proto__:null, '\\x5f_proto__': null})"),
      njs_str("SyntaxError: Duplicate __proto__ fields are not allowed in object literals in 1") },

    { njs_str("var __proto__ = 'a'; ({__proto__}).__proto__"),
      njs_str("a") },

    { njs_str("var __proto__ = 'a'; ({__proto__, '__proto__':'b'}).__proto__"),
      njs_str("a") },

    { njs_str("var __proto__ = 'a'; ({__proto__:null, __proto__}).__proto__"),
      njs_str("a") },

    { njs_str("({__proto__:null, ['__proto__']:'a'}).__proto__"),
      njs_str("a") },

    { njs_str("({__proto__() { return 123; }}).__proto__()"),
      njs_str("123") },

    { njs_str("({['__prot' + 'o__']: 123}).__proto__"),
      njs_str("123") },

    { njs_str("({}).__proto__.constructor === Object"),
      njs_str("true") },

    { njs_str("({}).constructor === Object"),
      njs_str("true") },

    { njs_str("({}).constructor()"),
      njs_str("[object Object]") },

    { njs_str("var a = Object.__proto__; a()"),
      njs_str("undefined") },

    { njs_str("Object.__proto__()"),
      njs_str("undefined") },

    { njs_str("var a = Array(3); a"),
      njs_str(",,") },

    { njs_str("var a = Array(); a.length"),
      njs_str("0") },

    { njs_str("var a = Array(0); a.length"),
      njs_str("0") },

    { njs_str("var a = Array(true); a"),
      njs_str("true") },

    { njs_str("var a = Array(1,'two',3); a"),
      njs_str("1,two,3") },

    { njs_str("var a = Array(-1)"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("var a = Array(2.5)"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("var a = Array(NaN)"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("var a = Array(Infinity)"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("var a = Array(1111111111)"),
      njs_str("MemoryError") },

    { njs_str("var x = Array(2**32)"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("var x = Array(2**28)"),
      njs_str("MemoryError") },

    { njs_str("var x = Array(2**20), y = Array(2**12).fill(x);"
                 "Array.prototype.concat.apply(y[0], y.slice(1))"),
      njs_str("RangeError: Invalid array length") },

    { njs_str("var a = new Array(3); a"),
      njs_str(",,") },

    { njs_str("Array.name"),
      njs_str("Array") },

    { njs_str("Array.length"),
      njs_str("1") },

    { njs_str("delete this.Array; Array"),
      njs_str("ReferenceError: \"Array\" is not defined in 1") },

    { njs_str("Array.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("Array.prototype.constructor === Array"),
      njs_str("true") },

    { njs_str("Array.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("Array.prototype.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("Object.prototype.toString.call(Array.prototype)"),
      njs_str("[object Array]") },

    { njs_str("Array.prototype"),
      njs_str("") },

    { njs_str("Array.prototype.length"),
      njs_str("0") },

    { njs_str("Array.prototype.length = 3, Array.prototype"),
      njs_str(",,") },

    { njs_str("var o = Object.create(Array.prototype); o.length = 3;"
                 "[Array.prototype, Array.prototype.length, o.length]"),
      njs_str(",0,3") },

    { njs_str("var o = Object.create(Array.prototype);"
                 "Object.defineProperty(o, 'length', {value: 3});"
                 "[Array.prototype, Array.prototype.length, o.length]"),
      njs_str(",0,3") },

    { njs_str("Array.constructor === Function"),
      njs_str("true") },

    { njs_str("var a = []; a.join = 'OK'; a"),
      njs_str("[object Array]") },

    { njs_str("[].__proto__ === Array.prototype"),
      njs_str("true") },

    { njs_str("[].__proto__.constructor === Array"),
      njs_str("true") },

    { njs_str("[].constructor === Array"),
      njs_str("true") },

    { njs_str("([]).constructor()"),
      njs_str("") },

    { njs_str("Boolean()"),
      njs_str("false") },

    { njs_str("new Boolean()"),
      njs_str("false") },

    { njs_str("new Boolean"),
      njs_str("false") },

    { njs_str("Boolean(0)"),
      njs_str("false") },

    { njs_str("Boolean('')"),
      njs_str("false") },

    { njs_str("Boolean(1)"),
      njs_str("true") },

    { njs_str("Boolean('a')"),
      njs_str("true") },

    { njs_str("Boolean({})"),
      njs_str("true") },

    { njs_str("Boolean([])"),
      njs_str("true") },

    { njs_str("typeof Boolean(1)"),
      njs_str("boolean") },

    { njs_str("typeof new Boolean(1)"),
      njs_str("object") },

    { njs_str("typeof new Boolean"),
      njs_str("object") },

    { njs_str("Boolean.name"),
      njs_str("Boolean") },

    { njs_str("Boolean.length"),
      njs_str("1") },

    { njs_str("Boolean.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("Boolean.prototype.constructor === Boolean"),
      njs_str("true") },

    { njs_str("Boolean.prototype.constructor = 1;"
                 "Boolean.prototype.constructor"),
      njs_str("1") },

    { njs_str("var c = Boolean.prototype.constructor;"
                 "Boolean.prototype.constructor = 1;"
                 "[c(0), Boolean.prototype.constructor]"),
      njs_str("false,1") },

    { njs_str("Boolean.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("Boolean.prototype.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("Object.prototype.toString.call(Boolean.prototype)"),
      njs_str("[object Boolean]") },

    { njs_str("Boolean.prototype"),
      njs_str("false") },

    { njs_str("Boolean.constructor === Function"),
      njs_str("true") },

    { njs_str("true.__proto__ === Boolean.prototype"),
      njs_str("true") },

    { njs_str("false.__proto__ === Boolean.prototype"),
      njs_str("true") },

    { njs_str("var b = Boolean(1); b.__proto__ === Boolean.prototype"),
      njs_str("true") },

    { njs_str("var b = new Boolean(1); b.__proto__ === Boolean.prototype"),
      njs_str("true") },

    { njs_str("Number()"),
      njs_str("0") },

    { njs_str("new Number()"),
      njs_str("0") },

    { njs_str("new Number"),
      njs_str("0") },

    { njs_str("new Number(undefined)"),
      njs_str("NaN") },

    { njs_str("new Number(null)"),
      njs_str("0") },

    { njs_str("new Number(true)"),
      njs_str("1") },

    { njs_str("new Number(false)"),
      njs_str("0") },

    { njs_str("Number(undefined)"),
      njs_str("NaN") },

    { njs_str("Number(null)"),
      njs_str("0") },

    { njs_str("Number(true)"),
      njs_str("1") },

    { njs_str("Number(false)"),
      njs_str("0") },

    { njs_str("Number(123)"),
      njs_str("123") },

    { njs_str("Number('123')"),
      njs_str("123") },

    { njs_str("Number('0.'+'1'.repeat(128))"),
      njs_str("0.1111111111111111") },

    { njs_str("Number('1'.repeat(128))"),
      njs_str("1.1111111111111113e+127") },

    { njs_str("Number('1'.repeat(129))"),
      njs_str("1.1111111111111112e+128") },

    { njs_str("Number('1'.repeat(129))"),
      njs_str("1.1111111111111112e+128") },

    { njs_str("Number('1'.repeat(129)+'e-100')"),
      njs_str("1.1111111111111112e+28") },

    { njs_str("Number('1'.repeat(310))"),
      njs_str("Infinity") },

    { njs_str("var o = { valueOf: function() { return 123 } };"
                 "Number(o)"),
      njs_str("123") },

    { njs_str("var o = { valueOf: function() { return 123 } };"
                 "new Number(o)"),
      njs_str("123") },

    { njs_str("typeof Number(1)"),
      njs_str("number") },

    { njs_str("typeof new Number(1)"),
      njs_str("object") },

    { njs_str("typeof new Number"),
      njs_str("object") },

    { njs_str("Number.name"),
      njs_str("Number") },

    { njs_str("Number.length"),
      njs_str("1") },

    { njs_str("Number.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("Number.prototype.constructor === Number"),
      njs_str("true") },

    { njs_str("Number.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("Number.prototype.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("Object.prototype.toString.call(Number.prototype)"),
      njs_str("[object Number]") },

    { njs_str("Number.prototype"),
      njs_str("0") },

    { njs_str("Number.constructor === Function"),
      njs_str("true") },

    { njs_str("0..__proto__ === Number.prototype"),
      njs_str("true") },

    { njs_str("var n = Number(1); n.__proto__ === Number.prototype"),
      njs_str("true") },

    { njs_str("var n = new Number(1); n.__proto__ === Number.prototype"),
      njs_str("true") },

    { njs_str("Number.isFinite()"),
      njs_str("false") },

    { njs_str("Number.isFinite(123)"),
      njs_str("true") },

    { njs_str("Number.isFinite('123')"),
      njs_str("false") },

    { njs_str("Number.isFinite(Infinity)"),
      njs_str("false") },

    { njs_str("Number.isFinite(NaN)"),
      njs_str("false") },

    { njs_str("Number.isInteger()"),
      njs_str("false") },

    { njs_str("Number.isInteger('123')"),
      njs_str("false") },

    { njs_str("Number.isInteger(123)"),
      njs_str("true") },

    { njs_str("Number.isInteger(-123.0)"),
      njs_str("true") },

    { njs_str("Number.isInteger(123.4)"),
      njs_str("false") },

    { njs_str("Number.isInteger(Infinity)"),
      njs_str("false") },

    { njs_str("Number.isInteger(NaN)"),
      njs_str("false") },

    { njs_str("Number.isSafeInteger()"),
      njs_str("false") },

    { njs_str("Number.isSafeInteger('123')"),
      njs_str("false") },

    { njs_str("Number.isSafeInteger(9007199254740991)"),
      njs_str("true") },

    { njs_str("Number.isSafeInteger(-9007199254740991.0)"),
      njs_str("true") },

    { njs_str("Number.isSafeInteger(9007199254740992)"),
      njs_str("false") },

    { njs_str("Number.isSafeInteger(-9007199254740992.0)"),
      njs_str("false") },

    { njs_str("Number.isSafeInteger(123.4)"),
      njs_str("false") },

    { njs_str("Number.isSafeInteger(Infinity)"),
      njs_str("false") },

    { njs_str("Number.isSafeInteger(-Infinity)"),
      njs_str("false") },

    { njs_str("Number.isSafeInteger(NaN)"),
      njs_str("false") },

    { njs_str("Number.isNaN()"),
      njs_str("false") },

    { njs_str("Number.isNaN('NaN')"),
      njs_str("false") },

    { njs_str("Number.isNaN(NaN)"),
      njs_str("true") },

    { njs_str("Number.isNaN(123)"),
      njs_str("false") },

    { njs_str("Number.isNaN(Infinity)"),
      njs_str("false") },

#if 0
    { njs_str("parseFloat === Number.parseFloat"),
      njs_str("true") },

    { njs_str("parseInt === Number.parseInt"),
      njs_str("true") },
#endif

    /* Symbol */

    { njs_str("typeof Symbol"),
      njs_str("function") },

    { njs_str("this.Symbol === Symbol"),
      njs_str("true") },

    { njs_str("Symbol.name"),
      njs_str("Symbol") },

    { njs_str("Object.getOwnPropertyDescriptor(Symbol, 'name').configurable"),
      njs_str("true") },

    { njs_str("Symbol.length"),
      njs_str("0") },

    { njs_str("Object.getOwnPropertyDescriptor(Symbol, 'length').configurable"),
      njs_str("true") },

    { njs_str("typeof Symbol.for"),
      njs_str("function") },

    { njs_str("Symbol.for.length"),
      njs_str("1") },

    { njs_str("typeof Symbol.keyFor"),
      njs_str("function") },

    { njs_str("Symbol.keyFor.length"),
      njs_str("1") },

    { njs_str("Symbol.prototype.constructor === Symbol"),
      njs_str("true") },

    { njs_str("Symbol.prototype.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("Object.prototype.toString.call(Symbol.prototype)"),
      njs_str("[object Symbol]") },

    { njs_str("Symbol.prototype.toString()"),
      njs_str("TypeError: unexpected value type:object") },

    { njs_str("new Symbol()"),
      njs_str("TypeError: Symbol is not a constructor") },

    { njs_str("typeof Symbol()"),
      njs_str("symbol") },

    { njs_str("typeof Symbol('desc')"),
      njs_str("symbol") },

    { njs_str("Symbol() === Symbol()"),
      njs_str("false") },

    { njs_str("Symbol('desc') === Symbol('desc')"),
      njs_str("false") },

    { njs_str("Symbol() == Symbol()"),
      njs_str("false") },

    { njs_str("Symbol('desc') == Symbol('desc')"),
      njs_str("false") },

    { njs_str("Symbol() == true"),
      njs_str("false") },

    { njs_str("Symbol() == false"),
      njs_str("false") },

    { njs_str("Symbol() != true"),
      njs_str("true") },

    { njs_str("Symbol() != 0"),
      njs_str("true") },

    { njs_str("Symbol() != ''"),
      njs_str("true") },

    { njs_str("Symbol() != undefined"),
      njs_str("true") },

    { njs_str("typeof Object(Symbol())"),
      njs_str("object") },

    { njs_str("typeof Object(Symbol('desc'))"),
      njs_str("object") },

    { njs_str("var x = Symbol(), o = Object(x); x == o"),
      njs_str("true") },

    { njs_str("var x = Symbol(), o = Object(x); o == x"),
      njs_str("true") },

    { njs_str("var x = Symbol(), o = Object(x); x !== o"),
      njs_str("true") },

    { njs_str("var x = Symbol(), o = Object(x); x === o.valueOf()"),
      njs_str("true") },

    { njs_str("var x = Symbol(); Object(x) == Object(x)"),
      njs_str("false") },

    { njs_str("!Symbol()"),
      njs_str("false") },

    { njs_str("!!Symbol()"),
      njs_str("true") },

    { njs_str("(Symbol('a') && Symbol('b')).toString()"),
      njs_str("Symbol(b)") },

    { njs_str("(Symbol('a') || Symbol('b')).toString()"),
      njs_str("Symbol(a)") },

    { njs_str("+Symbol()"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("-Symbol()"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("~Symbol()"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("var x = Symbol(); ++x"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("var x = Symbol(); x++"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("var x = Symbol(); --x"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("var x = Symbol(); x--"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("var msg = 'Cannot convert a Symbol value to a number',"
              "    ops = ['+', '-', '*', '**', '/', '%', '>>>', '>>', '<<',"
              "           '&', '|', '^', '<', '<=', '>', '>='],"
              "    test = (lhs, rhs, cls, msg, stop, op) => {"
              "        if (stop) {"
              "            return stop;"
              "        }"
              "        var op = `${lhs} ${op} ${rhs}`;"
              "        try {"
              "            (new Function(op))();"
              "        } catch (e) {"
              "            if (e instanceof cls && msg == e.message) {"
              "                return '';"
              "            }"
              "        }"
              "        return `'${op}' - failed`;"
              "   };"
              "ops.reduce(test.bind({}, 'Symbol()', '42', TypeError, msg), '') ||"
              "ops.reduce(test.bind({}, '42', 'Symbol()', TypeError, msg), '');"),
      njs_str("") },

    { njs_str("Symbol() > 'abc'"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("'abc' > Symbol()"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("'abc' + Symbol()"),
      njs_str("TypeError: Cannot convert a Symbol value to a string") },

    { njs_str("Symbol() + 'abc'"),
      njs_str("TypeError: Cannot convert a Symbol value to a string") },

    { njs_str("Math.min(Symbol())"),
      njs_str("TypeError: Cannot convert a Symbol value to a number") },

    { njs_str("[Symbol(), Symbol()].join()"),
      njs_str("TypeError: Cannot convert a Symbol value to a string") },

    { njs_str("Symbol().toString()"),
      njs_str("Symbol()") },

    { njs_str("Symbol('desc').toString()"),
      njs_str("Symbol(desc)") },

    { njs_str("Symbol('α'.repeat(16)).toString()"),
      njs_str("Symbol(αααααααααααααααα)") },

    { njs_str("Symbol(undefined).toString()"),
      njs_str("Symbol()") },

    { njs_str("Symbol(null).toString()"),
      njs_str("Symbol(null)") },

    { njs_str("Symbol(123).toString()"),
      njs_str("Symbol(123)") },

    { njs_str("Symbol(false).toString()"),
      njs_str("Symbol(false)") },

    { njs_str("Symbol(Symbol()).toString()"),
      njs_str("TypeError: Cannot convert a Symbol value to a string") },

    { njs_str("var all = [ 'asyncIterator',"
              "            'hasInstance',"
              "            'isConcatSpreadable',"
              "            'iterator',"
              "            'match',"
              "            'matchAll',"
              "            'replace',"
              "            'search',"
              "            'species',"
              "            'split',"
              "            'toPrimitive',"
              "            'toStringTag',"
              "            'unscopables' ]; "
              "Object.getOwnPropertyNames(Symbol)"
              ".filter((x) => typeof Symbol[x] == 'symbol')"
              ".length == all.length"),
      njs_str("true") },

    { njs_str("var all = [ 'asyncIterator',"
              "            'hasInstance',"
              "            'isConcatSpreadable',"
              "            'iterator',"
              "            'match',"
              "            'matchAll',"
              "            'replace',"
              "            'search',"
              "            'species',"
              "            'split',"
              "            'toPrimitive',"
              "            'toStringTag',"
              "            'unscopables' ]; "
              "Object.getOwnPropertyNames(Symbol)"
              ".filter((x) => typeof Symbol[x] == 'symbol')"
              ".filter((x, i) => !all.includes(x))"),
      njs_str("") },

    { njs_str("Object.getOwnPropertyNames(Symbol)"
              ".filter((x) => typeof Symbol[x] == 'symbol')"
              ".map(x => ({ k: x, v: Symbol[x] }))"
              ".every((x) => 'Symbol(Symbol.' + x.k + ')' == String(x.v))"),
      njs_str("true") },

    { njs_str("typeof Symbol.prototype.description"),
      njs_str("TypeError: unexpected value type:object") },

    { njs_str("Symbol.prototype.description = 1"),
      njs_str("TypeError: Cannot set property \"description\" of object which has only a getter") },

    { njs_str("typeof Symbol().description"),
      njs_str("undefined") },

    { njs_str("Symbol('desc').description"),
      njs_str("desc") },

    { njs_str("Symbol.iterator.description"),
      njs_str("Symbol.iterator") },

    { njs_str("var o = {}; o[Symbol.isConcatSpreadable] = true; "
              "o[Symbol.isConcatSpreadable]"),
      njs_str("true") },

    { njs_str("var o = {[Symbol.isConcatSpreadable]:true}; "
              "o[Symbol.isConcatSpreadable]"),
      njs_str("true") },

    { njs_str("var o = {[Symbol.isConcatSpreadable]:true}; "
              "delete o[Symbol.isConcatSpreadable];"
              "o[Symbol.isConcatSpreadable]"),
      njs_str("undefined") },

    { njs_str("var o = {get [Symbol.isConcatSpreadable](){return true}}; "
              "o[Symbol.isConcatSpreadable]"),
      njs_str("true") },

    { njs_str("({[Symbol.isConcatSpreadable]:()=>true})[Symbol.isConcatSpreadable]()"),
      njs_str("true") },

    { njs_str("var o = {a:1, [Symbol.isConcatSpreadable]:true}; "
              "["
              " 'getOwnPropertyNames',"
              " 'keys', "
              " 'getOwnPropertySymbols',"
              " 'getOwnPropertyDescriptors',"
               "]"
              ".map(v=>Object[v](o)).map(v=>njs.dump(v))"),
      njs_str("['a'],['a'],[Symbol(Symbol.isConcatSpreadable)],"
              "{a:{value:1,writable:true,enumerable:true,configurable:true},"
              "Symbol(Symbol.isConcatSpreadable):{value:true,writable:true,enumerable:true,configurable:true}}") },

    { njs_str("var o = {}; o[Symbol.isConcatSpreadable] = true; "
              "Object.getOwnPropertyDescriptors(o)[Symbol.isConcatSpreadable].value"),
      njs_str("true") },

    { njs_str("var o = Object.defineProperties({}, {[Symbol.isConcatSpreadable]:{value:true}}); "
              "o[Symbol.isConcatSpreadable]"),
      njs_str("true") },

    { njs_str("var o = Object.defineProperty({}, Symbol.isConcatSpreadable, "
              "{configurable:false, writable:false, value:true}); "
              "o[Symbol.isConcatSpreadable]"),
      njs_str("true") },

    { njs_str("var o = {}; o[Symbol.isConcatSpreadable] = true; "
              "Object.getOwnPropertyDescriptor(o, Symbol.isConcatSpreadable).value"),
      njs_str("true") },

    { njs_str("var o = {}, n = 5381 /* NJS_DJB_HASH_INIT */;"
              "while(n--) o[Symbol()] = 'test'; o[''];"),
      njs_str("undefined") },

    { njs_str("["
              " Object.prototype,"
              " Symbol.prototype,"
              " Math,"
              " JSON,"
              " process,"
              " njs,"
              " this,"
               "]"
              ".map(v=>Object.getOwnPropertyDescriptor(v, Symbol.toStringTag))"
              ".map(d=>{if (d && !d.writable && !d.enumerable && d.configurable) return d.value})"
              ".map(v=>njs.dump(v))"),
      njs_str("undefined,Symbol,Math,JSON,process,njs,global") },

    /* String */

    { njs_str("String()"),
      njs_str("") },

    { njs_str("new String()"),
      njs_str("") },

    { njs_str("new String"),
      njs_str("") },

    { njs_str("String(123)"),
      njs_str("123") },

    { njs_str("new String(123)"),
      njs_str("123") },

    { njs_str("String(Symbol())"),
      njs_str("Symbol()") },

    { njs_str("String(Symbol('desc'))"),
      njs_str("Symbol(desc)") },

    { njs_str("new String(Symbol())"),
      njs_str("TypeError: Cannot convert a Symbol value to a string") },

    { njs_str("String(Object(Symbol()))"),
      njs_str("TypeError: Cannot convert a Symbol value to a string") },

    { njs_str("Object('123').length"),
      njs_str("3") },

    { njs_str("new String(123).length"),
      njs_str("3") },

    { njs_str("new String(123).toString()"),
      njs_str("123") },

    { njs_str("String([1,2,3])"),
      njs_str("1,2,3") },

    { njs_str("new String([1,2,3])"),
      njs_str("1,2,3") },

    { njs_str("var s = new String('αβ'); s.one = 1; 'one' in s"),
      njs_str("true") },

    { njs_str("var s = new String('αβ'); 'one' in s"),
      njs_str("false") },

    { njs_str("var s = new String('αβ'); s.one = 1; '1' in s"),
      njs_str("true") },

    { njs_str("var s = new String('αβ'); s.one = 1; 1 in s"),
      njs_str("true") },

    { njs_str("var s = new String('αβ'); s.one = 1; 2 in s"),
      njs_str("false") },

    { njs_str("var s = new String('αβ'); s[1]"),
      njs_str("β") },

    { njs_str("Object.create(new String('αβ'))[1]"),
      njs_str("β") },

    { njs_str("var s = new String('αβ'); s[1] = 'b'"),
      njs_str("TypeError: Cannot assign to read-only property \"1\" of object string") },

    { njs_str("var o = Object.create(new String('αβ')); o[1] = 'a'"),
      njs_str("TypeError: Cannot assign to read-only property \"1\" of object") },

    { njs_str("var s = new String('αβ'); s[4] = 'ab'; s[4]"),
      njs_str("ab") },

    { njs_str("Object.create(new String('αβ')).length"),
      njs_str("2") },

    { njs_str("var s = new String('αβ'); s.valueOf()[1]"),
      njs_str("β") },

    { njs_str("var o = { toString: function() { return 'OK' } };"
                 "String(o)"),
      njs_str("OK") },

    { njs_str("var o = { toString: function() { return 'OK' } };"
                 "new String(o)"),
      njs_str("OK") },

    { njs_str("typeof String('abc')"),
      njs_str("string") },

    { njs_str("typeof new String('abc')"),
      njs_str("object") },

    { njs_str("typeof new String"),
      njs_str("object") },

    { njs_str("String.name"),
      njs_str("String") },

    { njs_str("String.length"),
      njs_str("1") },

    /* values_hash long vs long string collision. */
    { njs_str("'XXXXXXXXXXXXXXXQWEEAB' + 'XXXXXXXXXXXXXXXZHGP'"),
      njs_str("XXXXXXXXXXXXXXXQWEEABXXXXXXXXXXXXXXXZHGP") },

    /* values_hash short vs long string collision. */
    { njs_str("'SHAAAB' + 'XXXXXXXXXXXXXXXUETBF'"),
      njs_str("SHAAABXXXXXXXXXXXXXXXUETBF") },

    /* values_hash long vs short string collision. */
    { njs_str("'XXXXXXXXXXXXXXXUETBF' + 'SHAAAB'"),
      njs_str("XXXXXXXXXXXXXXXUETBFSHAAAB") },

    /* values_hash short vs short string collision. */
    { njs_str("'XUBAAAB' + 'XGYXKY'"),
      njs_str("XUBAAABXGYXKY") },

    { njs_str("String.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("Object.prototype.toString.call(String.prototype)"),
      njs_str("[object String]") },

    { njs_str("String.prototype"),
      njs_str("") },

    { njs_str("String.prototype.length"),
      njs_str("0") },

    { njs_str("String.prototype.constructor === String"),
      njs_str("true") },

    { njs_str("String.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("String.prototype.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("''.__proto__ === String.prototype"),
      njs_str("true") },

    { njs_str("String.constructor === Function"),
      njs_str("true") },

    { njs_str("'test'.__proto__ === String.prototype"),
      njs_str("true") },

    { njs_str("var s = String('abc'); s.__proto__ === String.prototype"),
      njs_str("true") },

    { njs_str("var s = new String('abc'); s.__proto__ === String.prototype"),
      njs_str("true") },

    { njs_str("'test'.constructor === String"),
      njs_str("true") },

    { njs_str("'test'.constructor.prototype === String.prototype"),
      njs_str("true") },

    { njs_str("var o = Object.create(String.prototype); o.length = 1"),
      njs_str("TypeError: Cannot assign to read-only property \"length\" of object") },

    { njs_str("Function.name"),
      njs_str("Function") },

    { njs_str("Function.length"),
      njs_str("1") },

    { njs_str("Function.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("Function.prototype.constructor === Function"),
      njs_str("true") },

    { njs_str("Function.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("Function.prototype.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("Object.prototype.toString.call(Function.prototype)"),
      njs_str("[object Function]") },

    { njs_str("Function.prototype"),
      njs_str("[object Function]") },

    { njs_str("Function.prototype.length"),
      njs_str("0") },

    { njs_str("Function.constructor === Function"),
      njs_str("true") },

    { njs_str("function f() {} f.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("Function()"),
      njs_str("[object Function]") },

    { njs_str("new Function();"),
      njs_str("[object Function]") },

    { njs_str("(function(){}).constructor === Function"),
      njs_str("true") },

#if NJS_HAVE_LARGE_STACK
    { njs_str("new Function(\"(\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"{\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"[\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"`\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"{[\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"{;\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"1;\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"~\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"new \".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"typeof \".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"1\" + \"** 1\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },

    { njs_str("new Function(\"var a; a\" + \"= a\".repeat(2**13));"),
      njs_str("RangeError: Maximum call stack size exceeded") },
#endif

    { njs_str("var f = new Function('return 1;'); f();"),
      njs_str("1") },

    { njs_str("var sum = new Function('a', 'b', 'return a + b');"
                 "sum(2, 4);"),
      njs_str("6") },

    { njs_str("var sum = new Function('a, b', 'return a + b');"
                 "sum(2, 4);"),
      njs_str("6") },

    { njs_str("var sum = new Function('a, b', 'c', 'return a + b + c');"
                 "sum(2, 4, 4);"),
      njs_str("10") },

    { njs_str("(new Function({ toString() { return '...a'; }}, { toString() { return 'return a;' }}))(1,2,3)"),
      njs_str("1,2,3") },

    { njs_str("var x = 10; function foo() { var x = 20; return new Function('return x;'); }"
                 "var f = foo(); f()"),
      njs_str("10") },

    { njs_str("var fn = (function() { return new Function('return this'); }).call({}), o = {}; fn.call(o) == o && fn.bind(o).call(this) == o"),
      njs_str("true") },

    { njs_str("this.NN = {}; var f = Function('eval = 42;'); f()"),
      njs_str("SyntaxError: Identifier \"eval\" is forbidden as left-hand in assignment in runtime:1") },

    { njs_str("RegExp()"),
      njs_str("/(?:)/") },

    { njs_str("RegExp('')"),
      njs_str("/(?:)/") },

    { njs_str("RegExp(123)"),
      njs_str("/123/") },

    { njs_str("RegExp.name"),
      njs_str("RegExp") },

    { njs_str("RegExp.length"),
      njs_str("2") },

    { njs_str("RegExp.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("RegExp.prototype.constructor === RegExp"),
      njs_str("true") },

    { njs_str("RegExp.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("RegExp.prototype.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("Object.prototype.toString.call(RegExp.prototype)"),
      njs_str("[object RegExp]") },

    { njs_str("RegExp.prototype"),
      njs_str("/(?:)/") },

    { njs_str("RegExp.constructor === Function"),
      njs_str("true") },

    { njs_str("/./.__proto__ === RegExp.prototype"),
      njs_str("true") },

    { njs_str("toString()"),
      njs_str("[object Undefined]") },

    { njs_str("toString() + Object.prototype.toString"),
      njs_str("[object Undefined][object Function]") },

#if 0
    { njs_str("toString === Object.prototype.toString"),
      njs_str("true") },

    { njs_str("Object.prototype.toString.yes = 'OK'; toString.yes"),
      njs_str("OK") },
#endif

    { njs_str("Object.prototype.toString.call()"),
      njs_str("[object Undefined]") },

    { njs_str("Object.prototype.toString.call(undefined)"),
      njs_str("[object Undefined]") },

    { njs_str("Object.prototype.toString.call(null)"),
      njs_str("[object Null]") },

    { njs_str("Object.prototype.toString.call(true)"),
      njs_str("[object Boolean]") },

    { njs_str("Boolean.prototype[Symbol.toStringTag] = 'XXX';"
              "Object.prototype.toString.call(true)"),
      njs_str("[object XXX]") },

    { njs_str("Object.prototype.toString.call(1)"),
      njs_str("[object Number]") },

    { njs_str("Object.prototype.toString.call('')"),
      njs_str("[object String]") },

    { njs_str("Object.prototype.toString.call({})"),
      njs_str("[object Object]") },

    { njs_str("Object.prototype.toString.call([])"),
      njs_str("[object Array]") },

    { njs_str("var a = []; a[Symbol.toStringTag] = 'XXX';"
              "Object.prototype.toString.call(a)"),
      njs_str("[object XXX]") },

    { njs_str("Object.prototype.toString.call(new Object(true))"),
      njs_str("[object Boolean]") },

    { njs_str("Object.prototype.toString.call(new Number(1))"),
      njs_str("[object Number]") },

    { njs_str("Object.prototype.toString.call(new Object(1))"),
      njs_str("[object Number]") },

    { njs_str("Object.prototype.toString.call(new Object(Symbol()))"),
      njs_str("[object Symbol]") },

    { njs_str("Object.prototype.toString.call(new Object(Symbol('desc')))"),
      njs_str("[object Symbol]") },

    { njs_str("Object.prototype.toString.call(new Object(''))"),
      njs_str("[object String]") },

    { njs_str("Object.prototype.toString.call(function(){})"),
      njs_str("[object Function]") },

    { njs_str("var f = ()=>1; f[Symbol.toStringTag] = 'α'.repeat(32);"
              "var toStr = Object.prototype.toString.call(f); [toStr, toStr.length]"),
      njs_str("[object αααααααααααααααααααααααααααααααα],41") },

    { njs_str("Object.prototype.toString.call(/./)"),
      njs_str("[object RegExp]") },

    { njs_str("Object.prototype.toString.call(Math)"),
      njs_str("[object Math]") },

    { njs_str("Object.prototype.toString.call(JSON)"),
      njs_str("[object JSON]") },

    { njs_str("var p = { a:5 }; var o = Object.create(p); o.a"),
      njs_str("5") },

    { njs_str("var p = { a:5 }; var o = Object.create(p);"
                 "o.__proto__ === p"),
      njs_str("true") },

    /* Object.create() */

    { njs_str("var o = Object.create(Object.prototype);"
                 "o.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("var o = Object.create(null); '__proto__' in o"),
      njs_str("false") },

    { njs_str("Object.create()"),
      njs_str("TypeError: prototype may only be an object or null: undefined") },

    { njs_str("Object.create(1)"),
      njs_str("TypeError: prototype may only be an object or null: number") },

    { njs_str("var o = Object.create(null, { a: { value: 1 } }); o.a"),
      njs_str("1") },

    { njs_str("var o = Object.create({ a: 0 }, { a: { value: 1 } }); o.a"),
      njs_str("1") },

    { njs_str("var o = Object.create({ get a() { return this.b; } }, { b: { value: 1 } }); o.a"),
      njs_str("1") },

    { njs_str("var o = {a:1, b:2, c:3};"
                 "Object.keys(o)"),
      njs_str("a,b,c") },

    { njs_str("var a = []; a.one = 7; Object.keys(a)"),
      njs_str("one") },

    { njs_str("var a = [,,]; a.one = 7; Object.keys(a)"),
      njs_str("one") },

    { njs_str("var a = [,6,,3]; a.one = 7; Object.keys(a)"),
      njs_str("1,3,one") },

    { njs_str("var o = {a:1,b:2}; delete o.a; Object.keys(o)"),
      njs_str("b") },

    { njs_str("Object.keys()"),
      njs_str("TypeError: cannot convert undefined argument to object") },

    { njs_str("Object.keys('αβZ')"),
      njs_str("0,1,2") },

    { njs_str("Object.keys(new String('αβZ'))"),
      njs_str("0,1,2") },

    { njs_str("var s = new String('αβZ'); s.a = 1; Object.keys(s)"),
      njs_str("0,1,2,a") },

    { njs_str("var r = new RegExp('αbc'); r.a = 1; Object.keys(r)"),
      njs_str("a") },

    { njs_str("Object.keys(Object.create(new String('abc')))"),
      njs_str("") },

    { njs_str("Object.keys(1)"),
      njs_str("") },

    { njs_str("Object.keys(true)"),
      njs_str("") },

    { njs_str("var o = {a:3, b:2, c:1}; Object.values(o)"),
      njs_str("3,2,1") },

    { njs_str("Object.values('s')"),
      njs_str("s") },

    { njs_str("Object.values('абв abc')"),
      njs_str("а,б,в, ,a,b,c") },

    { njs_str("var s = new String('abc'); s.three = 3; Object.values(s)"),
      njs_str("a,b,c,3") },

    { njs_str("var a = [,,5,,4,,,3,2]; a.one = 1; Object.values(a)"),
      njs_str("5,4,3,2,1") },

    { njs_str("Object.values([{}, null, false, NaN, function() {}])"),
      njs_str("[object Object],,false,NaN,[object Function]") },

    { njs_str("Object.values(1)"),
      njs_str("") },

    { njs_str("Object.values(njs)[0] === njs.version"),
      njs_str("true") },

    { njs_str("Object.values(process)"),
      njs_str("") },

    { njs_str("Object.values()"),
      njs_str("TypeError: cannot convert undefined argument to object") },

    { njs_str("Object.entries('abc')"),
      njs_str("0,a,1,b,2,c") },

    { njs_str("JSON.stringify(Object.entries('эюя'))"),
      njs_str("[[\"0\",\"э\"],[\"1\",\"ю\"],[\"2\",\"я\"]]") },

    { njs_str("var o = {a:\"а\", b:\"б\", c:\"ц\"};"
                 "JSON.stringify(Object.entries(o))"),
      njs_str("[[\"a\",\"а\"],[\"b\",\"б\"],[\"c\",\"ц\"]]") },

    { njs_str("JSON.stringify(Object.entries([0]))"),
      njs_str("[[\"0\",0]]") },

    { njs_str("var s = new String('αβ'); s.two = null; s[3] = true;"
                 "Object.entries(s)"),
      njs_str("0,α,1,β,two,,3,true") },

    { njs_str("Object.entries(true)"),
      njs_str("") },

    { njs_str("Object.entries(njs)[0][1] === njs.version"),
      njs_str("true") },

    { njs_str("Object.entries()"),
      njs_str("TypeError: cannot convert undefined argument to object") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {}); o.a"),
      njs_str("undefined") },

    { njs_str("Object.defineProperty({}, 'a', {value:1})"),
      njs_str("[object Object]") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {value:1}); o.a"),
      njs_str("1") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', Object.create({value:1})); o.a"),
      njs_str("1") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {writable:'x', enumerable:'y', configurable:'z'});"
              "var d = Object.getOwnPropertyDescriptor(o, 'a');"
              "d.writable && d.enumerable && d.configurable"),
      njs_str("true") },

    { njs_str("var o = {a:1, c:2}; Object.defineProperty(o, 'b', {});"
                 "Object.keys(o)"),
      njs_str("a,c") },

    { njs_str("var o = {a:1, c:2};"
                 "Object.defineProperty(o, 'b', {enumerable:false});"
                 "Object.keys(o)"),
      njs_str("a,c") },

    { njs_str("var o = {a:1, c:3};"
                 "Object.defineProperty(o, 'b', {enumerable:true});"
                 "Object.keys(o)"),
      njs_str("a,c,b") },

    { njs_str("var o = {a:1, c:2}; Object.defineProperty(o, 'b', {});"
                 "Object.values(o)"),
      njs_str("1,2") },

    { njs_str("var o = {a:1, c:2};"
                 "Object.defineProperty(o, 'b', {enumerable:false, value:3});"
                 "Object.values(o)"),
      njs_str("1,2") },

    { njs_str("var o = {a:1, c:3};"
                 "Object.defineProperty(o, 'b', {enumerable:true, value:2});"
                 "Object.values(o)"),
      njs_str("1,3,2") },

    { njs_str("var o = {a:1, c:2}; Object.defineProperty(o, 'b', {});"
                 "Object.entries(o)"),
      njs_str("a,1,c,2") },

    { njs_str("var o = {a:1, c:2};"
                 "Object.defineProperty(o, 'b', {enumerable:false, value:3});"
                 "Object.entries(o)"),
      njs_str("a,1,c,2") },

    { njs_str("var o = {a:1, c:3};"
                 "Object.defineProperty(o, 'b', {enumerable:true, value:2});"
                 "Object.entries(o)"),
      njs_str("a,1,c,3,b,2") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {}); o.a = 1"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of object") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {writable:false}); o.a = 1"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of object") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {writable:true});"
                 "o.a = 1; o.a"),
      njs_str("1") },

    { njs_str("Object.defineProperty(Object.prototype, 'a', {writable:false});"
                 "var o = {a: 1}; [o.a++, o.a]"),
      njs_str("1,2") },

    { njs_str("var o = {};"
                 "Object.defineProperty(Object.prototype, 'a', {writable:false});"
                 "o.a = 1"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of object") },

    { njs_str("var o = {};"
                 "Object.defineProperty(Object.prototype, 'a', {writable:true});"
                 "o.a = 1; o.a"),
      njs_str("1") },

    { njs_str("var p = Object.create(Function);"
                 "Object.defineProperty(p, 'length', {writable: true});"
                 "p.length = 32; p.length"),
      njs_str("32") },

    { njs_str("var p = Object.create(Math.abs);"
                 "Object.defineProperty(p, 'length', {writable: true});"
                 "p.length = 23; p.length"),
      njs_str("23") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {value:1}); delete o.a"),
      njs_str("TypeError: Cannot delete property \"a\" of object") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {value:1, configurable:true});"
                 "delete o.a; o.a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {value:1, configurable:false});"
                 "delete o.a"),
      njs_str("TypeError: Cannot delete property \"a\" of object") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', Object.create({value:2})); o.a"),
      njs_str("2") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:false});"
                 "Object.defineProperty(o, 'a', {configurable:true})"),
      njs_str("TypeError: Cannot redefine property: \"a\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:false});"
                 "Object.defineProperty(o, 'a', {enumerable:true})"),
      njs_str("TypeError: Cannot redefine property: \"a\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:false});"
                 "Object.defineProperty(o, 'a', {writable:true})"),
      njs_str("TypeError: Cannot redefine property: \"a\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:false});"
                 "Object.defineProperty(o, 'a', {enumerable:false}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:false});"
                 "Object.defineProperty(o, 'a', {}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:false, writable:true});"
                 "Object.defineProperty(o, 'a', {writable:false}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:true, writable:false});"
                 "Object.defineProperty(o, 'a', {writable:true}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {});"
                 "Object.defineProperty(o, 'a', {}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {value:1});"
                 "Object.defineProperty(o, 'a', {value:1}).a"),
      njs_str("1") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {value:1});"
                 "Object.defineProperty(o, 'a', {value:2}).a"),
      njs_str("TypeError: Cannot redefine property: \"a\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:true});"
                 "Object.defineProperty(o, 'a', {value:1}).a"),
      njs_str("1") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:true, get:()=>1});"
                 "Object.defineProperty(o, 'a', {value:1});"
                 "var d = Object.getOwnPropertyDescriptor(o, 'a'); "
                 "[d.value, d.writable, d.enumerable, d.configurable, d.get, d.set]"),
      njs_str("1,false,false,true,,") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {configurable:true, value:1});"
                 "Object.defineProperty(o, 'a', {set:()=>1});"
                 "var d = Object.getOwnPropertyDescriptor(o, 'a'); "
                 "[d.value, d.writable, d.enumerable, d.configurable, d.get, d.set]"),
      njs_str(",,false,true,,[object Function]") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {get: ()=>1, configurable:true}); "
                 "Object.defineProperty(o, 'a', {value:123}); o.a =2"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of object") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {get: ()=>1, configurable:true}); "
                 "Object.defineProperty(o, 'a', {writable:false}); o.a"),
      njs_str("undefined") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {value: 1, configurable:true}); "
                 "Object.defineProperty(o, 'a', {get:()=>1}); o.a = 2"),
      njs_str("TypeError: Cannot set property \"a\" of object which has only a getter") },

    { njs_str("var o = Object.create(Object.defineProperty({}, 'x', { set: function(v) { this.y = v; }})); "
                 "o.x = 123; Object.getOwnPropertyDescriptor(o, 'y').value"),
      njs_str("123") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', { configurable: true, value: 0 });"
                 "Object.defineProperty(o, 'a', { value: 1 });"
                 "Object.defineProperty(o, 'a', { configurable: false, value: 2 }).a"),
      njs_str("2") },

    { njs_str("var o = {}; Object.defineProperty()"),
      njs_str("TypeError: Object.defineProperty is called on non-object") },

    { njs_str("var o = {}; Object.defineProperty(o)"),
      njs_str("TypeError: descriptor is not an object") },

    { njs_str("Object.defineProperty(Function.prototype, 'name', {value:'x'}).name"),
      njs_str("x") },

    { njs_str("Object.defineProperty(Function.prototype, 'xxx', {value:'x'}).xxx"),
      njs_str("x") },

    { njs_str("Object.defineProperty(Object, 'name', {value:'x'}).name"),
      njs_str("x") },

    { njs_str("Object.defineProperty(Object.prototype, 'toString', {value:1}).toString"),
      njs_str("1") },

    { njs_str("var o = Object.defineProperties({}, {a:{value:1}}); o.a"),
      njs_str("1") },

    { njs_str("var o = Object.defineProperties({}, {a:{enumerable:true}, b:{enumerable:true}});"
                 "Object.keys(o)"),
      njs_str("a,b") },

    { njs_str("var desc = Object.defineProperty({b:{value:1, enumerable:true}}, 'a', {});"
                 "var o = Object.defineProperties({}, desc);"
                 "Object.keys(o)"),
      njs_str("b") },

    { njs_str("var o = Object.defineProperties({}, { get x() { return { value: 1 }; } });"
              "Object.getOwnPropertyDescriptor(o, 'x').value"),
      njs_str("1") },

    { njs_str("Object.defineProperties({}, { get x() { return  1; } })"),
      njs_str("TypeError: property descriptor must be an object") },

    { njs_str("var obj = {}; var desc = {value:NaN}; Object.defineProperty(obj, 'foo', desc); "
              "Object.defineProperties(obj, { foo: desc } ).foo"),
      njs_str("NaN") },

    { njs_str("var obj = {}; var desc = {value:-0}; Object.defineProperty(obj, 'foo', desc); "
              "Object.defineProperties(obj, { foo: desc } ).foo"),
      njs_str("-0") },

    { njs_str("var obj = {}; var desc = {value:-0}; Object.defineProperty(obj, 'foo', {value:0}); "
              "Object.defineProperties(obj, { foo: desc } ).foo"),
      njs_str("TypeError: Cannot redefine property: \"foo\"") },

    { njs_str("var obj = {}; var desc = {value:0}; Object.defineProperty(obj, 'foo', {value:-0}); "
              "Object.defineProperties(obj, { foo: desc } ).foo"),
      njs_str("TypeError: Cannot redefine property: \"foo\"") },

    { njs_str("var descs = {a:{value:1}}; Object.defineProperty(descs, 'b', {value:{value:2}});"
              "var o = Object.defineProperties({}, descs);"
              "njs.dump([o.a, o.b])"),
      njs_str("[1,undefined]") },

    { njs_str("var descs = {a:{value:1}}; Object.defineProperty(descs, 'b', {value:{value:2}, enumerable:true});"
              "var o = Object.defineProperties({}, descs);"
              "njs.dump([o.a, o.b])"),
      njs_str("[1,2]") },

    { njs_str("var o = {a:1}; delete o.a;"
                 "Object.defineProperty(o, 'a', { value: 1 }); o.a"),
      njs_str("1") },

    { njs_str("var o = {a:1}; delete o.a;"
                 "Object.defineProperty(o, 'a', { value: 1 }); o.a = 2; o.a"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of object") },

    { njs_str("var o = {a:1}; delete o.a;"
                 "Object.defineProperty(o, 'a', { value: 1, writable:1 }); o.a = 2; o.a"),
      njs_str("2") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, new String('a'), { value: 1}); o.a"),
      njs_str("1") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, {toString:function(){return 'a'}}, { value: 1}); o.a"),
      njs_str("1") },

    { njs_str("var a = [1, 2];"
                 "Object.defineProperty(a, '1', {value: 5}); a[1]"),
      njs_str("5") },

    { njs_str("var a = [1, 2];"
                 "Object.defineProperty(a, '3', {}); njs.dump(a)"),
      njs_str("[1,2,<empty>,undefined]") },

    { njs_str("var a = [1, 2];"
                 "Object.defineProperty(a, 'length', {}); a"),
      njs_str("1,2") },

    { njs_str("var a = [1, 2];"
                 "Object.defineProperty(a, 'length', {value: 1}); a"),
      njs_str("1") },

    { njs_str("var a = [1, 2];"
                 "Object.defineProperty(a, 'length', {value: 5}); a"),
      njs_str("1,2,,,") },

    { njs_str("var o = Object.defineProperties({a:1}, {}); o.a"),
      njs_str("1") },

    { njs_str("var arr = [0, 1]; Object.defineProperty(arr, 'length', {value:3}); arr.length"),
      njs_str("3") },

    { njs_str("Object.defineProperty(Array.prototype, 'toString', { get: function() {return () => 1}});"
                 "'a' + []"),
      njs_str("a1") },

    { njs_str("Object.defineProperty(Array.prototype, 'toJSON', { get: function() {return () => 1}});"
                 "JSON.stringify([])"),
      njs_str("1") },

    { njs_str("Object.defineProperty(Array.prototype, 'join', { get: function() {return () => 1}});"
                 "([]).toString()"),
      njs_str("1") },

    { njs_str("var o = {}, desc = {};"
              "Object.defineProperty(desc, 'get', { get() { return () => 1 } });"
              "Object.defineProperty(o, 'a', desc); o.a"),
      njs_str("1") },

    { njs_str("Object.defineProperty(Error.prototype, 'message', { get() {return 'm'}});"
                 "Object.defineProperty(Error.prototype, 'name', { get() {return 'n'}});"
                 "Error()"),
      njs_str("n: m") },

    { njs_str("var o = {}, desc = {};"
              "Object.defineProperty(desc, 'value', { get() { return 'x'}});"
              "Object.defineProperty(o, 'a', desc); o.a"),
      njs_str("x") },

    { njs_str("var o = {}, desc = {};"
              "Object.defineProperty(desc, 'value', { get() { return 'x'}});"
              "Object.defineProperty(desc, 'enumerable', { get() { return !NaN}});"
              "Object.defineProperty(desc, 'writable', { get() { return 'x'}});"
              "Object.defineProperty(desc, 'configurable', { get() { return 1}});"
              "Object.defineProperty(o, 'a', desc);"
              "var d = Object.getOwnPropertyDescriptor(o, 'a');"
              "d.enumerable && d.writable && d.configurable"),
      njs_str("true") },

    { njs_str("Object.defineProperties()"),
      njs_str("TypeError: Object.defineProperties is called on non-object") },

    { njs_str("Object.defineProperties(1, {})"),
      njs_str("TypeError: Object.defineProperties is called on non-object") },

    { njs_str("njs.dump(Object.defineProperties({}, 1))"),
      njs_str("{}") },

    { njs_str("Object.defineProperties(Object.freeze({b:1}), {b:{value:1}}).b"),
      njs_str("1") },

    { njs_str("Object.defineProperties(Object.freeze({b:1}), {b:{value:2}})"),
      njs_str("TypeError: Cannot redefine property: \"b\"") },

    { njs_str("Object.defineProperties(Object.freeze({b:1}), {c:{value:1}})"),
      njs_str("TypeError: Cannot add property \"c\", object is not extensible") },

    { njs_str("var o = {a:1}; o.hasOwnProperty('a')"),
      njs_str("true") },

    { njs_str("var o = Object.create({a:2}); o.hasOwnProperty('a')"),
      njs_str("false") },

    { njs_str("var o = {a:1}; o.hasOwnProperty('b')"),
      njs_str("false") },

    { njs_str("var a = []; a.hasOwnProperty('0')"),
      njs_str("false") },

    { njs_str("var a = [,,]; a.hasOwnProperty('0')"),
      njs_str("false") },

    { njs_str("var a = [3,,]; a.hasOwnProperty('0')"),
      njs_str("true") },

    { njs_str("var a = [,4]; a.hasOwnProperty('1')"),
      njs_str("true") },

    { njs_str("var a = [3,4]; a.hasOwnProperty('2')"),
      njs_str("false") },

    { njs_str("var a = [3,4]; a.one = 1; a.hasOwnProperty('one')"),
      njs_str("true") },

    { njs_str("var o = {a:1}; o.hasOwnProperty(o)"),
      njs_str("false") },

    { njs_str("var o = {a:1}; o.hasOwnProperty(1)"),
      njs_str("false") },

    { njs_str("var o = {a:1}; o.hasOwnProperty()"),
      njs_str("false") },

    { njs_str("[,].hasOwnProperty()"),
      njs_str("false") },

    { njs_str("[1,2].hasOwnProperty('len')"),
      njs_str("false") },

    { njs_str("[1,2].hasOwnProperty('0')"),
      njs_str("true") },

    { njs_str("[1,2].hasOwnProperty('2')"),
      njs_str("false") },

    { njs_str("[].hasOwnProperty('length')"),
      njs_str("true") },

    { njs_str("[1,2].hasOwnProperty('length')"),
      njs_str("true") },

    { njs_str("(new Array()).hasOwnProperty('length')"),
      njs_str("true") },

    { njs_str("(new Array(10)).hasOwnProperty('length')"),
      njs_str("true") },

    { njs_str("Object.valueOf.hasOwnProperty()"),
      njs_str("false") },

    { njs_str("1..hasOwnProperty('b')"),
      njs_str("false") },

    { njs_str("'s'.hasOwnProperty('b')"),
      njs_str("false") },

    { njs_str("'s'.hasOwnProperty('0')"),
      njs_str("true") },

    { njs_str("'s'.hasOwnProperty('1')"),
      njs_str("false") },

    { njs_str("Object.hasOwnProperty('hasOwnProperty')"),
      njs_str("false") },

    { njs_str("Object.prototype.hasOwnProperty('hasOwnProperty')"),
      njs_str("true") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:undefined, set:undefined}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:1})"),
      njs_str("TypeError: Getter must be a function") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:undefined}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:function(){return 1}}).a"),
      njs_str("1") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {set:1})"),
      njs_str("TypeError: Setter must be a function") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {set:undefined}); o.a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {set:undefined}); o.a = 4;"),
      njs_str("TypeError: Cannot set property \"a\" of object which has only a getter") },

    { njs_str("var o = {a: 0};"
                 "Object.defineProperty(o, 'b', {set:function(x){this.a = x / 2;}}); o.b = 4; o.a;"),
      njs_str("2") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {value:undefined});"
                 "Object.defineProperty(o, 'a', {get:undefined})"),
      njs_str("TypeError: Cannot redefine property: \"a\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {});"
                 "Object.defineProperty(o, 'a', {get:undefined})"),
      njs_str("TypeError: Cannot redefine property: \"a\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:undefined});"
                 "Object.defineProperty(o, 'a', {get:undefined}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:undefined});"
                 "Object.defineProperty(o, 'a', {set:undefined}).a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:undefined});"
                 "Object.defineProperty(o, 'a', {}); o.a"),
      njs_str("undefined") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:undefined});"
                 "Object.defineProperty(o, 'a', {set:function(){}})"),
      njs_str("TypeError: Cannot redefine property: \"a\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:undefined});"
                 "Object.defineProperty(o, 'a', {get:function(){}})"),
      njs_str("TypeError: Cannot redefine property: \"a\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:()=>1, configurable:true});"
                 "Object.defineProperty(o, 'a', {get:()=>2}); o.a"),
      njs_str("2") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {set:function(v){this.aa=v;}, configurable:true});"
                 "Object.defineProperty(o, 'a', {get:function(){return this.aa}}); o.a = 1; o.a"),
      njs_str("1") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:()=>1, configurable:true});"
                 "Object.defineProperty(o, 'a', {value:2}).a"),
      njs_str("2") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {get:()=>1, configurable:true});"
                 "Object.defineProperty(o, 'a', {value:2});"
                 "var d = Object.getOwnPropertyDescriptor(o, 'a');"
                 "d.get === undefined && d.set === undefined"),
      njs_str("true") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'a', {value:1, configurable:true});"
                 "Object.defineProperty(o, 'a', {get:()=>2});"
                 "var d = Object.getOwnPropertyDescriptor(o, 'a');"
                 "d.writable === undefined && d.value === undefined"),
      njs_str("true") },

    { njs_str("Object.defineProperty(Date.prototype, 'year', {get: function() { return this.getFullYear();}});"
                 "var d = new Date(0); d.year"),
      njs_str("1970") },

    { njs_str("var o = [];"
                 "Object.defineProperty(o, 'fill', {get:undefined}).fill"),
      njs_str("undefined") },

    { njs_str("var o = [];"
                 "Object.defineProperty(o, 'length', {get:undefined})"),
      njs_str("TypeError: Cannot redefine property: \"length\"") },

    { njs_str("var o = {};"
                 "Object.defineProperty(o, 'foo', {get:()=>10});"
                 "Object.preventExtensions(o);Object.isFrozen(o)"),
      njs_str("true"), },

    { njs_str("var o = {}; Object.defineProperty(o, 'r', { get: function() { return this.r_value; } }); o.r_value = 1; o.r;"),
      njs_str("1") },

    { njs_str("var o = {}; Object.defineProperty(o, 'rw', { get: function() { return this.rw_value; }, set: function(x) { this.rw_value = x / 2; } }); o.rw = 10; o.rw"),
      njs_str("5") },

    { njs_str("var o = {}; function foo() {}; Object.defineProperty(o, 'a', {get: foo});"
                 "Object.getOwnPropertyDescriptor(o, 'a').get === foo"),
      njs_str("true") },

    { njs_str("var o = {}; Object.defineProperty(o, 'a', {get: undefined});"
                "JSON.stringify(Object.getOwnPropertyDescriptor(o, 'a')).set"),
      njs_str("undefined") },

    { njs_str("var get = 'get'; var o = { get }; o.get"),
      njs_str("get") },

    { njs_str("var o = { get foo() { return 'bar'; } }; o.foo"),
      njs_str("bar") },

    { njs_str("var o = { get get() { return 'bar'; } }; o.get"),
      njs_str("bar") },

    { njs_str("var o = { get() { return 'bar'; } }; o.get()"),
      njs_str("bar") },

    { njs_str("var o = { get lazy() { delete this.lazy; return this.lazy = Math.pow(2,3)} };o.lazy"),
      njs_str("8") },

    { njs_str("var o = { get lazy() { delete this.lazy; return this.lazy = Math.pow(2,3)} }; o.lazy;"
              "Object.getOwnPropertyDescriptor(o, 'lazy').value"),
      njs_str("8") },

    { njs_str("var expr = 'foo'; var o = { get [expr]() { return 'bar'; } }; o.foo"),
      njs_str("bar") },

    { njs_str("var o = { get [{toString(){return 'get'}}]() { return 'bar'; } }; o.get"),
      njs_str("bar") },

    { njs_str("var o = { get [{toString(){return {} }}]() { return 'bar'; } }; o.get"),
      njs_str("InternalError: failed conversion of type \"object\" to string while property define") },

    { njs_str("var o = { get foo(v1, v2) { return 'bar'; } }; o.foo"),
      njs_str("SyntaxError: Getter must not have any formal parameters in 1") },

    { njs_str("var o = { baz: 'bar', set foo(v) { this.baz = v; } }; o.foo = 'baz'; o.baz"),
      njs_str("baz") },

    { njs_str("var o = { baz: 'bar', set set(v) { this.baz = v; } }; o.set = 'baz'; o.baz"),
      njs_str("baz") },

    { njs_str("var expr = 'foo'; var o = { baz: 'bar', set [expr](v) { this.baz = v; } }; o.foo = 'baz'; o.baz"),
      njs_str("baz") },

    { njs_str("var o = { baz: 'bar', set foo(v1, v2) { this.baz = v; } }; o.foo = 'baz'; o.baz"),
      njs_str("SyntaxError: Setter must have exactly one formal parameter in 1") },

    { njs_str("var o = { get foo() { return 'bar'; }, set foo(v) { this.baz = v; } }; o.foo"),
      njs_str("bar") },

    { njs_str("var expr = 'foo'; var o = { get [expr]() { return 'bar'; }, set [expr](v) { this.baz = v; } }; o.foo"),
      njs_str("bar") },

    { njs_str("Object.getOwnPropertyDescriptor({get foo() {}}, 'foo').enumerable"),
      njs_str("true") },

    { njs_str("Object.getOwnPropertyDescriptor({get foo() {}}, 'foo').configurable"),
      njs_str("true") },

    { njs_str("var p = { a:5 }; var o = Object.create(p);"
                 "Object.getPrototypeOf(o) === p"),
      njs_str("true") },

    { njs_str("var p = { a:5 }; var o = Object.create(p);"
                 "Object.getPrototypeOf(o) === o.__proto__"),
      njs_str("true") },

    { njs_str("var o = Object.create(Object.prototype);"
                 "Object.getPrototypeOf(o) === Object.prototype"),
      njs_str("true") },

    { njs_str("[true, 42, '', Symbol()]"
              ".every((x) => Object.getPrototypeOf(x) == Object.getPrototypeOf(Object(x)))"),
      njs_str("true") },

    /* Object.setPrototypeOf() */

    { njs_str("Object.setPrototypeOf()"),
      njs_str("TypeError: cannot convert undefined argument to object") },

    { njs_str("Object.setPrototypeOf(null)"),
      njs_str("TypeError: cannot convert null argument to object") },

    { njs_str("Object.setPrototypeOf({})"),
      njs_str("TypeError: prototype may only be an object or null: undefined") },

    { njs_str("Object.setPrototypeOf(Object.preventExtensions({}))"),
      njs_str("TypeError: prototype may only be an object or null: undefined") },

    { njs_str("[true, 42, '', Symbol()]"
              ".every((x) => Object.setPrototypeOf(x, {}) === x)"),
      njs_str("true") },

    { njs_str("Object.setPrototypeOf(Object.preventExtensions({}), {})"),
      njs_str("TypeError: Cannot set property \"prototype\", object is not extensible") },

    { njs_str("var p = {}, o = Object.create(p); Object.setPrototypeOf(p, o)"),
      njs_str("TypeError: Cyclic __proto__ value") },

    { njs_str("var p = {}, o = {}; Object.setPrototypeOf(o, p);"
              "p.isPrototypeOf(o)"),
      njs_str("true") },

    { njs_str("var p = {}, o = Object.create(p); Object.setPrototypeOf(o, null);"
              "Object.getPrototypeOf(o)"),
      njs_str("null") },

    { njs_str("typeof Object.setPrototypeOf({}, null)"),
      njs_str("object") },

    { njs_str("var p = {}; var o = Object.create(p);"
                 "p.isPrototypeOf(o)"),
      njs_str("true") },

    { njs_str("var pp = {}; var p = Object.create(pp);"
                 "var o = Object.create(p);"
                 "pp.isPrototypeOf(o)"),
      njs_str("true") },

    { njs_str("var p = {}; var o = Object.create(p);"
                 "o.isPrototypeOf(p)"),
      njs_str("false") },

    { njs_str("var p = {}; var o = Object.create(p);"
                 "o.isPrototypeOf()"),
      njs_str("false") },

    { njs_str("Object.valueOf.isPrototypeOf()"),
      njs_str("false") },

    { njs_str("var p = {}; var o = Object.create(p);"
                 "o.isPrototypeOf(1)"),
      njs_str("false") },

    { njs_str("var p = {}; var o = Object.create(p);"
                 "1..isPrototypeOf(p)"),
      njs_str("false") },

    { njs_str("Object.create(new String('asdf')).length"),
      njs_str("4") },

    { njs_str("Object.create(Object('123')).length"),
      njs_str("3") },

    { njs_str("Object.create([1,2]).length"),
      njs_str("2") },

    { njs_str("Object.create(function(a,b,c){}).length"),
      njs_str("3") },

    { njs_str("Object.create(Math).hasOwnProperty('abs')"),
      njs_str("false") },

    { njs_str("var m = Object.create(Math); m.abs = 3;"
                 "[m.hasOwnProperty('abs'), m.abs]"),
      njs_str("true,3") },

    { njs_str("var m = Object.create(Math); m.abs = Math.floor;"
                 "[m.hasOwnProperty('abs'), delete m.abs, m.abs(-1)]"),
      njs_str("true,true,1") },

    { njs_str("Object.getOwnPropertyDescriptor({a:1}, 'a').value"),
      njs_str("1") },

    { njs_str("Object.getOwnPropertyDescriptor({a:1}, 'a').configurable"),
      njs_str("true") },

    { njs_str("Object.getOwnPropertyDescriptor({a:1}, 'a').enumerable"),
      njs_str("true") },

    { njs_str("Object.getOwnPropertyDescriptor({a:1}, 'a').writable"),
      njs_str("true") },

    { njs_str("Object.getOwnPropertyDescriptor({a:1}, 'b')"),
      njs_str("undefined") },

    { njs_str("Object.getOwnPropertyDescriptor({}, 'a')"),
      njs_str("undefined") },

    { njs_str("Object.getOwnPropertyDescriptor(Object.create({a:1}), 'a')"),
      njs_str("undefined") },

    { njs_str("Object.getOwnPropertyDescriptor([3,4], '1').value"),
      njs_str("4") },

    { njs_str("Object.getOwnPropertyDescriptor([3,4], 1).value"),
      njs_str("4") },

    { njs_str("Object.getOwnPropertyDescriptor([], 'length').value"),
      njs_str("0") },

    { njs_str("Object.getOwnPropertyDescriptor([], '0')"),
      njs_str("undefined") },

    { njs_str("Object.getOwnPropertyDescriptor([1,2], '1').value"),
      njs_str("2") },

    { njs_str("Object.getOwnPropertyDescriptor([1,2], new String('1')).value"),
      njs_str("2") },

    { njs_str("Object.getOwnPropertyDescriptor({undefined:1}, void 0).value"),
      njs_str("1") },

    { njs_str("Object.getOwnPropertyDescriptor([1,2], 1).value"),
      njs_str("2") },

    { njs_str("Object.getOwnPropertyDescriptor([1,,,3], '1')"),
      njs_str("undefined") },

    { njs_str("Object.getOwnPropertyDescriptor([1,2], '3')"),
      njs_str("undefined") },

    { njs_str("JSON.stringify(Object.getOwnPropertyDescriptor([3,4], 'length'))"),
      njs_str("{\"value\":2,\"writable\":true,\"enumerable\":false,\"configurable\":false}") },

    { njs_str("Object.getOwnPropertyDescriptor(Array.of, 'length').value"),
      njs_str("0") },

    { njs_str("Object.getOwnPropertyDescriptor('αβγδ', '1').value"),
      njs_str("β") },

    { njs_str("Object.getOwnPropertyDescriptor(new String('αβγδ'), '1').value"),
      njs_str("β") },

    { njs_str("var s = new String('αβγδ'); s.a = 1;"
                 "Object.getOwnPropertyDescriptor(s, 'a').value"),
      njs_str("1") },

    { njs_str("JSON.stringify(Object.getOwnPropertyDescriptor('αβγδ', '2'))"),
      njs_str("{\"value\":\"γ\",\"writable\":false,\"enumerable\":true,\"configurable\":false}") },

    { njs_str("JSON.stringify(Object.getOwnPropertyDescriptor(new String('abc'), 'length'))"),
      njs_str("{\"value\":3,\"writable\":false,\"enumerable\":false,\"configurable\":false}") },

    { njs_str("Object.getOwnPropertyDescriptor(1, '0')"),
      njs_str("undefined") },

    { njs_str("'αβγδ'.propertyIsEnumerable('0')"),
      njs_str("true") },

    { njs_str("({a:1}).propertyIsEnumerable({toString:function () {return 'a';}})"),
      njs_str("true") },

    { njs_str("'αβγδ'.propertyIsEnumerable('a')"),
      njs_str("false") },

    { njs_str("'αβγδ'.propertyIsEnumerable('length')"),
      njs_str("false") },

    { njs_str("var min = Object.getOwnPropertyDescriptor(Math, 'min').value;"
                 "[min(1,2), min(2,1), min(-1,1)]"),
      njs_str("1,1,-1") },

    { njs_str("Object.getOwnPropertyDescriptor()"),
      njs_str("TypeError: cannot convert undefined argument to object") },

    { njs_str("Object.getOwnPropertyDescriptor(undefined)"),
      njs_str("TypeError: cannot convert undefined argument to object") },

    { njs_str("var o = {}; o[void 0] = 'a'; Object.getOwnPropertyDescriptor(o).value"),
      njs_str("a") },

    { njs_str("var o = {}; o[void 0] = 'a'; Object.getOwnPropertyDescriptor(o, undefined).value"),
      njs_str("a") },

    { njs_str("Object.getOwnPropertyDescriptors()"),
      njs_str("TypeError: cannot convert undefined argument to object") },

    { njs_str("typeof Object.getOwnPropertyDescriptors(1)"),
      njs_str("object") },

    { njs_str("Object.keys(Object.getOwnPropertyDescriptors([]))"),
      njs_str("length") },

    { njs_str("Object.getOwnPropertyDescriptors(function(a,b,c) {}).length.value"),
      njs_str("3") },

    { njs_str("Object.values(Object.getOwnPropertyDescriptors('abc'))"
                 ".reduce(function(a, x) { return a += x.value; }, '')"),
      njs_str("abc3") },

    { njs_str("Object.getOwnPropertyNames()"),
      njs_str("TypeError: cannot convert undefined argument to object") },

    { njs_str("Array.isArray(Object.getOwnPropertyNames({}))"),
      njs_str("true") },

    { njs_str("Object.getOwnPropertyNames({a:1, b:1, c:1})"),
      njs_str("a,b,c") },

    { njs_str("Object.getOwnPropertyNames(Object.defineProperty({a:1}, 'b', {}))"),
      njs_str("a,b") },

    { njs_str("Object.getOwnPropertyNames(Object.defineProperty([], 'b', {}))"),
      njs_str("b,length") },

    { njs_str("Object.getOwnPropertyNames(Object.defineProperty(new String(), 'b', {}))"),
      njs_str("b,length") },

    { njs_str("Object.getOwnPropertyNames([1,2,3])"),
      njs_str("0,1,2,length") },

    { njs_str("Object.getOwnPropertyNames('abc')"),
      njs_str("0,1,2,length") },

    { njs_str("Object.getOwnPropertyNames(function() {})"),
      njs_str("length,prototype") },

    { njs_str("Object.getOwnPropertyNames(Array)"),
      njs_str("name,length,prototype,isArray,of") },

    { njs_str("Object.getOwnPropertyNames(Array.isArray)"),
      njs_str("name,length") },

    { njs_str("Object.defineProperty(Object.freeze({}), 'b', {})"),
      njs_str("TypeError: Cannot add property \"b\", object is not extensible") },

    { njs_str("Object.defineProperties(Object.freeze({}), {b:{}})"),
      njs_str("TypeError: Cannot add property \"b\", object is not extensible") },

    { njs_str("Object.freeze()"),
      njs_str("undefined") },

    { njs_str("var o = Object.freeze({a:1}); o.a = 2"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of object") },

    { njs_str("var o = Object.freeze({a:1}); delete o.a"),
      njs_str("TypeError: Cannot delete property \"a\" of object") },

    { njs_str("var o = Object.freeze({a:1}); o.b = 1; o.b"),
      njs_str("TypeError: Cannot add property \"b\", object is not extensible") },

    { njs_str("var o = Object.freeze(Object.create({a:1})); o.a = 2; o.a"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("var o = Object.freeze({a:{b:1}}); o.a.b = 2; o.a.b"),
      njs_str("2") },

    { njs_str("Object.defineProperty([1,2], 'a', {value:1}).a"),
      njs_str("1") },

    { njs_str("var a = Object.freeze([1,2]);"
                 "Object.defineProperty(a, 'a', {value:1}).a"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("var a = [1,2]; a.a = 1; Object.freeze(a); delete a.a"),
      njs_str("TypeError: Cannot delete property \"a\" of array") },

    { njs_str("var a = [1,2]; a.a = 1; Object.freeze(a); a.a = 2"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of array") },

    { njs_str("var a = Object.freeze([1,2]); a.a = 1"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("Object.defineProperty(function() {}, 'a', {value:1}).a"),
      njs_str("1") },

    { njs_str("var f = Object.freeze(function() {});"
                 "Object.defineProperty(f, 'a', {value:1}).a"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("var f = function() {}; f.a = 1; Object.freeze(f); delete f.a"),
      njs_str("TypeError: Cannot delete property \"a\" of function") },

    { njs_str("var f = function() {}; f.a = 1; Object.freeze(f); f.a = 2"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of function") },

    { njs_str("var f = Object.freeze(function() {}); f.a = 1"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("Object.defineProperty(new Date(''), 'a', {value:1}).a"),
      njs_str("1") },

    { njs_str("var d = Object.freeze(new Date(''));"
                 "Object.defineProperty(d, 'a', {value:1}).a"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("var d = new Date(''); d.a = 1; Object.freeze(d);"
                 "delete d.a"),
      njs_str("TypeError: Cannot delete property \"a\" of date") },

    { njs_str("var d = new Date(''); d.a = 1; Object.freeze(d); d.a = 2"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of date") },

    { njs_str("var d = Object.freeze(new Date('')); d.a = 1"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("Object.defineProperty(new RegExp(''), 'a', {value:1}).a"),
      njs_str("1") },

    { njs_str("var r = Object.freeze(new RegExp(''));"
                 "Object.defineProperty(r, 'a', {value:1}).a"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("var r = new RegExp(''); r.a = 1; Object.freeze(r); delete r.a"),
      njs_str("TypeError: Cannot delete property \"a\" of regexp") },

    { njs_str("var r = new RegExp(''); r.a = 1; Object.freeze(r); r.a = 2"),
      njs_str("TypeError: Cannot assign to read-only property \"a\" of regexp") },

    { njs_str("var r = Object.freeze(new RegExp('')); r.a = 1"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("var o = Object.freeze({ get x() { return 10; } }); o.x"),
      njs_str("10") },

    { njs_str("var o = Object.freeze({ get x() { return 10; } });"
              "Object.getOwnPropertyDescriptors(o).x.get"),
      njs_str("[object Function]") },

    { njs_str("var o = Object.freeze({ get x() { return 10; } });"
              "Object.getOwnPropertyDescriptor(o, 'x').writable"),
      njs_str("undefined") },

    { njs_str("Object.isFrozen({a:1})"),
      njs_str("false") },

    { njs_str("Object.isFrozen([1,2])"),
      njs_str("false") },

    { njs_str("Object.isFrozen(function() {})"),
      njs_str("false") },

    { njs_str("Object.isFrozen(new Date(''))"),
      njs_str("false") },

    { njs_str("Object.isFrozen(new RegExp(''))"),
      njs_str("false") },

    { njs_str("Object.isFrozen()"),
      njs_str("true") },

    { njs_str("Object.isFrozen(1)"),
      njs_str("true") },

    { njs_str("Object.isFrozen('')"),
      njs_str("true") },

    { njs_str("Object.isFrozen(Object.defineProperties({}, {a:{value:1}}))"),
      njs_str("false") },

    { njs_str("var o = Object.defineProperties({}, {a:{}, b:{}});"
                 "o = Object.preventExtensions(o);"
                 "Object.isFrozen(o)"),
      njs_str("true") },

    { njs_str("var o = Object.defineProperties({}, {a:{}, b:{writable:1}});"
                 "o = Object.preventExtensions(o);"
                 "Object.isFrozen(o)"),
      njs_str("false") },

    { njs_str("var o = Object.defineProperties({}, {a:{writable:1}});"
                 "o = Object.preventExtensions(o);"
                 "Object.isFrozen(o)"),
      njs_str("false") },

    { njs_str("var o = Object.defineProperties({}, {a:{configurable:1}});"
                 "o = Object.preventExtensions(o);"
                 "Object.isFrozen(o)"),
      njs_str("false") },

    { njs_str("var o = Object.preventExtensions({a:1});"
                 "Object.isFrozen(o)"),
      njs_str("false") },

    { njs_str("var o = Object.freeze({a:1}); Object.isFrozen(o)"),
      njs_str("true") },

    { njs_str("Object.isFrozen(undefined)"),
      njs_str("true") },

    { njs_str("var o = Object.seal({a:1}); o.a = 2; o.a"),
      njs_str("2") },

    { njs_str("var o = Object.seal({a:1}); delete o.a"),
      njs_str("TypeError: Cannot delete property \"a\" of object") },

    { njs_str("var o = Object.seal({a:1}); o.b = 1; o.b"),
      njs_str("TypeError: Cannot add property \"b\", object is not extensible") },

    { njs_str("var o = Object.seal(Object.create({a:1})); o.a = 2; o.a"),
      njs_str("TypeError: Cannot add property \"a\", object is not extensible") },

    { njs_str("var o = Object.seal({a:{b:1}}); o.a.b = 2; o.a.b"),
      njs_str("2") },

    { njs_str("Object.seal()"),
      njs_str("undefined") },

    { njs_str("Object.seal(1)"),
      njs_str("1") },

    { njs_str("Object.seal('')"),
      njs_str("") },

    { njs_str("Object.seal(undefined)"),
      njs_str("undefined") },

    { njs_str("Object.isSealed({a:1})"),
      njs_str("false") },

    { njs_str("Object.isSealed([1,2])"),
      njs_str("false") },

    { njs_str("Object.isSealed(function() {})"),
      njs_str("false") },

    { njs_str("Object.isSealed(new Date(''))"),
      njs_str("false") },

    { njs_str("Object.isSealed(new RegExp(''))"),
      njs_str("false") },

    { njs_str("Object.isSealed()"),
      njs_str("true") },

    { njs_str("Object.isSealed(1)"),
      njs_str("true") },

    { njs_str("Object.isSealed('')"),
      njs_str("true") },

    { njs_str("Object.isSealed(Object.defineProperties({}, {a:{value:1}}))"),
      njs_str("false") },

    { njs_str("var o = Object.defineProperties({}, {a:{}, b:{}});"
                 "o = Object.preventExtensions(o);"
                 "Object.isSealed(o)"),
      njs_str("true") },

    { njs_str("var o = Object.defineProperties({}, {a:{}, b:{writable:1}});"
                 "o = Object.preventExtensions(o);"
                 "Object.isSealed(o)"),
      njs_str("true") },

    { njs_str("var o = Object.defineProperties({}, {a:{writable:1}});"
                 "o = Object.preventExtensions(o);"
                 "Object.isSealed(o)"),
      njs_str("true") },

    { njs_str("var o = Object.defineProperties({}, {a:{configurable:1}});"
                 "o = Object.preventExtensions(o);"
                 "Object.isSealed(o)"),
      njs_str("false") },

    { njs_str("var o = Object.preventExtensions({a:1});"
                 "Object.isFrozen(o)"),
      njs_str("false") },

    { njs_str("var o = Object.freeze({a:1}); Object.isFrozen(o)"),
      njs_str("true") },

    { njs_str("var o = Object.preventExtensions({a:1});"
                 "Object.defineProperty(o, 'b', {value:1})"),
      njs_str("TypeError: Cannot add property \"b\", object is not extensible") },

    { njs_str("var o = Object.preventExtensions({});"
                 "Object.defineProperty(o, Symbol.unscopables, {})"),
      njs_str("TypeError: Cannot add property \"Symbol(Symbol.unscopables)\", object is not extensible") },

    { njs_str("var o = Object.preventExtensions({a:1});"
                 "Object.defineProperties(o, {b:{value:1}})"),
      njs_str("TypeError: Cannot add property \"b\", object is not extensible") },

    { njs_str("var o = Object.preventExtensions({a:1}); o.a = 2; o.a"),
      njs_str("2") },

    { njs_str("var o = Object.preventExtensions({a:1}); delete o.a; o.a"),
      njs_str("undefined") },

    { njs_str("var o = Object.preventExtensions({a:1}); o.b = 1; o.b"),
      njs_str("TypeError: Cannot add property \"b\", object is not extensible") },

    { njs_str("var o = Object.preventExtensions({a:1}); o[Symbol.unscopables] = 1"),
      njs_str("TypeError: Cannot add property \"Symbol(Symbol.unscopables)\", object is not extensible") },

    { njs_str("Object.preventExtensions()"),
      njs_str("undefined") },

    { njs_str("Object.preventExtensions(1)"),
      njs_str("1") },

    { njs_str("Object.preventExtensions('')"),
      njs_str("") },

    { njs_str("Object.isExtensible({})"),
      njs_str("true") },

    { njs_str("Object.isExtensible([])"),
      njs_str("true") },

    { njs_str("var arrObj = [];Object.preventExtensions(arrObj); arrObj[1] = 1"),
      njs_str("TypeError: Cannot add property \"1\", object is not extensible") },

    { njs_str("var arrObj = [1,2];Object.preventExtensions(arrObj); arrObj[1] = 1"),
      njs_str("1") },

    { njs_str("Object.isExtensible(function() {})"),
      njs_str("true") },

    { njs_str("Object.isExtensible(new Date(''))"),
      njs_str("true") },

    { njs_str("Object.isExtensible(new RegExp(''))"),
      njs_str("true") },

    { njs_str("Object.isExtensible()"),
      njs_str("false") },

    { njs_str("Object.isExtensible(1)"),
      njs_str("false") },

    { njs_str("Object.isExtensible('')"),
      njs_str("false") },

    { njs_str("Object.isExtensible(Object.preventExtensions({}))"),
      njs_str("false") },

    { njs_str("Object.isExtensible(Object.preventExtensions([]))"),
      njs_str("false") },

    { njs_str("Object.isExtensible(Object.freeze({}))"),
      njs_str("false") },

    { njs_str("Object.isExtensible(Object.freeze([]))"),
      njs_str("false") },

    { njs_str("Object.isExtensible(undefined)"),
      njs_str("false") },

    /* Object.is() */

    { njs_str("typeof Object.is"),
      njs_str("function") },

    { njs_str("Object.is.length == 2"),
      njs_str("true") },

    { njs_str("Object.is()"),
      njs_str("true") },

    { njs_str("[undefined, null, false, NaN, '', Symbol(), {}]"
              ".map((x) => Object.is(x, x))"
              ".every((x) => x === true)"),
      njs_str("true") },

    { njs_str("[null, false, NaN, '', Symbol(), {}]"
              ".map((x) => Object.is(x) || Object.is(void 0, x))"
              ".every((x) => x === false)"),
      njs_str("true") },

    { njs_str("[false, NaN, '', Symbol()]"
              ".map((x) => Object.is(Object(x), Object(x)))"
              ".every((x) => x === false)"),
      njs_str("true") },

    { njs_str("Object.is(0, -0)"),
      njs_str("false") },

    { njs_str("Object.is(0, null)"),
      njs_str("false") },

    { njs_str("Object.is(42, '42')"),
      njs_str("false") },

    { njs_str(
        "var fail;"
        "function isConfigurableMethods(o) {"
        "    var except = ["
        "        'prototype',"
        "        'caller',"
        "        'arguments',"
        "        'description',"
        "    ];"
        "    return Object.getOwnPropertyNames(o)"
        "                 .filter(v => !except.includes(v)"
        "                              && typeof o[v] == 'function')"
        "                 .every(v => Object.getOwnPropertyDescriptor(o, v)"
        "                                   .configurable"
        "                             || !(fail = `${o.name}.${v}`));"
        "}"
        "["
        "    Boolean, Boolean.prototype,"
        "    Number, Number.prototype,"
        "    Symbol, Symbol.prototype,"
        "    String, String.prototype,"
        "    Object, Object.prototype,"
        "    Array, Array.prototype,"
        "    Function, Function.prototype,"
        "    RegExp, RegExp.prototype,"
        "    Date, Date.prototype,"
        "    Error, Error.prototype,"
        "    Math,"
        "    JSON,"
        "].every(obj => isConfigurableMethods(obj)) || fail"),

      njs_str("true") },

    { njs_str(
        "var fail;"
        "function isWritableMethods(o) {"
        "    var except = ["
        "        'prototype',"
        "        'caller',"
        "        'arguments',"
        "        'description',"
        "    ];"
        "    return Object.getOwnPropertyNames(o)"
        "                 .filter(v => !except.includes(v)"
        "                              && typeof o[v] == 'function')"
        "                 .every(v => Object.getOwnPropertyDescriptor(o, v)"
        "                                   .writable"
        "                             || !(fail = `${o.name}.${v}`));"
        "}"
        "["
        "    Boolean, Boolean.prototype,"
        "    Number, Number.prototype,"
        "    Symbol, Symbol.prototype,"
        "    String, String.prototype,"
        "    Object, Object.prototype,"
        "    Array, Array.prototype,"
        "    Function, Function.prototype,"
        "    RegExp, RegExp.prototype,"
        "    Date, Date.prototype,"
        "    Error, Error.prototype,"
        "    Math,"
        "    JSON,"
        "].every(obj => isWritableMethods(obj)) || fail"),

      njs_str("true") },

    { njs_str("new Date(undefined)"),
      njs_str("Invalid Date") },

    { njs_str("new Date(Infinity)"),
      njs_str("Invalid Date") },

    { njs_str("new Date(NaN)"),
      njs_str("Invalid Date") },

    { njs_str("new Date(8.65e15)"),
      njs_str("Invalid Date") },

    { njs_str("new Date(0e0.o0)"),
      njs_str("Invalid Date") },

    { njs_str("(new Date(8.639e15)).getTime()"),
      njs_str("8639000000000000") },

    { njs_str("new Date(8.641e15)"),
      njs_str("Invalid Date") },

    { njs_str("(new Date(null)).getTime()"),
      njs_str("0") },

    { njs_str("(new Date(86400)).getTime()"),
      njs_str("86400") },

    { njs_str("Date().split(' ')[0] in {'Mon':1, 'Tue':1, 'Wed':1, 'Thu':1, 'Fri':1, 'Sat':1, 'Sun':1}"),
      njs_str("true") },

    { njs_str("var d = new Date(''); d +' '+ d.getTime()"),
      njs_str("Invalid Date NaN") },

    { njs_str("var d = new Date(1); d = d + ''; d.slice(0, 33)"),
      njs_str("Thu Jan 01 1970 00:00:00 GMT+0000") },

    { njs_str("var d = new Date({valueOf:()=>86400000}); d = d + ''; d.slice(0, 33)"),
      njs_str("Fri Jan 02 1970 00:00:00 GMT+0000") },

    { njs_str("(new Date({toString:()=>'2011'})).getTime()"),
      njs_str("1293840000000") },

    { njs_str("(new Date({valueOf: ()=>86400, toString:()=>'2011'})).getTime()"),
      njs_str("86400") },

    { njs_str("var d = new Date(1308895200000); d.getTime()"),
      njs_str("1308895200000") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getTime()"),
      njs_str("1308941100000") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.valueOf()"),
      njs_str("1308941100000") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45);"
                 "d.toString().slice(0, 33)"),
      njs_str("Fri Jun 24 2011 18:45:00 GMT+0000") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.toDateString()"),
      njs_str("Fri Jun 24 2011") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45);"
                 "d.toTimeString().slice(0, 17)"),
      njs_str("18:45:00 GMT+0000") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.toUTCString()"),
      njs_str("Fri, 24 Jun 2011 18:45:00 GMT") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                 "d.toISOString()"),
      njs_str("2011-06-24T18:45:12.625Z") },

    { njs_str("var d = new Date(1999, 9, 10, 10, 10, 10, 10);"
              "var local = new Date(d.getTime() - d.getTimezoneOffset() * 60000);"
              "local.toISOString()"),
      njs_str("1999-10-10T10:10:10.010Z") },

#if (NJS_TIME_T_SIZE == 8)
    { njs_str("["
              "'-010000-01-01T00:00:00.000Z',"
              "'+010000-01-01T00:00:00.000Z',"
              "'0002-01-01T00:00:00.000Z',"
              "'0123-01-01T00:00:00.000Z',"
              "].every((iso)=> (new Date(iso)).toISOString() === iso)"),
      njs_str("true") },

    { njs_str("new Date('0020-01-01T00:00:00Z').toUTCString()"),
      njs_str("Wed, 01 Jan 0020 00:00:00 GMT") },

    { njs_str("new Date('0020-01-01T00:00:00Z').toString().slice(0, 15)"),
      njs_str("Wed Jan 01 0020") },

    { njs_str("(new Date('-000001-07-01T00:00Z')).toUTCString()"),
      njs_str("Thu, 01 Jul -0001 00:00:00 GMT") },

    { njs_str("(new Date('-000012-07-01T00:00Z')).toUTCString()"),
      njs_str("Fri, 01 Jul -0012 00:00:00 GMT") },

    { njs_str("(new Date('-000123-07-01T00:00Z')).toUTCString()"),
      njs_str("Sun, 01 Jul -0123 00:00:00 GMT") },

    { njs_str("(new Date('-001234-07-01T00:00Z')).toUTCString()"),
      njs_str("Fri, 01 Jul -1234 00:00:00 GMT") },

    { njs_str("(new Date('-012345-07-01T00:00Z')).toUTCString()"),
      njs_str("Thu, 01 Jul -12345 00:00:00 GMT") },

    { njs_str("var d = new Date(-62167219200000); d.toISOString()"),
      njs_str("0000-01-01T00:00:00.000Z") },

    { njs_str("var d = new Date(-62135596800000); d.toISOString()"),
      njs_str("0001-01-01T00:00:00.000Z") },

    { njs_str("var d = new Date(-62198755200000); d.toISOString()"),
      njs_str("-000001-01-01T00:00:00.000Z") },
#endif

    { njs_str("Date.UTC(2011, 5, 24, 6, 0)"),
      njs_str("1308895200000") },

    { njs_str("Date.UTC({valueOf:()=>2011}, 5, 24, 6, 0)"),
      njs_str("1308895200000") },

    { njs_str("Date.UTC()"),
      njs_str("NaN") },

    { njs_str("Date.UTC(Infinity)"),
      njs_str("NaN") },

    { njs_str("Date.UTC(Infinity, 0)"),
      njs_str("NaN") },

    { njs_str("Date.UTC(1970)"),
      njs_str("0") },

    { njs_str("Date.UTC(1968, 24)"),
      njs_str("0") },

    { njs_str("[-1,0,1,99,100].map(yr => Date.UTC(yr))"),
      njs_str("-62198755200000,-2208988800000,-2177452800000,915148800000,-59011459200000") },

    { njs_str("Date.UTC(1970.9, 0.9, 1.9, 0.9, 0.9, 0.9, 0.9)"),
      njs_str("0") },

    { njs_str("Date.UTC(-1970.9, -0.9, -0.9, -0.9, -0.9, -0.9, -0.9)"),
      njs_str("-124334438400000") },

    { njs_str("Date.UTC(275760, 8, 13, 0, 0, 0, 0)"),
      njs_str("8640000000000000") },

    { njs_str("Date.UTC(275760, 8, 13, 0, 0, 0, 1)"),
      njs_str("NaN") },

    { njs_str("Date.UTC(-271821, 3, 20, 0, 0, 0, 0)"),
      njs_str("-8640000000000000") },

    { njs_str("Date.UTC(-271821, 3, 20, 0, 0, 0, -1)"),
      njs_str("NaN") },

    { njs_str("Date.UTC(1970, 0)"),
      njs_str("0") },

    { njs_str("Date.UTC(1970, 0, 0)"),
      njs_str("-86400000") },

    { njs_str("Date.parse()"),
      njs_str("NaN") },

    { njs_str("Date.parse('2011')"),
      njs_str("1293840000000") },

    { njs_str("Date.parse('+002011')"),
      njs_str("1293840000000") },

    { njs_str("Date.parse('2011-06')"),
      njs_str("1306886400000") },

    { njs_str("Date.parse('2011-06-24')"),
      njs_str("1308873600000") },

    { njs_str("Date.parse('2011-06-24T06')"),
      njs_str("NaN") },

    { njs_str("Date.parse('2011-06-24T06:')"),
      njs_str("NaN") },

    { njs_str("Date.parse('2011-06-24T06:01:')"),
      njs_str("NaN") },

    { njs_str("Date.parse('2011-06-24T06:01Z')"),
      njs_str("1308895260000") },

    { njs_str("Date.parse('2011-06-24T06:01:02:')"),
      njs_str("NaN") },

    { njs_str("Date.parse('2011-06-24T06:01:02Z')"),
      njs_str("1308895262000") },

    { njs_str("Date.parse('2011-06-24T06:01:02.Z')"),
      njs_str("NaN") },

    { njs_str("Date.parse('2011-06-24T06:01:02.6Z')"),
      njs_str("1308895262600") },

    { njs_str("Date.parse('2011-06-24T06:01:02.62Z')"),
      njs_str("1308895262620") },

    { njs_str("Date.parse('2011-06-24T06:01:02:625Z')"),
      njs_str("NaN") },

    { njs_str("Date.parse('2011-06-24T06:01:02.625Z')"),
      njs_str("1308895262625") },

    { njs_str("Date.parse('2011-06-24T06:01:02.6255555Z')"),
      njs_str("1308895262625") },

    { njs_str("Date.parse('2011-06-24T06:01:02.625555Z5')"),
      njs_str("NaN") },

    { njs_str("var d = new Date(); var str = d.toISOString();"
                 "var diff = Date.parse(str) - Date.parse(str.substring(0, str.length - 1));"
                 "d.getTimezoneOffset() == -diff/1000/60"),
      njs_str("true") },

    { njs_str("Date.parse('24 Jun 2011')"),
      njs_str("1308873600000") },

    { njs_str("Date.parse('Fri, 24 Jun 2011 18:48')"),
      njs_str("1308941280000") },

    { njs_str("Date.parse('Fri, 24 Jun 2011 18:48:02')"),
      njs_str("1308941282000") },

    { njs_str("Date.parse('Fri, 24 Jun 2011 18:48:02 GMT')"),
      njs_str("1308941282000") },

    { njs_str("Date.parse('Fri, 24 Jun 2011 18:48:02 +1245')"),
      njs_str("1308895382000") },

    { njs_str("Date.parse('Jun 24 2011')"),
      njs_str("1308873600000") },

    { njs_str("Date.parse('Fri Jun 24 2011 18:48')"),
      njs_str("1308941280000") },

    { njs_str("Date.parse('Fri Jun 24 2011 18:48:02')"),
      njs_str("1308941282000") },

    { njs_str("Date.parse('Fri Jun 24 2011 18:48:02 GMT+1245')"),
      njs_str("1308895382000") },

    /* Jan 1, 1. */
    { njs_str("Date.parse('+000001-01-01T00:00:00.000Z')"),
      njs_str("-62135596800000") },

    /* Mar 2, 1 BCE. */
    { njs_str("Date.parse('+000000-03-02T00:00:00.000Z')"),
      njs_str("-62161948800000") },

    /* Mar 1, 1 BCE. */
    { njs_str("Date.parse('+000000-03-01T00:00:00.000Z')"),
      njs_str("-62162035200000") },

    /* Feb 29, 1 BCE. */
    { njs_str("Date.parse('+000000-02-29T00:00:00.000Z')"),
      njs_str("-62162121600000") },

    /* Feb 28, 1 BCE. */
    { njs_str("Date.parse('+000000-02-28T00:00:00.000Z')"),
      njs_str("-62162208000000") },

    /* Jan 1, 1 BCE. */
    { njs_str("Date.parse('+000000-01-01T00:00:00.000Z')"),
      njs_str("-62167219200000") },

    /* Jan 1, 2 BCE. */
    { njs_str("Date.parse('-000001-01-01T00:00:00.000Z')"),
      njs_str("-62198755200000") },

    { njs_str("var d = new Date(); d == Date.parse(d.toISOString())"),
      njs_str("true") },

    { njs_str("var s = Date(); s === Date(Date.parse(s))"),
      njs_str("true") },

    { njs_str("var n = Date.now(); n == new Date(n)"),
      njs_str("true") },

    { njs_str("var d = new Date(2011,0); d.getFullYear()"),
      njs_str("2011") },

    { njs_str("var d = new Date(2011, 0, 1, 0, 0, 0, -1); d.getFullYear()"),
      njs_str("2010") },

    { njs_str("var d = new Date(2011, 11, 31, 23, 59, 59, 999); d.getFullYear()"),
      njs_str("2011") },

    { njs_str("var d = new Date(2011, 11, 31, 23, 59, 59, 1000); d.getFullYear()"),
      njs_str("2012") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getFullYear()"),
      njs_str("2011") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getUTCFullYear()"),
      njs_str("2011") },

    { njs_str("var d = new Date(2011, 5); d.getMonth()"),
      njs_str("5") },

    { njs_str("var d = new Date(2011, 6, 0, 0, 0, 0, -1); d.getMonth()"),
      njs_str("5") },

    { njs_str("var d = new Date(2011, 6, 31, 23, 59, 59, 999); d.getMonth()"),
      njs_str("6") },

    { njs_str("var d = new Date(2011, 6, 31, 23, 59, 59, 1000); d.getMonth()"),
      njs_str("7") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getMonth()"),
      njs_str("5") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getUTCMonth()"),
      njs_str("5") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getDate()"),
      njs_str("24") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getUTCDate()"),
      njs_str("24") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getDay()"),
      njs_str("5") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getUTCDay()"),
      njs_str("5") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getHours()"),
      njs_str("18") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getUTCHours()"),
      njs_str("18") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getMinutes()"),
      njs_str("45") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getUTCMinutes()"),
      njs_str("45") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12);"
                 "d.getSeconds()"),
      njs_str("12") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12);"
                 "d.getUTCSeconds()"),
      njs_str("12") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                 "d.getMilliseconds()"),
      njs_str("625") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                 "d.getUTCMilliseconds()"),
      njs_str("625") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                 "d.getTimezoneOffset()"),
      njs_str("0") },

    { njs_str("var d = new Date(); d.setTime(1308895200000); d.getTime()"),
      njs_str("1308895200000") },

    { njs_str("var d = new Date(); d.setTime(); d.getTime()"),
      njs_str("NaN") },

    { njs_str("var d = new Date(); d.setTime(Infinity); d.getTime()"),
      njs_str("NaN") },

    { njs_str("var d = new Date(); d.setTime(8.64e15 +1); d.getTime()"),
      njs_str("NaN") },

    { njs_str("var d = new Date(NaN); d.setTime(0); d.getTime()"),
      njs_str("0") },

    { njs_str("var d = new Date(); d.setTime(8.64e15); d.getTime()"),
      njs_str("8640000000000000") },

    { njs_str("var d = new Date(1308895201625); d.setMilliseconds(5003);"
                 "d.getTime()"),
      njs_str("1308895206003") },

    { njs_str("var d = new Date(1308895201625); d.setSeconds(2, 5003);"
                 "d.getTime()"),
      njs_str("1308895207003") },

    { njs_str("var d = new Date(1308895201625); d.setSeconds(2);"
                 "d.getTime()"),
      njs_str("1308895202625") },

    { njs_str("var d = new Date(1308895323625); d.setMinutes(3, 2, 5003);"
                 "d.getTime()"),
      njs_str("1308895387003") },

    { njs_str("var d = new Date(1308895323625); d.setMinutes(3, 2);"
                 "d.getTime()"),
      njs_str("1308895382625") },

    { njs_str("var d = new Date(1308895323625); d.setMinutes(3);"
                 "d.getTime()"),
      njs_str("1308895383625") },

    { njs_str("var d = new Date(1308895323625); d.setUTCMinutes(3, 2, 5003);"
                 "d.getTime()"),
      njs_str("1308895387003") },

    { njs_str("var d = new Date(1308895323625); d.setUTCMinutes(3, 2, 5003, 111111);"
                 "d.getTime()"),
      njs_str("1308895387003") },

    { njs_str("var d = new Date(1308895323625); d.setUTCMinutes(3, 2);"
                 "d.getTime()"),
      njs_str("1308895382625") },

    { njs_str("var d = new Date(1308895323625); d.setUTCMinutes(3);"
                 "d.getTime()"),
      njs_str("1308895383625") },

    { njs_str("var d = new Date(1308895323625); d.setHours(20, 3, 2, 5003);"
                 "d.getTime()"),
      njs_str("1308945787003") },

    { njs_str("var d = new Date(1308895323625); d.setHours(20, 3, 2);"
                 "d.getTime()"),
      njs_str("1308945782625") },

    { njs_str("var d = new Date(1308895323625); d.setHours(20, 3);"
                 "d.getTime()"),
      njs_str("1308945783625") },

    { njs_str("var d = new Date(1308895323625); d.setHours(20);"
                 "d.getTime()"),
      njs_str("1308945723625") },

    { njs_str("var d = new Date(1308895323625);"
                 "d.setUTCHours(20, 3, 2, 5003); d.getTime()"),
      njs_str("1308945787003") },

    { njs_str("var d = new Date(1308895323625); d.setUTCHours(20, 3, 2);"
                 "d.getTime()"),
      njs_str("1308945782625") },

    { njs_str("var d = new Date(1308895323625); d.setUTCHours(20, 3);"
                 "d.getTime()"),
      njs_str("1308945783625") },

    { njs_str("var d = new Date(1308895323625); d.setUTCHours(20);"
                 "d.getTime()"),
      njs_str("1308945723625") },

    { njs_str("var d = new Date(1308895323625); d.setDate(10);"
                 "d.getTime()"),
      njs_str("1307685723625") },

    { njs_str("var d = new Date(1308895323625); d.setUTCDate(10);"
                 "d.getTime()"),
      njs_str("1307685723625") },

    { njs_str("var d = new Date(1308895323625); d.setMonth(2, 10);"
                 "d.getTime()"),
      njs_str("1299736923625") },

    { njs_str("var d = new Date(1308895323625); d.setUTCMonth(2, 10);"
                 "d.getTime()"),
      njs_str("1299736923625") },

    { njs_str("var d = new Date(1308895323625); d.setMonth(2);"
                 "d.getTime()"),
      njs_str("1300946523625") },

    { njs_str("var d = new Date(1308895323625); d.setMonth(2, undefined);"
                 "d.getTime()"),
      njs_str("NaN") },

    { njs_str("var d = new Date(1308895323625); d.setMonth(2, undefined)"),
      njs_str("NaN") },

    { njs_str("var d = new Date(1308895323625); d.setUTCMonth(2);"
                 "d.getTime()"),
      njs_str("1300946523625") },

    { njs_str("var d = new Date(1308895323625); d.setFullYear(2010, 2, 10);"
                 "d.getTime()"),
      njs_str("1268200923625") },

    { njs_str("var d = new Date(NaN); d.setFullYear(2010);"
                 "d.getTime() === (new Date(2010,0)).getTime()"),
      njs_str("true") },

    { njs_str("var d = new Date(1308895323625);"
                 "d.setUTCFullYear(2010, 2, 10); d.getTime()"),
      njs_str("1268200923625") },

    { njs_str("var d = new Date(1308895323625); d.setFullYear(2010, 2);"
                 "d.getTime()"),
      njs_str("1269410523625") },

    { njs_str("var d = new Date(1308895323625); d.setUTCFullYear(2010, 2);"
                 "d.getTime()"),
      njs_str("1269410523625") },

    { njs_str("var d = new Date(1308895323625); d.setFullYear(2010);"
                 "d.getTime()"),
      njs_str("1277359323625") },

    { njs_str("var date = new Date(2016, 6, 7, 11, 36, 23, 2); "
              "date.setFullYear(null) === new Date(-1, 18, 7, 11, 36, 23, 2).getTime()"),
      njs_str("true") },

    { njs_str("var d = new Date(1308895323625); d.setUTCFullYear(2010);"
                 "d.getTime()"),
      njs_str("1277359323625") },

    { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                 "d.toJSON(1)"),
      njs_str("2011-06-24T18:45:12.625Z") },

    { njs_str("var o = { toISOString: function() { return 'OK' } };"
                 "Date.prototype.toJSON.call(o, 1)"),
      njs_str("OK") },

    { njs_str("var d = new Date(); d.__proto__ === Date.prototype"),
      njs_str("true") },

    { njs_str("new Date(NaN)"),
      njs_str("Invalid Date") },

#ifndef NJS_SUNC
    { njs_str("new Date(-0).getTime()"),
      njs_str("0") },
#endif

    { njs_str("new Date(6.54321).valueOf()"),
      njs_str("6") },

    { njs_str("[0].map(new Date().getDate)"),
      njs_str("TypeError: cannot convert undefined to date") },

    { njs_str("new Date(eval)"),
      njs_str("Invalid Date") },

    { njs_str("Date.UTC(eval)"),
      njs_str("NaN") },

    { njs_str("Date.name"),
      njs_str("Date") },

    { njs_str("Date.length"),
      njs_str("7") },

    { njs_str("Date.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("Date.prototype.constructor === Date"),
      njs_str("true") },

    { njs_str("Date.prototype.hasOwnProperty('constructor')"),
      njs_str("true") },

    { njs_str("Date.prototype.__proto__ === Object.prototype"),
      njs_str("true") },

    { njs_str("njs.dump(Date.prototype)"),
      njs_str("{}") },

    { njs_str("Date.prototype.valueOf()"),
      njs_str("TypeError: cannot convert object to date") },

    { njs_str("Date.constructor === Function"),
      njs_str("true") },

    /* eval(). */

    { njs_str("eval.name"),
      njs_str("eval") },

    { njs_str("eval.length"),
      njs_str("1") },

    { njs_str("eval.prototype"),
      njs_str("undefined") },

    { njs_str("eval.__proto__ === Function.prototype"),
      njs_str("true") },

    { njs_str("eval.constructor === Function"),
      njs_str("true") },

    { njs_str("eval()"),
      njs_str("InternalError: Not implemented") },

    { njs_str("delete this.eval; eval"),
      njs_str("ReferenceError: \"eval\" is not defined in 1") },

    { njs_str("var d = Object.getOwnPropertyDescriptor(this, 'eval');"
              "d.writable && !d.enumerable && d.configurable"),
      njs_str("true") },

    /* Math. */

    { njs_str("Math.PI"),
      njs_str("3.141592653589793") },

    { njs_str("Math.abs()"),
      njs_str("NaN") },

    { njs_str("Math.abs(5)"),
      njs_str("5") },

    { njs_str("Math.abs(-5)"),
      njs_str("5") },

    { njs_str("Math.abs('5.0')"),
      njs_str("5") },

    { njs_str("Math.abs('abc')"),
      njs_str("NaN") },

    { njs_str("Math.acos()"),
      njs_str("NaN") },

    { njs_str("Math.acos(NaN)"),
      njs_str("NaN") },

    { njs_str("Math.acos('abc')"),
      njs_str("NaN") },

    { njs_str("Math.acos(1.1)"),
      njs_str("NaN") },

    { njs_str("Math.acos(-1.1)"),
      njs_str("NaN") },

    { njs_str("Math.acos('1')"),
      njs_str("0") },

    { njs_str("Math.acos(0) - Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.acosh()"),
      njs_str("NaN") },

    { njs_str("Math.acosh('abc')"),
      njs_str("NaN") },

    { njs_str("Math.acosh(0.9)"),
      njs_str("NaN") },

    { njs_str("Math.acosh(1)"),
      njs_str("0") },

    { njs_str("Math.acosh('Infinity')"),
      njs_str("Infinity") },

    /*
     * The difference is Number.EPSILON on Linux/i686
     * and zero on other platforms.
     */
    { njs_str("Math.abs(Math.acosh((1/Math.E + Math.E)/2) - 1)"
                 " <= Number.EPSILON"),
      njs_str("true") },

    { njs_str("Math.asin()"),
      njs_str("NaN") },

    { njs_str("Math.asin(NaN)"),
      njs_str("NaN") },

    { njs_str("Math.asin('abc')"),
      njs_str("NaN") },

    { njs_str("Math.asin(1.1)"),
      njs_str("NaN") },

    { njs_str("Math.asin(-1.1)"),
      njs_str("NaN") },

    { njs_str("Math.asin(0)"),
      njs_str("0") },

    { njs_str("Math.asin('-0')"),
      njs_str("-0") },

    { njs_str("Math.asin(1) - Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.asinh()"),
      njs_str("NaN") },

    { njs_str("Math.asinh('abc')"),
      njs_str("NaN") },

    { njs_str("Math.asinh(0)"),
      njs_str("0") },

    { njs_str("Math.asinh('-0')"),
      njs_str("-0") },

    { njs_str("Math.asinh(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.asinh(-Infinity)"),
      njs_str("-Infinity") },

    { njs_str("Math.asinh((Math.E - 1/Math.E)/2)"),
      njs_str("1") },

    { njs_str("Math.atan()"),
      njs_str("NaN") },

    { njs_str("Math.atan(NaN)"),
      njs_str("NaN") },

    { njs_str("Math.atan('abc')"),
      njs_str("NaN") },

    { njs_str("Math.atan('Infinity') - Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.atan(-Infinity) + Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.atan(0)"),
      njs_str("0") },

    { njs_str("Math.atan('-0')"),
      njs_str("-0") },

    { njs_str("Math.atan(1) - Math.PI/4"),
      njs_str("0") },

    { njs_str("Math.atan2()"),
      njs_str("NaN") },

    { njs_str("Math.atan2(1)"),
      njs_str("NaN") },

    { njs_str("Math.atan2('abc', 1)"),
      njs_str("NaN") },

    { njs_str("Math.atan2(1, 0) - Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.atan2('1', -0) - Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.atan2(0, '1')"),
      njs_str("0") },

    { njs_str("Math.atan2(0, 0)"),
      njs_str("0") },

    { njs_str("Math.atan2(0, -0) - Math.PI"),
      njs_str("0") },

    { njs_str("Math.atan2('0', -1) - Math.PI"),
      njs_str("0") },

    { njs_str("Math.atan2(-0, '0.1')"),
      njs_str("-0") },

    { njs_str("Math.atan2(-0, 0)"),
      njs_str("-0") },

    { njs_str("Math.atan2(-0, -0) + Math.PI"),
      njs_str("0") },

    { njs_str("Math.atan2('-0', '-1') + Math.PI"),
      njs_str("0") },

    { njs_str("Math.atan2(-0.1, 0) + Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.atan2(-1, -0) + Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.atan2(1, 'Infinity')"),
      njs_str("0") },

    { njs_str("Math.atan2(0.1, -Infinity) - Math.PI"),
      njs_str("0") },

    { njs_str("Math.atan2(-1, Infinity)"),
      njs_str("-0") },

    { njs_str("Math.atan2('-0.1', -Infinity) + Math.PI"),
      njs_str("0") },

    { njs_str("Math.atan2(Infinity, -5) - Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.atan2(-Infinity, 5) + Math.PI/2"),
      njs_str("0") },

    { njs_str("Math.atan2('Infinity', 'Infinity') - Math.PI/4"),
      njs_str("0") },

    { njs_str("Math.atan2(Infinity, -Infinity) - 3*Math.PI/4"),
      njs_str("0") },

    { njs_str("Math.atan2(-Infinity, 'Infinity') + Math.PI/4"),
      njs_str("0") },

    { njs_str("Math.atan2('-Infinity', -Infinity) + 3*Math.PI/4"),
      njs_str("0") },

    { njs_str("Math.atan2(1, 1) - Math.atan2(-5, -5) - Math.PI"),
      njs_str("0") },

    { njs_str("Math.atanh()"),
      njs_str("NaN") },

    { njs_str("Math.atanh('abc')"),
      njs_str("NaN") },

    { njs_str("Math.atanh(-1.1)"),
      njs_str("NaN") },

    { njs_str("Math.atanh(1.1)"),
      njs_str("NaN") },

    { njs_str("Math.atanh(1)"),
      njs_str("Infinity") },

    { njs_str("Math.atanh('-1')"),
      njs_str("-Infinity") },

    { njs_str("Math.atanh(0)"),
      njs_str("0") },

    { njs_str("Math.atanh(-0)"),
      njs_str("-0") },

    /*
     * The difference is Number.EPSILON on Linux/i686
     * and zero on other platforms.
     */
    { njs_str("Math.abs(1 - Math.atanh((Math.E - 1)/(Math.E + 1)) * 2)"
                 " <= Number.EPSILON"),
      njs_str("true") },

    { njs_str("Math.cbrt()"),
      njs_str("NaN") },

    { njs_str("Math.cbrt('abc')"),
      njs_str("NaN") },

    { njs_str("Math.cbrt(0)"),
      njs_str("0") },

    { njs_str("Math.cbrt('-0')"),
      njs_str("-0") },

    { njs_str("Math.cbrt(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.cbrt(-Infinity)"),
      njs_str("-Infinity") },

    { njs_str("(Math.cbrt('27') - 3) < 1e-15"),
      njs_str("true") },

    { njs_str("Math.cbrt(-1)"),
      njs_str("-1") },

    { njs_str("Math.ceil()"),
      njs_str("NaN") },

    { njs_str("Math.ceil('abc')"),
      njs_str("NaN") },

    { njs_str("Math.ceil(0)"),
      njs_str("0") },

    { njs_str("Math.ceil('-0')"),
      njs_str("-0") },

    { njs_str("Math.ceil('Infinity')"),
      njs_str("Infinity") },

    { njs_str("Math.ceil(-Infinity)"),
      njs_str("-Infinity") },

    { njs_str("Math.ceil(-0.9)"),
      njs_str("-0") },

    { njs_str("Math.ceil(3.1)"),
      njs_str("4") },

    { njs_str("Math.clz32()"),
      njs_str("32") },

    { njs_str("Math.clz32('abc')"),
      njs_str("32") },

    { njs_str("Math.clz32(NaN)"),
      njs_str("32") },

    { njs_str("Math.clz32(Infinity)"),
      njs_str("32") },

    { njs_str("Math.clz32('1')"),
      njs_str("31") },

    { njs_str("Math.clz32(0)"),
      njs_str("32") },

    { njs_str("Math.clz32('65535')"),
      njs_str("16") },

    { njs_str("Math.clz32(-1)"),
      njs_str("0") },

    { njs_str("Math.clz32(4294967298)"),
      njs_str("30") },

    { njs_str("Math.cos()"),
      njs_str("NaN") },

    { njs_str("Math.cos('abc')"),
      njs_str("NaN") },

    { njs_str("Math.cos('0')"),
      njs_str("1") },

    { njs_str("Math.cos(-0)"),
      njs_str("1") },

    { njs_str("Math.cos(Infinity)"),
      njs_str("NaN") },

    { njs_str("Math.cos(-Infinity)"),
      njs_str("NaN") },

    { njs_str("Math.cos(Math.PI*2)"),
      njs_str("1") },

    { njs_str("Math.cosh()"),
      njs_str("NaN") },

    { njs_str("Math.cosh('abc')"),
      njs_str("NaN") },

    { njs_str("Math.cosh('0')"),
      njs_str("1") },

    { njs_str("Math.cosh(-0)"),
      njs_str("1") },

    { njs_str("Math.cosh(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.cosh(-Infinity)"),
      njs_str("Infinity") },

    /*
     * The difference is Number.EPSILON on FreeBSD
     * and zero on other platforms.
     */
    { njs_str("Math.abs(Math.cosh(1) - (1/Math.E + Math.E)/2)"
                 " <= Number.EPSILON"),
      njs_str("true") },

    { njs_str("Math.exp()"),
      njs_str("NaN") },

    { njs_str("Math.exp('abc')"),
      njs_str("NaN") },

    { njs_str("Math.exp('0')"),
      njs_str("1") },

    { njs_str("Math.exp(-0)"),
      njs_str("1") },

    { njs_str("Math.exp(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.exp(-Infinity)"),
      njs_str("0") },

    /*
     * The difference is 2 * Number.EPSILON on FreeBSD
     * and zero on other platforms.
     */
    { njs_str("Math.abs(Math.exp(1) - Math.E) <= 2 * Number.EPSILON"),
      njs_str("true") },

    { njs_str("Math.expm1()"),
      njs_str("NaN") },

    { njs_str("Math.expm1('abc')"),
      njs_str("NaN") },

    { njs_str("Math.expm1('0')"),
      njs_str("0") },

    { njs_str("Math.expm1(-0)"),
      njs_str("-0") },

    { njs_str("Math.expm1(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.expm1(-Infinity)"),
      njs_str("-1") },

    /*
     * The difference is 2 * Number.EPSILON on FreeBSD, Solaris,
     * and MacOSX and zero on other platforms.
     */
    { njs_str("Math.abs(1 + Math.expm1(1) - Math.E) <= 2 * Number.EPSILON"),
      njs_str("true") },

    { njs_str("Math.floor()"),
      njs_str("NaN") },

    { njs_str("Math.floor('abc')"),
      njs_str("NaN") },

    { njs_str("Math.floor(0)"),
      njs_str("0") },

    { njs_str("Math.floor('-0')"),
      njs_str("-0") },

    { njs_str("Math.floor('Infinity')"),
      njs_str("Infinity") },

    { njs_str("Math.floor(-Infinity)"),
      njs_str("-Infinity") },

    { njs_str("Math.floor(0.9)"),
      njs_str("0") },

    { njs_str("Math.floor(-3.1)"),
      njs_str("-4") },

    { njs_str("Math.fround()"),
      njs_str("NaN") },

    { njs_str("Math.fround('abc')"),
      njs_str("NaN") },

    { njs_str("Math.fround(0)"),
      njs_str("0") },

    { njs_str("Math.fround('-0')"),
      njs_str("-0") },

    { njs_str("Math.fround('Infinity')"),
      njs_str("Infinity") },

    { njs_str("Math.fround(-Infinity)"),
      njs_str("-Infinity") },

    { njs_str("Math.fround('-1.5')"),
      njs_str("-1.5") },

    { njs_str("Math.fround(16777216)"),
      njs_str("16777216") },

    { njs_str("Math.fround(-16777217)"),
      njs_str("-16777216") },

    { njs_str("Math.hypot()"),
      njs_str("0") },

    { njs_str("Math.hypot(1, 2, 'abc')"),
      njs_str("NaN") },

    { njs_str("Math.hypot(1, NaN, 3)"),
      njs_str("NaN") },

    { njs_str("Math.hypot(1, NaN, -Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.hypot(-42)"),
      njs_str("42") },

    { njs_str("Math.hypot(8, -15)"),
      njs_str("17") },

    { njs_str("Math.hypot(3, -4, 12.0, '84', 132)"),
      njs_str("157") },

    { njs_str("Math.imul()"),
      njs_str("0") },

    { njs_str("Math.imul(1)"),
      njs_str("0") },

    { njs_str("Math.imul('a', 1)"),
      njs_str("0") },

    { njs_str("Math.imul(1, NaN)"),
      njs_str("0") },

    { njs_str("Math.imul(2, '3')"),
      njs_str("6") },

    { njs_str("Math.imul('3.9', -2.1)"),
      njs_str("-6") },

    { njs_str("Math.imul(2, 2147483647)"),
      njs_str("-2") },

    { njs_str("Math.imul(Number.MAX_SAFE_INTEGER, 2)"),
      njs_str("-2") },

    { njs_str("Math.imul(1, Number.MAX_SAFE_INTEGER + 1)"),
      njs_str("0") },

    { njs_str("Math.imul(2, Number.MIN_SAFE_INTEGER)"),
      njs_str("2") },

    { njs_str("Math.imul(Number.MIN_SAFE_INTEGER - 1, 1)"),
      njs_str("0") },

    { njs_str("Math.imul(2, 4294967297)"),
      njs_str("2") },

    { njs_str("Math.imul(-4294967297, 4294967297)"),
      njs_str("-1") },

    { njs_str("Math.imul(4294967297, -4294967298)"),
      njs_str("-2") },

    { njs_str("Math.imul(-4294967290, 4294967290)"),
      njs_str("-36") },

    { njs_str("Math.imul(-Infinity, 1)"),
      njs_str("0") },

    { njs_str("Math.imul(1, Infinity)"),
      njs_str("0") },

    { njs_str("Math.imul(Number.MAX_VALUE, 1)"),
      njs_str("0") },

    { njs_str("Math.imul(1, -Number.MAX_VALUE)"),
      njs_str("0") },

    { njs_str("Math.log()"),
      njs_str("NaN") },

    { njs_str("Math.log('abc')"),
      njs_str("NaN") },

    { njs_str("Math.log(-1)"),
      njs_str("NaN") },

    { njs_str("Math.log(0)"),
      njs_str("-Infinity") },

    { njs_str("Math.log('-0')"),
      njs_str("-Infinity") },

    { njs_str("Math.log(1)"),
      njs_str("0") },

    { njs_str("Math.log(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.log(Math.E)"),
      njs_str("1") },

    { njs_str("Math.log10()"),
      njs_str("NaN") },

    { njs_str("Math.log10('abc')"),
      njs_str("NaN") },

    { njs_str("Math.log10(-1)"),
      njs_str("NaN") },

    { njs_str("Math.log10(0)"),
      njs_str("-Infinity") },

    { njs_str("Math.log10('-0')"),
      njs_str("-Infinity") },

    { njs_str("Math.log10(1)"),
      njs_str("0") },

    { njs_str("Math.log10(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.log10(1000)"),
      njs_str("3") },

    { njs_str("Math.log1p()"),
      njs_str("NaN") },

    { njs_str("Math.log1p('abc')"),
      njs_str("NaN") },

    { njs_str("Math.log1p(-2)"),
      njs_str("NaN") },

    { njs_str("Math.log1p('-1')"),
      njs_str("-Infinity") },

    { njs_str("Math.log1p(0)"),
      njs_str("0") },

    { njs_str("Math.log1p(-0)"),
      njs_str("-0") },

    { njs_str("Math.log1p(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.log1p(Math.E - 1)"),
      njs_str("1") },

    { njs_str("Math.log2()"),
      njs_str("NaN") },

    { njs_str("Math.log2('abc')"),
      njs_str("NaN") },

    { njs_str("Math.log2(-1)"),
      njs_str("NaN") },

    { njs_str("Math.log2(0)"),
      njs_str("-Infinity") },

    { njs_str("Math.log2('-0')"),
      njs_str("-Infinity") },

    { njs_str("Math.log2(1)"),
      njs_str("0") },

    { njs_str("Math.log2(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.log2(128)"),
      njs_str("7") },

    { njs_str("Math.max()"),
      njs_str("-Infinity") },

    { njs_str("Math.max(0, -0)"),
      njs_str("0") },

    { njs_str("Math.max(-0, 0)"),
      njs_str("0") },

    { njs_str("Math.max(null)"),
      njs_str("0") },

    { njs_str("Math.max(undefined)"),
      njs_str("NaN") },

    { njs_str("Math.max(1, 2, 3, undefined)"),
      njs_str("NaN") },

    { njs_str("Math.max(1, 2, 3, NaN)"),
      njs_str("NaN") },

    { njs_str("Math.max('1', '2', '5')"),
      njs_str("5") },

    { njs_str("Math.max(5, {valueOf: function () {return 10}}, 6)"),
      njs_str("10") },

    { njs_str("Math.max(5, {valueOf: function () {return 10}}, 20)"),
      njs_str("20") },

    { njs_str("Math.max(5, undefined, 20)"),
      njs_str("NaN") },

    { njs_str("Math.max(-10, null, -30)"),
      njs_str("0") },

    { njs_str("Math.min()"),
      njs_str("Infinity") },

    { njs_str("Math.min(0, -0)"),
      njs_str("-0") },

    { njs_str("Math.min(-0, 0)"),
      njs_str("-0") },

    { njs_str("Math.min(null)"),
      njs_str("0") },

    { njs_str("Math.min(undefined)"),
      njs_str("NaN") },

    { njs_str("Math.min(1, 2, 3, undefined)"),
      njs_str("NaN") },

    { njs_str("Math.min(1, 2, 3, NaN)"),
      njs_str("NaN") },

    { njs_str("Math.min('1', '2', '5')"),
      njs_str("1") },

    { njs_str("Math.pow(2, 5)"),
      njs_str("32") },

    { njs_str("Math.pow(2)"),
      njs_str("NaN") },

    { njs_str("Math.pow()"),
      njs_str("NaN") },

    { njs_str("Math.pow(1, NaN)"),
      njs_str("NaN") },

    { njs_str("Math.pow(3, NaN)"),
      njs_str("NaN") },

    { njs_str("Math.pow('a', -0)"),
      njs_str("1") },

    { njs_str("Math.pow(1.1, Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.pow(-1.1, -Infinity)"),
      njs_str("0") },

    { njs_str("Math.pow(-1, Infinity)"),
      njs_str("NaN") },

    { njs_str("Math.pow(1, -Infinity)"),
      njs_str("NaN") },

    { njs_str("Math.pow(-0.9, Infinity)"),
      njs_str("0") },

    { njs_str("Math.pow(0.9, -Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.pow('Infinity', 0.1)"),
      njs_str("Infinity") },

    { njs_str("Math.pow(Infinity, '-0.1')"),
      njs_str("0") },

    { njs_str("Math.pow(-Infinity, 3)"),
      njs_str("-Infinity") },

    { njs_str("Math.pow('-Infinity', '3.1')"),
      njs_str("Infinity") },

    { njs_str("Math.pow(-Infinity, '-3')"),
      njs_str("-0") },

    { njs_str("Math.pow('-Infinity', -2)"),
      njs_str("0") },

    { njs_str("Math.pow('0', 0.1)"),
      njs_str("0") },

#ifndef __NetBSD__  /* NetBSD 7: pow(0, negative) == -Infinity. */
    { njs_str("Math.pow(0, '-0.1')"),
      njs_str("Infinity") },
#endif

    { njs_str("Math.pow(-0, 3)"),
      njs_str("-0") },

    { njs_str("Math.pow('-0', '3.1')"),
      njs_str("0") },

    { njs_str("Math.pow(-0, '-3')"),
      njs_str("-Infinity") },

#ifndef __NetBSD__  /* NetBSD 7: pow(0, negative) == -Infinity. */
    { njs_str("Math.pow('-0', -2)"),
      njs_str("Infinity") },
#endif

    { njs_str("Math.pow(-3, 0.1)"),
      njs_str("NaN") },

    { njs_str("var a = Math.random(); a >= 0 && a < 1"),
      njs_str("true") },

    { njs_str("Math.round()"),
      njs_str("NaN") },

    { njs_str("Math.round('abc')"),
      njs_str("NaN") },

    { njs_str("Math.round(0)"),
      njs_str("0") },

    { njs_str("Math.round('-0')"),
      njs_str("-0") },

    { njs_str("Math.round('Infinity')"),
      njs_str("Infinity") },

    { njs_str("Math.round(-Infinity)"),
      njs_str("-Infinity") },

    { njs_str("Math.round(0.4)"),
      njs_str("0") },

    { njs_str("Math.round('0.5')"),
      njs_str("1") },

    { njs_str("Math.round('-0.4')"),
      njs_str("-0") },

    { njs_str("Math.round(-0.5)"),
      njs_str("-0") },

    { njs_str("Math.round(-0.50000000000000001)"),
      njs_str("-0") },

    { njs_str("Math.round(-0.5000000000000001)"),
      njs_str("-1") },

    { njs_str("[0.500001, 0.5000001,0.50000001].map((v)=>Math.round((2**32) + v) - 2**32)"),
      njs_str("1,1,1") },

    { njs_str("[0.500001, 0.5000001,0.50000001].map((v)=>Math.round(-(2**32) + v) + 2**32)"),
      njs_str("1,1,1") },

    { njs_str("[0.500001, 0.5000001,0.50000001].map((v)=>Math.round((2**32) - v) - 2**32)"),
      njs_str("-1,0,0") },

    { njs_str("[0.500001, 0.5000001,0.50000001].map((v)=>Math.round(-(2**32) - v) + 2**32)"),
      njs_str("-1,0,0") },

    { njs_str("Math.sign(5)"),
      njs_str("1") },

    { njs_str("Math.sign(-5)"),
      njs_str("-1") },

    { njs_str("Math.sign(0)"),
      njs_str("0") },

    { njs_str("Math.sign(-0.0)"),
      njs_str("-0") },

    { njs_str("Math.sign(NaN)"),
      njs_str("NaN") },

    { njs_str("Math.sign()"),
      njs_str("NaN") },

    { njs_str("Math.sin()"),
      njs_str("NaN") },

    { njs_str("Math.sin('abc')"),
      njs_str("NaN") },

    { njs_str("Math.sin('0')"),
      njs_str("0") },

    { njs_str("Math.sin(-0)"),
      njs_str("-0") },

    { njs_str("Math.sin(Infinity)"),
      njs_str("NaN") },

    { njs_str("Math.sin(-Infinity)"),
      njs_str("NaN") },

    { njs_str("Math.sin(-Math.PI/2)"),
      njs_str("-1") },

    { njs_str("Math.sinh()"),
      njs_str("NaN") },

    { njs_str("Math.sinh('abc')"),
      njs_str("NaN") },

    { njs_str("Math.sinh('0')"),
      njs_str("0") },

    { njs_str("Math.sinh(-0)"),
      njs_str("-0") },

    { njs_str("Math.sinh(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.sinh(-Infinity)"),
      njs_str("-Infinity") },

    /*
     * The difference is Number.EPSILON on Solaris
     * and zero on other platforms.
     */
    { njs_str("Math.abs(Math.sinh(1) - (Math.E - 1/Math.E)/2)"
                 " <= Number.EPSILON"),
      njs_str("true") },

    { njs_str("Math.sqrt()"),
      njs_str("NaN") },

    { njs_str("Math.sqrt('abc')"),
      njs_str("NaN") },

    { njs_str("Math.sqrt('0')"),
      njs_str("0") },

    { njs_str("Math.sqrt(-0)"),
      njs_str("-0") },

    { njs_str("Math.sqrt(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.sqrt(-0.1)"),
      njs_str("NaN") },

    { njs_str("Math.sqrt('9.0')"),
      njs_str("3") },

    { njs_str("Math.tan()"),
      njs_str("NaN") },

    { njs_str("Math.tan('abc')"),
      njs_str("NaN") },

    { njs_str("Math.tan('0')"),
      njs_str("0") },

    { njs_str("Math.tan(-0)"),
      njs_str("-0") },

    { njs_str("Math.tan(Infinity)"),
      njs_str("NaN") },

    { njs_str("Math.tan(-Infinity)"),
      njs_str("NaN") },

    { njs_str("Math.tan(Math.PI/3) + Math.tan(-Math.PI/3)"),
      njs_str("0") },

    { njs_str("Math.tanh()"),
      njs_str("NaN") },

    { njs_str("Math.tanh('abc')"),
      njs_str("NaN") },

    { njs_str("Math.tanh('0')"),
      njs_str("0") },

    { njs_str("Math.tanh(-0)"),
      njs_str("-0") },

    { njs_str("Math.tanh(Infinity)"),
      njs_str("1") },

    { njs_str("Math.tanh(-Infinity)"),
      njs_str("-1") },

    { njs_str("Math.tanh(0.5) - (Math.E - 1)/(Math.E + 1)"),
      njs_str("0") },

    { njs_str("Math.trunc(3.9)"),
      njs_str("3") },

    { njs_str("Math.trunc(-3.9)"),
      njs_str("-3") },

    { njs_str("Math.trunc(0)"),
      njs_str("0") },

    { njs_str("Math.trunc(-0)"),
      njs_str("-0") },

    { njs_str("Math.trunc(0.9)"),
      njs_str("0") },

    { njs_str("Math.trunc(-0.9)"),
      njs_str("-0") },

    { njs_str("Math.trunc(Infinity)"),
      njs_str("Infinity") },

    { njs_str("Math.trunc(-Infinity)"),
      njs_str("-Infinity") },

    { njs_str("Math.trunc(NaN)"),
      njs_str("NaN") },

    { njs_str("Math.trunc()"),
      njs_str("NaN") },

    { njs_str("Math"),
      njs_str("[object Math]") },

    { njs_str("Math.x = function (x) {return 2*x;}; Math.x(3)"),
      njs_str("6") },

    { njs_str("isNaN"),
      njs_str("[object Function]") },

    { njs_str("isNaN.name"),
      njs_str("isNaN") },

    { njs_str("isNaN.length"),
      njs_str("1") },

    { njs_str("typeof isNaN"),
      njs_str("function") },

    { njs_str("typeof isNaN.length"),
      njs_str("number") },

    { njs_str("isNaN()"),
      njs_str("true") },

    { njs_str("isNaN(123)"),
      njs_str("false") },

    { njs_str("isNaN('123')"),
      njs_str("false") },

    { njs_str("isNaN('Infinity')"),
      njs_str("false") },

    { njs_str("isNaN('abc')"),
      njs_str("true") },

    { njs_str("isFinite"),
      njs_str("[object Function]") },

    { njs_str("isFinite.name"),
      njs_str("isFinite") },

    { njs_str("isFinite.length"),
      njs_str("1") },

    { njs_str("isFinite()"),
      njs_str("false") },

    { njs_str("isFinite(123)"),
      njs_str("true") },

    { njs_str("isFinite('123')"),
      njs_str("true") },

    { njs_str("isFinite('Infinity')"),
      njs_str("false") },

    { njs_str("isFinite('abc')"),
      njs_str("false") },

    { njs_str("parseInt.name"),
      njs_str("parseInt") },

    { njs_str("parseInt.length"),
      njs_str("2") },

    { njs_str("parseInt('12345abc')"),
      njs_str("12345") },

    { njs_str("parseInt('123', 0)"),
      njs_str("123") },

    { njs_str("parseInt('0XaBc', 0)"),
      njs_str("2748") },

    { njs_str("parseInt(' 123')"),
      njs_str("123") },

    { njs_str("parseInt('1010', 2)"),
      njs_str("10") },

    { njs_str("parseInt('aBc', 16)"),
      njs_str("2748") },

    { njs_str("parseInt('0XaBc')"),
      njs_str("2748") },

    { njs_str("parseInt('-0xabc')"),
      njs_str("-2748") },

    { njs_str("parseInt('njscript', 36)"),
      njs_str("1845449130881") },

    { njs_str("parseInt('0x')"),
      njs_str("NaN") },

    { njs_str("parseInt('z')"),
      njs_str("NaN") },

    { njs_str("parseInt('0xz')"),
      njs_str("NaN") },

    { njs_str("parseInt('0x', 16)"),
      njs_str("NaN") },

    { njs_str("parseInt('0x', 33)"),
      njs_str("0") },

    { njs_str("parseInt('0x', 34)"),
      njs_str("33") },

    { njs_str("parseInt('0', 1)"),
      njs_str("NaN") },

    { njs_str("parseInt('0', 37)"),
      njs_str("NaN") },

    { njs_str("1/parseInt('-0')"),
      njs_str("-Infinity") },

    { njs_str("parseInt('11', new Number(Infinity)) === parseInt('11', Infinity)"),
      njs_str("true") },

    { njs_str("parseInt('11', Number.POSITIVE_INFINITY)"),
      njs_str("11") },

    { njs_str("parseFloat.name"),
      njs_str("parseFloat") },

    { njs_str("parseFloat.length"),
      njs_str("1") },

    { njs_str("parseFloat('12345abc')"),
      njs_str("12345") },

    { njs_str("parseFloat('')"),
      njs_str("NaN") },

    { njs_str("parseFloat('     \t')"),
      njs_str("NaN") },

    { njs_str("parseFloat('\\u20281')"),
      njs_str("1") },

    { njs_str("parseFloat('e11')"),
      njs_str("NaN") },

    { njs_str("parseFloat({toString(){return '  1'}})"),
      njs_str("1") },

    { njs_str("parseFloat('1e2147483647')"),
      njs_str("Infinity") },

    { njs_str("parseFloat('1e-2147483647')"),
      njs_str("0") },

    { njs_str("parseFloat('1e-2147483648')"),
      njs_str("0") },

    { njs_str("parseFloat('1e' + '5'.repeat(16))"),
      njs_str("Infinity") },

    { njs_str("parseFloat('1e-' + '5'.repeat(16))"),
      njs_str("0") },

    { njs_str("parseFloat('0x')"),
      njs_str("0") },

    { njs_str("parseFloat('0xff')"),
      njs_str("0") },

    { njs_str("parseFloat('Infinity')"),
      njs_str("Infinity") },

    { njs_str("parseFloat(' Infinityzz')"),
      njs_str("Infinity") },

    { njs_str("parseFloat('Infinit')"),
      njs_str("NaN") },

    { njs_str("parseFloat('5.7e1')"),
      njs_str("57") },

    { njs_str("parseFloat('-5.7e-1')"),
      njs_str("-0.57") },

    { njs_str("parseFloat('-5.e-1')"),
      njs_str("-0.5") },

    { njs_str("parseFloat('5.7e+01')"),
      njs_str("57") },

    { njs_str("parseFloat(' 5.7e+01abc')"),
      njs_str("57") },

    { njs_str("parseFloat('-5.7e-1abc')"),
      njs_str("-0.57") },

    { njs_str("parseFloat('-5.7e')"),
      njs_str("-5.7") },

    { njs_str("parseFloat('-5.7e+')"),
      njs_str("-5.7") },

    { njs_str("parseFloat('-5.7e+abc')"),
      njs_str("-5.7") },

    /* Top-level objects. */

    { njs_str("var global = this;"
              "function isMutableObject(v) {"
              "    var d = Object.getOwnPropertyDescriptor(global, v);"
              "    /* Custom top-level objects are enumerable. */"
              "    var enumerable = (v in {'njs':1, 'process':1}) ^ !d.enumerable;"
              "    return d.writable && enumerable && d.configurable;"
              "};"
              "['njs', 'process', 'Math', 'JSON'].every((v)=>isMutableObject(v))"),
      njs_str("true") },

    { njs_str("njs === njs"),
      njs_str("true") },

    { njs_str("this.njs = 1; njs"),
      njs_str("1") },

    { njs_str("process === process"),
      njs_str("true") },

    { njs_str("this.process = 1; process"),
      njs_str("1") },

    { njs_str("Math === Math"),
      njs_str("true") },

    { njs_str("this.Math = 1; Math"),
      njs_str("1") },

    { njs_str("JSON"),
      njs_str("[object JSON]") },

    { njs_str("JSON === JSON"),
      njs_str("true") },

    { njs_str("this.JSON = 1; JSON"),
      njs_str("1") },

    { njs_str("delete this.JSON; JSON"),
      njs_str("ReferenceError: \"JSON\" is not defined in 1") },

    /* Top-level constructors. */

    { njs_str(
        "var global = this;"
        "function isValidConstructor(c) {"
        "   var props = Object.getOwnPropertyDescriptor(global, c.name);"
        "   props = props.writable && !props.enumerable && props.configurable;"
        "   var same = c === global[c.name];"
        ""
        "   return props && same;"
        "};"
        "Object.getOwnPropertyNames(global)"
        ".filter((k)=>(global[k] && global[k].prototype && global[k].prototype.constructor))"
        ".map(k=>global[k])"
        ".every(c => isValidConstructor(c))"),
      njs_str("true") },

    /* JSON.parse() */

    { njs_str("JSON.parse('null')"),
      njs_str("null") },

    { njs_str("JSON.parse('true')"),
      njs_str("true") },

    { njs_str("JSON.parse('false')"),
      njs_str("false") },

    { njs_str("JSON.parse('0')"),
      njs_str("0") },

    { njs_str("JSON.parse('-1234.56e2')"),
      njs_str("-123456") },

    { njs_str("typeof(JSON.parse('true'))"),
      njs_str("boolean") },

    { njs_str("typeof(JSON.parse('false'))"),
      njs_str("boolean") },

    { njs_str("typeof(JSON.parse('1'))"),
      njs_str("number") },

    { njs_str("typeof(JSON.parse('\"\"'))"),
      njs_str("string") },

    { njs_str("typeof(JSON.parse('{}'))"),
      njs_str("object") },

    { njs_str("typeof(JSON.parse('[]'))"),
      njs_str("object") },

    { njs_str("JSON.parse('\"abc\"')"),
      njs_str("abc") },

    { njs_str("JSON.parse('\"\\\\\"\"')"),
      njs_str("\"") },

    { njs_str("JSON.parse('\"\\\\n\"')"),
      njs_str("\n") },

    { njs_str("JSON.parse('\"\\\\t\"')"),
      njs_str("\t") },

    { njs_str("JSON.parse('\"ab\\\\\"c\"')"),
      njs_str("ab\"c") },

    { njs_str("JSON.parse('\"abcdefghijklmopqr\\\\\"s\"')"),
      njs_str("abcdefghijklmopqr\"s") },

    { njs_str("JSON.parse('\"ab\\\\\"c\"').length"),
      njs_str("4") },

    { njs_str("JSON.parse('\"аб\\\\\"в\"')"),
      njs_str("аб\"в") },

    { njs_str("JSON.parse('\"аб\\\\\"в\"').length"),
      njs_str("4") },

    { njs_str("JSON.parse('\"абвгдеёжзийкл\"').length"),
      njs_str("13") },

    { njs_str("JSON.parse('[\"' + 'α'.repeat(33) + '\"]')[0][32]"),
      njs_str("α") },

    { njs_str("JSON.parse('\"\\\\u03B1\"')"),
      njs_str("α") },

    { njs_str("JSON.parse('\"\\\\uD801\\\\uDC00\"')"),
      njs_str("𐐀") },

    { njs_str("JSON.parse('\"\\\\u03B1\"') == JSON.parse('\"\\\\u03b1\"')"),
      njs_str("true") },

    { njs_str("JSON.parse('\"\\\\u03B1\"').length"),
      njs_str("1") },

    { njs_str("JSON.parse('{\"a\":1}').a"),
      njs_str("1") },

    { njs_str("JSON.parse('{\"a\":1,\"a\":2}').a"),
      njs_str("2") },

    { njs_str("JSON.parse('{   \"a\" :  \"b\"   }').a"),
      njs_str("b") },

    { njs_str("JSON.parse('{\"a\":{\"b\":1}}').a.b"),
      njs_str("1") },

    { njs_str("JSON.parse('[{}, true ,1.1e2, {\"a\":[3,\"b\"]}]')[3].a[1]"),
      njs_str("b") },

    { njs_str("var o = JSON.parse('{\"a\":2}');"
                 "Object.getOwnPropertyDescriptor(o, 'a').configurable"),
      njs_str("true") },

    { njs_str("var o = JSON.parse('{\"a\":2}');"
                 "Object.getOwnPropertyDescriptor(o, 'a').writable"),
      njs_str("true") },

    { njs_str("var o = JSON.parse('{\"a\":2}');"
                 "Object.getOwnPropertyDescriptor(o, 'a').enumerable"),
      njs_str("true") },

    { njs_str("var o = JSON.parse('{\"a\":2}');"
                 "o.a = 3; o.a"),
      njs_str("3") },

    { njs_str("var o = JSON.parse('{\"a\":2}');"
                 "o.b = 3; o.b"),
      njs_str("3") },

    { njs_str("JSON.parse('2') || 10"),
      njs_str("2") },

    { njs_str("JSON.parse('0') || 10"),
      njs_str("10") },

    { njs_str("JSON.parse('-0') || 10"),
      njs_str("10") },

    { njs_str("JSON.parse('\"a\"') || 10"),
      njs_str("a") },

    { njs_str("JSON.parse('\"\"') || 10"),
      njs_str("10") },

    { njs_str("JSON.parse('true') || 10"),
      njs_str("true") },

    { njs_str("JSON.parse('false') || 10"),
      njs_str("10") },

    { njs_str("JSON.parse('null') || 10"),
      njs_str("10") },

    { njs_str("var o = JSON.parse('{}', function(k, v) {return v;}); o"),
      njs_str("[object Object]") },

    { njs_str("var o = JSON.parse('{\"a\":2, \"b\":4, \"a\":{}}',"
                 "                    function(k, v) {return undefined;});"
                 "o"),
      njs_str("undefined") },

    { njs_str("var o = JSON.parse('{\"a\":2, \"c\":4, \"b\":\"x\"}',"
                 "  function(k, v) {if (k === '' || typeof v === 'number') return v });"
                 "Object.keys(o)"),
      njs_str("a,c") },

    { njs_str("var o = JSON.parse('{\"a\":2, \"b\":{}}',"
                 "                    function(k, v) {return k;});"
                 "o+typeof(o)"),
      njs_str("string") },

    { njs_str("var o = JSON.parse('[\"a\", \"b\"]',"
                 "                    function(k, v) {return v;});"
                 "o"),
      njs_str("a,b") },

    { njs_str("var o = JSON.parse('{\"a\":[1,{\"b\":1},3]}',"
                 "                    function(k, v) {return v;});"
                 "o.a[1].b"),
      njs_str("1") },

    { njs_str("var o = JSON.parse('{\"a\":[1,2]}',"
                 "  function(k, v) {if (k === '' || k === 'a') {return v;}});"
                 "o.a"),
      njs_str(",") },

    { njs_str("var o = JSON.parse('{\"a\":[1,2]}',"
                 "  function(k, v) {return (k === '' || k === 'a') ? v : v*2});"
                 "o.a"),
      njs_str("2,4") },

    { njs_str("var o = JSON.parse('{\"a\":2, \"b\":{\"c\":[\"xx\"]}}',"
                 "   function(k, v) {return typeof v === 'number' ? v * 2 : v;});"
                 "o.a+o.b.c[0]"),
      njs_str("4xx") },

    { njs_str("var o = JSON.parse('{\"aa\":{\"b\":1}, \"abb\":1, \"c\":1}',"
                 "   function(k, v) {return (k === '' || /^a/.test(k)) ? v : undefined;});"
                 "Object.keys(o)"),
      njs_str("aa,abb") },

    { njs_str("var o = JSON.parse('{\"a\":\"x\"}',"
                 "   function(k, v) {if (k === 'a') {this.b='y';} return v});"
                 "o.a+o.b"),
      njs_str("xy") },

    { njs_str("var o = JSON.parse('{\"a\":\"x\"}',"
                 "   function(k, v) {return (k === 'a' ? {x:1} : v)});"
                 "o.a.x"),
      njs_str("1") },

    { njs_str("var keys = []; var o = JSON.parse('{\"a\":2, \"b\":{\"c\":\"xx\"}}',"
                 "   function(k, v) {keys.push(k); return v;});"
                 "keys"),
      njs_str("a,c,b,") },

    { njs_str("var args = []; var o = JSON.parse('[2,{\"a\":3}]',"
                 "   function(k, v) {args.push(k+\":\"+v); return v;});"
                 "args.join('|')"),
      njs_str("0:2|a:3|1:[object Object]|:2,[object Object]") },

    { njs_str("JSON.parse()"),
      njs_str("SyntaxError: Unexpected token at position 0") },

    { njs_str("JSON.parse([])"),
      njs_str("SyntaxError: Unexpected end of input at position 0") },

    { njs_str("JSON.parse('')"),
      njs_str("SyntaxError: Unexpected end of input at position 0") },

    { njs_str("JSON.parse('fals')"),
      njs_str("SyntaxError: Unexpected token at position 0") },

    { njs_str("JSON.parse(' t')"),
      njs_str("SyntaxError: Unexpected token at position 1") },

    { njs_str("JSON.parse('nu')"),
      njs_str("SyntaxError: Unexpected token at position 0") },

    { njs_str("JSON.parse('-')"),
      njs_str("SyntaxError: Unexpected number at position 0") },

    { njs_str("JSON.parse('--')"),
      njs_str("SyntaxError: Unexpected number at position 1") },

    { njs_str("JSON.parse('1-')"),
      njs_str("SyntaxError: Unexpected token at position 1") },

    { njs_str("JSON.parse('1ee1')"),
      njs_str("SyntaxError: Unexpected token at position 1") },

    { njs_str("JSON.parse('1eg')"),
      njs_str("SyntaxError: Unexpected token at position 1") },

    { njs_str("JSON.parse('0x01')"),
      njs_str("SyntaxError: Unexpected token at position 1") },

    { njs_str("JSON.parse('\"абв')"),
      njs_str("SyntaxError: Unexpected end of input at position 4") },

    { njs_str("JSON.parse('\"\b')"),
      njs_str("SyntaxError: Forbidden source char at position 1") },

    { njs_str("JSON.parse('\"\\\\u')"),
      njs_str("SyntaxError: Unexpected end of input at position 3") },

    { njs_str("JSON.parse('\"\\\\q\"')"),
      njs_str("SyntaxError: Unknown escape char at position 2") },

    { njs_str("JSON.parse('\"\\\\uDC01\"')"),
      njs_str("�") },

    { njs_str("JSON.parse('\"\\\\uD801\\\\uE000\"')"),
      njs_str("�") },

    { njs_str("JSON.parse('\"\\\\uD83D\"')"),
      njs_str("�") },

    { njs_str("JSON.parse('\"\\\\uD800\\\\uDB00\"')"),
      njs_str("��") },

    { njs_str("JSON.parse('\"\\\\ud800[\"')"),
      njs_str("�[") },

    { njs_str("JSON.parse('{')"),
      njs_str("SyntaxError: Unexpected end of input at position 1") },

    { njs_str("JSON.parse('{{')"),
      njs_str("SyntaxError: Unexpected token at position 1") },

    { njs_str("JSON.parse('{[')"),
      njs_str("SyntaxError: Unexpected token at position 1") },

    { njs_str("JSON.parse('{\"a\"')"),
      njs_str("SyntaxError: Unexpected token at position 4") },

    { njs_str("JSON.parse('{\"a\":')"),
      njs_str("SyntaxError: Unexpected end of input at position 5") },

    { njs_str("JSON.parse('{\"a\":{')"),
      njs_str("SyntaxError: Unexpected end of input at position 6") },

    { njs_str("JSON.parse('{\"a\":{}')"),
      njs_str("SyntaxError: Unexpected end of input at position 7") },

    { njs_str("JSON.parse('{\"a\":{}g')"),
      njs_str("SyntaxError: Unexpected token at position 7") },

    { njs_str("JSON.parse('{\"a\":{},')"),
      njs_str("SyntaxError: Unexpected end of input at position 8") },

    { njs_str("JSON.parse('{\"a\":{},}')"),
      njs_str("SyntaxError: Trailing comma at position 7") },

    { njs_str("JSON.parse('{\"a\":{},,')"),
      njs_str("SyntaxError: Unexpected token at position 8") },

    { njs_str("JSON.parse('{\"a\":{},,}')"),
      njs_str("SyntaxError: Unexpected token at position 8") },

    { njs_str("JSON.parse('[')"),
      njs_str("SyntaxError: Unexpected end of input at position 1") },

    { njs_str("JSON.parse('[q')"),
      njs_str("SyntaxError: Unexpected token at position 1") },

    { njs_str("JSON.parse('[\"a')"),
      njs_str("SyntaxError: Unexpected end of input at position 3") },

    { njs_str("JSON.parse('[1 ')"),
      njs_str("SyntaxError: Unexpected end of input at position 3") },

    { njs_str("JSON.parse('[1,]')"),
      njs_str("SyntaxError: Trailing comma at position 2") },

    { njs_str("JSON.parse('[1 , 5 ')"),
      njs_str("SyntaxError: Unexpected end of input at position 7") },

    { njs_str("JSON.parse('{\"a\":'.repeat(32))"),
      njs_str("SyntaxError: Nested too deep at position 155") },

    { njs_str("JSON.parse('['.repeat(32))"),
      njs_str("SyntaxError: Nested too deep at position 31") },

    { njs_str("var o = JSON.parse('{', function(k, v) {return v;});o"),
      njs_str("SyntaxError: Unexpected end of input at position 1") },

    { njs_str("var o = JSON.parse('{\"a\":1}', "
                 "                   function(k, v) {return v.a.a;}); o"),
      njs_str("TypeError: cannot get property \"a\" of undefined") },

    /* JSON.stringify() */

    { njs_str("JSON.stringify()"),
      njs_str("undefined") },

    { njs_str("JSON.stringify('')"),
      njs_str("\"\"") },

    { njs_str("JSON.stringify('abc')"),
      njs_str("\"abc\"") },

    { njs_str("JSON.stringify(new String('abc'))"),
      njs_str("\"abc\"") },

    { njs_str("JSON.stringify(123)"),
      njs_str("123") },

    { njs_str("JSON.stringify(-0)"),
      njs_str("0") },

    { njs_str("JSON.stringify(0.00000123)"),
      njs_str("0.00000123") },

    { njs_str("JSON.stringify(new Number(123))"),
      njs_str("123") },

    { njs_str("JSON.stringify(true)"),
      njs_str("true") },

    { njs_str("JSON.stringify(false)"),
      njs_str("false") },

    { njs_str("JSON.stringify(new Boolean(1))"),
      njs_str("true") },

    { njs_str("JSON.stringify(new Boolean(0))"),
      njs_str("false") },

    { njs_str("JSON.stringify(null)"),
      njs_str("null") },

    { njs_str("JSON.stringify(undefined)"),
      njs_str("undefined") },

    { njs_str("JSON.stringify(Symbol())"),
      njs_str("undefined") },

    { njs_str("JSON.stringify({})"),
      njs_str("{}") },

    { njs_str("JSON.stringify([])"),
      njs_str("[]") },

    { njs_str("var a = [1]; a[2] = 'x'; JSON.stringify(a)"),
      njs_str("[1,null,\"x\"]") },

    { njs_str("JSON.stringify({a:\"b\",c:19,e:null,t:true,f:false})"),
      njs_str("{\"a\":\"b\",\"c\":19,\"e\":null,\"t\":true,\"f\":false}") },

    { njs_str("JSON.stringify({a:1, b:undefined})"),
      njs_str("{\"a\":1}") },

    { njs_str("JSON.stringify({a:1, b:Symbol()})"),
      njs_str("{\"a\":1}") },

    { njs_str("var o = {a:1, c:2};"
                 "Object.defineProperty(o, 'b', {enumerable:false, value:3});"
                 "JSON.stringify(o)"),
      njs_str("{\"a\":1,\"c\":2}") },

    { njs_str("JSON.stringify({a:{}, b:[function(v){}]})"),
      njs_str("{\"a\":{},\"b\":[null]}") },

    { njs_str("JSON.stringify(RegExp())"),
      njs_str("{}") },

    { njs_str("JSON.stringify(Object(Symbol()))"),
      njs_str("{}") },

    { njs_str("var s = Object(Symbol()); s.test = 'test'; JSON.stringify(s)"),
      njs_str("{\"test\":\"test\"}") },

    { njs_str("JSON.stringify(SyntaxError('e'))"),
      njs_str("{}") },

    { njs_str("JSON.stringify(URIError('e'))"),
      njs_str("{}") },

    { njs_str("var e = URIError('e'); e.name = 'E'; JSON.stringify(e)"),
      njs_str("{\"name\":\"E\"}") },

    { njs_str("var e = URIError('e'); e.message = 'E'; JSON.stringify(e)"),
      njs_str("{}") },

    { njs_str("var e = URIError('e'); e.foo = 'E'; JSON.stringify(e)"),
      njs_str("{\"foo\":\"E\"}") },

    { njs_str("var r = JSON.parse(JSON.stringify($r));"
              "[r.uri, r.host, r.props.a, njs.dump(r.vars), njs.dump(r.consts), r.header['02']]"),
      njs_str("АБВ,АБВГДЕЁЖЗИЙ,1,{},{},02|АБВ") },

    { njs_str("JSON.stringify({get key() {throw new Error('Oops')}})"),
      njs_str("Error: Oops") },

    { njs_str("JSON.stringify({toJSON() {throw new Error('Oops')}})"),
      njs_str("Error: Oops") },

    /* Ignoring named properties of an array. */

    { njs_str("var a = [1,2]; a.a = 1;"
                 "JSON.stringify(a)"),
      njs_str("[1,2]") },

    { njs_str("JSON.stringify({a:{b:{c:{d:1}, e:function(v){}}}})"),
      njs_str("{\"a\":{\"b\":{\"c\":{\"d\":1}}}}") },

    { njs_str("JSON.stringify([[\"b\",undefined],1,[5],{a:1}])"),
      njs_str("[[\"b\",null],1,[5],{\"a\":1}]") },

    { njs_str("var json = '{\"a\":{\"b\":{\"c\":{\"d\":1},\"e\":[true]}}}';"
                 "json == JSON.stringify(JSON.parse(json))"),
      njs_str("true") },

    { njs_str("var json = '{\"a\":\"абв\",\"b\":\"α\"}';"
                 "json == JSON.stringify(JSON.parse(json))"),
      njs_str("true") },

    /* Multibyte characters: z - 1 byte, α - 2 bytes, 𐐀 - 4 bytes */

    { njs_str("JSON.stringify('α𐐀z'.repeat(10))"),
      njs_str("\"α𐐀zα𐐀zα𐐀zα𐐀zα𐐀zα𐐀zα𐐀zα𐐀zα𐐀zα𐐀z\"") },

    { njs_str("JSON.stringify('α𐐀z'.repeat(10)).length"),
      njs_str("32") },

    { njs_str("JSON.stringify('α'.repeat(33))[32]"),
      njs_str("α") },

    { njs_str("JSON.stringify('a\\nbc')"),
      njs_str("\"a\\nbc\"") },

    { njs_str("JSON.stringify('а\tбв')"),
      njs_str("\"а\\tбв\"") },

    { njs_str("JSON.stringify('\\n\\t\\r\\\"\\f\\b')"),
      njs_str("\"\\n\\t\\r\\\"\\f\\b\"") },

    { njs_str("JSON.stringify('\x00\x01\x02\x1f')"),
      njs_str("\"\\u0000\\u0001\\u0002\\u001F\"") },

    { njs_str("JSON.stringify('abc\x00')"),
      njs_str("\"abc\\u0000\"") },

    { njs_str("JSON.stringify('\x00zz')"),
      njs_str("\"\\u0000zz\"") },

    { njs_str("JSON.stringify('\x00')"),
      njs_str("\"\\u0000\"") },

    { njs_str("JSON.stringify('a\x00z')"),
      njs_str("\"a\\u0000z\"") },

    { njs_str("JSON.stringify('\x00z\x00')"),
      njs_str("\"\\u0000z\\u0000\"") },

    { njs_str("var i, s, r = true;"
                 " for (i = 0; i < 128; i++) {"
                 "  s = 'α𐐀z'.repeat(i);"
                 "  r &= (JSON.stringify(s) == ('\"' + s + '\"'));"
                 "}; r"),
      njs_str("1") },

    { njs_str("JSON.stringify('\\u0000'.repeat(10)) == ('\"' + '\\\\u0000'.repeat(10) + '\"')"),
      njs_str("true") },

    { njs_str("JSON.stringify('abc'.repeat(100)).length"),
      njs_str("302") },

    { njs_str("JSON.stringify('абв'.repeat(100)).length"),
      njs_str("302") },

    /* Byte strings. */

    { njs_str("JSON.stringify('\\u00CE\\u00B1\\u00C2\\u00B6'.toBytes())"),
      njs_str("\"α¶\"") },

    { njs_str("JSON.stringify('µ§±®'.toBytes())"),
      njs_str("\"\xB5\xA7\xB1\xAE\"") },

    /* Optional arguments. */

    { njs_str("JSON.stringify(undefined, undefined, 1)"),
      njs_str("undefined") },

    { njs_str("JSON.stringify([{a:1,b:{c:2}},1], undefined, 0)"),
      njs_str("[{\"a\":1,\"b\":{\"c\":2}},1]") },

    { njs_str("JSON.stringify([{a:1,b:{c:2}},1], undefined, 1)"),
      njs_str("[\n {\n  \"a\": 1,\n  \"b\": {\n   \"c\": 2\n  }\n },\n 1\n]") },

    { njs_str("JSON.stringify([{a:1,b:{c:2}},1], undefined, ' ')"),
      njs_str("[\n {\n  \"a\": 1,\n  \"b\": {\n   \"c\": 2\n  }\n },\n 1\n]") },

    { njs_str("JSON.stringify([{a:1,b:{c:2}},1], undefined, '#')"),
      njs_str("[\n#{\n##\"a\": 1,\n##\"b\": {\n###\"c\": 2\n##}\n#},\n#1\n]") },

    { njs_str("JSON.stringify([1], undefined, 'AAAAABBBBBC')"),
      njs_str("[\nAAAAABBBBB1\n]") },

    { njs_str("JSON.stringify([1], undefined, 11)"),
      njs_str("[\n          1\n]") },

    { njs_str("JSON.stringify([{a:1,b:{c:2}},1], undefined, -1)"),
      njs_str("[{\"a\":1,\"b\":{\"c\":2}},1]") },

    { njs_str("JSON.stringify([{a:1,b:{c:2}},1], undefined, new Date())"),
      njs_str("[{\"a\":1,\"b\":{\"c\":2}},1]") },

    { njs_str("var o = Object.defineProperty({}, 'a', { get() { return ()=> 1}, enumerable: true });"
              "JSON.stringify(o)"),
      njs_str("{}") },

    { njs_str("var o = Object.defineProperty({}, 'a', { get: () => ({b:1, c:2}), enumerable: true });"
              "JSON.stringify(o)"),
      njs_str("{\"a\":{\"b\":1,\"c\":2}}") },

    { njs_str("var o = Object.defineProperty({}, 'a', { get: () => ({})});"
              "JSON.stringify(o)"),
      njs_str("{}") },

    { njs_str("JSON.stringify({toJSON:function(k){}})"),
      njs_str("undefined") },

    { njs_str("JSON.stringify({toJSON:function(k){return k}})"),
      njs_str("\"\"") },

    { njs_str("JSON.stringify(new Date(1308895323625))"),
      njs_str("\"2011-06-24T06:02:03.625Z\"") },

    { njs_str("JSON.stringify({a:new Date(1308895323625)})"),
      njs_str("{\"a\":\"2011-06-24T06:02:03.625Z\"}") },

    { njs_str("JSON.stringify({b:{toJSON:function(k){return undefined}}})"),
      njs_str("{}") },

    { njs_str("JSON.stringify({b:{toJSON:function(k){}},c:1})"),
      njs_str("{\"c\":1}") },

    { njs_str("JSON.stringify({b:{toJSON:function(k){return k}}})"),
      njs_str("{\"b\":\"b\"}") },

    { njs_str("JSON.stringify({a:1,b:new Date(1308895323625),c:2})"),
      njs_str("{\"a\":1,\"b\":\"2011-06-24T06:02:03.625Z\",\"c\":2}") },

    { njs_str("JSON.stringify({a:{b:new Date(1308895323625)}})"),
      njs_str("{\"a\":{\"b\":\"2011-06-24T06:02:03.625Z\"}}") },

    { njs_str("function key(k){return k}; function und(k){}"
                 "JSON.stringify([{toJSON:key},{toJSON:und},{toJSON:key}])"),
      njs_str("[\"0\",null,\"2\"]") },

    { njs_str("JSON.stringify({b:{a:1,c:[2]}}, function(k,v){return v})"),
      njs_str("{\"b\":{\"a\":1,\"c\":[2]}}") },

    { njs_str("JSON.stringify([{a:1}, 2], function(k,v){return v})"),
      njs_str("[{\"a\":1},2]") },

    { njs_str("JSON.stringify({a:{toJSON:function(k){}}}, function(k,v){return v})"),
      njs_str("{}") },

    { njs_str("JSON.stringify({a:{toJSON:function(k){return 1}}}, function(k,v){return v})"),
      njs_str("{\"a\":1}") },

    { njs_str("JSON.stringify([{toJSON:function(k){}}], function(k,v){return v})"),
      njs_str("[null]") },

    { njs_str("JSON.stringify([{toJSON:function(k){return 1}}], function(k,v){return v})"),
      njs_str("[1]") },

    { njs_str("JSON.stringify({a:new Date(1308895323625)}, function(k,v){return v})"),
      njs_str("{\"a\":\"2011-06-24T06:02:03.625Z\"}") },

    { njs_str("JSON.stringify([new Date(1308895323625)], function(k,v){return v})"),
      njs_str("[\"2011-06-24T06:02:03.625Z\"]") },

    { njs_str("JSON.stringify([new Date(1308895323625)], "
                 "  function(k,v){return (typeof v === 'string') ? v.toLowerCase() : v})"),
      njs_str("[\"2011-06-24t06:02:03.625z\"]") },

    { njs_str("JSON.stringify([new Date(1308895323625)], "
                 "  function(k,v){return (typeof v === 'string') ? v.toLowerCase() : v}, '#')"),
      njs_str("[\n#\"2011-06-24t06:02:03.625z\"\n]") },

    { njs_str("JSON.stringify({a:new Date(1308895323625),b:1,c:'a'}, "
                 "  function(k,v){return (typeof v === 'string') ? undefined : v})"),
      njs_str("{\"b\":1}") },

    { njs_str("JSON.stringify({a:new Date(1308895323625),b:1,c:'a'}, "
                 "  function(k,v){return (typeof v === 'string') ? undefined : v}, '#')"),
      njs_str("{\n#\"b\": 1\n}") },

    { njs_str("JSON.stringify([new Date(1308895323625),1,'a'], "
                 "  function(k,v){return (typeof v === 'string') ? undefined : v})"),
      njs_str("[null,1,null]") },

    { njs_str("var keys = []; var o = JSON.stringify({a:2, b:{c:1}},"
                 "   function(k, v) {keys.push(k); return v;});"
                 "keys"),
      njs_str(",a,b,c") },

    { njs_str("JSON.stringify(['a', 'b', 'c'], "
                 "    function(i, v) { if (i === '0') {return undefined} "
                 "                     else if (i == 1) {return 2} "
                 "                     else {return v}})"),
      njs_str("[null,2,\"c\"]") },

    { njs_str("JSON.stringify({a:2, b:{c:1}},"
                 "               function(k, v) {delete this['b']; return v;})"),
      njs_str("{\"a\":2}") },

    { njs_str("JSON.stringify(JSON.parse('{\"a\":1,\"b\":2}', "
                 "          function(k, v) {delete this['b']; return v;}))"),
      njs_str("{\"a\":1}") },

    { njs_str("var keys = []; var o = JSON.stringify([[1,2],{a:3}, 4],"
                 "   function(k, v) {keys.push(k); return v;});"
                 "keys"),
      njs_str(",0,0,1,1,a,2") },

    { njs_str("JSON.stringify({b:{a:1,c:[2]}}, ['a', undefined, 'b', {}, 'a'])"),
      njs_str("{\"b\":{\"a\":1}}") },

    { njs_str("JSON.stringify({b:{a:1,c:[2]}}, [new String('a'), new String('b')])"),
      njs_str("{\"b\":{\"a\":1}}") },

    { njs_str("JSON.stringify({'1':1,'2':2,'3':3}, [1, new Number(2)])"),
      njs_str("{\"1\":1,\"2\":2}") },

    { njs_str("var objs = []; var o = JSON.stringify({a:1},"
                 "   function(k, v) {objs.push(this); return v});"
                 "JSON.stringify(objs)"),
      njs_str("[{\"\":{\"a\":1}},{\"a\":1}]") },

    { njs_str("JSON.stringify({a: () => 1, b: Symbol(), c: undefined},"
                             "(k, v) => k.length ? String(v) : v)"),
      njs_str("{\"a\":\"[object Function]\",\"b\":\"Symbol()\",\"c\":\"undefined\"}") },

    { njs_str("var a = []; a[0] = a; JSON.stringify(a)"),
      njs_str("TypeError: Nested too deep or a cyclic structure") },

    { njs_str("var a = {}; a.a = a; JSON.stringify(a)"),
      njs_str("TypeError: Nested too deep or a cyclic structure") },

    /* njs.dump(). */

    { njs_str("njs.dump({a:1, b:[1,,2,{c:new Boolean(1)}]})"),
      njs_str("{a:1,b:[1,<empty>,2,{c:[Boolean: true]}]}") },

    { njs_str("njs.dump([InternalError(),TypeError('msg'), new RegExp(), /^undef$/m, new Date(0)])"),
      njs_str("[InternalError,TypeError: msg,/(?:)/,/^undef$/m,1970-01-01T00:00:00.000Z]") },

    { njs_str("njs.dump(Array.prototype.slice.call({'1':'b', length:2}))"),
      njs_str("[<empty>,'b']") },

    { njs_str("var o = Object.defineProperty({}, 'a', { get: () => 1, enumerable: true }); njs.dump(o)"),
      njs_str("{a:'[Getter]'}") },

    { njs_str("var o = Object.defineProperty({}, 'a', { get: () => 1, set(){}, enumerable: true }); njs.dump(o)"),
      njs_str("{a:'[Getter/Setter]'}") },

    { njs_str("var o = Object.defineProperty({}, 'a', { set(){}, enumerable: true }); njs.dump(o)"),
      njs_str("{a:'[Setter]'}") },

    { njs_str("njs.dump($r.props)"),
      njs_str("{a:'1',b:42}") },

    { njs_str("njs.dump($r.header)"),
      njs_str("{01:'01|АБВ',02:'02|АБВ',03:'03|АБВ'}") },

    { njs_str("njs.dump(njs) == `njs {version:'${njs.version}'}`"),
      njs_str("true") },

    { njs_str("njs.dump(-0)"),
      njs_str("-0") },

    { njs_str("njs.dump(Object(-0))"),
      njs_str("[Number: -0]") },

    { njs_str("njs.dump([0, -0])"),
      njs_str("[0,-0]") },

    { njs_str("njs.dump(Symbol())"),
      njs_str("Symbol()") },

    { njs_str("njs.dump(Object(Symbol()))"),
      njs_str("[Symbol: Symbol()]") },

    { njs_str("njs.dump(Symbol('desc'))"),
      njs_str("Symbol(desc)") },

    { njs_str("njs.dump(Object(Symbol('desc')))"),
      njs_str("[Symbol: Symbol(desc)]") },

    { njs_str("njs.dump(Symbol.iterator)"),
      njs_str("Symbol(Symbol.iterator)") },

    { njs_str("njs.dump(Object(Symbol.iterator))"),
      njs_str("[Symbol: Symbol(Symbol.iterator)]") },

    /* Built-in methods name. */

    { njs_str(
        "var fail;"
        "function isMethodsHaveName(o) {"
        "    var except = ["
        "        'prototype',"
        "        'constructor',"
        "        'caller',"
        "        'arguments',"
        "        'description',"
        "    ];"
        "    return Object.getOwnPropertyNames(o)"
        "                 .filter(v => !except.includes(v)"
        "                              && typeof o[v] == 'function')"
        "                 .every(v => o[v].name == v"
        "                             || !(fail = `${o.name}.${v}: ${o[v].name}`));"
        "}"
        "["
        "    Boolean, Boolean.prototype,"
        "    Number, Number.prototype,"
        "    Symbol, Symbol.prototype,"
        "    String, String.prototype,"
        "    Object, Object.prototype,"
        "    Array, Array.prototype,"
        "    Function, Function.prototype,"
        "    RegExp, RegExp.prototype,"
        "    Date, Date.prototype,"
        "    Error, Error.prototype,"
        "    Math,"
        "    JSON,"
        "].every(obj => isMethodsHaveName(obj)) || fail"),

      njs_str("true") },

    /* require(). */

    { njs_str("require('unknown_module')"),
      njs_str("Error: Cannot find module \"unknown_module\"") },

    { njs_str("require()"),
      njs_str("TypeError: missing path") },

    { njs_str("require.length"),
      njs_str("1") },

    { njs_str("require.name"),
      njs_str("require") },

    { njs_str("typeof require"),
      njs_str("function") },

    { njs_str("require.hasOwnProperty('length')"),
      njs_str("true") },

    { njs_str("var fs = require('fs'); typeof fs"),
      njs_str("object") },

    /* require('fs').readFile() */

    { njs_str("var fs = require('fs');"
                 "fs.readFile()"),
      njs_str("TypeError: too few arguments") },

    { njs_str("var fs = require('fs');"
                 "fs.readFile('/njs_unknown_path')"),
      njs_str("TypeError: too few arguments") },

    { njs_str("var fs = require('fs');"
                 "fs.readFile('/njs_unknown_path', {flag:'xx'})"),
      njs_str("TypeError: callback must be a function") },

    { njs_str("var fs = require('fs');"
                 "fs.readFile('/njs_unknown_path', {flag:'xx'}, 1)"),
      njs_str("TypeError: callback must be a function") },

    { njs_str("var fs = require('fs');"
                 "fs.readFile('/njs_unknown_path', {flag:'xx'}, function () {})"),
      njs_str("TypeError: Unknown file open flags: \"xx\"") },

    { njs_str("var fs = require('fs');"
                 "fs.readFile('/njs_unknown_path', {encoding:'ascii'}, function () {})"),
      njs_str("TypeError: Unknown encoding: \"ascii\"") },

    { njs_str("var fs = require('fs');"
                 "fs.readFile('/njs_unknown_path', 'ascii', function () {})"),
      njs_str("TypeError: Unknown encoding: \"ascii\"") },

    /* require('fs').readFileSync() */

    { njs_str("var fs = require('fs');"
                 "fs.readFileSync()"),
      njs_str("TypeError: too few arguments") },

    { njs_str("var fs = require('fs');"
                 "fs.readFileSync({})"),
      njs_str("TypeError: path must be a string") },

    { njs_str("var fs = require('fs');"
                 "fs.readFileSync('/njs_unknown_path', {flag:'xx'})"),
      njs_str("TypeError: Unknown file open flags: \"xx\"") },

    { njs_str("var fs = require('fs');"
                 "fs.readFileSync('/njs_unknown_path', {encoding:'ascii'})"),
      njs_str("TypeError: Unknown encoding: \"ascii\"") },

    { njs_str("var fs = require('fs');"
                 "fs.readFileSync('/njs_unknown_path', 'ascii')"),
      njs_str("TypeError: Unknown encoding: \"ascii\"") },

    { njs_str("var fs = require('fs');"
                 "fs.readFileSync('/njs_unknown_path', true)"),
      njs_str("TypeError: Unknown options type (a string or object required)") },


    /* require('fs').writeFile() */

    { njs_str("var fs = require('fs');"
                 "fs.writeFile()"),
      njs_str("TypeError: too few arguments") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFile('/njs_unknown_path')"),
      njs_str("TypeError: too few arguments") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFile('/njs_unknown_path', '')"),
      njs_str("TypeError: too few arguments") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFile({}, '', function () {})"),
      njs_str("TypeError: path must be a string") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFile('/njs_unknown_path', '', 'utf8')"),
      njs_str("TypeError: callback must be a function") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFile('/njs_unknown_path', '', {flag:'xx'}, function () {})"),
      njs_str("TypeError: Unknown file open flags: \"xx\"") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFile('/njs_unknown_path', '', {encoding:'ascii'}, function () {})"),
      njs_str("TypeError: Unknown encoding: \"ascii\"") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFile('/njs_unknown_path', '', 'ascii', function () {})"),
      njs_str("TypeError: Unknown encoding: \"ascii\"") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFile('/njs_unknown_path', '', true, function () {})"),
      njs_str("TypeError: Unknown options type (a string or object required)") },

    /* require('fs').writeFileSync() */

    { njs_str("var fs = require('fs');"
                 "fs.writeFileSync()"),
      njs_str("TypeError: too few arguments") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFileSync('/njs_unknown_path')"),
      njs_str("TypeError: too few arguments") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFileSync({}, '')"),
      njs_str("TypeError: path must be a string") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFileSync('/njs_unknown_path', '', {flag:'xx'})"),
      njs_str("TypeError: Unknown file open flags: \"xx\"") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFileSync('/njs_unknown_path', '', {encoding:'ascii'})"),
      njs_str("TypeError: Unknown encoding: \"ascii\"") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFileSync('/njs_unknown_path', '', 'ascii')"),
      njs_str("TypeError: Unknown encoding: \"ascii\"") },

    { njs_str("var fs = require('fs');"
                 "fs.writeFileSync('/njs_unknown_path', '', true)"),
      njs_str("TypeError: Unknown options type (a string or object required)") },

    /* require('fs').writeFileSync() */

    { njs_str("var fs = require('fs');"
              "fs.renameSync()"),
      njs_str("TypeError: oldPath must be a string") },

    { njs_str("var fs = require('fs');"
              "fs.renameSync({toString(){return '/path/1'}})"),
      njs_str("TypeError: newPath must be a string") },

    /* require('crypto').createHash() */

    { njs_str("var h = require('crypto').createHash('sha1');"
              "[Object.prototype.toString.call(h), njs.dump(h),h]"),
      njs_str("[object Hash],Hash {},[object Hash]") },

    { njs_str("var h = require('crypto').createHash('sha1');"
              "var Hash = h.constructor; "
              "Hash('sha1').update('AB').digest('hex')"),
      njs_str("06d945942aa26a61be18c3e22bf19bbca8dd2b5d") },

    { njs_str("var h = require('crypto').createHash('sha1');"
              "h.constructor.name"),
      njs_str("Hash") },

    { njs_str("var h = require('crypto').createHash('md5');"
                 "h.update('AB').digest('hex')"),
      njs_str("b86fc6b051f63d73de262d4c34e3a0a9") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('A').update('B').digest('hex')"),
      njs_str("06d945942aa26a61be18c3e22bf19bbca8dd2b5d") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('AB').digest('hex')"),
      njs_str("06d945942aa26a61be18c3e22bf19bbca8dd2b5d") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('AB').digest().toString('hex')"),
      njs_str("06d945942aa26a61be18c3e22bf19bbca8dd2b5d") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('AB').digest('base64')"),
      njs_str("BtlFlCqiamG+GMPiK/GbvKjdK10=") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('AB').digest('base64url')"),
      njs_str("BtlFlCqiamG-GMPiK_GbvKjdK10") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('AB').digest().toString('base64')"),
      njs_str("BtlFlCqiamG+GMPiK/GbvKjdK10=") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('abc'.repeat(100)).digest('hex')"),
      njs_str("c95466320eaae6d19ee314ae4f135b12d45ced9a") },

    { njs_str("var h = require('crypto').createHash('sha256');"
                 "h.update('A').update('B').digest('hex')"),
      njs_str("38164fbd17603d73f696b8b4d72664d735bb6a7c88577687fd2ae33fd6964153") },

    { njs_str("var h = require('crypto').createHash('sha256');"
                 "h.update('AB').digest('hex')"),
      njs_str("38164fbd17603d73f696b8b4d72664d735bb6a7c88577687fd2ae33fd6964153") },

    { njs_str("var h = require('crypto').createHash('sha256');"
                 "h.update('abc'.repeat(100)).digest('hex')"),
      njs_str("d9f5aeb06abebb3be3f38adec9a2e3b94228d52193be923eb4e24c9b56ee0930") },

    { njs_str("var h = require('crypto').createHash()"),
      njs_str("TypeError: algorithm must be a string") },

    { njs_str("var h = require('crypto').createHash([])"),
      njs_str("TypeError: algorithm must be a string") },

    { njs_str("var h = require('crypto').createHash('sha512')"),
      njs_str("TypeError: not supported algorithm: \"sha512\"") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update()"),
      njs_str("TypeError: data must be a string") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update({})"),
      njs_str("TypeError: data must be a string") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('A').digest('latin1')"),
      njs_str("TypeError: Unknown digest encoding: \"latin1\"") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('A').digest('hex'); h.digest('hex')"),
      njs_str("Error: Digest already called") },

    { njs_str("var h = require('crypto').createHash('sha1');"
                 "h.update('A').digest('hex'); h.update('B')"),
      njs_str("Error: Digest already called") },

    { njs_str("typeof require('crypto').createHash('md5')"),
      njs_str("object") },

    /* require('crypto').createHmac() */

    { njs_str("var h = require('crypto').createHmac('sha1', '');"
              "[Object.prototype.toString.call(h), njs.dump(h),h]"),
      njs_str("[object Hmac],Hmac {},[object Hmac]") },

    { njs_str("var h = require('crypto').createHmac('md5', '');"
                 "h.digest('hex')"),
      njs_str("74e6f7298a9c2d168935f58c001bad88") },

    { njs_str("var h = require('crypto').createHmac('sha1', '');"
                 "h.digest('hex')"),
      njs_str("fbdb1d1b18aa6c08324b7d64b71fb76370690e1d") },

    { njs_str("var h = require('crypto').createHmac('sha1', '');"
                 "h.digest().toString('hex')"),
      njs_str("fbdb1d1b18aa6c08324b7d64b71fb76370690e1d") },

    { njs_str("var h = require('crypto').createHmac('sha1', '');"
              "var Hmac = h.constructor; "
              "Hmac('sha1', '').digest('hex')"),
      njs_str("fbdb1d1b18aa6c08324b7d64b71fb76370690e1d") },

    { njs_str("var h = require('crypto').createHmac('sha1', '');"
              "h.constructor.name"),
      njs_str("Hmac") },

    { njs_str("var h = require('crypto').createHmac('md5', 'secret key');"
                 "h.update('AB').digest('hex')"),
      njs_str("9c72728915eb26620a5caeafd0063b29") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('A').update('B').digest('hex')"),
      njs_str("adc60e03459c4bae7cf4eb6d9730003e9490b22f") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('AB').digest('hex')"),
      njs_str("adc60e03459c4bae7cf4eb6d9730003e9490b22f") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('AB').digest('base64')"),
      njs_str("rcYOA0WcS6589OttlzAAPpSQsi8=") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('AB').digest('base64url')"),
      njs_str("rcYOA0WcS6589OttlzAAPpSQsi8") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('AB').digest().toString('base64')"),
      njs_str("rcYOA0WcS6589OttlzAAPpSQsi8=") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('abc'.repeat(100)).digest('hex')"),
      njs_str("b105ad6921e4c54d3fa0a9ec3f7f0ee9bd2c659d") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'A'.repeat(40));"
                 "h.update('AB').digest('hex')"),
      njs_str("0b84f78ca5275d76d4b7dafb5845ee2b6a79c4c2") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'A'.repeat(64));"
                 "h.update('AB').digest('hex')"),
      njs_str("400ce530816c6b3247e2959f3982a12aaf58c0c9") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'A'.repeat(100));"
                 "h.update('AB').digest('hex')"),
      njs_str("670e7cdebae6392797e000e79e51d3b6589d8fad") },

    { njs_str("var h = require('crypto').createHmac('sha256', '');"
                 "h.digest('hex')"),
      njs_str("b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad") },

    { njs_str("var h = require('crypto').createHmac('sha256', 'secret key');"
                 "h.update('A').update('B').digest('hex')"),
      njs_str("46085184b3b45a13d838bf71a0ce03675dab30931e0f1f68fa636ea65fdb286d") },

    { njs_str("var h = require('crypto').createHmac('sha256', 'secret key');"
                 "h.update('AB').digest('hex')"),
      njs_str("46085184b3b45a13d838bf71a0ce03675dab30931e0f1f68fa636ea65fdb286d") },

    { njs_str("var h = require('crypto').createHmac('sha256', 'A'.repeat(64));"
                 "h.update('AB').digest('hex')"),
      njs_str("ee9dce43b12eb3e865614ad9c1a8d4fad4b6eac2b64647bd24cd192888d3f367") },

    { njs_str("var h = require('crypto').createHmac('sha256', 'A'.repeat(100));"
                 "h.update('AB').digest('hex')"),
      njs_str("5647b6c429701ff512f0f18232b4507065d2376ca8899a816a0a6e721bf8ddcc") },

    { njs_str("var h = require('crypto').createHmac('md5', 'secret key');"
                 "h.update('abc'.repeat(100)).digest('hex')"),
      njs_str("5dd706af43536f8c9c83e7ea55b1a5a2") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('abc'.repeat(100)).digest('hex')"),
      njs_str("b105ad6921e4c54d3fa0a9ec3f7f0ee9bd2c659d") },

    { njs_str("var h = require('crypto').createHmac('sha256', 'secret key');"
                 "h.update('abc'.repeat(100)).digest('hex')"),
      njs_str("f6550d398ce350ee8d94a0f44f2cf6b9bc8d316ae4625fb4434f22980a276bac") },

    { njs_str("var h = require('crypto').createHmac()"),
      njs_str("TypeError: algorithm must be a string") },

    { njs_str("var h = require('crypto').createHmac([])"),
      njs_str("TypeError: algorithm must be a string") },

    { njs_str("var h = require('crypto').createHmac('sha512', '')"),
      njs_str("TypeError: not supported algorithm: \"sha512\"") },

    { njs_str("var h = require('crypto').createHmac('sha1', [])"),
      njs_str("TypeError: key must be a string") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('A').digest('hex'); h.digest('hex')"),
      njs_str("Error: Digest already called") },

    { njs_str("var h = require('crypto').createHmac('sha1', 'secret key');"
                 "h.update('A').digest('hex'); h.update('B')"),
      njs_str("Error: Digest already called") },

    { njs_str("typeof require('crypto').createHmac('md5', 'a')"),
      njs_str("object") },

    /* setTimeout(). */

    { njs_str("setTimeout()"),
      njs_str("TypeError: too few arguments") },

    { njs_str("setTimeout(function(){})"),
      njs_str("InternalError: not supported by host environment") },

    { njs_str("setTimeout(function(){}, 12)"),
      njs_str("InternalError: not supported by host environment") },

    /* clearTimeout(). */

    { njs_str("clearTimeout()"),
      njs_str("undefined") },

    { njs_str("clearTimeout(123)"),
      njs_str("undefined") },

    /* Trick: number to boolean. */

    { njs_str("var a = 0; !!a"),
      njs_str("false") },

    { njs_str("var a = 5; !!a"),
      njs_str("true") },

    /* Trick: flooring. */

    { njs_str("var n = -10.12345; ~~n"),
      njs_str("-10") },

    { njs_str("var n = 10.12345; ~~n"),
      njs_str("10") },

    /* es5id: 8.2_A1_T1 */
    /* es5id: 8.2_A1_T2 */

    { njs_str("var x = null;"),
      njs_str("undefined") },

    /* es5id: 8.2_A2 */

    { njs_str("var null;"),
      njs_str("SyntaxError: Unexpected token \"null\" in 1") },

    /* es5id: 8.2_A3 */

    { njs_str("typeof(null) === \"object\""),
      njs_str("true") },

    /* Module. */

    { njs_str("import;"),
      njs_str("SyntaxError: Non-default import is not supported in 1") },

    { njs_str("import {x} from y"),
      njs_str("SyntaxError: Non-default import is not supported in 1") },

    { njs_str("import x from y"),
      njs_str("SyntaxError: Unexpected token \"y\" in 1") },

    { njs_str("import x from {"),
      njs_str("SyntaxError: Unexpected token \"{\" in 1") },

    { njs_str("import x from ''"),
      njs_str("SyntaxError: Cannot find module \"\" in 1") },

    { njs_str("import x from 'crypto'"),
      njs_str("undefined") },

    { njs_str("import x from 'crypto' 1"),
      njs_str("SyntaxError: Unexpected token \"1\" in 1") },

    { njs_str("if (1) {import x from 'crypto'}"),
      njs_str("SyntaxError: Illegal import statement in 1") },

    { njs_str("export"),
      njs_str("SyntaxError: Illegal export statement in 1") },

    { njs_str("Object.assign(undefined)"),
      njs_str("TypeError: cannot convert null or undefined to object") },

    { njs_str("Object.assign(null)"),
      njs_str("TypeError: cannot convert null or undefined to object") },

    { njs_str("Object.assign({x:123}).toString()"),
      njs_str("[object Object]") },

    { njs_str("Object.assign({x:123}).x"),
      njs_str("123") },

    { njs_str("Object.assign(true)"),
      njs_str("true") },

    { njs_str("Object.assign(123)"),
      njs_str("123") },

    { njs_str("var o1 = {a:1, b:1, c:1}; var o2 = {b:2, c:2}; "
                 "var o3 = {c:3}; var obj = Object.assign({}, o1, o2, o3); "
                 "Object.values(obj);"),
      njs_str("1,2,3") },

    { njs_str("var v1 = 'abc'; var v2 = true; var v3 = 10; "
                 "var obj = Object.assign({}, v1, null, v2, undefined, v3); "
                 "Object.values(obj);"),
      njs_str("a,b,c") },

    { njs_str("Object.assign(true, {a:123})"),
      njs_str("true") },

    { njs_str("Object.assign(true, {a:123}).a"),
      njs_str("123") },

    { njs_str("var y = Object.create({s:123}); y.z = 456;"
                 "Object.assign({}, y).s;"),
      njs_str("undefined") },

    { njs_str("var obj = {s:123}; Object.defineProperty(obj,"
                 "'p1', {value:12, enumerable:false});"
                 "Object.assign({}, obj).p1"),
      njs_str("undefined") },

    { njs_str("var obj = {s:123}; Object.defineProperty(obj,"
                 "'x', {value:12, writable:false});"
                 "Object.assign(obj, {x:4})"),
      njs_str("TypeError: Cannot assign to read-only property \"x\" of object") },

    { njs_str("var obj = {foo:1, get bar() {return 2;}};"
                 "var copy = Object.assign({}, obj);"
                 "Object.getOwnPropertyDescriptor(copy, 'bar').get"),
      njs_str("undefined") },

    { njs_str("try{var x = Object.defineProperty({}, 'foo',"
                 "{value:1, writable:false});"
                 "Object.assign(x, {bar:2}, {foo:2});}catch(error){};"
                 "x.bar"),
      njs_str("2") },

    { njs_str("var a = Object.defineProperty({}, 'a',"
                 "{get(){Object.defineProperty(this, 'b',"
                 "{value:2,enumerable:false});"
                 "return 1}, enumerable:1}); a.b =1;"
                 "var x = Object.assign({}, a);x.b;"),
      njs_str("undefined") },
};


static njs_unit_test_t  njs_denormals_test[] =
{
    { njs_str("2.2250738585072014e-308"),
      njs_str("2.2250738585072014e-308") },

#ifndef NJS_SUNC
    { njs_str("2.2250738585072014E-308.toString(2) == ('0.' + '0'.repeat(1021) + '1')"),
      njs_str("true") },

    { njs_str("Number('2.2250738585072014E-323')"),
      njs_str("2.5e-323") },

    { njs_str("Number('2.2250738585072014E-323') + 0"),
      njs_str("2.5e-323") },

    /* Smallest positive double (next_double(0)). */
    { njs_str("5E-324.toString(36) === '0.' + '0'.repeat(207) + '3'"),
      njs_str("true") },

    /* Maximum fraction length. */
    { njs_str("2.2250738585072014E-323.toString(2) == ('0.' + '0'.repeat(1071) + '101')"),
      njs_str("true") },

    /* Denormals. */
    { njs_str("var zeros = count => '0'.repeat(count);"
              "["
              "  [1.8858070859709815e-308, `0.${zeros(1022)}1101100011110111011100000100011001111101110001010111`],"
              // FIXME: "  [Number.MIN_VALUE, `0.${zeros(1073)}1`]"
              "  [-5.06631661953108e-309, `-0.${zeros(1024)}11101001001010000001101111010101011111111011010111`],"
              "  [6.22574126804e-313, `0.${zeros(1037)}11101010101101100111000110100111001`],"
              "  [-4e-323, `-0.${zeros(1070)}1`],"
              "].every(t=>t[0].toString(2) === t[1])"),
      njs_str("true") },

    { njs_str("4.94065645841246544176568792868e-324.toExponential()"),
      njs_str("5e-324") },

    { njs_str("4.94065645841246544176568792868e-324.toExponential(10)"),
      njs_str("4.9406564584e-324") },
#endif

};


static njs_unit_test_t  njs_disabled_denormals_test[] =
{
    { njs_str("Number('2.2250738585072014E-323')"),
      njs_str("0") },

    { njs_str("Number('2.2250738585072014E-323') + 0"),
      njs_str("0") },

    /* Smallest positive double (next_double(0)). */
    { njs_str("5E-324.toString(36)"),
      njs_str("0") },

    { njs_str("2.2250738585072014E-323.toString(2)"),
      njs_str("0") },

    /* Smallest normal double. */

    { njs_str("2.2250738585072014e-308"),
      njs_str("2.2250738585072014e-308") },

    { njs_str("2.2250738585072014e-308/2"),
      njs_str("0") },

    /* Denormals. */
    { njs_str("["
              "1.8858070859709815e-308,"
              "-5.06631661953108e-309,"
              "6.22574126804e-313,"
              "-4e-323,"
              "].map(v=>v.toString(2))"),
      njs_str("0,0,0,0") },
};


static njs_unit_test_t  njs_module_test[] =
{
    { njs_str("function f(){return 2}; var f; f()"),
      njs_str("SyntaxError: \"f\" has already been declared in 1") },

    { njs_str("function f(){return 2}; var f = 1; f()"),
      njs_str("SyntaxError: \"f\" has already been declared in 1") },

    { njs_str("function f(){return 1}; function f(){return 2}; f()"),
      njs_str("SyntaxError: \"f\" has already been declared in 1") },

    { njs_str("var f = 1; function f() {};"),
      njs_str("SyntaxError: \"f\" has already been declared in 1") },

    { njs_str("{ var f = 1; } function f() {};"),
      njs_str("SyntaxError: \"f\" has already been declared in 1") },
};


static njs_unit_test_t  njs_shared_test[] =
{
    { njs_str("var cr = require('crypto'); cr.createHash"),
      njs_str("[object Function]") },

    { njs_str("var cr = require('crypto'); cr.createHash('md5')"),
      njs_str("[object Hash]") },

    { njs_str("import cr from 'crypto'; cr.createHash"),
      njs_str("[object Function]") },

    { njs_str("import cr from 'crypto'; cr.createHash('md5')"),
      njs_str("[object Hash]") },

    { njs_str("isFinite()"),
      njs_str("false") },

    { njs_str("Number.isFinite(function(){})"),
      njs_str("false") },

    { njs_str("isFin()"),
      njs_str("ReferenceError: \"isFin\" is not defined in 1") },

    { njs_str("isNaN(function(){})"),
      njs_str("true") },

    { njs_str("var r; for (var i = 0; i < 2**10; i++) {r = $r.create('XXX').uri;}"),
      njs_str("undefined") },

    { njs_str("delete $r3.vars.p; $r3.vars.p"),
      njs_str("undefined") },

    { njs_str("var sr = $r.create('XXX'); sr.vars.p = 'a'; sr.vars.p"),
      njs_str("a") },

    { njs_str("$r.bind('XXX', 37); XXX"),
      njs_str("37") },
};


static njs_unit_test_t  njs_tz_test[] =
{
     { njs_str("var d = new Date(1); d = d + ''; d.slice(0, 33)"),
       njs_str("Thu Jan 01 1970 12:45:00 GMT+1245") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getTime()"),
       njs_str("1308895200000") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.valueOf()"),
       njs_str("1308895200000") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45);"
                  "d.toString().slice(0, 33)"),
       njs_str("Fri Jun 24 2011 18:45:00 GMT+1245") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45);"
                  "d.toTimeString().slice(0, 17)"),
       njs_str("18:45:00 GMT+1245") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.toUTCString()"),
       njs_str("Fri, 24 Jun 2011 06:00:00 GMT") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                  "d.toISOString()"),
       njs_str("2011-06-24T06:00:12.625Z") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getUTCHours()"),
       njs_str("6") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45); d.getUTCMinutes()"),
       njs_str("0") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                  "d.getTimezoneOffset()"),
       njs_str("-765") },

     { njs_str("var d = new Date(1308895323625); d.setMinutes(3, 2, 5003);"
                  "d.getTime()"),
       njs_str("1308892687003") },

     { njs_str("var d = new Date(1308895323625); d.setMinutes(3, 2);"
                  "d.getTime()"),
       njs_str("1308892682625") },

     { njs_str("var d = new Date(1308895323625); d.setMinutes(3);"
                  "d.getTime()"),
       njs_str("1308892683625") },

     { njs_str("var d = new Date(1308895323625); d.setHours(20, 3, 2, 5003);"
                  "d.getTime()"),
       njs_str("1308899887003") },

     { njs_str("var d = new Date(1308895323625); d.setHours(20, 3, 2);"
                  "d.getTime()"),
       njs_str("1308899882625") },

     { njs_str("var d = new Date(1308895323625); d.setHours(20, 3);"
                  "d.getTime()"),
       njs_str("1308899883625") },

     { njs_str("var d = new Date(1308895323625); d.setHours(20);"
                  "d.getTime()"),
       njs_str("1308902523625") },

     { njs_str("var d = new Date(NaN); d.setHours(20);"
                  "d.getTime()"),
       njs_str("NaN") },

     { njs_str("var d = new Date(1308895323625); d.setMonth(2, 10);"
                  "d.getTime()"),
       njs_str("1299733323625") },

     { njs_str("var d = new Date(1308895323625); d.setMonth(2);"
                  "d.getTime()"),
       njs_str("1300942923625") },

     { njs_str("var d = new Date(1308895323625); d.setFullYear(2010, 2, 10);"
                  "d.getTime()"),
       njs_str("1268197323625") },

     { njs_str("var d = new Date(1308895323625); d.setFullYear(2010, 2);"
                  "d.getTime()"),
       njs_str("1269406923625") },

     { njs_str("var d = new Date(2011, 5, 24, 18, 45, 12, 625);"
                  "d.toJSON(1)"),
       njs_str("2011-06-24T06:00:12.625Z") },

     { njs_str("Date.parse('1970-09-28T06:00:00.000')"),
       njs_str("23303700000") },

#if (NJS_TIME_T_SIZE == 8)

     { njs_str("var d = new Date(-1, 0);"
                  "d.toISOString()"),
       njs_str("-000002-12-31T11:47:00.000Z") },

     { njs_str("var d = new Date(-1, 0);"
                  "d.getTime()"),
       njs_str("-62198799180000") },

     { njs_str("var d = new Date(-1, 0);"
                  "d.getTimezoneOffset()"),
       njs_str("-733") },
#endif

     { njs_str("var d = new Date(1970, 6);"
                  "d.getTimezoneOffset()"),
       njs_str("-765") },
};


static njs_unit_test_t  njs_regexp_test[] =
{
    { njs_str("/[\\\\u02E0-\\\\u02E4]/"),
      njs_str("/[\\\\u02E0-\\\\u02E4]/") },

    { njs_str("/[\\u02E0-\\u02E4]/"),
      njs_str("/[\\u02E0-\\u02E4]/") },

    { njs_str("RegExp('[\\\\u02E0-\\\\u02E4]')"),
      njs_str("/[\\u02E0-\\u02E4]/") },

    { njs_str("/[\\u0430-\\u044f]+/.test('тест')"),
      njs_str("true") },

    { njs_str("RegExp('[\\\\u0430-\\\\u044f]+').test('тест')"),
      njs_str("true") },

    { njs_str("RegExp('[\\\\u0430-\\\\u044f]+').exec('тест')[0]"),
      njs_str("тест") },

    { njs_str("/[\\uFDE0-\\uFFFD]/g; export default 1"),
      njs_str("SyntaxError: Illegal export statement in 1") },

    { njs_str("RegExp(RegExp('\x00]]')).test('\x00]]')"),
      njs_str("true") },

    { njs_str("RegExp('\0').test('\0')"),
      njs_str("true") },

    { njs_str("RegExp('\x00').test('\0')"),
      njs_str("true") },

    { njs_str("RegExp('\x00\\\\x00').source"),
      njs_str("\\u0000\\x00") },

    { njs_str("/\\\0/"),
      njs_str("/\\\\u0000/") },

    { njs_str("RegExp('\\\\\\0').source"),
      njs_str("\\\\u0000") },

    { njs_str("RegExp('[\0]').test('\0')"),
      njs_str("true") },
};


typedef struct {
    njs_lvlhsh_t          hash;
    const njs_extern_t    *proto;
    njs_mp_t              *pool;

    uint32_t              a;
    njs_str_t             uri;

    njs_opaque_value_t    value;
} njs_unit_test_req_t;


typedef struct {
    njs_value_t           name;
    njs_value_t           value;
} njs_unit_test_prop_t;


static njs_int_t
lvlhsh_unit_test_key_test(njs_lvlhsh_query_t *lhq, void *data)
{
    njs_str_t             name;
    njs_unit_test_prop_t  *prop;

    prop = data;
    njs_string_get(&prop->name, &name);

    if (name.length != lhq->key.length) {
        return NJS_DECLINED;
    }

    if (memcmp(name.start, lhq->key.start, lhq->key.length) == 0) {
        return NJS_OK;
    }

    return NJS_DECLINED;
}


static void *
lvlhsh_unit_test_pool_alloc(void *pool, size_t size)
{
    return njs_mp_align(pool, size, size);
}


static void
lvlhsh_unit_test_pool_free(void *pool, void *p, size_t size)
{
    njs_mp_free(pool, p);
}


static const njs_lvlhsh_proto_t  lvlhsh_proto  njs_aligned(64) = {
    NJS_LVLHSH_LARGE_SLAB,
    lvlhsh_unit_test_key_test,
    lvlhsh_unit_test_pool_alloc,
    lvlhsh_unit_test_pool_free,
};


static njs_unit_test_prop_t *
lvlhsh_unit_test_alloc(njs_mp_t *pool, const njs_value_t *name,
    const njs_value_t *value)
{
    njs_unit_test_prop_t *prop;

    prop = njs_mp_alloc(pool, sizeof(njs_unit_test_prop_t));
    if (prop == NULL) {
        return NULL;
    }

    prop->name = *name;
    prop->value = *value;

    return prop;
}


static njs_int_t
lvlhsh_unit_test_add(njs_unit_test_req_t *r, njs_unit_test_prop_t *prop)
{
    njs_lvlhsh_query_t  lhq;

    njs_string_get(&prop->name, &lhq.key);
    lhq.key_hash = njs_djb_hash(lhq.key.start, lhq.key.length);

    lhq.replace = 1;
    lhq.value = (void *) prop;
    lhq.proto = &lvlhsh_proto;
    lhq.pool = r->pool;

    switch (njs_lvlhsh_insert(&r->hash, &lhq)) {

    case NJS_OK:
        return NJS_OK;

    case NJS_DECLINED:
    default:
        return NJS_ERROR;
    }
}


static njs_int_t
njs_unit_test_r_get_uri_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    char *p = obj;

    njs_str_t  *field;

    field = (njs_str_t *) (p + data);

    return njs_vm_value_string_set(vm, value, field->start, field->length);
}


static njs_int_t
njs_unit_test_r_set_uri_external(njs_vm_t *vm, void *obj, uintptr_t data,
    njs_str_t *value)
{
    char *p = obj;

    njs_str_t  *field;

    field = (njs_str_t *) (p + data);

    *field = *value;

    return NJS_OK;
}


static njs_int_t
njs_unit_test_r_get_a_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    u_char               buf[16], *p;
    njs_unit_test_req_t  *r;

    r = (njs_unit_test_req_t *) obj;

    p = njs_sprintf(buf, buf + njs_length(buf), "%uD", r->a);

    return njs_vm_value_string_set(vm, value, buf, p - buf);
}


static njs_int_t
njs_unit_test_r_get_b_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    njs_value_number_set(value, data);

    return NJS_OK;
}


static njs_int_t
njs_unit_test_host_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    return njs_vm_value_string_set(vm, value, (u_char *) "АБВГДЕЁЖЗИЙ", 22);
}


static njs_int_t
njs_unit_test_r_get_vars(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    njs_int_t             ret;
    njs_str_t             *key;
    njs_lvlhsh_query_t    lhq;
    njs_unit_test_req_t   *r;
    njs_unit_test_prop_t  *prop;

    r = (njs_unit_test_req_t *) obj;
    key = (njs_str_t *) data;

    lhq.key = *key;
    lhq.key_hash = njs_djb_hash(key->start, key->length);
    lhq.proto = &lvlhsh_proto;

    ret = njs_lvlhsh_find(&r->hash, &lhq);

    prop = lhq.value;

    if (ret == NJS_OK && njs_is_valid(&prop->value)) {
        *value = prop->value;
        return NJS_OK;
    }

    njs_value_undefined_set(value);

    return NJS_OK;
}


static njs_int_t
njs_unit_test_r_set_vars(njs_vm_t *vm, void *obj, uintptr_t data,
    njs_str_t *value)
{
    njs_int_t             ret;
    njs_str_t             *key;
    njs_value_t           name, val;
    njs_unit_test_req_t   *r;
    njs_unit_test_prop_t  *prop;

    r = (njs_unit_test_req_t *) obj;
    key = (njs_str_t *) data;

    if (key->length == 5 && memcmp(key->start, "error", 5) == 0) {
        njs_vm_error(vm, "cannot set \"error\" prop");
        return NJS_ERROR;
    }

    njs_vm_value_string_set(vm, &name, key->start, key->length);
    njs_vm_value_string_set(vm, &val, value->start, value->length);

    prop = lvlhsh_unit_test_alloc(vm->mem_pool, &name, &val);
    if (prop == NULL) {
        njs_memory_error(vm);
        return NJS_ERROR;
    }

    ret = lvlhsh_unit_test_add(r, prop);
    if (ret != NJS_OK) {
        njs_vm_error(vm, "lvlhsh_unit_test_add() failed");
        return NJS_ERROR;
    }

    return NJS_OK;
}


static njs_int_t
njs_unit_test_r_del_vars(njs_vm_t *vm, void *obj, uintptr_t data,
    njs_bool_t delete)
{
    njs_int_t             ret;
    njs_str_t             *key;
    njs_lvlhsh_query_t    lhq;
    njs_unit_test_req_t   *r;
    njs_unit_test_prop_t  *prop;

    r = (njs_unit_test_req_t *) obj;
    key = (njs_str_t *) data;

    if (key->length == 5 && memcmp(key->start, "error", 5) == 0) {
        njs_vm_error(vm, "cannot delete \"error\" prop");
        return NJS_ERROR;
    }

    lhq.key = *key;
    lhq.key_hash = njs_djb_hash(key->start, key->length);
    lhq.proto = &lvlhsh_proto;

    ret = njs_lvlhsh_find(&r->hash, &lhq);

    prop = lhq.value;

    if (ret == NJS_OK) {
        njs_set_invalid(&prop->value);
    }

    return NJS_OK;
}


static njs_int_t
njs_unit_test_header_external(njs_vm_t *vm, njs_value_t *value, void *obj,
    uintptr_t data)
{
    u_char     *p;
    uint32_t   size;
    njs_str_t  *h;

    h = (njs_str_t *) data;

    size = 7 + h->length;

    p = njs_vm_value_string_alloc(vm, value, size);
    if (p == NULL) {
        return NJS_ERROR;
    }

    p = njs_cpymem(p, h->start, h->length);
    *p++ = '|';
    memcpy(p, "АБВ", 6);

    return NJS_OK;
}


static njs_int_t
njs_unit_test_header_keys_external(njs_vm_t *vm, void *obj, njs_value_t *keys)
{
    njs_int_t    rc, i;
    njs_value_t  *value;
    u_char       k[2];

    rc = njs_vm_array_alloc(vm, keys, 4);
    if (rc != NJS_OK) {
        return NJS_ERROR;
    }

    k[0] = '0';
    k[1] = '1';

    for (i = 0; i < 3; i++) {
        value = njs_vm_array_push(vm, keys);
        if (value == NULL) {
            return NJS_ERROR;
        }

        (void) njs_vm_value_string_set(vm, value, k, 2);

        k[1]++;
    }

    return NJS_OK;
}


static njs_int_t
njs_unit_test_method_external(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_int_t            ret;
    njs_str_t            s;
    njs_unit_test_req_t  *r;

    r = njs_vm_external(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(r == NULL)) {
        return NJS_ERROR;
    }

    ret = njs_vm_value_to_string(vm, &s, njs_arg(args, nargs, 1));
    if (ret == NJS_OK && s.length == 3 && memcmp(s.start, "YES", 3) == 0) {
        return njs_vm_value_string_set(vm, njs_vm_retval(vm), r->uri.start,
                                       r->uri.length);
    }

    njs_set_undefined(&vm->retval);

    return NJS_OK;
}


static njs_int_t
njs_unit_test_create_external(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_int_t            ret;
    njs_str_t            uri;
    njs_value_t          *value;
    njs_unit_test_req_t  *r, *sr;

    r = njs_vm_external(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(r == NULL)) {
        return NJS_ERROR;
    }

    if (njs_vm_value_to_string(vm, &uri, njs_arg(args, nargs, 1)) != NJS_OK) {
        return NJS_ERROR;
    }

    value = njs_mp_zalloc(r->pool, sizeof(njs_opaque_value_t));
    if (value == NULL) {
        goto memory_error;
    }

    sr = njs_mp_zalloc(r->pool, sizeof(njs_unit_test_req_t));
    if (sr == NULL) {
        goto memory_error;
    }

    sr->uri = uri;
    sr->pool = r->pool;
    sr->proto = r->proto;

    ret = njs_vm_external_create(vm, value, sr->proto, sr);
    if (ret != NJS_OK) {
        return NJS_ERROR;
    }

    njs_vm_retval_set(vm, value);

    return NJS_OK;

memory_error:

    njs_memory_error(vm);

    return NJS_ERROR;
}


static njs_int_t
njs_unit_test_bind_external(njs_vm_t *vm, njs_value_t *args, njs_uint_t nargs,
    njs_index_t unused)
{
    njs_str_t            name;
    njs_unit_test_req_t  *r;

    r = njs_vm_external(vm, njs_arg(args, nargs, 0));
    if (njs_slow_path(r == NULL)) {
        return NJS_ERROR;
    }

    if (njs_vm_value_to_string(vm, &name, njs_arg(args, nargs, 1)) != NJS_OK) {
        return NJS_ERROR;
    }

    return njs_vm_bind(vm, &name, njs_arg(args, nargs, 2), 0);
}


static njs_external_t  njs_unit_test_r_props[] = {

    { njs_str("a"),
      NJS_EXTERN_PROPERTY,
      NULL,
      0,
      njs_unit_test_r_get_a_external,
      NULL,
      NULL,
      NULL,
      NULL,
      0 },

    { njs_str("b"),
      NJS_EXTERN_PROPERTY,
      NULL,
      0,
      njs_unit_test_r_get_b_external,
      NULL,
      NULL,
      NULL,
      NULL,
      42 },
};


static njs_external_t  njs_unit_test_r_external[] = {

    { njs_str("uri"),
      NJS_EXTERN_PROPERTY,
      NULL,
      0,
      njs_unit_test_r_get_uri_external,
      njs_unit_test_r_set_uri_external,
      NULL,
      NULL,
      NULL,
      offsetof(njs_unit_test_req_t, uri) },

    { njs_str("host"),
      NJS_EXTERN_PROPERTY,
      NULL,
      0,
      njs_unit_test_host_external,
      NULL,
      NULL,
      NULL,
      NULL,
      0 },

    { njs_str("props"),
      NJS_EXTERN_OBJECT,
      njs_unit_test_r_props,
      njs_nitems(njs_unit_test_r_props),
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      0 },

    { njs_str("vars"),
      NJS_EXTERN_OBJECT,
      NULL,
      0,
      njs_unit_test_r_get_vars,
      njs_unit_test_r_set_vars,
      njs_unit_test_r_del_vars,
      NULL,
      NULL,
      0 },

    { njs_str("consts"),
      NJS_EXTERN_OBJECT,
      NULL,
      0,
      njs_unit_test_r_get_vars,
      NULL,
      NULL,
      NULL,
      NULL,
      0 },

    { njs_str("header"),
      NJS_EXTERN_OBJECT,
      NULL,
      0,
      njs_unit_test_header_external,
      NULL,
      NULL,
      njs_unit_test_header_keys_external,
      NULL,
      0 },

    { njs_str("some_method"),
      NJS_EXTERN_METHOD,
      NULL,
      0,
      NULL,
      NULL,
      NULL,
      NULL,
      njs_unit_test_method_external,
      0 },

    { njs_str("create"),
      NJS_EXTERN_METHOD,
      NULL,
      0,
      NULL,
      NULL,
      NULL,
      NULL,
      njs_unit_test_create_external,
      0 },

    { njs_str("bind"),
      NJS_EXTERN_METHOD,
      NULL,
      0,
      NULL,
      NULL,
      NULL,
      NULL,
      njs_unit_test_bind_external,
      0 },

};


static njs_external_t  njs_test_external[] = {

    { njs_str("request.proto"),
      NJS_EXTERN_OBJECT,
      njs_unit_test_r_external,
      njs_nitems(njs_unit_test_r_external),
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      0 },

};


typedef struct {
    njs_str_t            name;
    njs_unit_test_req_t  request;
    njs_unit_test_prop_t props[2];
} njs_unit_test_req_t_init_t;


static const njs_unit_test_req_t_init_t njs_test_requests[] = {

    { njs_str("$r"),
     {
         .uri = njs_str("АБВ"),
         .a = 1
     },
     {
         { njs_string("p"), njs_string("pval") },
         { njs_string("p2"), njs_string("p2val") },
     }
    },

    { njs_str("$r2"),
     {
         .uri = njs_str("αβγ"),
         .a = 2
     },
     {
         { njs_string("q"), njs_string("qval") },
         { njs_string("q2"), njs_string("q2val") },
     }
    },

    { njs_str("$r3"),
     {
         .uri = njs_str("abc"),
         .a = 3
     },
     {
         { njs_string("k"), njs_string("kval") },
         { njs_string("k2"), njs_string("k2val") },
     }
    },
};


static njs_int_t
njs_externals_init(njs_vm_t *vm)
{
    njs_int_t             ret;
    njs_uint_t            i, j;
    const njs_extern_t    *proto;
    njs_unit_test_req_t   *requests;
    njs_unit_test_prop_t  *prop;

    proto = njs_vm_external_prototype(vm, &njs_test_external[0]);
    if (njs_slow_path(proto == NULL)) {
        njs_printf("njs_vm_external_prototype() failed\n");
        return NJS_ERROR;
    }

    requests = njs_mp_zalloc(vm->mem_pool, njs_nitems(njs_test_requests)
                             * sizeof(njs_unit_test_req_t));
    if (njs_slow_path(requests == NULL)) {
        return NJS_ERROR;
    }

    for (i = 0; i < njs_nitems(njs_test_requests); i++) {

        requests[i] = njs_test_requests[i].request;
        requests[i].pool = vm->mem_pool;
        requests[i].proto = proto;

        ret = njs_vm_external_create(vm, njs_value_arg(&requests[i].value),
                                     proto, &requests[i]);
        if (njs_slow_path(ret != NJS_OK)) {
            njs_printf("njs_vm_external_create() failed\n");
            return NJS_ERROR;
        }

        ret = njs_vm_bind(vm, &njs_test_requests[i].name,
                          njs_value_arg(&requests[i].value), 1);
        if (njs_slow_path(ret != NJS_OK)) {
            njs_printf("njs_vm_bind() failed\n");
            return NJS_ERROR;
        }

        for (j = 0; j < njs_nitems(njs_test_requests[i].props); j++) {
            prop = lvlhsh_unit_test_alloc(vm->mem_pool,
                                          &njs_test_requests[i].props[j].name,
                                          &njs_test_requests[i].props[j].value);

            if (njs_slow_path(prop == NULL)) {
                njs_printf("lvlhsh_unit_test_alloc() failed\n");
                return NJS_ERROR;
            }

            ret = lvlhsh_unit_test_add(&requests[i], prop);
            if (njs_slow_path(ret != NJS_OK)) {
                njs_printf("lvlhsh_unit_test_add() failed\n");
                return NJS_ERROR;
            }
        }
    }

    return NJS_OK;
}


typedef struct {
    njs_bool_t  disassemble;
    njs_bool_t  verbose;
    njs_bool_t  unsafe;
    njs_bool_t  module;
    njs_uint_t  repeat;
} njs_opts_t;


typedef struct {
    njs_uint_t  passed;
    njs_uint_t  failed;
} njs_stat_t;


static void
njs_unit_test_report(const char *msg, njs_stat_t *prev, njs_stat_t *current)
{
    njs_stat_t  stat;

    stat.failed = current->failed - prev->failed;
    stat.passed = current->passed - prev->passed;

    njs_printf("%s: %s [%d/%d]\n", msg, stat.failed ? "FAILED" : "PASSED",
               stat.passed, stat.passed + stat.failed);
}


static njs_int_t
njs_unit_test(njs_unit_test_t tests[], size_t num, const char *name,
    njs_opts_t *opts, njs_stat_t *stat)
{
    u_char        *start;
    njs_vm_t      *vm, *nvm;
    njs_int_t     ret;
    njs_str_t     s;
    njs_uint_t    i, repeat;
    njs_stat_t    prev;
    njs_bool_t    success;
    njs_vm_opt_t  options;

    vm = NULL;
    nvm = NULL;

    prev = *stat;

    ret = NJS_ERROR;

    for (i = 0; i < num; i++) {

        if (opts->verbose) {
            njs_printf("\"%V\"\n", &tests[i].script);
        }

        njs_memzero(&options, sizeof(njs_vm_opt_t));

        options.module = opts->module;
        options.unsafe = opts->unsafe;

        vm = njs_vm_create(&options);
        if (vm == NULL) {
            njs_printf("njs_vm_create() failed\n");
            goto done;
        }

        ret = njs_externals_init(vm);
        if (ret != NJS_OK) {
            goto done;
        }

        start = tests[i].script.start;

        ret = njs_vm_compile(vm, &start, start + tests[i].script.length);

        if (ret == NJS_OK) {
            if (opts->disassemble) {
                njs_disassembler(vm);
            }

            repeat = opts->repeat;

            do {
                if (nvm != NULL) {
                    njs_vm_destroy(nvm);
                }

                nvm = njs_vm_clone(vm, NULL);
                if (nvm == NULL) {
                    njs_printf("njs_vm_clone() failed\n");
                    goto done;
                }

                ret = njs_vm_start(nvm);
            } while (--repeat != 0);

            if (njs_vm_retval_string(nvm, &s) != NJS_OK) {
                njs_printf("njs_vm_retval_string() failed\n");
                goto done;
            }

        } else {
            if (njs_vm_retval_string(vm, &s) != NJS_OK) {
                njs_printf("njs_vm_retval_string() failed\n");
                goto done;
            }
        }

        success = njs_strstr_eq(&tests[i].ret, &s);

        if (!success) {
            njs_printf("njs(\"%V\")\nexpected: \"%V\"\n     got: \"%V\"\n",
                       &tests[i].script, &tests[i].ret, &s);

            stat->failed++;

        } else {
            stat->passed++;
        }

        if (nvm != NULL) {
            njs_vm_destroy(nvm);
            nvm = NULL;
        }

        njs_vm_destroy(vm);
        vm = NULL;
    }

    ret = NJS_OK;

done:

    if (nvm != NULL) {
        njs_vm_destroy(nvm);
    }

    if (vm != NULL) {
        njs_vm_destroy(vm);
    }

    njs_unit_test_report(name, &prev, stat);

    return ret;
}


static njs_int_t
njs_timezone_optional_test(njs_opts_t *opts, njs_stat_t *stat)
{
    size_t     size;
    u_char     buf[16];
    time_t     clock;
    struct tm  tm;
    njs_int_t  ret;

    /*
     * Chatham Islands NZ-CHAT time zone.
     * Standard time: UTC+12:45, Daylight Saving time: UTC+13:45.
     */
    (void) putenv((char *) "TZ=Pacific/Chatham");
    tzset();

    clock = 0;
    localtime_r(&clock, &tm);

    size = strftime((char *) buf, sizeof(buf), "%z", &tm);

    if (memcmp(buf, "+1245", size) == 0) {
        ret = njs_unit_test(njs_tz_test, njs_nitems(njs_tz_test),
                            "timezone tests", opts, stat);
        if (ret != NJS_OK) {
            return ret;
        }

    } else {
        njs_printf("njs timezone tests skipped, timezone is unavailable\n");
    }

    return NJS_OK;
}


static njs_int_t
njs_regexp_optional_test(njs_opts_t *opts, njs_stat_t *stat)
{
    int         erroff;
    pcre        *re1, *re2;
    njs_int_t   ret;
    const char  *errstr;

    /*
     * pcre-8.21 crashes when it compiles unicode escape codes inside
     * square brackets when PCRE_UTF8 option is provided.
     * Catching it in runtime by compiling it without PCRE_UTF8. Normally it
     * should return NULL and "character value in \u.... sequence is too large"
     * error string.
     */
    re1 = pcre_compile("/[\\u0410]/", PCRE_JAVASCRIPT_COMPAT, &errstr, &erroff,
                      NULL);

    /*
     * pcre-7.8 fails to compile unicode escape codes inside square brackets
     * even when PCRE_UTF8 option is provided.
     */
    re2 = pcre_compile("/[\\u0410]/", PCRE_JAVASCRIPT_COMPAT | PCRE_UTF8,
                       &errstr, &erroff, NULL);

    if (re1 == NULL && re2 != NULL) {
        ret = njs_unit_test(njs_regexp_test, njs_nitems(njs_regexp_test),
                            "unicode regexp tests", opts, stat);
        if (ret != NJS_OK) {
            return ret;
        }

    } else {
        njs_printf("njs unicode regexp tests skipped, libpcre fails\n");
    }

    if (re1 != NULL) {
        pcre_free(re1);
    }

    if (re2 != NULL) {
        pcre_free(re2);
    }

    return NJS_OK;
}


static njs_int_t
njs_vm_json_test(njs_opts_t *opts, njs_stat_t *stat)
{
    njs_vm_t      *vm;
    njs_int_t     ret;
    njs_str_t     s, *script;
    njs_uint_t    i;
    njs_bool_t    success;
    njs_stat_t    prev;
    njs_value_t   args[3];
    njs_vm_opt_t  options;

    static const njs_str_t fname = njs_str("replacer");
    static const njs_str_t iname = njs_str("indent");

    static njs_unit_test_t tests[] = {
        { njs_str("'[1, true, \"x\", {\"a\": {}}]'"),
          njs_str("[1,true,\"x\",{\"a\":{}}]") },
        { njs_str("'{\"a\":{\"b\":1}}'"),
          njs_str("{\"a\":{\"b\":1}}") },
        { njs_str("'[[[],{}]]'"),
          njs_str("[[[],{}]]") },
        { njs_str("var indent = 1; '[]'"),
          njs_str("[\n \n]") },
        { njs_str("function replacer(k, v) {return v}; '{\"a\":{\"b\":1}}'"),
          njs_str("{\"a\":{\"b\":1}}") },
        { njs_str("function replacer(k, v) {"
                     "   return (typeof v === 'string') ? undefined : v};"
                     "'{\"a\":1, \"b\":\"x\"}'"),
          njs_str("{\"a\":1}") },
    };

    vm = NULL;

    prev = *stat;

    ret = NJS_ERROR;

    for (i = 0; i < njs_nitems(tests); i++) {

        memset(&options, 0, sizeof(njs_vm_opt_t));
        options.init = 1;

        vm = njs_vm_create(&options);
        if (vm == NULL) {
            njs_printf("njs_vm_create() failed\n");
            goto done;
        }

        script = &tests[i].script;

        ret = njs_vm_compile(vm, &script->start,
                             script->start + script->length);

        if (ret != NJS_OK) {
            njs_printf("njs_vm_compile() failed\n");
            goto done;
        }

        ret = njs_vm_start(vm);
        if (ret != NJS_OK) {
            njs_printf("njs_vm_run() failed\n");
            goto done;
        }

        args[0] = *njs_vm_retval(vm);

        ret = njs_vm_json_parse(vm, args, 1);
        if (ret != NJS_OK) {
            njs_printf("njs_vm_json_parse() failed\n");
            goto done;
        }

        args[0] = vm->retval;
        args[1] = *njs_vm_value(vm, &fname);
        args[2] = *njs_vm_value(vm, &iname);

        ret = njs_vm_json_stringify(vm, args, 3);
        if (ret != NJS_OK) {
            njs_printf("njs_vm_json_stringify() failed\n");
            goto done;
        }

        if (njs_vm_retval_string(vm, &s) != NJS_OK) {
            njs_printf("njs_vm_retval_string() failed\n");
            goto done;
        }

        success = njs_strstr_eq(&tests[i].ret, &s);

        if (!success) {
            njs_printf("njs_vm_json_test(\"%V\")\n"
                       "expected: \"%V\"\n     got: \"%V\"\n", script,
                       &tests[i].ret, &s);

            stat->failed++;

        } else {
            stat->passed++;
        }

        njs_vm_destroy(vm);
        vm = NULL;

    }

    ret = NJS_OK;

done:

    if (ret != NJS_OK) {
        if (njs_vm_retval_string(vm, &s) != NJS_OK) {
            njs_printf("njs_vm_retval_string() failed\n");

        } else {
            njs_printf("%V\n", &s);
        }
    }

    njs_unit_test_report("VM json API tests", &prev, stat);

    if (vm != NULL) {
        njs_vm_destroy(vm);
    }

    return ret;
}


static njs_int_t
njs_vm_object_alloc_test(njs_vm_t *vm, njs_opts_t *opts, njs_stat_t *stat)
{
    njs_int_t    ret;
    njs_value_t  args[2], obj;

    static const njs_value_t num_key = njs_string("num");
    static const njs_value_t bool_key = njs_string("bool");

    njs_value_number_set(njs_argument(&args, 0), 1);
    njs_value_boolean_set(njs_argument(&args, 1), 0);

    ret = njs_vm_object_alloc(vm, &obj, NULL);
    if (ret != NJS_OK) {
        return NJS_ERROR;
    }

    ret = njs_vm_object_alloc(vm, &obj, &num_key, NULL);
    if (ret == NJS_OK) {
        return NJS_ERROR;
    }

    ret = njs_vm_object_alloc(vm, &obj, &num_key, &args[0], NULL);
    if (ret != NJS_OK) {
        return NJS_ERROR;
    }

    ret = njs_vm_object_alloc(vm, &obj, &num_key, &args[0], &bool_key,
                              &args[1], NULL);
    if (ret != NJS_OK) {
        stat->failed++;
        return NJS_OK;
    }

    stat->passed++;

    return NJS_OK;
}


static njs_int_t
njs_file_basename_test(njs_vm_t *vm, njs_opts_t *opts, njs_stat_t *stat)
{
    njs_str_t   name;
    njs_bool_t  success;
    njs_uint_t  i;

    static const struct {
        njs_str_t   path;
        njs_str_t   expected;
    } tests[] = {
        { njs_str(""),            njs_str("") },
        { njs_str("/"),           njs_str("") },
        { njs_str("/a"),          njs_str("a") },
        { njs_str("///"),         njs_str("") },
        { njs_str("///a"),        njs_str("a") },
        { njs_str("///a/"),       njs_str("") },
        { njs_str("a"),           njs_str("a") },
        { njs_str("a/"),          njs_str("") },
        { njs_str("a//"),         njs_str("") },
        { njs_str("path/name"),   njs_str("name") },
        { njs_str("/path/name"),  njs_str("name") },
        { njs_str("/path/name/"), njs_str("") },
    };

    for (i = 0; i < njs_nitems(tests); i++) {
        njs_file_basename(&tests[i].path, &name);

        success = njs_strstr_eq(&tests[i].expected, &name);

        if (!success) {
            njs_printf("njs_file_basename_test(\"%V\"):\n"
                       "expected: \"%V\"\n     got: \"%V\"\n",
                       &tests[i].path, &tests[i].expected, &name);

            stat->failed++;

        } else {
            stat->passed++;
        }
    }

    return NJS_OK;
}


static njs_int_t
njs_file_dirname_test(njs_vm_t *vm, njs_opts_t *opts, njs_stat_t *stat)
{
    njs_str_t   name;
    njs_bool_t  success;
    njs_uint_t  i;

    static const struct {
        njs_str_t   path;
        njs_str_t   expected;
    } tests[] = {
        { njs_str(""),               njs_str(".") },
        { njs_str("/"),              njs_str("/") },
        { njs_str("/a"),             njs_str("/") },
        { njs_str("///"),            njs_str("///") },
        { njs_str("///a"),           njs_str("///") },
        { njs_str("///a/"),          njs_str("///a") },
        { njs_str("a"),              njs_str(".") },
        { njs_str("a/"),             njs_str("a") },
        { njs_str("a//"),            njs_str("a") },
        { njs_str("p1/p2/name"),     njs_str("p1/p2") },
        { njs_str("/p1/p2/name"),    njs_str("/p1/p2") },
        { njs_str("/p1/p2///name"),  njs_str("/p1/p2") },
        { njs_str("/p1/p2/name/"),   njs_str("/p1/p2/name") },
    };

    for (i = 0; i < njs_nitems(tests); i++) {
        njs_file_dirname(&tests[i].path, &name);

        success = njs_strstr_eq(&tests[i].expected, &name);

        if (!success) {
            njs_printf("njs_file_dirname_test(\"%V\"):\n"
                       "expected: \"%V\"\n     got: \"%V\"\n",
                       &tests[i].path, &tests[i].expected, &name);

            stat->failed++;
        } else {
            stat->passed++;
        }

    }

    return NJS_OK;
}


static njs_int_t
njs_chb_test(njs_vm_t *vm, njs_opts_t *opts, njs_stat_t *stat)
{
    u_char     *p;
    njs_int_t  ret, i;
    njs_chb_t  chain;
    njs_str_t  string, arg;

    static const njs_str_t  expected = njs_str("arg: \"XYZ\" -5");

    njs_chb_init(&chain, vm->mem_pool);

    p = njs_chb_reserve(&chain, 513);
    if (p == NULL) {
        ret = NJS_ERROR;
        njs_printf("njs_chb_reserve() failed\n");
        goto done;
    }

    njs_memset(p, 'Z', 256);

    njs_chb_written(&chain, 256);

    for (i = 0; i < 768; i++) {
        njs_chb_append(&chain, "X", 1);
    }

    ret = njs_chb_join(&chain, &string);
    if (ret != NJS_OK) {
        njs_printf("njs_chb_join() failed\n");
        goto done;
    }

    if (string.length != 1024 || njs_chb_size(&chain) != 1024) {
        ret = NJS_ERROR;
        njs_printf("njs_chb_join() corrupts "
                   "string.length:%z njs_chb_size(&chain):%z != 1024\n",
                   string.length, njs_chb_size(&chain));
        goto done;
    }

    for (i = 0; i < 1024; i++) {
        if (string.start[i] != ((i < 256) ? 'Z' : 'X')) {
            ret = NJS_ERROR;
            njs_printf("njs_chb_join() corrupts string[%i]:%c != '%c'\n",
                       i, string.start[i], (i < 256) ? 'Z' : 'X');
            goto done;
        }
    }

    njs_mp_free(vm->mem_pool, string.start);

    for (i = 0; i < 222; i++) {;
        njs_chb_drain(&chain, 3);
    }

    ret = njs_chb_join(&chain, &string);
    if (ret != NJS_OK) {
        njs_printf("njs_chb_join() failed\n");
        goto done;
    }

    if (string.length != 358 || njs_chb_size(&chain) != 358) {
        ret = NJS_ERROR;
        njs_printf("njs_chb_join() corrupts "
                   "string.length:%z njs_chb_size(&chain):%z != 358\n",
                   string.length, njs_chb_size(&chain));
        goto done;
    }

    for (i = 0; i < 358; i++) {
        if (string.start[i] != 'X') {
            ret = NJS_ERROR;
            njs_printf("njs_chb_join() corrupts string[%i]:%c != 'X'\n",
                       i, string.start[i]);
            goto done;
        }
    }

    for (i = 0; i < 512; i++) {
        njs_chb_append(&chain, "ABC", 3);
    }

    for (i = 0; i < 447; i++) {
        njs_chb_drop(&chain, 2);
    }

    ret = njs_chb_join(&chain, &string);
    if (ret != NJS_OK) {
        njs_printf("njs_chb_join() failed\n");
        goto done;
    }

    if (string.length != 1000 || njs_chb_size(&chain) != 1000) {
        ret = NJS_ERROR;
        njs_printf("njs_chb_join() corrupts "
                   "string.length:%z njs_chb_size(&chain):%z != 1000\n",
                   string.length, njs_chb_size(&chain));
        goto done;
    }

    njs_chb_drop(&chain, 500);
    njs_chb_drain(&chain, 501);

    if (njs_chb_size(&chain) != 0) {
        ret = NJS_ERROR;
        njs_printf("njs_chb_drop() corrupts "
                   "njs_chb_size(&chain):%z != 0\n", njs_chb_size(&chain));
        goto done;
    }

    arg = njs_str_value("XYZ");

    njs_chb_sprintf(&chain, 32, "arg: \"%V\" %d", &arg, -5);

    ret = njs_chb_join(&chain, &string);
    if (ret != NJS_OK) {
        njs_printf("njs_chb_join() failed\n");
        goto done;
    }

    if (!njs_strstr_eq(&string, &expected)) {
        ret = NJS_ERROR;
        njs_printf("njs_chb_sprintf() corrupts \"%V\" != \"%V\"\n", &string,
                   &expected);
        goto done;
    }

    njs_chb_destroy(&chain);
    njs_mp_free(vm->mem_pool, string.start);

done:

    if (ret != NJS_OK) {
        stat->failed++;
        return NJS_OK;
    }

    stat->passed++;

    return NJS_OK;
}


static njs_int_t
njs_api_test(njs_opts_t *opts, njs_stat_t *stat)
{
    njs_vm_t      *vm;
    njs_int_t     ret;
    njs_uint_t    i;
    njs_stat_t    prev;
    njs_vm_opt_t  options;

    static const struct {
        njs_int_t  (*test)(njs_vm_t *, njs_opts_t *, njs_stat_t *stat);
        njs_str_t  name;
    } tests[] = {
        { njs_vm_object_alloc_test,
          njs_str("njs_vm_object_alloc_test") },
        { njs_file_basename_test,
          njs_str("njs_file_basename_test") },
        { njs_file_dirname_test,
          njs_str("njs_file_dirname_test") },
        { njs_chb_test,
          njs_str("njs_chb_test") },
    };

    vm = NULL;
    njs_memzero(&options, sizeof(njs_vm_opt_t));

    prev = *stat;

    ret = NJS_ERROR;

    for (i = 0; i < njs_nitems(tests); i++) {
        vm = njs_vm_create(&options);
        if (vm == NULL) {
            njs_printf("njs_vm_create() failed\n");
            goto done;
        }

        ret = tests[i].test(vm, opts, stat);
        if (njs_slow_path(ret != NJS_OK)) {
            njs_printf("njs_api_test: \"%V\" test failed\n", &tests[i].name);
            goto done;
        }

        njs_vm_destroy(vm);
        vm = NULL;
    }

    ret = NJS_OK;

done:

    njs_unit_test_report("API tests", &prev, stat);

    if (vm != NULL) {
        njs_vm_destroy(vm);
    }

    return ret;
}


int njs_cdecl
main(int argc, char **argv)
{
    njs_int_t   ret;
    njs_opts_t  opts;
    njs_stat_t  stat;

    njs_memzero(&opts, sizeof(njs_opts_t));

    if (argc > 1) {
        switch (argv[1][0]) {

        case 'd':
            opts.disassemble = 1;
            break;

        case 'v':
            opts.verbose = 1;
            break;

        default:
            break;
        }
    }

    (void) putenv((char *) "TZ=UTC");
    tzset();

    njs_memzero(&stat, sizeof(njs_stat_t));

    opts.repeat = 1;
    opts.unsafe = 1;

    njs_mm_denormals(1);

    ret = njs_unit_test(njs_test, njs_nitems(njs_test), "script tests",
                        &opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    ret = njs_unit_test(njs_denormals_test, njs_nitems(njs_denormals_test),
                        "denormals tests", &opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

#if (NJS_HAVE_DENORMALS_CONTROL)

    njs_mm_denormals(0);

    ret = njs_unit_test(njs_test, njs_nitems(njs_test),
                        "script tests (disabled denormals)", &opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    ret = njs_unit_test(njs_disabled_denormals_test,
                        njs_nitems(njs_disabled_denormals_test),
                        "disabled denormals tests", &opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    njs_mm_denormals(1);

#else

    (void) njs_disabled_denormals_test;

#endif

    ret = njs_timezone_optional_test(&opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    ret = njs_regexp_optional_test(&opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    ret = njs_vm_json_test(&opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    ret = njs_api_test(&opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    opts.module = 1;

    ret = njs_unit_test(njs_module_test, njs_nitems(njs_module_test),
                        "module tests", &opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    opts.module = 0;
    opts.repeat = 128;

    ret = njs_unit_test(njs_shared_test, njs_nitems(njs_shared_test),
                        "shared tests", &opts, &stat);
    if (ret != NJS_OK) {
        return ret;
    }

    njs_printf("TOTAL: %s [%ui/%ui]\n", stat.failed ? "FAILED" : "PASSED",
               stat.passed, stat.passed + stat.failed);

    return stat.failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
