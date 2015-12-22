/* stringfile.c: manage strings, files, and directories for edbrowse.
* This file is part of the edbrowse project, released under GPL.
 */

#include "eb.h"

#include <sys/stat.h>
#include <dirent.h>
#ifdef DOSLIKE
#include <dos.h>
#else
#include <termios.h>
#include <unistd.h>
#include <glob.h>
#include <pwd.h>
#include <grp.h>
#endif

char emptyString[] = "";
bool showHiddenFiles, isInteractive;
int debugLevel = 1;
char *downDir, *home;

/*********************************************************************
Allocate and manage memory.
Allocate and copy strings.
If we're out of memory, the program aborts.  No error legs.
Soooooo much easier! With 32gb of RAM, we shouldn't run out.
*********************************************************************/

void *allocMem(size_t n)
{
	void *s;
	if (!n)
		return emptyString;
	if (!(s = malloc(n)))
		i_printfExit(MSG_MemAllocError, n);
	return s;
}				/* allocMem */

void *allocZeroMem(size_t n)
{
	void *s;
	if (!n)
		return emptyString;
	if (!(s = calloc(n, 1)))
		i_printfExit(MSG_MemCallocError, n);
	return s;
}				/* allocZeroMem */

void *reallocMem(void *p, size_t n)
{
	void *s;
	if (!n)
		i_printfExit(MSG_ReallocP);
/* small check against allocated strings getting huge */
	if (n < 0)
		i_printfExit(MSG_MemAllocError, n);
	if (!p)
		i_printfExit(MSG_Realloc0, n);
	if (p == emptyString)
		return allocMem(n);
	if (!(s = realloc(p, n)))
		i_printfExit(MSG_ErrorRealloc, n);
	return s;
}				/* reallocMem */

/* When you know the allocated thing is a string. */
char *allocString(size_t n)
{
	return (char *)allocMem(n);
}				/* allocString */

char *allocZeroString(size_t n)
{
	return (char *)allocZeroMem(n);
}				/* allocZeroString */

char *reallocString(void *p, size_t n)
{
	return (char *)reallocMem(p, n);
}				/* reallocString */

void nzFree(void *s)
{
	if (s && s != emptyString)
		free(s);
}				/* nzFree */

/* some compilers care whether it's void * or const void * */
void cnzFree(const void *v)
{
	nzFree((void *)v);
}				/* cnzFree */

uchar fromHex(char d, char e)
{
	d |= 0x20, e |= 0x20;
	if (d >= 'a')
		d -= ('a' - '9' - 1);
	if (e >= 'a')
		e -= ('a' - '9' - 1);
	d -= '0', e -= '0';
	return ((((uchar) d) << 4) | (uchar) e);
}				/* fromHex */

char *appendString(char *s, const char *p)
{
	int slen = strlen(s);
	int plen = strlen(p);
	s = reallocString(s, slen + plen + 1);
	strcpy(s + slen, p);
	return s;
}				/* appendstring */

char *prependString(char *s, const char *p)
{
	int slen = strlen(s);
	int plen = strlen(p);
	char *t = allocString(slen + plen + 1);
	strcpy(t, p);
	strcpy(t + plen, s);
	nzFree(s);
	return t;
}				/* prependstring */

void skipWhite(const char **s)
{
	const char *t = *s;
	while (isspaceByte(*t))
		++t;
	*s = t;
}				/* skipWhite */

void trimWhite(char *s)
{
	int l;
	if (!s)
		return;
	l = strlen(s);
	while (l && isspaceByte(s[l - 1]))
		--l;
	s[l] = 0;
}				/* trimWhite */

void stripWhite(char *s)
{
	const char *t = s;
	skipWhite(&t);
	if (t > s)
		strmove(s, t);
	trimWhite(s);
}				/* stripWhite */

/* compress white space */
void spaceCrunch(char *s, bool onespace, bool unprint)
{
	int i, j;
	char c;
	bool space = true;
	for (i = j = 0; c = s[i]; ++i) {
		if (isspaceByte(c)) {
			if (!onespace)
				continue;
			if (!space)
				s[j++] = ' ', space = true;
			continue;
		}
		if (unprint && !isprintByte(c))
			continue;
		s[j++] = c, space = false;
	}
	if (space && j)
		--j;		/* drop trailing space */
	s[j] = 0;
}				/* spaceCrunch */

/* Like strcpy, but able to cope with overlapping strings. */
char *strmove(char *dest, const char *src)
{
	return (char *)memmove(dest, src, strlen(src) + 1);
}				/* strmove */

/* OO has a lot of unnecessary overhead, and a few inconveniences,
 * but I really miss it right now.  The following
 * routines make up for the lack of simple string concatenation in C.
 * The string space allocated is always a power of 2 - 1, starting with 1.
 * Each of these routines puts an extra 0 on the end of the "string". */

char *initString(int *l)
{
	*l = 0;
	return emptyString;
}

/* String management routines realloc to one less than a power of 2 */
void stringAndString(char **s, int *l, const char *t)
{
	char *p = *s;
	int oldlen, newlen, x;
	oldlen = *l;
	newlen = oldlen + strlen(t);
	*l = newlen;
	++newlen;		/* room for the 0 */
	x = oldlen ^ newlen;
	if (x > oldlen) {	/* must realloc */
		newlen |= (newlen >> 1);
		newlen |= (newlen >> 2);
		newlen |= (newlen >> 4);
		newlen |= (newlen >> 8);
		newlen |= (newlen >> 16);
		p = reallocString(p, newlen);
		*s = p;
	}
	strcpy(p + oldlen, t);
}				/* stringAndString */

void stringAndBytes(char **s, int *l, const char *t, int cnt)
{
	char *p = *s;
	int oldlen, newlen, x;
	oldlen = *l;
	newlen = oldlen + cnt;
	*l = newlen;
	++newlen;
	x = oldlen ^ newlen;
	if (x > oldlen) {	/* must realloc */
		newlen |= (newlen >> 1);
		newlen |= (newlen >> 2);
		newlen |= (newlen >> 4);
		newlen |= (newlen >> 8);
		newlen |= (newlen >> 16);
		p = reallocString(p, newlen);
		*s = p;
	}
	memcpy(p + oldlen, t, cnt);
	p[oldlen + cnt] = 0;
}				/* stringAndBytes */

void stringAndChar(char **s, int *l, char c)
{
	char *p = *s;
	int oldlen, newlen, x;
	oldlen = *l;
	newlen = oldlen + 1;
	*l = newlen;
	++newlen;
	x = oldlen ^ newlen;
	if (x > oldlen) {	/* must realloc */
		newlen |= (newlen >> 1);
		newlen |= (newlen >> 2);
		newlen |= (newlen >> 4);
		newlen |= (newlen >> 8);
		newlen |= (newlen >> 16);
		p = reallocString(p, newlen);
		*s = p;
	}
	p[oldlen] = c;
	p[oldlen + 1] = 0;
}				/* stringAndChar */

void stringAndNum(char **s, int *l, int n)
{
	char a[16];
	sprintf(a, "%d", n);
	stringAndString(s, l, a);
}				/* stringAndNum */

/* 64M 16K etc */
void stringAndKnum(char **s, int *l, int n)
{
	char a[16];
	if (n && n / (1024 * 1024) * (1024 * 1024) == n)
		sprintf(a, "%dM", n / (1024 * 1024));
	else if (n && n / 1024 * 1024 == n)
		sprintf(a, "%dK", n / 1024);
	else
		sprintf(a, "%d", n);
	stringAndString(s, l, a);
}				/* stringAndKnum */

char *cloneString(const char *s)
{
	char *t;
	unsigned len;

	if (!s)
		return 0;
	if (!*s)
		return emptyString;
	len = strlen(s) + 1;
	t = allocString(len);
	strcpy(t, s);
	return t;
}				/* cloneString */

char *cloneMemory(const char *s, int n)
{
	char *t = allocString(n);
	if (n)
		memcpy(t, s, n);
	return t;
}				/* cloneMemory */

void leftClipString(char *s)
{
	char *t;
	if (!s)
		return;
	for (t = s; *t; ++t)
		if (!isspace(*t))
			break;
	if (t > s)
		strmove(s, t);
}				/* leftClipString */

/* shift string one to the right */
void shiftRight(char *s, char first)
{
	int l = strlen(s);
	for (; l >= 0; --l)
		s[l + 1] = s[l];
	s[0] = first;
}				/* shiftRight */

char *Cify(const char *s, int n)
{
	char *u;
	char *t = allocString(n + 1);
	if (n)
		memcpy(t, s, n);
	for (u = t; u < t + n; ++u)
		if (*u == 0)
			*u = ' ';
	*u = 0;
	return t;
}				/* Cify */

/* pull a substring out of a larger string,
 * and make it its own allocated string */
char *pullString(const char *s, int l)
{
	char *t;
	if (!l)
		return emptyString;
	t = allocString(l + 1);
	memcpy(t, s, l);
	t[l] = 0;
	return t;
}				/* pullString */

char *pullString1(const char *s, const char *t)
{
	return pullString(s, t - s);
}

/* return the number, if string is a number, else -1 */
int stringIsNum(const char *s)
{
	int n;
	if (!isdigitByte(s[0]))
		return -1;
	n = strtol(s, (char **)&s, 10);
	if (*s)
		return -1;
	return n;
}				/* stringIsNum */

bool stringIsDate(const char *s)
{
	if (!isdigit(*s))
		return false;
	++s;
	if (isdigit(*s))
		++s;
	if (*s++ != '/')
		return false;
	if (!isdigit(*s))
		return false;
	++s;
	if (isdigit(*s))
		++s;
	if (*s++ != '/')
		return false;
	if (!isdigit(*s))
		return false;
	++s;
	if (isdigit(*s))
		++s;
	if (isdigit(*s))
		++s;
	if (isdigit(*s))
		++s;
	if (*s)
		return false;
	return true;
}				/* stringIsDate */

bool stringIsFloat(const char *s, double *dp)
{
	const char *t;
	*dp = strtod(s, (char **)&t);
	if (*t)
		return false;	/* extra stuff at the end */
	return true;
}				/* stringIsFloat */

bool memEqualCI(const char *s, const char *t, int len)
{
	char c, d;
	if (s == t)
		return true;
	if (!s || !t)
		return false;
	while (len--) {
		c = *s, d = *t;
		if (islowerByte(c))
			c = toupper(c);
		if (islowerByte(d))
			d = toupper(d);
		if (c != d)
			return false;
		++s, ++t;
	}
	return true;
}				/* memEqualCI */

char *strstrCI(const char *base, const char *search)
{
	int l = strlen(search);
	while (*base) {
		if (memEqualCI(base, search, l))
			return (char *)base;
		++base;
	}
	return 0;
}				/* strstrCI */

bool stringEqual(const char *s, const char *t)
{
/* check equality of strings with handling of null pointers */
	if (s == t)
		return true;
	if (!s || !t)
		return false;
	if (strcmp(s, t))
		return false;
	return true;
}				/* stringEqual */

bool stringEqualCI(const char *s, const char *t)
{
	char c, d;
/* if two pointers are equal we can return */
	if (s == t)
		return true;
/* if one is NULL then the strings can't be equal */
	if (!s || !t)
		return false;
	while ((c = *s) && (d = *t)) {
		if (islowerByte(c))
			c = toupper(c);
		if (islowerByte(d))
			d = toupper(d);
		if (c != d)
			return false;
		++s, ++t;
	}
	if (*s)
		return false;
	if (*t)
		return false;
	return true;
}				/* stringEqualCI */

int stringInList(const char *const *list, const char *s)
{
	int i = 0;
	if (!list)
		i_printfExit(MSG_NullStrList);
	if (s)
		while (*list) {
			if (stringEqual(s, *list))
				return i;
			++i;
			++list;
		}
	return -1;
}				/* stringInList */

int stringInListCI(const char *const *list, const char *s)
{
	int i = 0;
	if (!list)
		i_printfExit(MSG_NullStrListCI);
	if (s)
		while (*list) {
			if (stringEqualCI(s, *list))
				return i;
			++i;
			++list;
		}
	return -1;
}				/* stringInListCI */

int charInList(const char *list, char c)
{
	char *s;
	if (!list)
		i_printfExit(MSG_NullCharInList);
	s = (char *)strchr(list, c);
	if (!s)
		return -1;
	return s - list;
}				/* charInList */

/* In an empty list, next and prev point back to the list, not to 0. */
/* We also allow zero. */
bool listIsEmpty(const struct listHead * l)
{
	return l->next == l || l->next == 0;
}				/* listIsEmpty */

void initList(struct listHead *l)
{
	l->prev = l->next = l;
}				/* initList */

void delFromList(void *x)
{
	struct listHead *xh = (struct listHead *)x;
	((struct listHead *)xh->next)->prev = xh->prev;
	((struct listHead *)xh->prev)->next = xh->next;
}				/* delFromList */

void addToListFront(struct listHead *l, void *x)
{
	struct listHead *xh = (struct listHead *)x;
	xh->next = l->next;
	xh->prev = l;
	l->next = (struct listHead *)x;
	((struct listHead *)xh->next)->prev = (struct listHead *)x;
}				/* addToListFront */

void addToListBack(struct listHead *l, void *x)
{
	struct listHead *xh = (struct listHead *)x;
	xh->prev = l->prev;
	xh->next = l;
	l->prev = (struct listHead *)x;
	((struct listHead *)xh->prev)->next = (struct listHead *)x;
}				/* addToListBack */

void addAtPosition(void *p, void *x)
{
	struct listHead *xh = (struct listHead *)x;
	struct listHead *ph = (struct listHead *)p;
	xh->prev = p;
	xh->next = ph->next;
	ph->next = (struct listHead *)x;
	((struct listHead *)xh->next)->prev = (struct listHead *)x;
}				/* addAtPosition */

void freeList(struct listHead *l)
{
	while (!listIsEmpty(l)) {
		void *p = l->next;
		delFromList(p);
		nzFree(p);
	}
}				/* freeList */

/* like isalnumByte, but allows _ and - */
bool isA(char c)
{
	if (isalnumByte(c))
		return true;
	return (c == '_' || c == '-');
}				/* isA */

bool isquote(char c)
{
	return c == '"' || c == '\'';
}				/* isquote */

/* print an error message */
void errorPrint(const char *msg, ...)
{
	char bailflag = 0;
	va_list p;

	va_start(p, msg);

	if (*msg == '@') {
		bailflag = 1;
		++msg;
/* I should internationalize this, but it's never suppose to happen! */
		fprintf(stderr, "disaster, ");
	} else if (isdigitByte(*msg)) {
		bailflag = *msg - '0';
		++msg;
	}

	vfprintf(stderr, msg, p);
	fprintf(stderr, "\n");
	va_end(p);

	if (bailflag)
		exit(bailflag);
}				/* errorPrint */

void debugPrint(int lev, const char *msg, ...)
{
	va_list p;
	if (lev > debugLevel)
		return;
	va_start(p, msg);
	vprintf(msg, p);
	va_end(p);
	nl();
	if (lev == 0 && !memcmp(msg, "warning", 7))
		eeCheck();
}				/* debugPrint */

void nl(void)
{
	puts("");
}				/* nl */

/* Turn perl string into C string, and complain about nulls. */
int perl2c(char *t)
{
	int n = 0;
	while (*t != '\n') {
		if (*t == 0)
			++n;
		++t;
	}
	*t = 0;			/* now it's a C string */
	return n;		/* number of nulls */
}				/* perl2c */

/* The length of a perl string includes its terminating newline */
unsigned pstLength(pst s)
{
	pst t;
	if (!s)
		i_printfExit(MSG_NullPtr);
	t = s;
	while (*t != '\n')
		++t;
	return t + 1 - s;
}				/* pstLength */

pst clonePstring(pst s)
{
	pst t;
	unsigned len;
	if (!s)
		return s;
	len = pstLength(s);
	t = (pst) allocMem(len);
	memcpy(t, s, len);
	return t;
}				/* clonePstring */

// Strings are assumed distinct, hence the use of memcpy.
void copyPstring(pst s, const pst t)
{
	int len = pstLength(t);
	memcpy(s, t, len);
}				/* copyPstring */

/*
 * fdIntoMemory reads data from a file descriptor, until EOF is reached.
 * It works even if we don't know the size beforehand.
 * We can now use it to read /proc files, pipes, and stdin.
 * This solves an outstanding issue, and it is needed for forthcoming
 * functionality, such as edpager.
 */
bool fdIntoMemory(int fd, char **data, int *len)
{
	int length, n;
	const int blocksize = 8192;
	char *chunk, *buf;

	chunk = allocZeroString(blocksize);
	buf = initString(&length);

	n = 0;
	do {
		n = read(fd, chunk, blocksize);
		if (n < 0) {
			nzFree(buf);
			nzFree(chunk);
			*data = emptyString;
			*len = 0;
			setError(MSG_NoRead, "file descriptor");
			return false;
		}

		if (n > 0)
			stringAndBytes(&buf, &length, chunk, n);
	} while (n != 0);

	nzFree(chunk);
	buf = reallocString(buf, length + 2);
	*data = buf;
	*len = length;
	return true;
}				/* fdIntoMemory */

bool fileIntoMemory(const char *filename, char **data, int *len)
{
	int fh;
	char ftype = fileTypeByName(filename, false);
	bool ret;
	if (ftype && ftype != 'f') {
		setError(MSG_RegularFile, filename);
		return false;
	}
	fh = open(filename, O_RDONLY | O_BINARY);
	if (fh < 0) {
		setError(MSG_NoOpen, filename);
		return false;
	}

	ret = fdIntoMemory(fh, data, len);
	if (ret == false)
		setError(MSG_NoRead2, filename);

	close(fh);
	return ret;
}				/* fileIntoMemory */

/* inverse of the above */
bool memoryOutToFile(const char *filename, const char *data, int len,
/* specify the error messages */
		     int msgcreate, int msgwrite)
{
	int fh = open(filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0666);
	if (fh < 0) {
		setError(msgcreate, filename, errno);
		return false;
	}
	if (write(fh, data, len) < len) {
		setError(msgwrite, filename, errno);
		close(fh);
		return false;
	}
	close(fh);
	return true;
}				/* memoryOutToFile */

/* shift string to upper, lower, or mixed case */
/* action is u, l, or m. */
void caseShift(char *s, char action)
{
	char c;
	int mc = 0;
	bool ws = true;

	for (; c = *s; ++s) {
		if (action == 'u') {
			if (isalphaByte(c))
				*s = toupper(c);
			continue;
		}
		if (action == 'l') {
			if (isalphaByte(c))
				*s = tolower(c);
			continue;
		}
/* mixed case left */
		if (isalphaByte(c)) {
			if (ws)
				c = toupper(c);
			else
				c = tolower(c);
			if (ws && c == 'M')
				mc = 1;
			else if (mc == 1 && c == 'c')
				mc = 2;
			else if (mc == 2) {
				c = toupper(c);
				mc = 0;
			} else
				mc = 0;
			*s = c;
			ws = false;
			continue;
		}
		ws = true, mc = 0;
	}			/* loop */
}				/* caseShift */

/*********************************************************************
Manage files, directories, and terminal IO.
You'll see some conditional compilation when this program
is ported to other operating systems.
*********************************************************************/

/* Return the type of a file.
 * Make it a capital letter if you are going through a link.
 * I think this will work on Windows, not sure.
 * But the link feature is Unix specific. */

char fileTypeByName(const char *name, bool showlink)
{
	struct stat buf;
	bool islink = false;
	char c;
	int mode;

#ifdef DOSLIKE
	if (stat(name, &buf)) {
		setError(MSG_NoAccess, name);
		return 0;
	}
	mode = buf.st_mode & S_IFMT;
#else // !DOSLIKE

	if (lstat(name, &buf)) {
		setError(MSG_NoAccess, name);
		return 0;
	}
	mode = buf.st_mode & S_IFMT;
	if (mode == S_IFLNK) {	/* symbolic link */
		islink = true;
/* If this fails, I'm guessing it's just a file. */
		if (stat(name, &buf))
			return (showlink ? 'F' : 0);
		mode = buf.st_mode & S_IFMT;
	}
#endif // DOSLIKE y/n

	c = 'f';
	if (mode == S_IFDIR)
		c = 'd';
#ifndef DOSLIKE
/* I don't think these are Windows constructs. */
	if (mode == S_IFBLK)
		c = 'b';
	if (mode == S_IFCHR)
		c = 'c';
	if (mode == S_IFIFO)
		c = 'p';
	if (mode == S_IFSOCK)
		c = 's';
#endif
	if (islink & showlink)
		c = toupper(c);
	return c;
}				/* fileTypeByName */

char fileTypeByHandle(int fd)
{
	struct stat buf;
	char c;
	int mode;
	if (fstat(fd, &buf)) {
		setError(MSG_NoAccess, "handle");
		return 0;
	}
	mode = buf.st_mode & S_IFMT;
	c = 'f';
	if (mode == S_IFDIR)
		c = 'd';
#ifndef DOSLIKE
/* I don't think these are Windows constructs. */
	if (mode == S_IFBLK)
		c = 'b';
	if (mode == S_IFCHR)
		c = 'c';
	if (mode == S_IFIFO)
		c = 'p';
	if (mode == S_IFSOCK)
		c = 's';
#endif
	return c;
}				/* fileTypeByHandle */

int fileSizeByName(const char *name)
{
	struct stat buf;
	if (stat(name, &buf)) {
		setError(MSG_NoAccess, name);
		return -1;
	}
	return buf.st_size;
}				/* fileSizeByName */

int fileSizeByHandle(int fd)
{
	struct stat buf;
	if (fstat(fd, &buf)) {
/* should never happen */
		return -1;
	}
	return buf.st_size;
}				/* fileSizeByHandle */

time_t fileTimeByName(const char *name)
{
	struct stat buf;
	if (stat(name, &buf)) {
		setError(MSG_NoAccess, name);
		return -1;
	}
	return buf.st_mtime;
}				/* fileTimeByName */

char *conciseSize(size_t n)
{
	static char buf[32];
	if (n >= (1 << 30))
		sprintf(buf, "%luG", (unsigned long)n >> 30);
	else if (n >= (1 << 20))
		sprintf(buf, "%luM", (unsigned long)n >> 20);
	else if (n >= (1 << 10))
		sprintf(buf, "%luK", (unsigned long)n >> 10);
	else
		sprintf(buf, "%lu", (unsigned long)n);
	return buf;
}				/* conciseSize */

char *conciseTime(time_t t)
{
	static char buffer[20];
	static const char *months[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	struct tm *tm = localtime(&t);
	sprintf(buffer, "%s %2d %d %02d:%02d",
		months[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min);
	return buffer;
}				/* conciseTime */

/* retain only characters l s t i k p m, for ls attributes */
bool lsattrChars(const char *buf, char *dest)
{
	bool rc = true;
	const char *s;
	char c, *t;
#ifdef DOSLIKE
	static const char ok_chars[] = "lst";
#else
	static const char ok_chars[] = "lstikpmy";
#endif
	char used[26];
	memset(used, 0, sizeof(used));
	t = dest;
	for (s = buf; (c = *s); ++s) {
		if (isspaceByte(c))
			continue;
		if (!strchr(ok_chars, c)) {
			rc = false;
			continue;
		}
		if (used[c - 'a'])
			continue;
		used[c - 'a'] = 1;
		*t++ = c;
	}
	*t = 0;
	return rc;
}				/* lsattrChars */

/* expand the ls attributes for a file into a static string. */
/* This assumes user/group names will not be too long. */
char *lsattr(const char *path, const char *flags)
{
	static char buf[200 + ABSPATH];
	char p[40];
	struct stat st, lst;
	struct passwd *pwbuf;
	struct group *grpbuf;
	char *s;
	int l, modebits;
	bool sympath = false;	// user is requesting symlink path
	bool statyes = true;	// stat call succeeds
	char newpath[ABSPATH];

	buf[0] = 0;

	if (!path || !path[0] || !flags || !flags[0])
		return buf;

	if (strchr(flags, 'y'))
		sympath = true;

/* we already glommed onto this file through the directory listing;
 * it ought to be there, except for broken symlink. */
	if (stat(path, &st)) {
		statyes = false;
		if (!sympath)
			return buf;
	}

	while (*flags) {
		if (buf[0])
			strcat(buf, " ");

/* exception when asking for information on a broken symlink */
		if (!statyes && *flags != 'y') {
			strcat(buf, "?");
			++flags;
			continue;
		}

		switch (*flags) {
		case 't':
			strcat(buf, conciseTime(st.st_mtime));
			break;
		case 'l':
			sprintf(p, "%lld", (long long)st.st_size);
#ifndef DOSLIKE
p:
#endif // #ifndef DOSLIKE
			strcat(buf, p);
			break;
		case 's':
			strcat(buf, conciseSize(st.st_size));
			break;
#ifndef DOSLIKE
/* not sure any of these work under windows */

		case 'i':
			sprintf(p, "%lu", (unsigned long)st.st_ino);
			goto p;
		case 'k':
			sprintf(p, "%lu", (unsigned long)st.st_nlink);
			goto p;
		case 'm':
			strcpy(p, "-");
			if (st.st_rdev)
				sprintf(p, "%d/%d",
					(int)(st.st_rdev >> 8),
					(int)(st.st_rdev & 0xff));
			goto p;
		case 'p':
			s = buf + strlen(buf);
			pwbuf = getpwuid(st.st_uid);
			if (pwbuf) {
				l = strlen(pwbuf->pw_name);
				if (l > 20)
					l = 20;
				strncpy(s, pwbuf->pw_name, l);
				s[l] = 0;
			} else
				sprintf(s, "%d", st.st_uid);
			s += strlen(s);
			*s++ = ' ';
			grpbuf = getgrgid(st.st_gid);
			if (grpbuf) {
				l = strlen(grpbuf->gr_name);
				if (l > 20)
					l = 20;
				strncpy(s, grpbuf->gr_name, l);
				s[l] = 0;
			} else
				sprintf(s, "%d", st.st_gid);
			s += strlen(s);
			*s++ = ' ';
			modebits = st.st_mode;
			modebits &= 07777;
			if (modebits & 07000)
				*s++ = '0' + (modebits >> 9);
			modebits &= 0777;
			*s++ = '0' + (modebits >> 6);
			modebits &= 077;
			*s++ = '0' + (modebits >> 3);
			modebits &= 7;
			*s++ = '0' + modebits;
			*s = 0;
			break;

		case 'y':
			if (lstat(path, &lst)) {
/* Wow, this call should always succeed */
				strcat(buf, "?");
				break;
			}
			if ((lst.st_mode & S_IFMT) != S_IFLNK) {
				strcat(buf, "-");
				break;
			}
/* yes it's a link, read the path */
			l = readlink(path, newpath, sizeof(newpath));
			if (l <= 0)
				strcat(buf, "?");
			else {
				s = buf + strlen(buf);
				strncpy(s, newpath, l);
				s[l] = 0;
			}
			break;

#endif

		}

		++flags;
	}

	return buf;
}				/* lsattr */

#ifdef DOSLIKE
void ttySaveSettings(void)
{
	// TODO: Anything needed here for WIN32?
	isInteractive = _isatty(0);
}
#else // !#ifdef DOSLIKE
static struct termios savettybuf;
void ttySaveSettings(void)
{
	isInteractive = isatty(0);
	if (isInteractive) {
		if (tcgetattr(0, &savettybuf))
			i_printfExit(MSG_IoctlError);
	}
}				/* ttySaveSettings */

static void ttyRestoreSettings(void)
{
	if (isInteractive)
		tcsetattr(0, TCSANOW, &savettybuf);
}				/* ttyRestoreSettings */

/* put the tty in raw mode.
 * Review your Unix manual on termio.
 * min>0 time>0:  return min chars, or as many as you have received
 *   when time/10 seconds have elapsed between characters.
 * min>0 time=0:  block until min chars are received.
 * min=0 time>0:  return 1 char, or 0 if the timer expires.
 * min=0 time=0:  nonblocking, return whatever chars have been received. */
static void ttyRaw(int charcount, int timeout, bool isecho)
{
	struct termios buf = savettybuf;	/* structure copy */
	buf.c_cc[VMIN] = charcount;
	buf.c_cc[VTIME] = timeout;
	buf.c_lflag &= ~(ICANON | ECHO);
	if (isecho)
		buf.c_lflag |= ECHO;
	tcsetattr(0, TCSANOW, &buf);
}				/* ttyRaw */

/* simulate MSDOS getche() system call */
int getche(void)
{
	char c;
	fflush(stdout);
	ttyRaw(1, 0, true);
	read(0, &c, 1);
	ttyRestoreSettings();
	return c;
}				/* getche */

int getch(void)
{
	char c;
	fflush(stdout);
	ttyRaw(1, 0, false);
	read(0, &c, 1);
	ttyRestoreSettings();
	return c;
}				/* getche */

#endif // #ifdef DOSLIKE y/n

char getLetter(const char *s)
{
	char c;
	while (true) {
		c = getch();
		if (strchr(s, c))
			break;
		printf("\a\b");
		fflush(stdout);
	}
	printf("%c", c);
	return c;
}				/* getLetter */

/* Parameters: message, default file name, must this file be new,
 * and can we except an input of white space,
 * that being converted to a single space. */
char *getFileName(int msg, const char *defname, bool isnew, bool ws)
{
	static char buf[ABSPATH];
	static char spacename[] = " ";
	int l;
	char *p;
	bool allspace;

	while (true) {
		i_printf(msg);
		if (defname)
			printf("[%s] ", defname);
		fflush(stdout);
		if (!fgets(buf, sizeof(buf), stdin))
			exit(0);
		allspace = false;
		for (p = buf; isspaceByte(*p); ++p)
			if (*p == ' ')
				allspace = true;
		l = strlen(p);
		while (l && isspaceByte(p[l - 1]))
			--l;
		p[l] = 0;
		if (!l) {
			if (ws & allspace)
				return spacename;
			if (!defname)
				continue;
/* make a copy just to be safe */
			l = strlen(defname);
			if (l >= ABSPATH)
				l = ABSPATH - 1;
			strncpy(buf, defname, l);
			buf[l] = 0;
			p = buf;
		} else
			defname = 0;
		if (isnew && fileTypeByName(p, false)) {
			i_printf(MSG_FileExists, p);
			defname = 0;
			continue;
		}
		return p;
	}
}				/* getFileName */

/* Protect a filename from expansion by the shell */
static const char shellmeta[] = "\\\n\t |&;<>(){}#'\"~$*?";
int shellProtectLength(const char *s)
{
	int l = 0;
	while (*s) {
		if (strchr(shellmeta, *s))
			++l;
		++l, ++s;
	}
	return l;
}				/* shellProtectLength */

void shellProtect(char *t, const char *s)
{
	while (*s) {
		if (strchr(shellmeta, *s))
			*t++ = '\\';
		*t++ = *s++;
	}
}				/* shellProtect */

/* loop through the files in a directory */
const char *nextScanFile(const char *base)
{
	static DIR *df;
	struct dirent *de;
	const char *s;

	if (!df) {
		if (!base)
			base = ".";
		df = opendir(base);
/* directory could be unreadable */
		if (!df) {
			i_puts(MSG_NoDirNoList);
			return 0;
		}
	}

	while (de = readdir(df)) {
		s = de->d_name;
		if (s[0] == '.') {
			if (stringEqual(s, "."))
				continue;
			if (stringEqual(s, ".."))
				continue;
			if (!showHiddenFiles)
				continue;
		}
		return s;
	}			/* end loop over files in directory */

	closedir(df);
	df = 0;
	return 0;
}				/* nextScanFile */

/* compare routine for quicksort */
static int dircmp(const void *s, const void *t)
{
	return strcoll((const char *)((const struct lineMap *)s)->text,
		       (const char *)((const struct lineMap *)t)->text);
}				/* dircmp */

bool sortedDirList(const char *dir, struct lineMap **map_p, int *count_p)
{
	const char *f;
	int linecount = 0, cap;
	struct lineMap *t, *map;

	cap = 128;
	map = t = (struct lineMap *)allocZeroMem(cap * LMSIZE);

	while (f = nextScanFile(dir)) {
		if (linecount == cap) {
			cap *= 2;
			map = (struct lineMap *)reallocMem(map, cap * LMSIZE);
			t = map + linecount;
		}
/* leave room for @ / newline */
		t->text = (pst) allocMem(strlen(f) + 3);
		strcpy((char *)t->text, f);
		t->ds1 = t->ds2 = 0;
		++t, ++linecount;
	}

	*count_p = linecount;
	*map_p = map;

	if (!linecount)
		return true;

/* sort the entries */
	qsort(map, linecount, LMSIZE, dircmp);

	return true;
}				/* sortedDirList */

/* Expand environment variables, then wild cards.
 * But the result should be one and only one file.
 * Return the new expanded line.
 * Neither the original line nore the new line is allocated.
 * They are static char buffers that are just plain long enough. */

static bool envExpand(const char *line, const char **expanded)
{
	const char *s;
	char *t;
	const char *v;		/* result of getenv call */
	bool inbrace;		/* ${foo} */
	struct passwd *pw;
	const char *udir;	/* user directory */
	int l;
	static char varline[ABSPATH];
	char var1[40];

/* quick check */
	if (line[0] != '~' && !strchr(line, '$')) {
		*expanded = line;
		return true;
	}

/* ok, need to crunch along */
	t = varline;
	s = line;

	if (line[0] != '~')
		goto dollars;

	l = 0;
	for (s = line + 1; isalnum(*s) || *s == '_'; ++s)
		++l;
	if (l >= sizeof(var1) || isdigit(line[1]) || *s && *s != '/') {
/* invalid syntax, put things back */
		s = line;
		goto dollars;
	}

	udir = 0;
	strncpy(var1, line + 1, l);
	var1[l] = 0;
#ifndef DOSLIKE
	if (l) {
		pw = getpwnam(var1);
		if (!pw) {
			setError(MSG_NoTilde, var1);
			return false;
		}
		if (pw->pw_dir && *pw->pw_dir)
			udir = pw->pw_dir;
	} else
#endif
		udir = home;
	if (!udir) {
		s = line;
		goto dollars;
	}
	l = strlen(udir);
	if (l >= sizeof(varline))
		goto longline;
	strcpy(varline, udir);
	t = varline + l;

dollars:
	for (; *s; ++s) {
		if (t - varline == ABSPATH - 1) {
longline:
			setError(MSG_ShellLineLong);
			return false;
		}
		if (*s == '\\' && s[1] == '$') {
/* this $ is escaped */
			++s;
appendchar:
			*t++ = *s;
			continue;
		}
		if (*s != '$')
			goto appendchar;

/* this is $, see if it is $var or ${var} */
		inbrace = false;
		v = s + 1;
		if (*v == '{')
			inbrace = true, ++v;
		if (!isalphaByte(*v) && *v != '_')
			goto appendchar;
		l = 0;
		while (isalnumByte(*v) || *v == '_') {
			if (l == sizeof(var1) - 1)
				goto longline;
			var1[l++] = *v++;
		}
		var1[l] = 0;
		if (inbrace) {
			if (*v != '}')
				goto appendchar;
			++v;
		}
		s = v - 1;
		v = getenv(var1);
		if (!v) {
			setError(MSG_NoEnvVar, var1);
			return false;
		}
		l = strlen(v);
		if (t - varline + l >= ABSPATH)
			goto longline;
		strcpy(t, v);
		t += l;
	}
	*t = 0;

	*expanded = varline;
	return true;
}				/* envExpand */

bool envFile(const char *line, const char **expanded)
{
	static char line2[ABSPATH];
	const char *varline;
	const char *s;
	char *t;
#ifndef DOSLIKE
	glob_t g;
#endif // #ifndef DOSLIKE
	int rc, flags;

/* ` disables this stuff */
/* but `` is a literal ` */
	if (line[0] == '`') {
		if (line[1] != '`') {
			*expanded = line + 1;
			return true;
		}
		++line;
	}

	if (!envExpand(line, &varline))
		return false;

#ifdef DOSLIKE
	*expanded = varline;
	return true;		// TODO: WIN32: Expand like glob...
#else // !#ifdef DOSLIKE

/* expanded the environment variables, if any, now time to glob */
	flags = GLOB_NOSORT;
	rc = glob(varline, flags, NULL, &g);

	if (rc == GLOB_NOMATCH) {
/* unescape the metas */
		t = line2;
		for (s = varline; *s; ++s) {
			if (*s == '\\' && s[1] && strchr("*?[", s[1]))
				++s;
			*t++ = *s;
		}
		*t = 0;
		*expanded = line2;
		return true;
	}

	if (rc) {
/* some other syntax error, whereup we can't expand. */
		setError(MSG_ShellExpand);
		globfree(&g);
		return false;
	}

	if (g.gl_pathc != 1) {
		setError(MSG_ShellManyMatch);
		globfree(&g);
		return false;
	}

/* looks good, if it isn't too long */
	s = g.gl_pathv[0];
	if (strlen(s) >= sizeof(line2)) {
		setError(MSG_ShellLineLong);
		globfree(&g);
		return false;
	}

	strcpy(line2, s);
	globfree(&g);
	*expanded = line2;
	return true;
#endif // #ifdef DOSLIKE y/n

}				/* envFile */

/* Call the above routine if filename contains a  slash,
 * or prepend the download directory if it does not.
 * If there is no download directory then always expand as above. */
bool envFileDown(const char *line, const char **expanded)
{
	static char line2[MAXTTYLINE];

	if (!downDir || strchr(line, '/'))
/* we don't necessarily expect there to be a file here */
		return envFile(line, expanded);

	if (strlen(downDir) + strlen(line) >= sizeof(line2) - 1) {
		setError(MSG_ShellLineLong);
		return false;
	}

	sprintf(line2, "%s/%s", downDir, line);
	*expanded = line2;
	return true;
}				/* envFileDown */

FILE *efopen(const char *name, const char *mode)
{
	FILE *f;

	if (name[0] == '-' && name[1] == 0) {
		if (*mode == 'r')
			return stdin;
		if (*mode == 'w' || *mode == 'a')
			return stdout;
	}

	f = fopen(name, mode);
	if (f)
		return f;

	if (*mode == 'r')
		i_printfExit(MSG_OpenFail, name);
	else if (*mode == 'w' || *mode == 'a')
		i_printfExit(MSG_CreateFail, name);
	else
		i_printfExit(MSG_InvalidFopen, mode);
	return 0;
}				/* efopen */

int eopen(const char *name, int mode, int perms)
{
	int fd;
	fd = open(name, mode, perms);
	if (fd >= 0)
		return fd;
	if (mode & O_WRONLY)
		i_printfExit(MSG_CreateFail, name);
	else
		i_printfExit(MSG_OpenFail, name);
	return -1;
}				/* eopen */

void appendFile(const char *fname, const char *message, ...)
{
	FILE *f;
	va_list p;
	f = efopen(fname, "a");
	va_start(p, message);
	vfprintf(f, message, p);
	va_end(p);
	fprintf(f, "\n");
	fclose(f);
}				/* appendFile */

/* like the above, but no formatting */
void appendFileNF(const char *filename, const char *msg)
{
	FILE *f = efopen(filename, "a");
	fprintf(f, "%s\n", msg);
	fclose(f);
}				/* appendFileNF */

/* Wrapper around system(). */
int eb_system(const char *cmd, bool print_on_success)
{
	int system_ret = 0;
#ifdef DOSLIKE
	system_ret = system(cmd);
#else
/* Ignoring of SIGPIPE propagates across fork-exec. */
/* So stop ignoring it for the duration of system(). */
	signal(SIGPIPE, SIG_DFL);
	system_ret = system(cmd);
	signal(SIGPIPE, SIG_IGN);
#endif

	if (system_ret == 0) {
		if (print_on_success)
			i_puts(MSG_OK);
	} else {
		i_printf(MSG_SystemCmdFail, system_ret);
		nl();
	}
	return system_ret;
}				/* eb_system */

/* Some variables and structures that are populated from the config file */
const char *version = "3.6.0+";
char *changeFileName;
char *configFile, *addressFile, *cookieFile;
char *mailDir, *mailUnread, *mailStash, *mailReply;
char *recycleBin, *sigFile, *sigFileEnd;
char *userAgents[10];
char *currentAgent, *currentReferrer;
bool allowRedirection = true, allowJS = true, sendReferrer = true;
int jsPool = 32;
int webTimeout = 20, mailTimeout = 0;
int displayLength = 500;
int verifyCertificates = 1;
char *sslCerts;
int localAccount, maxAccount;
struct MACCOUNT accounts[MAXACCOUNT];
int maxMime;
struct MIMETYPE mimetypes[MAXMIME];
int maxproxy;
struct PXENT proxyEntries[MAXPROXY];
struct FILTERDESC mailFilters[MAXFILTER];
int n_filters;
char *ebScript[MAXEBSCRIPT + 1];
char *ebScriptName[MAXEBSCRIPT + 1];
#define MAXNOJS 500
static const char *javaDis[MAXNOJS];
static int javaDisCount;
struct DBTABLE dbtables[MAXDBT];
int numTables;
volatile bool intFlag;

/* Read the config file and populate the corresponding data structures. */
/* This routine succeeds, or aborts via i_printfExit */
void readConfigFile(void)
{
	char *buf, *s, *t, *v, *q;
	int buflen, n;
	char c, ftype;
	bool cmt = false;
	bool startline = true;
	uchar mailblock = 0;
	bool mimeblock = false, tabblock = false;
	int nest, ln, j;
	int sn = 0;		/* script number */
	char stack[MAXNEST];
	char last[24];
	int lidx = 0;
	struct MACCOUNT *act;
	struct PXENT *px;
	struct MIMETYPE *mt;
	struct DBTABLE *td;

/* Order is important here: mail{}, mime{}, table{}, then global keywords */
#define MAILWORDS 0
#define MIMEWORDS 8
#define TABLEWORDS 15
#define GLOBALWORDS 19
	static const char *const keywords[] = {
		"inserver", "outserver", "login", "password", "from", "reply",
		"inport", "outport",
		"type", "desc", "suffix", "protocol", "program",
		"content", "outtype",
		"tname", "tshort", "cols", "keycol",
		"adbook", "downdir", "maildir", "agent",
		"jar", "nojs", "xyz@xyz",
		"webtimer", "mailtimer", "certfile", "datasource", "proxy",
		"linelength", "localizeweb", "jspool", "novs",
		0
	};

	if (!fileTypeByName(configFile, false))
		return;		/* config file not present */
	if (!fileIntoMemory(configFile, &buf, &buflen))
		showErrorAbort();
/* An extra newline won't hurt. */
	if (buflen && buf[buflen - 1] != '\n')
		buf[buflen++] = '\n';

/* Undos, uncomment, watch for nulls */
/* Encode mail{ as hex 81 m, and other encodings. */
	ln = 1;
	for (s = t = v = buf; s < buf + buflen; ++s) {
		c = *s;
		if (c == '\0')
			i_printfExit(MSG_EBRC_Nulls, ln);
		if (c == '\r' && s[1] == '\n')
			continue;

		if (cmt) {
			if (c != '\n')
				continue;
			cmt = false;
		}

		if (c == '#' && startline) {
			cmt = true;
			goto putc;
		}

		if (c == '\n') {
			last[lidx] = 0;
			lidx = 0;
			if (stringEqual(last, "}")) {
				*v = '\x82';
				t = v + 1;
			}
			if (stringEqual(last, "}else{")) {
				*v = '\x83';
				t = v + 1;
			}
			if (stringEqual(last, "mail{")) {
				*v = '\x81';
				v[1] = 'm';
				t = v + 2;
			}
			if (stringEqual(last, "plugin{") ||
			    stringEqual(last, "mime{")) {
				*v = '\x81';
				v[1] = 'e';
				t = v + 2;
			}
			if (stringEqual(last, "table{")) {
				*v = '\x81';
				v[1] = 'b';
				t = v + 2;
			}
			if (stringEqual(last, "fromfilter{")) {
				*v = '\x81';
				v[1] = 'r';
				t = v + 2;
			}
			if (stringEqual(last, "tofilter{")) {
				*v = '\x81';
				v[1] = 't';
				t = v + 2;
			}
			if (stringEqual(last, "subjfilter{")) {
				*v = '\x81';
				v[1] = 's';
				t = v + 2;
			}
			if (stringEqual(last, "if(*){")) {
				*v = '\x81';
				v[1] = 'I';
				t = v + 2;
			}
			if (stringEqual(last, "if(?){")) {
				*v = '\x81';
				v[1] = 'i';
				t = v + 2;
			}
			if (stringEqual(last, "while(*){")) {
				*v = '\x81';
				v[1] = 'W';
				t = v + 2;
			}
			if (stringEqual(last, "while(?){")) {
				*v = '\x81';
				v[1] = 'w';
				t = v + 2;
			}
			if (stringEqual(last, "until(*){")) {
				*v = '\x81';
				v[1] = 'U';
				t = v + 2;
			}
			if (stringEqual(last, "until(?){")) {
				*v = '\x81';
				v[1] = 'u';
				t = v + 2;
			}
			if (!strncmp(last, "loop(", 5) && isdigitByte(last[5])) {
				q = last + 6;
				while (isdigitByte(*q))
					++q;
				if (stringEqual(q, "){")) {
					*q = 0;
					last[4] = 'l';
					last[3] = '\x81';
					strcpy(v, last + 3);
					t = v + strlen(v);
				}
			}
			if (!strncmp(last, "function", 8) &&
			    (last[8] == '+' || last[8] == ':')) {
				q = last + 9;
				if (*q == 0 || *q == '{' || *q == '(')
					i_printfExit(MSG_EBRC_NoFnName, ln);
				if (isdigitByte(*q))
					i_printfExit(MSG_EBRC_FnDigit, ln);
				while (isalnumByte(*q))
					++q;
				if (q - last - 9 > 10)
					i_printfExit(MSG_EBRC_FnTooLong, ln);
				if (*q != '{' || q[1])
					i_printfExit(MSG_EBRC_SyntaxErr, ln);
				last[7] = 'f';
				last[6] = '\x81';
				strcpy(v, last + 6);
				t = v + strlen(v);
			}

			*t++ = c;
			v = t;
			++ln;
			startline = true;
			continue;
		}

		if (c == ' ' || c == '\t') {
			if (startline)
				continue;
		} else {
			if (lidx < sizeof(last) - 1)
				last[lidx++] = c;
			startline = false;
		}

putc:
		*t++ = c;
	}
	*t = 0;			/* now it's a string */

/* Go line by line */
	ln = 1;
	nest = 0;
	stack[0] = ' ';

	for (s = buf; *s; s = t + 1, ++ln) {
		t = strchr(s, '\n');
		if (t == s)
			continue;	/* empty line */
		if (t == s + 1 && *s == '#')
			continue;	/* comment */
		*t = 0;		/* I'll put it back later */

/* Gather the filters in a mail filter block */
		if (mailblock > 1 && !strchr("\x81\x82\x83", *s)) {
			v = strchr(s, '>');
			if (!v)
				i_printfExit(MSG_EBRC_NoCondFile, ln);
			while (v > s && (v[-1] == ' ' || v[-1] == '\t'))
				--v;
			if (v == s)
				i_printfExit(MSG_EBRC_NoMatchStr, ln);
			c = *v, *v++ = 0;
			if (c != '>') {
				while (*v != '>')
					++v;
				++v;
			}
			while (*v == ' ' || *v == '\t')
				++v;
			if (!*v)
				i_printfExit(MSG_EBRC_MatchNowh, ln, s);
			if (n_filters == MAXFILTER - 1)
				i_printfExit(MSG_EBRC_Filters, ln);
			mailFilters[n_filters].match = s;
			mailFilters[n_filters].redirect = v;
			mailFilters[n_filters].type = mailblock;
			++n_filters;
			continue;
		}

		v = strchr(s, '=');
		if (!v)
			goto nokeyword;

		while (v > s && (v[-1] == ' ' || v[-1] == '\t'))
			--v;
		if (v == s)
			goto nokeyword;
		c = *v, *v = 0;
		for (q = s; q < v; ++q)
			if (!isalphaByte(*q)) {
				*v = c;
				goto nokeyword;
			}

		n = stringInList(keywords, s);
		if (n < 0) {
			if (!nest)
				i_printfExit(MSG_EBRC_BadKeyword, s, ln);
			*v = c;	/* put it back */
			goto nokeyword;
		}

		if (nest)
			i_printfExit(MSG_EBRC_KeyInFunc, ln);

		if (n < MIMEWORDS && mailblock != 1)
			i_printfExit(MSG_EBRC_MailAttrOut, ln, s);

		if (n >= MIMEWORDS && n < TABLEWORDS && !mimeblock)
			i_printfExit(MSG_EBRC_MimeAttrOut, ln, s);

		if (n >= TABLEWORDS && n < GLOBALWORDS && !tabblock)
			i_printfExit(MSG_EBRC_TableAttrOut, ln, s);

		if (n >= MIMEWORDS && mailblock)
			i_printfExit(MSG_EBRC_MailAttrIn, ln, s);

		if ((n < MIMEWORDS || n >= TABLEWORDS) && mimeblock)
			i_printfExit(MSG_EBRC_MimeAttrIn, ln, s);

		if ((n < TABLEWORDS || n >= GLOBALWORDS) && tabblock)
			i_printfExit(MSG_EBRC_TableAttrIn, ln, s);

/* act upon the keywords */
		++v;
		if (c != '=') {
			while (*v != '=')
				++v;
			++v;
		}
		while (*v == ' ' || *v == '\t')
			++v;
		if (!*v)
			i_printfExit(MSG_EBRC_NoAttr, ln, s);

		switch (n) {
		case 0:	/* inserver */
			act->inurl = v;
			continue;

		case 1:	/* outserver */
			act->outurl = v;
			continue;

		case 2:	/* login */
			act->login = v;
			continue;

		case 3:	/* password */
			act->password = v;
			continue;

		case 4:	/* from */
			act->from = v;
			continue;

		case 5:	/* reply */
			act->reply = v;
			continue;

		case 6:	/* inport */
			if (*v == '*')
				act->inssl = 1, ++v;
			act->inport = atoi(v);
			continue;

		case 7:	/* outport */
			if (*v == '+')
				act->outssl = 4, ++v;
			if (*v == '^')
				act->outssl = 2, ++v;
			if (*v == '*')
				act->outssl = 1, ++v;
			act->outport = atoi(v);
			continue;

		case 8:	/* type */
			mt->type = v;
			continue;

		case 9:	/* desc */
			mt->desc = v;
			continue;

		case 10:	/* suffix */
			mt->suffix = v;
			continue;

		case 11:	/* protocol */
			mt->prot = v;
			mt->stream = true;
			continue;

		case 12:	/* program */
			mt->program = v;
			continue;

		case 13:	/* content */
			mt->content = v;
			continue;

		case 14:	/* outtype */
			c = tolower(*v);
			if (c != 'h' && c != 't')
				i_printfExit(MSG_EBRC_Outtype, ln);
			mt->outtype = c;
			continue;

		case 15:	/* tname */
			td->name = v;
			continue;

		case 16:	/* tshort */
			td->shortname = v;
			continue;

		case 17:	/* cols */
			while (*v) {
				if (td->ncols == MAXTCOLS)
					i_printfExit(MSG_EBRC_ManyCols, ln,
						     MAXTCOLS);
				td->cols[td->ncols++] = v;
				q = strchr(v, ',');
				if (!q)
					break;
				*q = 0;
				v = q + 1;
			}
			continue;

		case 18:	/* keycol */
			if (!isdigitByte(*v))
				i_printfExit(MSG_EBRC_KeyNotNb, ln);
			td->key1 = strtol(v, &v, 10);
			if (*v == ',' && isdigitByte(v[1]))
				td->key2 = strtol(v + 1, &v, 10);
			if (td->key1 > td->ncols || td->key2 > td->ncols)
				i_printfExit(MSG_EBRC_KeyOutRange, ln,
					     td->ncols);
			continue;

		case 19:	/* adbook */
			addressFile = v;
			ftype = fileTypeByName(v, false);
			if (ftype && ftype != 'f')
				i_printfExit(MSG_EBRC_AbNotFile, v);
			continue;

		case 20:	/* downdir */
			downDir = v;
			if (fileTypeByName(v, false) != 'd')
				i_printfExit(MSG_EBRC_NotDir, v);
			continue;

		case 21:	/* maildir */
			mailDir = v;
			if (fileTypeByName(v, false) != 'd')
				i_printfExit(MSG_EBRC_NotDir, v);
			mailUnread = allocMem(strlen(v) + 20);
			sprintf(mailUnread, "%s/unread", v);
/* We need the unread directory, else we can't fetch mail. */
/* Create it if it isn't there. */
			if (fileTypeByName(mailUnread, false) != 'd') {
				if (mkdir(mailUnread, 0700))
					i_printfExit(MSG_EBRC_NotDir,
						     mailUnread);
			}
			mailReply = allocMem(strlen(v) + 20);
			sprintf(mailReply, "%s/.reply", v);
			continue;

		case 22:	/* agent */
			for (j = 0; j < 10; ++j)
				if (!userAgents[j])
					break;
			if (j == 10)
				i_printfExit(MSG_EBRC_ManyAgents, ln);
			userAgents[j] = v;
			continue;

		case 23:	/* jar */
			cookieFile = v;
			ftype = fileTypeByName(v, false);
			if (ftype && ftype != 'f')
				i_printfExit(MSG_EBRC_JarNotFile, v);
			j = open(v, O_WRONLY | O_APPEND | O_CREAT, 0600);
			if (j < 0)
				i_printfExit(MSG_EBRC_JarNoWrite, v);
			close(j);
			continue;

		case 24:	/* nojs */
			if (javaDisCount == MAXNOJS)
				i_printfExit(MSG_EBRC_NoJS, MAXNOJS);
			if (*v == '.')
				++v;
			q = strchr(v, '.');
			if (!q || q[1] == 0)
				i_printfExit(MSG_EBRC_DomainDot, ln, v);
			javaDis[javaDisCount++] = v;
			continue;

		case 26:	/* webtimer */
			webTimeout = atoi(v);
			continue;

		case 27:	/* mailtimer */
			mailTimeout = atoi(v);
			continue;

		case 28:	/* certfile */
			sslCerts = v;
			ftype = fileTypeByName(v, false);
			if (ftype && ftype != 'f')
				i_printfExit(MSG_EBRC_SSLNoFile, v);
			j = open(v, O_RDONLY);
			if (j < 0)
				i_printfExit(MSG_EBRC_SSLNoRead, v);
			close(j);
			continue;

		case 29:	/* datasource */
			setDataSource(v);
			continue;

		case 30:	/* proxy */
			if (maxproxy == MAXPROXY)
				i_printfExit(MSG_EBRC_NoPROXY, MAXPROXY);
			px = proxyEntries + maxproxy;
			maxproxy++;
			spaceCrunch(v, true, true);
			q = strchr(v, ' ');
			if (q) {
				*q = 0;
				if (!stringEqual(v, "*"))
					px->prot = v;
				v = q + 1;
				q = strchr(v, ' ');
				if (q) {
					*q = 0;
					if (!stringEqual(v, "*"))
						px->domain = v;
					v = q + 1;
				}
			}
			if (!stringEqualCI(v, "direct"))
				px->proxy = v;
			continue;

		case 31:	/* linelength */
			displayLength = atoi(v);
			if (displayLength < 80)
				displayLength = 80;
			continue;

		case 32:	/* localizeweb */
/* We should probably allow autodetection of language. */
/* E.G., the keyword auto indicates that you want autodetection. */
			setHTTPLanguage(v);
			continue;

		case 33:	/* jspool */
			jsPool = atoi(v);
			if (jsPool < 2)
				jsPool = 2;
			if (jsPool > 1000)
				jsPool = 1000;
			continue;

		case 34:	/* novs */
			if (*v == '.')
				++v;
			q = strchr(v, '.');
			if (!q || q[1] == 0)
				i_printfExit(MSG_EBRC_DomainDot, ln, v);
			addNovsHost(v);
			continue;

		default:
			i_printfExit(MSG_EBRC_KeywordNYI, ln, s);
		}		/* switch */

nokeyword:

		if (stringEqual(s, "default") && mailblock == 1) {
			if (localAccount == maxAccount + 1)
				continue;
			if (localAccount)
				i_printfExit(MSG_EBRC_SevDefaults);
			localAccount = maxAccount + 1;
			continue;
		}

		if (stringEqual(s, "nofetch") && mailblock == 1) {
			act->nofetch = true;
			continue;
		}

		if (stringEqual(s, "secure") && mailblock == 1) {
			act->secure = true;
			continue;
		}

		if (stringEqual(s, "imap") && mailblock == 1) {
			act->imap = act->nofetch = true;
			continue;
		}

		if (stringEqual(s, "download") && mimeblock == 1) {
			mt->download = true;
			continue;
		}
		if (stringEqual(s, "stream") && mimeblock == 1) {
			mt->stream = true;
			continue;
		}

		if (*s == '\x82' && s[1] == 0) {
			if (mailblock == 1) {
				++maxAccount;
				mailblock = 0;
				if (!act->inurl)
					i_printfExit(MSG_EBRC_NoInserver, ln);
				if (!act->outurl)
					i_printfExit(MSG_EBRC_NoOutserver, ln);
				if (!act->login)
					i_printfExit(MSG_EBRC_NoLogin, ln);
				if (!act->password)
					i_printfExit(MSG_EBRC_NPasswd, ln);
				if (!act->from)
					i_printfExit(MSG_EBRC_NoFrom, ln);
				if (!act->reply)
					i_printfExit(MSG_EBRC_NoReply, ln);
				if (act->secure)
					act->inssl = act->outssl = 1;
				if (!act->inport)
					if (act->secure)
						act->inport =
						    (act->imap ? 993 : 995);
					else
						act->inport =
						    (act->imap ? 143 : 110);
				if (!act->outport)
					act->outport = (act->secure ? 465 : 25);
				continue;
			}

			if (mailblock) {
				mailblock = 0;
				continue;
			}

			if (mimeblock) {
				++maxMime;
				mimeblock = false;
				if (!mt->type)
					i_printfExit(MSG_EBRC_NoType, ln);
				if (!mt->desc)
					i_printfExit(MSG_EBRC_NDesc, ln);
				if (!mt->suffix && !mt->prot)
					i_printfExit(MSG_EBRC_NoSuffix, ln);
				if (!mt->program)
					i_printfExit(MSG_EBRC_NoProgram, ln);
				continue;
			}

			if (tabblock) {
				++numTables;
				tabblock = false;
				if (!td->name)
					i_printfExit(MSG_EBRC_NoTblName, ln);
				if (!td->shortname)
					i_printfExit(MSG_EBRC_NoShortName, ln);
				if (!td->ncols)
					i_printfExit(MSG_EBRC_NColumns, ln);
				continue;
			}

			if (--nest < 0)
				i_printfExit(MSG_EBRC_UnexpBrace, ln);
			if (nest)
				goto putback;
/* This ends the function */
			*s = 0;	/* null terminate the script */
			++sn;
			continue;
		}

		if (*s == '\x83' && s[1] == 0) {
/* Does else make sense here? */
			c = toupper(stack[nest]);
			if (c != 'I')
				i_printfExit(MSG_EBRC_UnexElse, ln);
			goto putback;
		}

		if (*s != '\x81') {
			if (!nest)
				i_printfExit(MSG_EBRC_GarblText, ln);
			goto putback;
		}

/* Starting something */
		c = s[1];
		if ((nest || mailblock || mimeblock) && strchr("fmerts", c)) {
			const char *curblock = "another function";
			if (mailblock)
				curblock = "a mail descriptor";
			if (mailblock > 1)
				curblock = "a filter block";
			if (mimeblock)
				curblock = "a plugin descriptor";
			i_printfExit(MSG_EBRC_FnNotStart, ln, curblock);
		}

		if (!strchr("fmertsb", c) && !nest)
			i_printfExit(MSG_EBRC_StatNotInFn, ln);

		if (c == 'm') {
			mailblock = 1;
			if (maxAccount == MAXACCOUNT)
				i_printfExit(MSG_EBRC_ManyAcc, MAXACCOUNT);
			act = accounts + maxAccount;
			continue;
		}

		if (c == 'e') {
			mimeblock = true;
			if (maxMime == MAXMIME)
				i_printfExit(MSG_EBRC_ManyTypes, MAXMIME);
			mt = mimetypes + maxMime;
			continue;
		}

		if (c == 'b') {
			tabblock = true;
			if (numTables == MAXDBT)
				i_printfExit(MSG_EBRC_ManyTables, MAXDBT);
			td = dbtables + numTables;
			continue;
		}

		if (c == 'r') {
			mailblock = 2;
			continue;
		}

		if (c == 't') {
			mailblock = 3;
			continue;
		}

		if (c == 's') {
			mailblock = 4;
			continue;
		}

		if (c == 'f') {
			stack[++nest] = c;
			if (sn == MAXEBSCRIPT)
				i_printfExit(MSG_EBRC_ManyFn, sn);
			ebScriptName[sn] = s + 2;
			t[-1] = 0;
			ebScript[sn] = t;
			goto putback;
		}

		if (++nest >= sizeof(stack))
			i_printfExit(MSG_EBRC_TooDeeply, ln);
		stack[nest] = c;

putback:
		*t = '\n';
	}			/* loop over lines */

	if (nest)
		i_printfExit(MSG_EBRC_FnNotClosed, ebScriptName[sn]);

	if (mailblock | mimeblock)
		i_printfExit(MSG_EBRC_MNotClosed);

	if (!sslCerts)
		verifyCertificates = 0;
}				/* readConfigFile */
