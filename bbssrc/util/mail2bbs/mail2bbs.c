/*-
 *              mail2bbs.c              -- mail -> bbs gateway for Firebird BBS 3.0
 *
 * $Id: mail2bbs.c,v 1.3 2000/01/20 14:10:50 edwardc Exp $
 */

#include "bbs.h"
#include "mail2bbs.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAILDIR     BBSHOME"/mail"
#define MYPASSFILE  BBSHOME"/.PASSWDS"
#define BLOCKFILE   BBSHOME"/.blockmail"

char    buf[256], userid[IDLEN+1];
struct UCACHE *uidshm;

void
report()
{
}

void
attach_err(shmkey, name, err)
int     shmkey;
char   *name;
int     err;
{
	sprintf(buf, "Error! %s error #%d! key = %x.\n", name, err, shmkey);
	write(1, buf, strlen(buf));
	exit(1);
}

void   *
attach_shm(shmstr, shmkey, shmsize)
char   *shmstr;
int     shmkey, shmsize;
{
	void   *shmptr;
	int     shmid;
	
	shmid = shmget(shmkey, shmsize, 0);
	if (shmid < 0) {
		shmid = shmget(shmkey, shmsize, IPC_CREAT | 0640);
		if (shmid < 0)
			attach_err(shmkey, "shmget", errno);
		shmptr = (void *) shmat(shmid, NULL, 0);
		if (shmptr == (void *) -1)
			attach_err(shmkey, "shmat", errno);
		memset(shmptr, 0, shmsize);
	} else {
		shmptr = (void *) shmat(shmid, NULL, 0);
		if (shmptr == (void *) -1)
			attach_err(shmkey, "shmat", errno);
	}
	return shmptr;
}

void
mailog(char *msg)
{
	FILE   *fp;
	char    xx[256];
	sprintf(xx, "%s/trace", BBSHOME);
	fp = fopen(xx, "a+");
	if (fp == NULL)
		return;
	fprintf(fp, "%s\n", msg);
	fclose(fp);
}

struct SPAM_MAIL *spam;

int
seek_mailflag(spam_a, spam_b, spam_c)
int     spam_a;
int     spam_b;
int     spam_c;
{
	int     i = 0, j = 0;

	if (spam_a == 0 && spam_b == 0 && spam_c == 0)
		return 0;

	for (i = 0; i < SPAMTABLE; i++) {
		if ( spam->mail_flag[i].spam_a == spam_a && 
		     spam->mail_flag[i].spam_b == spam_b && 
		     spam->mail_flag[i].spam_c == spam_c) {
			if (spam->mail_flag[i].spam_times < 3) {
				spam->mail_flag[i].spam_times++;
				return 0;
			} else {
				spam->mail_flag[i].spam_times++;
				return 1;
			}

		}
	}
	for (i = 0; i < SPAMTABLE; i++) {
		if (spam->mail_flag[i].spam_times == 0) {
			spam->mail_flag[i].spam_a = spam_a;
			spam->mail_flag[i].spam_b = spam_b;
			spam->mail_flag[i].spam_c = spam_c;
			spam->mail_flag[i].spam_times = 1;
			j = 1;
			break;
		}
	}
	
	if (j != 1) {
		for (i = 0; i < SPAMTABLE; i++) {
			if (spam->mail_flag[i].spam_times <= 3) {
				spam->mail_flag[i].spam_times = 0;
				spam->mail_flag[i].spam_a = 0;
				spam->mail_flag[i].spam_b = 0;
				spam->mail_flag[i].spam_c = 0;
			}
		}
	}
	return 0;
}

int
cmpuids(uid, up)
char   *uid;
struct userec *up;
{
	if (!strncasecmp(uid, up->userid, sizeof(up->userid))) {
		strncpy(uid, up->userid, sizeof(up->userid));
		return 1;
	} else {
		return 0;
	}
}

int
dashd(fname)
char   *fname;
{
	struct stat st;
	return (stat(fname, &st) == 0 && S_ISDIR(st.st_mode));
}

int
dashf(fname)
char   *fname;
{
	struct stat st;
	return (stat(fname, &st) == 0 && S_ISREG(st.st_mode));
}

int
dosearchuser(uid)
char   *uid;
{
	char buf[256];
	int i;
		
	sprintf(buf,"%s/mail/%c/%s", BBSHOME, uid[0], uid);
	if ( !dashd(buf) ) {
		for ( i = 0 ; i < uidshm->number ; i++ ) {
			if ( !strcasecmp(uid, uidshm->userid[i]) ) {
				strcpy(userid, uidshm->userid[i]);
				return 1;
			}
		}
		return -1;
	}
	strcpy(userid, uid);
	return 2;
}

int
append_mail(fin, sender1, sender, uid, title, received)
FILE   *fin;
char   *uid, *sender1, *sender, *title, *received;
{
	struct fileheader newmessage;

	char    fname[512], buf[256], genbuf[256], fff[80], fff2[80], mailaddr[80];
	char   *ip, maildir[256], timechk[13];
	int     fp;
	FILE   *fout, *dp, *rmail;
	int     yyyy, zzzz, passcheck = 0;
	char    conv_buf[256];
	int     spam_a = 0, spam_b = 0, spam_c = 0;

	/* check if the userid is in our bbs now */

	userid[0] = '\0';
	
	if ( dosearchuser(uid) == -1 )
#ifdef DONT_RETURN_ANYTHING
		return 0;		/* ?????S?? .. */
#else
		return -1;		/* ?h?H?????????F .... */
#endif
	
	/* check for the mail dir for the userid */
	sprintf(genbuf, "%s/%c/%s", MAILDIR, toupper(userid[0]), userid);

	if ( dashf(genbuf) )
		return -1;
	else {
		if ( !dashd(genbuf) )
			if (mkdir(genbuf, 0755) == -1)
				return -1;
	}		

	printf("Ok, dir is %s\n", genbuf);

	str_decode(conv_buf, title);

	if (!strcmp(userid, "SYSOP") && strstr(conv_buf, " mail check.")) {
		passcheck = 1;
		if ((!strstr(sender, "bbs")) && (strchr(conv_buf, '@'))) {

			yyyy = 0;
			zzzz = 0;
			while ((conv_buf[yyyy] != '@') && (yyyy < NAMELEN))
				yyyy = yyyy + 1;
			yyyy = yyyy + 1;
			while ((conv_buf[yyyy] != '@') && (yyyy < NAMELEN)) {
				sender1[zzzz] = conv_buf[yyyy];
				yyyy = yyyy + 1;
				zzzz = zzzz + 1;
			}
			sender1[zzzz] = '\0';
			strcpy(userid, sender1);
			sprintf(fff, "%s/home/%c/%s/mailcheck", BBSHOME, toupper(sender1[0]), sender1);
			if ((dp = fopen(fff, "r")) != NULL) {
				printf("open mailcheck\n");
				fgets(fff2, sizeof(fff2), dp);
				fclose(dp);
				strcpy(timechk, (char *) strchr(fff2, ':') + 1);
				timechk[strlen(timechk)] = '\0';
				sprintf(fff2, "%9.9s", fff2);
				if (dosearchuser(sender1) && strstr(conv_buf, fff2)	/* &&strstr(sender,checku
					    ser.email) */ ) {
					printf("pass1\n");

					unlink(fff);
					passcheck = 5;
					/* Modify for SmallPig */
					sprintf(genbuf, "%s", sender /* checkuser.email */ );
					sprintf(buf, "%s/home/%c/%s/register", BBSHOME, toupper(sender1[0]), sender1);
					if (dashf(buf)) {
						sprintf(buf, "%s/home/%c/%s/register.old", BBSHOME, toupper(sender1[0]), sender1);
						rename(buf, conv_buf);
					}
					if ((fout = fopen(buf, "w")) != NULL) {
						fprintf(fout, "%s\n", genbuf);
						fclose(fout);
						/* return 0; */
					}
				}
			}
		}
	}
	/* allocate a record for the new mail */
	bzero(&newmessage, sizeof(newmessage));
	sprintf(fname, "M.%d.A", time(0));
	sprintf(genbuf, "%s/%c/%s/%s", MAILDIR, toupper(userid[0]), userid, fname);
	sprintf(maildir, "%s/%c/%s", MAILDIR, toupper(userid[0]), userid);
	if (!dashd(maildir)) {
		mkdir(maildir, 0755);
		chmod(maildir, 0755);
	}
	ip = (char *) rindex(fname, 'A');
	while ((fp = open(genbuf, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1) {
		if (*ip == 'Z')
			ip++, *ip = 'A', *(ip + 1) = '\0';
		else
			(*ip)++;
		sprintf(genbuf, "%s/%c/%s/%s", MAILDIR, toupper(userid[0]), userid, fname);
	}
	close(fp);
	strcpy(newmessage.filename, fname);
	strncpy(newmessage.title, conv_buf, STRLEN);
	strncpy(newmessage.owner, sender, STRLEN);

	printf("Ok, the file is %s\n", genbuf);

	/* copy the stdin to the specified file */
	sprintf(genbuf, "%s/%c/%s/%s", MAILDIR, toupper(userid[0]), userid, fname);
	if ((fout = fopen(genbuf, "w")) == NULL) {
		printf("Cannot open %s \n", genbuf);
		return -1;
	} else {
		time_t  tmp_time;
		time(&tmp_time);
		fprintf(fout, "?H?H?H: %-.70s \n", sender);
		fprintf(fout, "??  ?D: %-.70s\n", conv_buf);
		fprintf(fout, "?o?H??: %s BBS ?H?t\n", BBSNAME);
		if (received[0] != '\0')
			fprintf(fout, "??  ??: %-.70s\n", received);
		fprintf(fout, "??  ??: %s\n", ctime(&tmp_time));
		if (passcheck >= 1) {
			fprintf(fout, "???R?? %s:\n", sender1);
			if (passcheck == 5)
				sprintf(maildir, "%s/etc/smail", BBSHOME);
			else if (passcheck == 99)
				sprintf(maildir, "%s/etc/regexpired", BBSHOME);
			else
				sprintf(maildir, "%s/etc/fmail", BBSHOME);

			if ((rmail = fopen(maildir, "r")) != NULL) {
				while (fgets(genbuf, 255, rmail) != NULL)
					fputs(genbuf, fout);
				fclose(rmail);
			}
		} else {
			passcheck = 0;
			while (fgets(genbuf, 255, fin) != NULL) {
				if (!strncmp(genbuf, "From: SYSOP.bbs", 15) && passcheck == 0)
					passcheck = 1;
				if (strstr(genbuf, " mail check.") && passcheck == 1) {
					passcheck = 2;
				} else if (strstr(genbuf, "registration mail.") &&
					passcheck == 2) {
					passcheck = 3;
				} else if (!strncmp(genbuf, "?{???t?X         :", 14) && passcheck == 3) {
					fclose(fout);
					fclose(fin);
					sprintf(genbuf, "%s/%c/%s/%s", MAILDIR, toupper(userid[0]), userid, fname);
					unlink(genbuf);
					fout = fopen(genbuf, "w");
					sprintf(genbuf, "%s/etc/wmail", BBSHOME);
					fin = fopen(genbuf, "r");
					if (fin == NULL || fout == NULL) {
						printf("wmail: cannot open fin or fout!!\n");
						return -1;
					}
					while (fgets(genbuf, 255, fin) != NULL)
						fputs(genbuf, fout);
					fclose(fin);
					break;
				}
				fputs(genbuf, fout);
				spam_b++;
				spam_a = spam_a + genbuf[strlen(genbuf) / ((spam_b % RULE) + 1)];
				spam_c = spam_c + genbuf[strlen(genbuf) / RULE];
			}
		}

		fclose(fout);

	}

	if (seek_mailflag(spam_a, spam_b, spam_c) == 1) {
		sprintf(genbuf, "%s/%c/%s/%s", MAILDIR, toupper(userid[0]), userid, fname);
		unlink(genbuf);
		sprintf(genbuf, "SPAM: %x %x %x => %s => %s", spam_a, spam_b, spam_c
			,sender, userid);
		mailog(genbuf);
		return 0;
	}
	/* append the record to the MAIL control file */
	sprintf(genbuf, "%s/%c/%s/%s", MAILDIR, toupper(userid[0]), userid, DOT_DIR);
	if (append_record(genbuf, &newmessage, sizeof(newmessage)) == -1)
		return 1;
	else
		return 0;
}

int
block_mail(addr)
char   *addr;
{
	FILE   *fp;
	char    temp[STRLEN];
	if ((fp = fopen(BLOCKFILE, "r")) != NULL) {
		while (fgets(temp, STRLEN, fp) != NULL) {
			strtok(temp, "\n");
			if (strstr(addr, temp)) {
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}
	return 0;
}


main(argc, argv)
int     argc;
char   *argv[];
{

	char    sender[256];
	char    username[256];
	char    receiver[256];
	char    nettyp[256];
	int     xxxx;

	/* argv[ 1 ] is original sender */
	/* argv[ 2 ] is userid in bbs   */
	/* argv[ 3 ] is the mail title  */
	/* argv[ 4 ] is the message-id  */

	setreuid((uid_t)BBSUID, (uid_t)BBSUID);
	setregid((gid_t)BBSGID, (gid_t)BBSGID);

	spam = (void *) attach_shm("SPAM_KEY", SPAM_SHMKEY, sizeof(*spam));
	uidshm = (void *) attach_shm("UCACHE_KEY", (30010L), sizeof(*uidshm));
	
	if (argc < 5 || argc > 7) {
		char   *p = (char *) rindex(argv[0], '/');
		printf("Usage: %s sender receiver_in_bbs mail_title\n",
			p ? p + 1 : argv[0]);
		return 1;
	}

	if (chroot(BBSHOME) == 0) {
		chdir("/");
	}

	if (strchr(argv[1], '@')) {
		strcpy(sender, argv[1]);
		/* added by netty  */
		xxxx = 0;
		while (sender[xxxx] != '@') {
			nettyp[xxxx] = sender[xxxx];
			xxxx = xxxx + 1;
		}
		nettyp[xxxx] = '\0';	/* added by netty  */
	} else {
		char   *p, *l, *r;
		char    buf[256];
		strcpy(buf, argv[1]);
		p = strtok(buf, " \t\n\r");
		l = strchr(argv[1], '(');
		r = strchr(argv[1], ')');
		if (l < r && l && r)
			strncpy(username, l, r - l + 1);
		sprintf(sender, "%s@%s %s", p, BBSHOST, username);
		strcpy(nettyp, p);
	}

	if (block_mail(sender) == YEA)
		return -2;

	strcpy(receiver, argv[2]);

#ifdef MAIL_LIST_MODULE
	/* 990405.edwardc mailling-list module for mail2bbs */

	if (!strcmp(receiver, MAILLIST_RECEIVER)) {
		str_decode(buf, argv[3]);
		return append_mailling(stdin, buf, sender, argv[4], argv[6], argv[5]);
	}
#endif

	return append_mail(stdin, nettyp, sender, receiver, argv[3], argv[4]);
}
