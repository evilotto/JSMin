/* jsminlib.c
   2013-03-29

This is JSMin, reworked as a reentrant function.

Original copyright:

Copyright (c) 2002 Douglas Crockford  (www.crockford.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jsminlib.h"

typedef struct _jsminCtx {
	int   theA;
	int   theB;
	int   theLookahead;
	int   theX;
	int   theY;
	void *clientData;
	jsminRead  *input;
	jsminWrite *output;
	jsminWrite *error;
} jsminCtx;

static void
error(jsminCtx *ctx, char* s)
{
    ctx->error(ctx->clientData, "JSMIN Error: ", 13);
	ctx->error(ctx->clientData, s, strlen(s));
	ctx->error(ctx->clientData, "\n", 1);
}

/* isAlphanum -- return true if the character is a letter, digit, underscore,
        dollar sign, or non-ASCII character.
*/

static int
isAlphanum(int c)
{
    return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c == '\\' ||
        c > 126);
}


/* get -- return the next character from stdin. Watch out for lookahead. If
        the character is a control character, translate it to a space or
        linefeed.
*/

static int
get(jsminCtx *ctx)
{
    int c = ctx->theLookahead;
    ctx->theLookahead = EOF;
    if (c == EOF) {
		c=ctx->input(ctx->clientData);
    }
    if (c >= ' ' || c == '\n' || c == EOF) {
        return c;
    }
    if (c == '\r') {
        return '\n';
    }
    return ' ';
}


/* peek -- get the next character without getting it.
*/

static int
peek(jsminCtx *ctx)
{
    ctx->theLookahead = get(ctx);
    return ctx->theLookahead;
}


/* next -- get the next character, excluding comments. peek() is used to see
        if a '/' is followed by a '/' or '*'.
*/

static int
next(jsminCtx *ctx, int *r)
{
    int c = get(ctx);
    if  (c == '/') {
        switch (peek(ctx)) {
        case '/':
            for (;;) {
                c = get(ctx);
                if (c <= '\n') {
                    break;
                }
            }
            break;
        case '*':
            get(ctx);
            while (c != ' ') {
                switch (get(ctx)) {
                case '*':
                    if (peek(ctx) == '/') {
                        get(ctx);
                        c = ' ';
                    }
                    break;
                case EOF:
                    error(ctx,"Unterminated comment.");
					return JSMIN_ERROR;
                }
            }
            break;
        }
    }
    ctx->theY = ctx->theX;
    ctx->theX = c;
    *r = c;
	return JSMIN_OK;
}


/* action -- do something! What you do is determined by the argument:
        1   Output A. Copy B to A. Get the next B.
        2   Copy B to A. Get the next B. (Delete A).
        3   Get the next B. (Delete B).
   action treats a string as a single character. Wow!
   action recognizes a regular expression if it is preceded by ( or , or =.
*/

#define OUTC(ctx, c) do{ char __ch[2]={(unsigned char)c,0}; ctx->output(ctx->clientData, __ch, 1); } while(0)

static int
action(jsminCtx *ctx, int d)
{
	int res = JSMIN_OK;
    switch (d) {
    case 1:
        OUTC(ctx, ctx->theA);
        if (
            (ctx->theY == '\n' || ctx->theY == ' ') &&
            (ctx->theA == '+' || ctx->theA == '-' || ctx->theA == '*' || ctx->theA == '/') &&
            (ctx->theB == '+' || ctx->theB == '-' || ctx->theB == '*' || ctx->theB == '/')
        ) {
            OUTC(ctx, ctx->theY);
        }
    case 2:
        ctx->theA = ctx->theB;
        if (ctx->theA == '\'' || ctx->theA == '"' || ctx->theA == '`') {
            for (;;) {
                OUTC(ctx, ctx->theA);
                ctx->theA = get(ctx);
                if (ctx->theA == ctx->theB) {
                    break;
                }
                if (ctx->theA == '\\') {
                    OUTC(ctx, ctx->theA);
                    ctx->theA = get(ctx);
                }
                if (ctx->theA == EOF) {
                    error(ctx,"Unterminated string literal.");
					return JSMIN_ERROR;
                }
            }
        }
    case 3:
        res = next(ctx, &ctx->theB);
		if (res != JSMIN_OK) return res;
        if (ctx->theB == '/' && (
            ctx->theA == '(' || ctx->theA == ',' || ctx->theA == '=' || ctx->theA == ':' ||
            ctx->theA == '[' || ctx->theA == '!' || ctx->theA == '&' || ctx->theA == '|' ||
            ctx->theA == '?' || ctx->theA == '+' || ctx->theA == '-' || ctx->theA == '~' ||
            ctx->theA == '*' || ctx->theA == '/' || ctx->theA == '{' || ctx->theA == '\n'
        )) {
            OUTC(ctx, ctx->theA);
            if (ctx->theA == '/' || ctx->theA == '*') {
                OUTC(ctx, ' ');
            }
            OUTC(ctx, ctx->theB);
            for (;;) {
                ctx->theA = get(ctx);
                if (ctx->theA == '[') {
                    for (;;) {
                        OUTC(ctx, ctx->theA);
                        ctx->theA = get(ctx);
                        if (ctx->theA == ']') {
                            break;
                        }
                        if (ctx->theA == '\\') {
                            OUTC(ctx, ctx->theA);
                            ctx->theA = get(ctx);
                        }
                        if (ctx->theA == EOF) {
                            error(ctx, "Unterminated set in Regular Expression literal.");
							return JSMIN_ERROR;
                        }
                    }
                } else if (ctx->theA == '/') {
                    switch (peek(ctx)) {
                    case '/':
                    case '*':
                        error(ctx, "Unterminated set in Regular Expression literal.");
						return JSMIN_ERROR;
                    }
                    break;
                } else if (ctx->theA =='\\') {
                    OUTC(ctx, ctx->theA);
                    ctx->theA = get(ctx);
                }
                if (ctx->theA == EOF) {
                    error(ctx, "Unterminated Regular Expression literal.");
					return JSMIN_ERROR;
                }
                OUTC(ctx, ctx->theA);
            }
            res = next(ctx, &ctx->theB);
			if (res != JSMIN_OK) return res;
        }
    }
	return JSMIN_OK;
}


/* jsmin -- Copy the input to the output, deleting the characters which are
        insignificant to JavaScript. Comments will be removed. Tabs will be
        replaced with spaces. Carriage returns will be replaced with linefeeds.
        Most spaces and linefeeds will be removed.
*/

int
jsmin(jsminCtx *ctx)
{
	int res = JSMIN_OK;

	ctx->theLookahead = EOF;
	ctx->theX = EOF;
	ctx->theY = EOF;

    if (peek(ctx) == 0xEF) {
        get(ctx);
        get(ctx);
        get(ctx);
    }
    ctx->theA = '\n';
    res = action(ctx,3);
	if (res != JSMIN_OK) return res;
    while (ctx->theA != EOF) {
        switch (ctx->theA) {
        case ' ':
            res = action(ctx, isAlphanum(ctx->theB) ? 1 : 2);
			if (res != JSMIN_OK) return res;
            break;
        case '\n':
            switch (ctx->theB) {
            case '{':
            case '[':
            case '(':
            case '+':
            case '-':
            case '!':
            case '~':
                res = action(ctx, 1);
				if (res != JSMIN_OK) return res;
                break;
            case ' ':
                res = action(ctx, 3);
				if (res != JSMIN_OK) return res;
                break;
            default:
                res = action(ctx, isAlphanum(ctx->theB) ? 1 : 2);
				if (res != JSMIN_OK) return res;
            }
            break;
        default:
            switch (ctx->theB) {
            case ' ':
                res = action(ctx, isAlphanum(ctx->theA) ? 1 : 3);
				if (res != JSMIN_OK) return res;
                break;
            case '\n':
                switch (ctx->theA) {
                case '}':
                case ']':
                case ')':
                case '+':
                case '-':
                case '"':
                case '\'':
                case '`':
                    res = action(ctx, 1);
					if (res != JSMIN_OK) return res;
                    break;
                default:
                    res = action(ctx, isAlphanum(ctx->theA) ? 1 : 3);
					if (res != JSMIN_OK) return res;
                }
                break;
            default:
                res = action(ctx, 1);
				if (res != JSMIN_OK) return res;
                break;
            }
        }
    }
	return res;
}

extern jsminCtx *jsminInit(
	void *clientData,
	jsminRead  *input,
	jsminWrite *output,
	jsminWrite *error)
{
	jsminCtx *ctx=malloc(sizeof(jsminCtx));
	ctx->clientData=clientData;
	ctx->input=input;
	ctx->output=output;
	ctx->error=error;

	return ctx;
}

extern int jsminCleanup(jsminCtx *ctx) {
	free(ctx);
	return JSMIN_OK;
}

