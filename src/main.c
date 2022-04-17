/* main.c
 * Entry point, arguments and options.
 * This file is part of the edbrowse project, released under GPL.
 */

#include "eb.h"

#include <pthread.h>
#include <signal.h>

/* Define the globals that are declared in eb.h. */
/* See eb.h for descriptive comments. */

const char *progname;
const char eol[] = "\r\n";
const char *version = "3.8.2.1+";
char *changeFileName;
char *configFile, *addressFile, *cookieFile, *emojiFile;
char *mailDir, *mailUnread, *mailStash, *mailReply;
char *recycleBin, *sigFile, *sigFileEnd;
char *cacheDir;
int cacheSize = 1000, cacheCount = 10000;
char *ebTempDir, *ebUserDir;
char *userAgents[MAXAGENT + 1];
char *currentAgent;
int agentIndex;
char selsep = ',';
bool allowRedirection = true, allowJS = true, sendReferrer = true;
bool blockJS;
bool ftpActive;
int webTimeout = 20, mailTimeout = 0;
int displayLength = 500;
int verifyCertificates = 1;
char *sslCerts;
int localAccount, maxAccount;
struct MACCOUNT accounts[MAXACCOUNT];
int maxMime;
struct MIMETYPE mimetypes[MAXMIME];
static struct DBTABLE dbtables[MAXDBT];
static int numTables;
volatile bool intFlag;
time_t intStart;
bool ismc, isimap, passMail;
// next two variables work around curl bug 7284
bool inInput, inInitFunction, listNA;
int fileSize;
char *dbarea, *dblogin, *dbpw;	/* to log into the database */
bool fetchBlobColumns;
bool caseInsensitive, searchStringsAll, searchWrap = true, ebre = true;
bool binaryDetect = true;
bool inputReadLine;
bool curlAuthNegotiate = false;
int context = 1;
pst linePending;
struct ebSession sessionList[MAXSESSION], *cs;
int maxSession;

/*********************************************************************
Redirect the incoming mail into a file, based on the subject or the sender.
Along with the filters in .ebrc, this routine dips into your addressbook,
to see if the sender (by email) is one of your established aliases.
If it is, we save it in a file of the same name.
This is saved formatted, unless you put a minus sign
at the start of the alias in your address book.
This is the same convention as the from filters in .ebrc.
If you don't want an alias to act as a redirect filter,
put a ! at the beginning of the alias name.
*********************************************************************/

static void *inputForever(void *ptr);
static pthread_t foreground_thread;
static void finishBrowse(void)
{
	Tag *t;
	int j;
	char *a, *newbuf;
	Frame *f;
// tags should certainly be set
	if (!tagList)
		return;
// set all scripts to complete
	for (t = cw->scriptlist; t; t = t->same)
		t->step = 6;
// kill any timers or pendings
	for (f = &cw->f0; f; f = f->next)
		delTimers(f);
	if (allowJS) {
		allowJS = false;
		blockJS = true;
		i_puts(MSG_JavaOff);
	}
	if (cw->browseMode)
		return;
// We were in the middle of the browse command; this is typical.
	a = render(0);
	newbuf = htmlReformat(a);
	nzFree(a);
	cw->rnlMode = cw->nlMode;
	cw->nlMode = false;
	cw->binMode = false;
	cw->r_dot = cw->dot, cw->r_dol = cw->dol;
	cw->dot = cw->dol = 0;
	cw->r_map = cw->map;
	cw->map = 0;
	memcpy(cw->r_labels, cw->labels, sizeof(cw->labels));
	memset(cw->labels, 0, sizeof(cw->labels));
	j = strlen(newbuf);
	addTextToBuffer((pst) newbuf, j, 0, false);
	free(newbuf);
	cw->undoable = false;
	cw->changeMode = false;
	if (cf->fileName) {
		j = strlen(cf->fileName);
		cf->fileName = reallocMem(cf->fileName, j + 8);
		strcat(cf->fileName, ".browse");
	}
	cw->browseMode = cf->browseMode = true;
}

/* Catch interrupt and react appropriately. */
static void catchSig(int n)
{
	time_t now;
	int d;
	pthread_t t1 = foreground_thread, t2;
	intFlag = true;
/* If we were reading from a file, or socket, this signal should
 * cause the read to fail.  Check for intFlag, so we know it was
 * interrupted, and not an io failure.
 * Then clean up appropriately. */
	signal(SIGINT, catchSig);
	if (inInput) {
		i_puts(MSG_EnterInterrupt);
		return;
	}
	if (!intStart) {	// first time hitting ^C
		time(&intStart);
		puts("0");
		return;
	}
	time(&now);
	d = now - intStart;
// should 45 seconds be configurable?
	if(d < 45) {
		printf("%d\n", d);
		return;
	}
// Let's do something drastic here; start a new thread and exit the current one.
	i_puts(MSG_IntForce);
	finishBrowse();
	if (pthread_equal(t1, pthread_self())) {
		if (pthread_create(&t2, NULL, inputForever, NULL))
			return;	// didn't work
		pthread_exit(NULL);
	} else {
		pthread_kill(t1, SIGINT);
	}
}

void setDataSource(char *v)
{
	dbarea = dblogin = dbpw = 0;
	if (!v)
		return;
	if (!*v)
		return;
	dbarea = v;
	v = strchr(v, ',');
	if (!v)
		return;
	*v++ = 0;
	dblogin = v;
	v = strchr(v, ',');
	if (!v)
		return;
	*v++ = 0;
	dbpw = v;
}

void ebClose(int n)
{
	bg_jobs(true);
	dbClose();
	if (curlActive) {
		mergeCookies();
		eb_curl_global_cleanup();
	}
	exit(n);
}

static struct ebhost {
// j = nojs, v = novs, p = proxy, f = function,
// s = subject, t = to, r = reply, a = agentsite
	char type, filler;
	short n;
// watch out, these fields are highly overloaded, depending on type
	char *host;
// for proxy entry we also have
	char *prot, *domain;
} *ebhosts;
static size_t ebhosts_avail, ebhosts_max;

static void add_ebhost(char *host, char type)
{
	if (ebhosts_max == 0) {
		ebhosts_max = 32;
		ebhosts = allocZeroMem(ebhosts_max * sizeof(struct ebhost));
	} else if (ebhosts_avail >= ebhosts_max) {
		ebhosts_max *= 2;
		ebhosts =
		    reallocMem(ebhosts, ebhosts_max * sizeof(struct ebhost));
	}
	ebhosts[ebhosts_avail].host = host;
	ebhosts[ebhosts_avail++].type = type;
}

static void delete_ebhosts(void)
{
	nzFree(ebhosts);
	ebhosts = NULL;
	ebhosts_avail = ebhosts_max = 0;
}

static void add_proxy(char *v)
{
	char *q;
	char *prot = 0, *domain = 0, *proxy = 0;
	spaceCrunch(v, true, true);
	q = strchr(v, ' ');
	if (q) {
		*q = 0;
		if (!stringEqual(v, "*"))
			prot = v;
		v = q + 1;
		q = strchr(v, ' ');
		if (q) {
			*q = 0;
			if (!stringEqual(v, "*"))
				domain = v;
			v = q + 1;
		}
	}
	if (!stringEqualCI(v, "direct"))
		proxy = v;
	add_ebhost(proxy, 'p');
	ebhosts[ebhosts_avail - 1].prot = prot;
	ebhosts[ebhosts_avail - 1].domain = domain;
}

// Are we ok to parse and execute javascript?
bool javaOK(const char *url)
{
	int j;
	if (!allowJS)
		return false;
	if (isDataURI(url))
		return true;
	for (j = 0; (unsigned)j < ebhosts_avail; ++j)
		if (ebhosts[j].type == 'j' &&
		    patternMatchURL(url, ebhosts[j].host))
			return false;
	return true;
}

/* Return true if the cert for this host should be verified. */
bool mustVerifyHost(const char *url)
{
	unsigned i;
	if (!verifyCertificates)
		return false;
	for (i = 0; i < ebhosts_avail; i++)
		if (ebhosts[i].type == 'v' &&
		    patternMatchURL(url, ebhosts[i].host))
			return false;
	return true;
}

/*********************************************************************
Given a protocol and a domain, find the proxy server
to mediate your request.
This is the C version, using entries in .ebrc.
There is a javascript version of the same name, that we will support later.
This is a beginning, and it can be used even when javascript is disabled.
A return of null means DIRECT, and this is the default
if we don't match any of the proxy entries.
*********************************************************************/

const char *findProxyForURL(const char *url)
{
	struct ebhost *px = ebhosts;
	unsigned i;
	char prot[MAXPROTLEN], host[MAXHOSTLEN];

	if (!getProtHostURL(url, prot, host)) {
/* this should never happen */
		return 0;
	}

/* first match wins */
	for (i = 0; i < ebhosts_avail; ++i, ++px) {
		if (px->type != 'p')
			continue;

		if (px->prot) {
			char *s = px->prot;
			char *t;
			int rc;
			while (*s) {
				t = strchr(s, '|');
				if (t)
					*t = 0;
				rc = stringEqualCI(s, prot);
				if (t)
					*t = '|';
				if (rc)
					goto domain;
				if (!t)
					break;
				s = t + 1;
			}
			continue;
		}

domain:
		if (!px->domain || patternMatchURL(url, px->domain))
			return px->host;
	}

	return 0;
}

const char *findAgentForURL(const char *url)
{
	struct ebhost *px = ebhosts;
	unsigned i;
	for (i = 0; i < ebhosts_avail; ++i, ++px)
		if (px->type == 'a' && patternMatchURL(url, px->host))
			return userAgents[px->n];
	return 0;
}

const char *mailRedirect(const char *to, const char *from,
			 const char *reply, const char *subj)
{
	int rlen = strlen(reply);
	int slen = strlen(subj);
	int tlen = strlen(to);
	int mlen;		// length of match string
	unsigned i;
	int k;
	struct ebhost *f = ebhosts;
	const char *m, *r;	// match and redirect

	for (i = 0; i < ebhosts_avail; ++i, ++f) {
		char type = f->type;
		if (strchr("rts", type)) {
			m = f->prot;
			mlen = strlen(m);
			r = f->host;
		}

		switch (type) {
		case 'r':
			if (stringEqualCI(m, from))
				return r;
			if (stringEqualCI(m, reply))
				return r;
			if (*m == '@' && mlen < rlen &&
			    stringEqualCI(m, reply + rlen - mlen))
				return r;
			break;

		case 't':
			if (stringEqualCI(m, to))
				return r;
			if (*m == '@' && mlen < tlen
			    && stringEqualCI(m, to + tlen - mlen))
				return r;
			break;

		case 's':
			if (mlen > slen)
				break;
			if (mlen == slen) {
				if (stringEqualCI(m, subj))
					return r;
				break;
			}
/* a prefix or suffix match is ok */
/* have to be at least half the subject line */
			if (slen > mlen + mlen)
				break;
			if (memEqualCI(m, subj, mlen))
				return r;
			k = slen - mlen;
			if (memEqualCI(m, subj + k, mlen))
				return r;
			break;
		}		/* switch */
	}			/* loop */

	r = reverseAlias(reply);
	return r;
}

static void setupEdbrowseTempDirectory(void)
{
	int userid;
#ifdef DOSLIKE
	int l;
	char *a;
	ebTempDir = getenv("TEMP");
	if (!ebTempDir) {
		i_printf(MSG_NoEnvVar, "TEMP");
		nl();
		exit(1);
	}
// put /edbrowse on the end
	l = strlen(ebTempDir);
	a = allocString(l + 10);
	sprintf(a, "%s/edbrowse", ebTempDir);
	ebTempDir = a;
	userid = 0;
#else
	ebTempDir = getenv("TMPDIR");
	if (!ebTempDir) {
		ebTempDir="/tmp/.edbrowse";
	}
	userid = geteuid();
#endif

// On a multiuser system, mkdir /tmp/.edbrowse at startup,
// by root, and then chmod 1777

	if (fileTypeByName(ebTempDir, false) != 'd') {
/* no such directory, try to make it */
/* this temp edbrowse directory is used by everyone system wide */
		if (mkdir(ebTempDir, MODE_rwx)) {
			i_printf(MSG_TempDir, ebTempDir);
			ebTempDir = 0;
			return;
		}
#ifndef DOSLIKE
// yes, we called mkdir with 777 above, but that was cut by umask.
		chmod(ebTempDir, MODE_rwx);
#endif
	}
// make room for user ID on the end
	ebUserDir = allocMem(strlen(ebTempDir) + 30);
	sprintf(ebUserDir, "%s/edbrowse.%d", ebTempDir, userid);
	if (fileTypeByName(ebUserDir, false) != 'd') {
/* no such directory, try to make it */
		if (mkdir(ebUserDir, 0700)) {
			i_printf(MSG_TempDir, ebUserDir);
			ebUserDir = 0;
			return;
		}
		chmod(ebUserDir, 0700);
	}
}

/*\ MSVC Debug: May need to provide path to 3rdParty DLLs, like
 *  set PATH=F:\Projects\software\bin;%PATH% ...
\*/

/* I'm not going to expand wild card arguments here.
 * I don't need to on Unix, and on Windows there is a
 * setargv.obj, or something like that, that performs the expansion.
 * I'll assume you have folded that object into libc.lib.
 * So now you can edit *.c, on any operating system,
 * and it will do the right thing, with no work on my part. */

static void loadReplacements(void);

int main(int argc, char **argv)
{
	int cx, account;
	bool rc, doConfig = true, autobrowse = false;
	bool dofetch = false, domail = false, setDebugOpt = false;
	static char agent0[64] = "edbrowse/";

#ifndef _MSC_VER		// port setlinebuf(stdout);, if required...
/* In case this is being piped over to a synthesizer, or whatever. */
	if (fileTypeByHandle(fileno(stdout)) != 'f')
		setlinebuf(stdout);
#endif // !_MSC_VER

	selectLanguage();
	setHTTPLanguage(eb_language);

/* Establish the home directory, and standard edbrowse files thereunder. */
	home = getenv("HOME");
#ifdef _MSC_VER
	if (!home) {
		home = getenv("APPDATA");
		if (home) {
			char *ebdata = (char *)allocMem(ABSPATH);
			sprintf(ebdata, "%s\\edbrowse", home);
			if (fileTypeByName(ebdata, false) != 'd') {
				FILE *fp;
				char *cfgfil;
				if (mkdir(ebdata, 0700)) {
					i_printfExit(MSG_NotHome);	// TODO: more appropriate exit message...
				}
				cfgfil = (char *)allocMem(ABSPATH);
				sprintf(cfgfil, "%s\\.ebrc", ebdata);
				fp = fopen(cfgfil, "w");
				if (fp) {
					fwrite(ebrc_string, 1,
					       strlen(ebrc_string), fp);
					fclose(fp);
				}
				i_printfExit(MSG_Personalize, cfgfil);

			}
			home = ebdata;
		}
	}
#endif // !_MSC_VER

/* Empty is the same as missing. */
	if (home && !*home)
		home = 0;
/* I require this, though I'm not sure what this means for non-Unix OS's */
	if (!home)
		i_printfExit(MSG_NotHome);
	if (fileTypeByName(home, false) != 'd')
		i_printfExit(MSG_NotDir, home);

	configFile = allocMem(strlen(home) + 7);
	sprintf(configFile, "%s/.ebrc", home);
/* if not present then create it, as was done above */
	if (fileTypeByName(configFile, false) == 0) {
		int fh = creat(configFile, MODE_private);
		if (fh >= 0) {
			write(fh, ebrc_string, strlen(ebrc_string));
			close(fh);
			i_printfExit(MSG_Personalize, configFile);
		}
	}

/* recycle bin and .signature files are unix-like, and not adjusted for windows. */
	recycleBin = allocMem(strlen(home) + 8);
	sprintf(recycleBin, "%s/.Trash", home);
	if (fileTypeByName(recycleBin, false) != 'd') {
		if (mkdir(recycleBin, 0700)) {
/* Don't want to abort here; we might be on a readonly filesystem.
 * Don't have a Trash directory and can't creat one; yet we should move on. */
			free(recycleBin);
			recycleBin = 0;
		}
	}

	if (recycleBin) {
		mailStash = allocMem(strlen(recycleBin) + 12);
		sprintf(mailStash, "%s/rawmail", recycleBin);
		if (fileTypeByName(mailStash, false) != 'd') {
			if (mkdir(mailStash, 0700)) {
				free(mailStash);
				mailStash = 0;
			}
		}
	}

	sigFile = allocMem(strlen(home) + 20);
	sprintf(sigFile, "%s/.signature", home);
	sigFileEnd = sigFile + strlen(sigFile);

	strcat(agent0, version);
	userAgents[0] = currentAgent = agent0;

	setupEdbrowseTempDirectory();

	progname = argv[0];
	++argv, --argc;

	ttySaveSettings();
	initializeReadline();

	loadReplacements();

	if (argc && stringEqual(argv[0], "-c")) {
		if (argc == 1) {
			*argv = configFile;
			doConfig = false;
		} else {
			configFile = argv[1];
			argv += 2, argc -= 2;
		}
	}
	if (doConfig)
		readConfigFile();
	account = localAccount;

	for (; argc && argv[0][0] == '-'; ++argv, --argc) {
		char *s = *argv;
		++s;

		if (stringEqual(s, "v")) {
			puts(version);
			exit(0);
		}

		if (stringEqual(s, "d")) {
			setDebugOpt = true;
			debugLevel = 4;
			continue;
		}

		if (*s == 'd' && isdigitByte(s[1]) && !s[2]) {
			setDebugOpt = true;
			debugLevel = s[1] - '0';
			continue;
		}

		if (stringEqual(s, "e")) {
			errorExit = true;
			continue;
		}

		if (stringEqual(s, "b")) {
			autobrowse = true;
			continue;
		}

		if (*s == 'p')
			++s, passMail = true;

		if (*s == 'm' || *s == 'f') {
			if (!maxAccount)
				i_printfExit(MSG_NoMailAcc);
			if (*s == 'f') {
				account = 0;
				dofetch = true;
				++s;
				if (*s == 'm')
					domail = true, ++s;
			} else {
				domail = true;
				++s;
			}
			if (isdigitByte(*s)) {
				account = strtol(s, &s, 10);
				if (account == 0 || account > maxAccount)
					i_printfExit(MSG_BadAccNb, maxAccount);
			}
			if (!*s) {
				ismc = true;	/* running as a mail client */
				allowJS = false;	/* no javascript in mail client */
				eb_curl_global_init();
				++argv, --argc;
				if (!argc || !dofetch)
					break;
			}
		}

		i_printfExit(MSG_Usage);
	}			/* options */

	srand(time(0));

	if (ismc) {
		char **reclist, **atlist;
		char **reclist2, **atlist2;
		char *s, *body;
		int nat, nalt, nrec;

		if (!argc) {
/* This is fetch / read mode */
			if (dofetch) {
				int nfetch = 0;
				if (account) {
					isimap = accounts[account - 1].imap;
					if (isimap)
						domail = false;
					nfetch = fetchMail(account);
				} else {
					nfetch = fetchAllMail();
				}
				if (!domail) {
					if (nfetch)
						i_printf(MSG_MessagesX, nfetch);
					else
						i_puts(MSG_NoMail);
				}
			}

			if (domail) {
				scanMail();
			}

			exit(0);
		}

/* now in sendmail mode */
		if (argc == 1)
			i_printfExit(MSG_MinOneRec);
/* I don't know that argv[argc] is 0, or that I can set it to 0,
 * so I back everything up by 1. */
		reclist = argv - 1;
		for (nat = nalt = 0; nat < argc; ++nat) {
			s = argv[argc - 1 - nat];
			if (*s != '+' && *s != '-')
				break;
			if (*s == '-')
				++nalt;
			strmove(s, s + 1);
		}
		atlist = argv + argc - nat - 1;
		if (atlist <= argv)
			i_printfExit(MSG_MinOneRecBefAtt);
		body = *atlist;
		if (nat)
			memmove(atlist, atlist + 1, sizeof(char *) * nat);
		atlist[nat] = 0;
		nrec = atlist - argv;
		memmove(reclist, reclist + 1, sizeof(char *) * nrec);
		atlist[-1] = 0;

// Make room for possible recipients or attachments in the mail descriptor.
		atlist2 = allocMem(sizeof(char*) * (nat + MAXCC + 1));
		memcpy(atlist2, atlist, sizeof(char*) * nat);
		atlist2[nat] = 0;
		reclist2 = allocMem(sizeof(char*) * (nrec + MAXCC + 1));
		memcpy(reclist2, reclist, sizeof(char*) * nrec);
		reclist2[nrec] = 0;
// These don't get freed, but we're going to exit anyways.

		if (sendMail(account, (const char **)reclist2, body, 1,
			     (const char **)atlist2, 0, nalt, true))
			exit(0);
		showError();
		exit(1);
	}

	signal(SIGINT, catchSig);

	js_main();

// This sanity check on number of files assumes they are all files,
// not functions to execute.
	if(argc >= MAXSESSION)
		i_printfExit(MSG_ManyOpen, MAXSESSION - 1);

	cx = 0;
	while (argc) {
		char *file = *argv;
		char *file2 = 0;	// will be allocated
		++cx;
		if (cx == MAXSESSION)
			i_printfExit(MSG_ManyOpen, MAXSESSION - 1);
		cxSwitch(cx, false);
		if (cx == 1) {
			inInitFunction = setDebugOpt;
			runEbFunction("init");
			inInitFunction = false;
		}

// function on the command line
		if (file[0] == '<') {
			runEbFunction(file + 1);
			++argv, --argc;
			continue;
		}

		changeFileName = 0;
		file2 = allocMem(strlen(file) + 10);
// Every URL needs a protocol.
		if (missingProtURL(file))
			sprintf(file2 + 2, "http://%s", file);
		else
			strcpy(file2 + 2, file);
		file = file2 + 2;

		if (autobrowse) {
			const struct MIMETYPE *mt;
			uchar sxfirst = 0;
			if (isURL(file))
				mt = findMimeByURL(file, &sxfirst);
			else
				mt = findMimeByFile(file);
			if (mt && !mt->outtype)
				playBuffer("pb", file);
			else {
				file2[0] = 'b';
				file2[1] = ' ';
				if (runCommand(file2))
					debugPrint(1, "%d", fileSize);
				else
					showError();
			}

		} else {

			char *newhash = findHash(file);
			if(newhash)
				newhash = cloneString(newhash + 1);
			cf->fileName = cloneString(file);
			cf->firstURL = cloneString(file);
			if (isSQL(file))
				cw->sqlMode = true;
			rc = readFileArgv(file, 0, 0);
			if (fileSize >= 0)
				debugPrint(1, "%d", fileSize);
			fileSize = -1;
			if (!rc) {
				showError();
			} else if (changeFileName) {
				nzFree(cf->fileName);
				cf->fileName = changeFileName;
				if(redirect_count) {
					nzFree(newhash);
					newhash = findHash(changeFileName);
					if(newhash) {
						*newhash++ = 0;
						newhash = cloneString(newhash);
					}
				}
				changeFileName = 0;
			}
			cw->undoable = cw->changeMode = false;
/* Browse the text if it's a url */
			if (rc && isURL(cf->fileName)
			    && ((cf->mt && cf->mt->outtype)
				|| isBrowseableURL(cf->fileName))) {
				if (runCommand("b")) {
					debugPrint(1, "%d", fileSize);
					if(newhash) {
						set_location_hash(newhash);
						if(!jump2anchor(0, newhash))
							showError();
					}
				} else
					showError();
			}
			nzFree(newhash);
		}

		nzFree(file2);
		++argv, --argc;
	}			/* loop over files */
	if (!cx) {		/* no files */
		++cx;
		cxSwitch(cx, false);
		inInitFunction = setDebugOpt;
		runEbFunction("init");
		inInitFunction = false;
		if(debugLevel >= 1)
			i_puts(MSG_Ready);
	}
	if (cx > 1)
		cxSwitch(1, false);

	inputForever(NULL);
	return 0;
}

static void *inputForever(void *ptr)
{
	foreground_thread = pthread_self();
	while (true) {
		pst p = inputLine();
		pst save_p = clonePstring(p);
		if (perl2c((char *)p)) {
			i_puts(MSG_EnterNull);
			nzFree(save_p);
		} else {
			edbrowseCommand((char *)p, false);
			nzFree(linePending);
			linePending = save_p;
		}
	}			/* infinite loop */
	return NULL;
}

/* Find the balancing brace in an edbrowse function */
static const char *balance(const char *ip, int direction)
{
	int nest = 0;
	uchar code;

	while (true) {
		if (direction > 0) {
			ip = strchr(ip, '\n') + 1;
		} else {
			for (ip -= 2; *ip != '\n'; --ip) ;
			++ip;
		}
		code = *ip;
		if (code == 0x83) {
			if (nest)
				continue;
			break;
		}
		if (code == 0x81)
			nest += direction;
		if (code == 0x82)
			nest -= direction;
		if (nest < 0)
			break;
	}

	return ip;
}

#define MAXNEST 20		// nested blocks
/* Run an edbrowse function, as defined in the config file. */
/* This function must be reentrant. */
int runEbFunction(const char *line)
{
	char *linecopy = cloneString(line);
	char *fncopy = 0;
	char *allargs = 0;
	const char *args[10];
	int argl[10];		/* lengths of args */
	const char *s;
	char *t, *new;
	int l, nest;
	unsigned j;
	const char *ip;		// think instruction pointer
	const char *endl;	// end of line to be processed
	bool nofail, ok, restore = false;
	uchar code;
	int rc = -1;
	char stack[MAXNEST];
	int loopcnt[MAXNEST];

// local copies of settings, to restore after function runs.
	struct {
		bool rl, endm, lna, H, ci, sg, su8, sw, ebre, bd, iu, hf, hr, vs, sr, can, ftpa, bg, jsbg, js, showall, pg, fbc, ls_reverse;
		uchar dw, ls_sort;
		char lsformat[12], showProgress;
		char *currentAgent;
		int agentIndex, debugLevel, timerspeed;
	} save;

/* Separate function name and arguments */
	spaceCrunch(linecopy, true, false);
	if (linecopy[0] == 0) {
		setError(MSG_NoFunction);
		goto done;
	}
	memset(args, 0, sizeof(args));
	memset(argl, 0, sizeof(argl));
	t = strchr(linecopy, ' ');
	if (t)
		*t = 0;
	for (s = linecopy; *s; ++s)
		if (!isalnumByte(*s)) {
			setError(MSG_BadFunctionName);
			goto done;
		}
	for (j = 0; j < ebhosts_avail; ++j)
		if (ebhosts[j].type == 'f' &&
		    stringEqualCI(linecopy, ebhosts[j].prot + 1))
			break;
	if (j == ebhosts_avail) {
		setError(MSG_NoSuchFunction, linecopy);
		goto done;
	}
// This or a downstream function could invoke config.
// Don't know why anybody would do that!
	fncopy = cloneString(ebhosts[j].host);
/* skip past the leading \n */
	ip = fncopy + 1;
	nofail = (ebhosts[j].prot[0] == '+');
	nest = 0;
	ok = true;

	if(!stringEqual(linecopy, "init") && strncmp(linecopy, "set", 3)) {
		restore = true;
		save.currentAgent = currentAgent, save.agentIndex = agentIndex;
		save.debugLevel = debugLevel;
		save.timerspeed = timerspeed;
		save.dw = dirWrite;
		save.ls_sort = ls_sort;
		strcpy(save.lsformat, lsformat);
		save.showProgress = showProgress;
		save.bg = down_bg;
		save.jsbg = down_jsbg;
		save.iu = iuConvert;
		save.bd = binaryDetect;
		save.rl = inputReadLine;
		save.can = curlAuthNegotiate;
		save.lna = listNA;
		save.fbc = fetchBlobColumns;
		save.ls_reverse = ls_reverse;
		save.sw = searchWrap;
		save.ebre = ebre;
		save.hr = allowRedirection;
		save.sr = sendReferrer;
		save.js = allowJS;
		save.ftpa = ftpActive;
		save.H = helpMessagesOn;
		save.pg = pluginsOn;
		save.hf = showHiddenFiles;
		save.showall = showHover;
		save.endm = endMarks;
		save.sg = searchStringsAll;
		save.ci = caseInsensitive;
		save.su8 = re_utf8;
		save.vs = verifyCertificates;
	}

/* collect arguments, ~0 first */
	if (t) {
		args[0] = allargs = cloneString(t + 1);
		argl[0] = strlen(allargs);
	} else {
		args[0] = allargs = emptyString;
		argl[0] = 0;
	}

	j = 0;
	for (s = t; s; s = t) {
		if (++j >= 10) {
//                      setError(MSG_ManyArgs);
//                      goto done;
			break;
		}
		args[j] = ++s;
		t = strchr(s, ' ');
		if (t)
			*t = 0;
		argl[j] = strlen(s);
	}

	while ((code = *ip)) {
		if (intFlag) {
			setError(MSG_Interrupted);
			goto done;
		}
		endl = strchr(ip, '\n');

		if (code == 0x83) {
			ip = balance(ip, 1) + 2;
			--nest;
			continue;
		}

		if (code == 0x82) {
			char control = stack[nest];
			char ucontrol = toupper(control);
			const char *start = balance(ip, -1);
			start = strchr(start, '\n') + 1;
			if (ucontrol == 'L') {	/* loop */
				if (--loopcnt[nest])
					ip = start;
				else
					ip = endl + 1, --nest;
				continue;
			}
			if (ucontrol == 'W' || ucontrol == 'U') {
				bool jump = ok;
				if (islowerByte(control))
					jump ^= true;
				if (ucontrol == 'U')
					jump ^= true;
				ok = true;
				if (jump)
					ip = start;
				else
					ip = endl + 1, --nest;
				continue;
			}
/* Apparently it's the close of an if or an else, just fall through */
			--nest;
			goto nextline;
		}

		if (code == 0x81) {
			const char *skip = balance(ip, 1);
			bool jump;
			char control = ip[1];
			char ucontrol = toupper(control);
			stack[++nest] = control;
			if (ucontrol == 'L') {
				loopcnt[nest] = j = atoi(ip + 2);
				if (j)
					goto nextline;
ahead:
				if (*skip == (char)0x82)
					--nest;
				ip = skip + 2;
				continue;
			}
			if (ucontrol == 'U')
				goto nextline;
/* if or while, test on ok */
			jump = ok;
			if (isupperByte(control))
				jump ^= true;
			ok = true;
			if (jump)
				goto ahead;
			goto nextline;
		}

		if (!ok && nofail)
			goto soft_fail;

/* compute length of line, then build the line */
		l = endl - ip;
		for (s = ip; s < endl; ++s)
			if (*s == '~' && isdigitByte(s[1]))
				l += argl[s[1] - '0'];
		t = new = allocMem(l + 1);
		for (s = ip; s < endl; ++s) {
			if (*s == '~' && isdigitByte(s[1])) {
				j = *++s - '0';
				if (!args[j]) {
					setError(MSG_NoArgument, j);
					nzFree(new);
					goto done;
				}
				strcpy(t, args[j]);
				t += argl[j];
				continue;
			}
			*t++ = *s;
		}
		*t = 0;

/* Here we go! */
		debugPrint(3, "< %s", new);
		if (cw->mustrender) {
			time_t now;
			time(&now);
			if (now >= cw->nextrender)
				rerender(-1);
		}
		jClearSync();
		ok = edbrowseCommand(new, true);
		free(new);

nextline:
		ip = endl + 1;
	}

	if (!ok && nofail)
		goto soft_fail;

	rc = 1;

done:
	nzFree(linecopy);
	nzFree(fncopy);
	nzFree(allargs);
	if(restore) {
		currentAgent = save.currentAgent, agentIndex = save.agentIndex;
		debugLevel = save.debugLevel;
		timerspeed = save.timerspeed;
		dirWrite = save.dw;
		ls_sort = save.ls_sort;
		strcpy(lsformat, save.lsformat);
		showProgress = save.showProgress;
		down_bg = save.bg;
		down_jsbg = save.jsbg;
		iuConvert = save.iu;
		binaryDetect = save.bd;
		inputReadLine = save.rl;
		curlAuthNegotiate = save.can;
		listNA = save.lna;
		ls_reverse = save.ls_reverse;
		searchWrap = save.sw;
		ebre = save.ebre;
		allowRedirection = save.hr;
		sendReferrer = save.sr;
		allowJS = save.js;
		ftpActive = save.ftpa;
		helpMessagesOn = save.H;
		pluginsOn = save.pg;
		showHiddenFiles = save.hf;
		showHover = save.showall;
		endMarks = save.endm;
		searchStringsAll = save.sg;
		caseInsensitive = save.ci;
		re_utf8 = save.su8;
		verifyCertificates = save.vs;
	}
	return rc;

soft_fail:
	rc = 0;
	goto done;
}

struct DBTABLE *findTableDescriptor(const char *sn)
{
	int i;
	struct DBTABLE *td = dbtables;
	for (i = 0; i < numTables; ++i, ++td)
		if (stringEqual(td->shortname, sn))
			return td;
	return 0;
}

struct DBTABLE *newTableDescriptor(const char *name)
{
	struct DBTABLE *td;
	if (numTables == MAXDBT) {
		setError(MSG_ManyTables, MAXDBT);
		return 0;
	}
	td = dbtables + numTables++;
	td->name = td->shortname = cloneString(name);
	td->ncols = 0;		/* it's already 0 */
	return td;
}

static char *configEnd;
static struct cfgFile {
	struct cfgFile *next, *parent;
	char *file, *end;
	char *base; // base memory holding the file
	char *lp; // line pointer
	int ln; // line number
} *cfgFirst;


// unread the config file, so we can read it again
void unreadConfigFile(void)
{
	struct cfgFile *f = cfgFirst, *g;
	if (!f)
		return;
	while(f) {
		g = f->next;
		free(f->base);
		if(g) free(f->file);
		free(f);
		f = g;
}
	cfgFirst = 0;

	memset(accounts, 0, sizeof(accounts));
	maxAccount = localAccount = 0;
	memset(mimetypes, 0, sizeof(mimetypes));
	maxMime = 0;
	memset(dbtables, 0, sizeof(dbtables));
	numTables = 0;
	memset(userAgents + 1, 0, sizeof(userAgents) - sizeof(userAgents[0]));

	nzFree(addressFile), addressFile = 0;
	nzFree(emojiFile), emojiFile = 0, clearEmojis();
	nzFree(cookieFile), cookieFile = 0;
	nzFree(sslCerts), sslCerts = 0;
	nzFree(downDir), downDir = 0;
	nzFree(mailDir), mailDir = 0;
	nzFree(cacheDir);
	cacheDir = 0;
	nzFree(mailUnread), mailUnread = 0;
	nzFree(mailReply), mailReply = 0;

	webTimeout = mailTimeout = 0;
	delete_ebhosts();
}

/* Order is important here: mail{}, mime{}, table{}, then global keywords */
#define MAILWORDS 0
#define MIMEWORDS 12
#define TABLEWORDS 20
#define GLOBALWORDS 24

static const char *const keywords[] = {
	"inserver", "outserver", "login", "password", "from", "reply",
	"inport", "outport",
	"to", "cc", "bcc", "attach",
	"type", "desc", "suffix", "protocol", "program",
	"content", "outtype", "urlmatch",
	"tname", "tshort", "cols", "keycol",
	"downdir", "maildir", "agent",
	"jar", "nojs", "cachedir",
	"webtimer", "mailtimer", "certfile", "datasource", "proxy",
	"agentsite", "localizeweb", "imapfetch", "novs", "cachesize",
	"adbook", "envelope", "emojis", "emoji",
"include", 0};

/* Read the config file and populate the corresponding data structures. */
/* This routine succeeds, or aborts via one of these macros. */
#define cfgAbort0(m) { i_printf(m, configEnd); nl(); return 0; }
#define cfgAbort1(m, arg) { i_printf(m, configEnd, arg); nl(); return 0; }
#define cfgLine0(m) { i_printf(m, configEnd, ln); nl(); return 0; }
#define cfgLine1(m, arg) { i_printf(m, configEnd, ln, arg); nl(); return 0; }
#define cfgLine1a(m, arg) { i_printf(m, configEnd, arg, ln); nl(); return 0; }

static bool preConfigFile(char *buf, int buflen)
{
	char *s, *t, *v, *q;
	int ln = 1;
	bool cmt = false;
	bool startline = true;
	char last[24];
	unsigned lidx = 0;

/* An extra newline won't hurt. */
	if (buflen && buf[buflen - 1] != '\n')
		buf[buflen++] = '\n';

/* Undos, uncomment, watch for nulls */
/* Encode mail{ as hex 81 m, and other encodings. */
	for (s = t = v = buf; s < buf + buflen; ++s) {
		char c = *s;
		if (c == '\0')
			cfgLine0(MSG_EBRC_Nulls);
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
					cfgLine0(MSG_EBRC_NoFnName);
#if 0
				if (isdigitByte(*q))
					cfgLine0(MSG_EBRC_FnDigit);
#endif
				while (isalnumByte(*q))
					++q;
				if (q - last - 9 > 10)
					cfgLine0(MSG_EBRC_FnTooLong);
				if (*q != '{' || q[1])
					cfgLine0(MSG_EBRC_SyntaxErr);
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
	*t = 0; // now it's a string
	return true;
}

bool readConfigFile(void)
{
	char *buf, *incbuf, *s, *t, *v, *q;
	int buflen, inclen, n;
	char c, ftype;
	uchar mailblock = 0;
	bool mimeblock = false, tabblock = false;
	int nest, ln, j;
	int sn = 0;		/* script number */
	char stack[MAXNEST];
	struct MACCOUNT *act;
	struct MIMETYPE *mt;
	struct DBTABLE *td;
	struct cfgFile *f, *g;

	unreadConfigFile();

	if (!fileIntoMemory(configFile, &buf, &buflen)) {
		i_printf(MSG_NoConfig, configFile);
		return 0;
	}

	configEnd = strrchr(configFile, '/');
	configEnd = (configEnd ? configEnd + 1 : configFile);
	f = allocZeroMem(sizeof(struct cfgFile));
	f->base = buf;
	f->file = configFile, f->end = configEnd;
	cfgFirst = f;

	if(!preConfigFile(buf, buflen))
		return 0;

	ln = 1;
	nest = 0;
	stack[0] = ' ';

top:
/* Go line by line */
	for (s = buf; *s; s = t + 1, ++ln) {
inside:
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
				cfgLine0(MSG_EBRC_NoCondFile);
			while (v > s && (v[-1] == ' ' || v[-1] == '\t'))
				--v;
			if (v == s)
				cfgLine0(MSG_EBRC_NoMatchStr);
			c = *v, *v++ = 0;
			if (c != '>') {
				while (*v != '>')
					++v;
				++v;
			}
			while (*v == ' ' || *v == '\t')
				++v;
			if (!*v)
				cfgLine1(MSG_EBRC_MatchNowh, s);
			add_ebhost(v, "xxrts"[mailblock]);
			ebhosts[ebhosts_avail - 1].prot = s;
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
				cfgLine1a(MSG_EBRC_BadKeyword, s);
			*v = c;	/* put it back */
			goto nokeyword;
		}

		if (nest)
			cfgLine0(MSG_EBRC_KeyInFunc);

		if (n < MIMEWORDS && mailblock != 1)
			cfgLine1(MSG_EBRC_MailAttrOut, s);

		if (n >= MIMEWORDS && n < TABLEWORDS && !mimeblock)
			cfgLine1(MSG_EBRC_MimeAttrOut, s);

		if (n >= TABLEWORDS && n < GLOBALWORDS && !tabblock)
			cfgLine1(MSG_EBRC_TableAttrOut, s);

		if (n >= MIMEWORDS && mailblock)
			cfgLine1(MSG_EBRC_MailAttrIn, s);

		if ((n < MIMEWORDS || n >= TABLEWORDS) && mimeblock)
			cfgLine1(MSG_EBRC_MimeAttrIn, s);

		if ((n < TABLEWORDS || n >= GLOBALWORDS) && tabblock)
			cfgLine1(MSG_EBRC_TableAttrIn, s);

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
			cfgLine1(MSG_EBRC_NoAttr, s);

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

		case 8: case 9: case 10: case 11: // to cc bcc attach
			for (j=0; act->cclist[j]; ++j)  ;
			if(j == MAXCC) {
				cfgLine1(MSG_EBRC_MailDirect, MAXCC);
			} else {
// ^ means cc, ? means bcc
				if(n == 9) *--v = '^';
				if(n == 10) *--v = '?';
				act->cclist[j] = v;
				act->cctype[j] = (n == 11);
			}
			continue;

		case 12:	/* type */
			mt->type = v;
			continue;

		case 13:	/* desc */
			mt->desc = v;
			continue;

		case 14:	/* suffix */
			mt->suffix = v;
			continue;

		case 15:	/* protocol */
			mt->prot = v;
			continue;

		case 16:	/* program */
			mt->program = v;
			continue;

		case 17:	/* content */
			mt->content = v;
			continue;

		case 18:	/* outtype */
			c = tolower(*v);
			if (c != 'h' && c != 't')
				cfgLine0(MSG_EBRC_Outtype);
			mt->outtype = c;
			continue;

		case 19:	/* urlmatch */
			mt->urlmatch = v;
			continue;

		case 20:	/* tname */
			td->name = v;
			continue;

		case 21:	/* tshort */
			td->shortname = v;
			continue;

		case 22:	/* cols */
			while (*v) {
				if (td->ncols == MAXTCOLS)
					cfgLine1(MSG_EBRC_ManyCols, MAXTCOLS);
				td->cols[td->ncols++] = v;
				q = strchr(v, ',');
				if (!q)
					break;
				*q = 0;
				v = q + 1;
			}
			continue;

		case 23:	/* keycol */
			if (!isdigitByte(*v))
				cfgLine0(MSG_EBRC_KeyNotNb);
			td->key1 = (uchar) strtol(v, &v, 10);
			if (*v == ',' && isdigitByte(v[1]))
				td->key2 = (uchar) strtol(v + 1, &v, 10);
			if (td->key1 > td->ncols || td->key2 > td->ncols)
				cfgLine1(MSG_EBRC_KeyOutRange, td->ncols);
			continue;

		case 24:	/* downdir */
			nzFree(downDir), downDir = 0; // in case called more than once
			v = envFileAlloc(v);
			if(!v) continue;
			if (fileTypeByName(v, false) != 'd')
// yeah, v is not freed in this pathway, oh well.
				cfgAbort1(MSG_EBRC_NotDir, v);
			downDir = v;
			continue;

		case 25:	/* maildir */
			nzFree(mailDir), mailDir = 0;
			nzFree(mailUnread), mailUnread = 0;
			nzFree(mailReply), mailReply = 0;
			v = envFileAlloc(v);
			if(!v) continue;
			if (fileTypeByName(v, false) != 'd')
				cfgAbort1(MSG_EBRC_NotDir, v);
			mailDir = v;
			mailUnread = allocMem(strlen(v) + 20);
			sprintf(mailUnread, "%s/unread", v);
/* We need the unread directory, else we can't fetch mail. */
/* Create it if it isn't there. */
			if (fileTypeByName(mailUnread, false) != 'd') {
				if (mkdir(mailUnread, 0700))
					cfgAbort1(MSG_EBRC_NotDir, mailUnread);
			}
			mailReply = allocMem(strlen(v) + 20);
			sprintf(mailReply, "%s/.reply", v);
			continue;

		case 26:	/* agent */
			for (j = 0; j < MAXAGENT; ++j)
				if (!userAgents[j])
					break;
			if (j == MAXAGENT)
				cfgLine1(MSG_EBRC_ManyAgents, MAXAGENT);
			userAgents[j] = v;
			continue;

		case 27:	/* jar */
			nzFree(cookieFile), cookieFile = 0;
			v = envFileAlloc(v);
			if(!v) continue;
			ftype = fileTypeByName(v, false);
			if (ftype && ftype != 'f')
				cfgAbort1(MSG_EBRC_JarNotFile, v);
			j = open(v, O_WRONLY | O_APPEND | O_CREAT,
				 MODE_private);
			if (j < 0)
				cfgAbort1(MSG_EBRC_JarNoWrite, v);
			close(j);
			cookieFile = v;
			continue;

		case 28:	/* nojs */
			if (*v == '.')
				++v;
			q = strchr(v, '.');
			if (!q || q[1] == 0)
				cfgLine1(MSG_EBRC_DomainDot, v);
			add_ebhost(v, 'j');
			continue;

		case 29:	/* cachedir */
			nzFree(cacheDir);
			cacheDir = cloneString(v);
			continue;

		case 30:	/* webtimer */
			webTimeout = atoi(v);
			continue;

		case 31:	/* mailtimer */
			mailTimeout = atoi(v);
			continue;

		case 32:	/* certfile */
			nzFree(sslCerts), sslCerts = 0;
			v = envFileAlloc(v);
			if(!v) continue;
			ftype = fileTypeByName(v, false);
			if (ftype && ftype != 'f')
				cfgAbort1(MSG_EBRC_SSLNoFile, v);
			j = open(v, O_RDONLY);
			if (j < 0)
				cfgAbort1(MSG_EBRC_SSLNoRead, v);
			close(j);
			sslCerts = v;
			continue;

		case 33:	/* datasource */
			setDataSource(v);
			continue;

		case 34:	/* proxy */
			add_proxy(v);
			continue;

		case 35:	// agentsite
			spaceCrunch(v, true, true);
			q = strchr(v, ' ');
			if (!q || strchr(q + 1, ' ') ||
			    (j = stringIsNum(q + 1)) < 0)
				break;
			if (j >= MAXAGENT || !userAgents[j]) {
				cfgLine1(MSG_EBRC_NoAgent, j);
				continue;
			}
			*q = 0;
			add_ebhost(v, 'a');
			ebhosts[ebhosts_avail - 1].n = j;
			continue;

		case 36:	/* localizeweb */
/* We should probably allow autodetection of language. */
/* E.G., the keyword auto indicates that you want autodetection. */
			setHTTPLanguage(v);
			continue;

		case 37:	/* imap fetch limit */
			imapfetch = atoi(v);
			if (imapfetch <= 10)
				imapfetch = 10;
// there isn't an upper limit when set interactively, so idk
			if (imapfetch >= 1000)
				imapfetch = 1000;
			continue;

		case 38:	/* novs */
			if (*v == '.')
				++v;
			q = strchr(v, '.');
			if (!q || q[1] == 0)
				cfgLine1(MSG_EBRC_DomainDot, v);
			add_ebhost(v, 'v');
			continue;

		case 39:	/* cachesize */
			cacheSize = atoi(v);
			if (cacheSize <= 0)
				cacheSize = 0;
			if (cacheSize >= 10000)
				cacheSize = 10000;
			continue;

		case 40:	// adbook
			nzFree(addressFile), addressFile = 0;
			v = envFileAlloc(v);
			if(!v) continue;
			ftype = fileTypeByName(v, false);
			if (!ftype || ftype != 'f')
				cfgAbort1(MSG_EBRC_AbNotFile, v);
			addressFile = v;
			continue;

		case 41:	// envelope
			setEnvelopeFormat(v);
			continue;

		case 42: case 43:	// emojis
			nzFree(emojiFile), emojiFile = 0;
			v = envFileAlloc(v);
			if(!v) continue;
			ftype = fileTypeByName(v, false);
			if (!ftype || ftype != 'f')
				cfgAbort1(MSG_EBRC_EmojiNotFile, v);
			emojiFile = v;
			loadEmojis();
			continue;

		case 44: // include
			v = envFileAlloc(v);
			if(!v) continue;
			if(!fileIntoMemory(v, &incbuf, &inclen)) {
				showError();
				setError(-1);
				nzFree(v);
				continue;
			}
			if(!preConfigFile(incbuf, inclen)) {
				nzFree(v), nzFree(incbuf);
				continue;
			}
			f->lp = t + 1;
			f->ln = ln + 1;
			g = allocZeroMem(sizeof(struct cfgFile));
			g->base = incbuf;
			configEnd = strrchr(v, '/');
			configEnd = (configEnd ? configEnd + 1 : v);
			g->file = v, g->end = configEnd;
			buf = incbuf;
			ln = 1;
			g->parent = f;
			g->next = cfgFirst, cfgFirst = g;
			f = g;
			goto top;

		default:
			cfgLine1(MSG_EBRC_KeywordNYI, s);
		}		/* switch */

nokeyword:

		if (stringEqual(s, "default") && mailblock == 1) {
			if (localAccount == maxAccount + 1)
				continue;
			if (localAccount)
				cfgAbort0(MSG_EBRC_SevDefaults);
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

		if (stringEqual(s, "from_file") && mimeblock == 1) {
			mt->from_file = true;
			continue;
		}
		if (stringEqual(s, "down_url") && mimeblock == 1) {
			mt->down_url = true;
			continue;
		}

		if (*s == '\x82' && s[1] == 0) {
			if (mailblock == 1) {
				++maxAccount;
				mailblock = 0;
				if (!act->inurl)
					cfgLine0(MSG_EBRC_NoInserver);
				if (!act->outurl)
					cfgLine0(MSG_EBRC_NoOutserver);
				if (!act->login)
					cfgLine0(MSG_EBRC_NoLogin);
				if (!act->password)
					cfgLine0(MSG_EBRC_NPasswd);
				if (!act->from)
					cfgLine0(MSG_EBRC_NoFrom);
				if (!act->reply)
					cfgLine0(MSG_EBRC_NoReply);
				if (act->secure)
					act->inssl = act->outssl = 1;
				if (!act->inport) {
					if (act->secure) {
						act->inport =
						    (act->imap ? 993 : 995);
					} else {
						act->inport =
						    (act->imap ? 143 : 110);
					}
				}
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
					cfgLine0(MSG_EBRC_NoType);
				if (!mt->desc)
					cfgLine0(MSG_EBRC_NDesc);
				if (!mt->suffix && !mt->prot && !mt->content)
					cfgLine0(MSG_EBRC_NoSuffix);
				if (!mt->program)
					cfgLine0(MSG_EBRC_NoProgram);
				continue;
			}

			if (tabblock) {
				++numTables;
				tabblock = false;
				if (!td->name)
					cfgLine0(MSG_EBRC_NoTblName);
				if (!td->shortname)
					cfgLine0(MSG_EBRC_NoShortName);
				if (!td->ncols)
					cfgLine0(MSG_EBRC_NColumns);
				continue;
			}

			if (--nest < 0)
				cfgLine0(MSG_EBRC_UnexpBrace);
			if (nest)
				goto putback;
/* This ends the function */
			*s = 0;	/* null terminate the script */
			continue;
		}

		if (*s == '\x83' && s[1] == 0) {
/* Does else make sense here? */
			c = toupper(stack[nest]);
			if (c != 'I')
				cfgLine0(MSG_EBRC_UnexElse);
			goto putback;
		}

		if (*s != '\x81') {
			if (!nest)
				cfgLine0(MSG_EBRC_GarblText);
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
			cfgLine1(MSG_EBRC_FnNotStart, curblock);
		}

		if (!strchr("fmertsb", c) && !nest)
			cfgLine0(MSG_EBRC_StatNotInFn);

		if (c == 'm') {
			mailblock = 1;
			if (maxAccount == MAXACCOUNT)
				cfgAbort1(MSG_EBRC_ManyAcc, MAXACCOUNT);
			act = accounts + maxAccount;
			continue;
		}

		if (c == 'e') {
			mimeblock = true;
			if (maxMime == MAXMIME)
				cfgAbort1(MSG_EBRC_ManyTypes, MAXMIME);
			mt = mimetypes + maxMime;
			continue;
		}

		if (c == 'b') {
			tabblock = true;
			if (numTables == MAXDBT)
				cfgAbort1(MSG_EBRC_ManyTables, MAXDBT);
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
			sn = ebhosts_avail;
			t[-1] = 0;
			add_ebhost(t, 'f');
			ebhosts[sn].prot = s + 2;
			goto putback;
		}

		if ((unsigned)(++nest) >= sizeof(stack))
			cfgLine0(MSG_EBRC_TooDeeply);
		stack[nest] = c;

putback:
		*t = '\n';
	}			// loop over lines

	while((f = f->parent)) { // pop the stack
		configEnd = f->end;
		buf = f->base;
		s = f->lp, ln = f->ln;
		if(*s) goto inside;
	}

	if (nest)
		cfgAbort1(MSG_EBRC_FnNotClosed, ebhosts[sn].prot + 1);

	if (mailblock | mimeblock)
		cfgAbort0(MSG_EBRC_MNotClosed);

	if (maxAccount && !localAccount)
		localAccount = 1;
	return true;
}

// local replacements for javascript and css
struct JSR {
	struct JSR *next;
	char *url, *locf;
};
static struct JSR *jsr_top;

const char *fetchReplace(const char *u)
{
	struct JSR *j = jsr_top;
	const char *s;
	int l;
	if (!j)
		return 0;	// high runner case
	s = strchr(u, '?');
	l = (s ? s - u : (int)strlen(u));
	while (j) {
		if (!strncmp(j->url, u, l) && j->url[l] == 0)
			return j->locf;
		j = j->next;
	}
	return 0;
}

static void loadReplacements(void)
{
	FILE *f = fopen("jslocal", "r");
	struct JSR *j;
	char *s;
	int n = 0;
	char line[1024];
	if (!f)
		return;
	while (fgets(line, sizeof(line), f)) {
		s = strchr(line, '\n');
		if (!s) {
			fprintf(stderr, "jslocal line too long\n");
			fclose(f);
			return;
		}
		*s = 0;
		if (s > line && s[-1] == '\r')
			*--s = 0;
		if (line[0] == '#' || line[0] == 0)
			continue;
		s = strchr(line, ':');
		if (!s) {
			fprintf(stderr, "jslocal line has no :\n");
			continue;
		}
		*s++ = 0;
		if (strchr(s, '?')) {
			fprintf(stderr, "jslocal line has ?\n");
			continue;
		}
		j = allocMem(sizeof(struct JSR));
		j->locf = cloneString(line);
		j->url = cloneString(s);
		j->next = jsr_top;
		jsr_top = j;
		++n;
	}
	fclose(f);
	debugPrint(3, "%d js or css file replacements", n);
}
