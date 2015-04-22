/*
jsminlib.h

Context structure definition
*/

typedef struct _jsminCtx jsminCtx;

typedef int jsminRead(void *clientData);
typedef int jsminWrite(void *clientData, char *text, int len);

enum {JSMIN_OK, JSMIN_ERROR};

extern jsminCtx *jsminInit(
	void *clientData,
	jsminRead  *input,
	jsminWrite *output,
	jsminWrite *error);

extern int jsmin(jsminCtx *ctx);

extern int jsminCleanup(jsminCtx *ctx);
