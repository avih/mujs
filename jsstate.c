#include "js.h"
#include "jsobject.h"
#include "jsrun.h"
#include "jsstate.h"

int js_loadstring(js_State *J, const char *source)
{
	return jsR_loadscript(J, "(string)", source);
}

int js_loadfile(js_State *J, const char *filename)
{
	FILE *f;
	char *s;
	int n, t;

	f = fopen(filename, "r");
	if (!f) {
		return js_error(J, "cannot open file: '%s'", filename);
	}

	if (fseek(f, 0, SEEK_END) < 0) {
		fclose(f);
		return js_error(J, "cannot seek in file: '%s'", filename);
	}
	n = ftell(f);
	fseek(f, 0, SEEK_SET);

	s = malloc(n + 1); /* add space for string terminator */
	if (!s) {
		fclose(f);
		return js_error(J, "cannot allocate storage for file contents: '%s'", filename);
	}

	t = fread(s, 1, n, f);
	if (t != n) {
		free(s);
		fclose(f);
		return js_error(J, "cannot read data from file: '%s'", filename);
	}

	s[n] = 0; /* zero-terminate string containing file data */

	t = jsR_loadscript(J, filename, s);

	free(s);
	fclose(f);
	return t;
}

int js_dostring(js_State *J, const char *source)
{
	int rv = js_loadstring(J, source);
	if (!rv) {
		if (setjmp(J->jb))
			return 1;
		js_dup(J, 0);
		js_eval(J);
		js_pop(J, 1);
	}
	return rv;
}

int js_dofile(js_State *J, const char *filename)
{
	int rv = js_loadfile(J, filename);
	if (!rv) {
		if (setjmp(J->jb))
			return 1;
		js_dup(J, 0);
		js_eval(J);
		js_pop(J, 1);
	}
	return rv;
}

static int jsB_print(js_State *J, int argc)
{
	int i;
	for (i = 1; i < argc; ++i) {
		const char *s = js_tostring(J, i);
		if (i > 1) putchar(' ');
		fputs(s, stdout);
	}
	putchar('\n');
	return 0;
}

static int jsB_eval(js_State *J, int argc)
{
	const char *s;

	if (!js_isstring(J, -1))
		return 1;

	// FIXME: return value if eval string is an expression

	s = js_tostring(J, -1);
	if (jsR_loadscript(J, "(eval)", s))
		jsR_error(J, "SyntaxError (eval)");

	js_dup(J, 0); /* copy this */
	js_eval(J); /* call with current scope chain */
	return 1;
}

js_State *js_newstate(void)
{
	js_State *J = malloc(sizeof *J);
	memset(J, 0, sizeof(*J));

	J->G = jsR_newobject(J, JS_COBJECT);
	J->E = jsR_newenvironment(J, J->G, NULL);

	js_pushcfunction(J, jsB_eval);
	js_setglobal(J, "eval");

	js_pushcfunction(J, jsB_print);
	js_setglobal(J, "print");

	return J;
}

void js_close(js_State *J)
{
	free(J->buf.text);
	free(J);
}

int js_error(js_State *J, const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "error: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	return 0;
}
