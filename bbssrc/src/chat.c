/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu
    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
                        Peng Piaw Foong, ppfoong@csie.ncu.edu.tw

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
/*
$Id: chat.c,v 1.1 2000/01/15 01:45:27 edwardc Exp $
*/

#include "bbs.h"

#ifdef lint
#include <sys/uio.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "chat.h"

char    chatroom[IDLEN];	/* Chat-Room Name */
int     chatline;		/* Where to display message now */
int     stop_line;		/* next line of bottom of message window area */
FILE   *rec;
int     recflag = 0;
char    buftopic[STRLEN];
char    chat_station[19];

extern char BoardName[];
extern char page_requestor[];
extern int talkrequest;
extern char *modestring();
extern char pagerchar();
extern void t_pager();
void    set_rec();
struct user_info *t_search();
#define b_lines t_lines-1
#define cuser currentuser
char   *msg_seperator = "\
───────────────────────────────────────";
char   *msg_shortulist = "[1;33;44m\
 使用者代號    目前狀態  │ 使用者代號    目前狀態  │ 使用者代號    目前狀態 [m";

struct chat_command {
	char   *cmdname;	/* Char-room command length */
	void    (*cmdfunc) ();	/* Pointer to function */
};

void
printchatline(str)
char   *str;
{
	move(chatline, 0);
	clrtoeol();
	outs(str);
	outc('\n');
	if (recflag == 1)
		fprintf(rec, "%s\n", str);
	if (++chatline == stop_line)
		chatline = 2;
	move(chatline, 0);
	clrtoeol();
	outs("→");
}


void
chat_clear()
{
	for (chatline = 2; chatline < stop_line; chatline++) {
		move(chatline, 0);
		clrtoeol();
	}
	chatline = stop_line - 1;
	printchatline("");
}


void
print_chatid(chatid)
char   *chatid;
{
	move(b_lines, 0);
	outs(chatid);
	outc(':');
}


int
chat_send(fd, buf)
int     fd;
char   *buf;
{
	int     len;
	sprintf(genbuf, "%s\n", buf);
	len = strlen(genbuf);
	return (send(fd, genbuf, len, 0) == len);
}

int
chat_recv(fd, chatid)
int     fd;
char   *chatid;
{
	static char buf[512];
	static int bufstart = 0;
	int     c, len;
	char   *bptr;
	len = sizeof(buf) - bufstart - 1;
	if ((c = recv(fd, buf + bufstart, len, 0)) <= 0)
		return -1;
	c += bufstart;
	bptr = buf;
	while (c > 0) {
		len = strlen(bptr) + 1;
		if (len > c && len < (sizeof(buf) / 2))
			break;

		if (*bptr == '/') {
			switch (bptr[1]) {
			case 'c':
				chat_clear();
				break;
			case 'n':
				strncpy(chatid, bptr + 2, 8);
				print_chatid(chatid);
				clrtoeol();
				break;
			case 'r':
				strncpy(chatroom, bptr + 2, IDLEN - 1);
				break;
			case 't':
				move(0, 0);
				clrtoeol();
				sprintf(genbuf, "房間： [32m%s", chatroom);
				strncpy(buftopic, bptr + 2, STRLEN - 1);
				prints("[1;44;33m %-21s  [33m話題：[36m%-47s[5;31m%6s[m", genbuf, bptr + 2, (recflag == 1) ? "錄音中" : "      ");
			}
		} else {
			printchatline(bptr);
		}

		c -= len;
		bptr += len;
	}

	if (c > 0) {
		strcpy(genbuf, bptr);
		strncpy(buf, genbuf, sizeof(buf));
		bufstart = len - 1;
	} else
		bufstart = 0;
	return 0;
}

void
fixchatid(chatid)
char   *chatid;
{
	chatid[CHAT_IDLEN] = '\0';
	while (*chatid != '\0' && *chatid != '\n') {
		if (strchr(BADCIDCHARS, *chatid))
			*chatid = '_';
		chatid++;
	}
}

int
ent_chat(chatbuf)
char   *chatbuf;
{
	char    inbuf[80], chatid[CHAT_IDLEN], lastcmd[MAXLASTCMD][80];
	struct sockaddr_in sin;
	struct hostent *h;
	int     cfd, cmdpos, ch;
	int     chatroom, chatport;
	int     currchar;
	int     newmail;
	extern int talkidletime;
	int     page_pending = NA;
	int     chatting = YEA;
	int     i, j;
	char    runchatbuf[STRLEN];
	char    stnname[12][20];
	char    stnaddr[12][30];
	char    temp[10];
	int     portnum[12];
	FILE   *stationrec;
	chatroom = atoi(chatbuf);
	switch (chatroom) {
	case 4:
		strcpy(chat_station, CHATNAME4);
		modify_user_mode(CHAT4);
		chatport = CHATPORT4;
		break;
	case 3:
		strcpy(chat_station, CHATNAME3);
		modify_user_mode(CHAT3);
		chatport = CHATPORT3;
		break;
	case 2:
		strcpy(chat_station, CHATNAME2);
		modify_user_mode(CHAT2);
		chatport = CHATPORT2;
		break;
	case 1:
	default:
		strcpy(chat_station, CHATNAME1);
		modify_user_mode(CHAT1);
		chatport = CHATPORT1;
		break;
	}

	if ((chatroom == 1) && (stationrec = fopen("etc/chatstation", "r")) != NULL) {
		i = 0;
		while (fgets(inbuf, STRLEN, stationrec) != NULL && i <= 11) {
			strncpy(stnname[i], (char *) strtok(inbuf, " \n\r\t"), 19);
			strncpy(stnaddr[i], (char *) strtok(NULL, " \n\r\t"), 29);
			strncpy(temp, (char *) strtok(NULL, " \n\r\t"), 9);
			portnum[i] = atoi(temp);
			i++;
		}
		fclose(stationrec);
		move(1, 0);
		clrtobot();
		prints("\n\n序 連線站名稱             連線站位址\n");
		prints("== =====================  ==============================\n");
		for (j = 0; j <= i - 1; j++) {
			move(5 + j, 0);
			prints("%2d %-22s %-32s", j, stnname[j], stnaddr[j]);
		}
		getdata(23, 0, "請輸入站台序號：", temp, 3, DOECHO, YEA);
		i = atoi(temp);
		if ((i > j - 1) || (i < 0))
			i = 0;
		clear();
		prints("連往『%s』，請稍候...\n", stnname[i]);
		if (!(h = gethostbyname(stnaddr[i]))) {
			perror("gethostbyname");
			return -1;
		}
		memset(&sin, 0, sizeof sin);
		sin.sin_family = PF_INET;
		memcpy(&sin.sin_addr, h->h_addr, h->h_length);
		sin.sin_port = htons(portnum[i]);
		cfd = socket(sin.sin_family, SOCK_STREAM, 0);

		if (connect(cfd, (struct sockaddr *) & sin, sizeof(sin))) {
			gethostname(inbuf, STRLEN);
			if (!(h = gethostbyname(inbuf))) {
				perror("gethostbyname");
				return -1;
			}
			memset(&sin, 0, sizeof sin);
			sin.sin_family = PF_INET;
			memcpy(&sin.sin_addr, h->h_addr, h->h_length);
			chatport = CHATPORT1;
			sin.sin_port = htons(chatport);
			close(cfd);
			move(1, 0);
			clrtoeol();
			prints("對方聊天室沒開，連進本站的國際會議廳...");
			sprintf(runchatbuf, "bin/chatd %d", chatroom);
			system(runchatbuf);
			cfd = socket(sin.sin_family, SOCK_STREAM, 0);
			if ((connect(cfd, (struct sockaddr *) & sin, sizeof(sin)))) {
				perror("connect failed");
				return -1;
			}
		}
	} else {
		gethostname(inbuf, STRLEN);
		if (!(h = gethostbyname(inbuf))) {
			perror("gethostbyname");
			return -1;
		}
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = PF_INET;
		memcpy(&sin.sin_addr, h->h_addr, h->h_length);
		sin.sin_port = htons(chatport);
		cfd = socket(sin.sin_family, SOCK_STREAM, 0);

		if (connect(cfd, (struct sockaddr *) & sin, sizeof(sin))) {
			close(cfd);
			move(1, 0);
			clrtoeol();
			prints("開啟聊天室...");
			sprintf(runchatbuf, "bin/chatd %d", chatroom);
			system(runchatbuf);
			sleep(1);
			cfd = socket(sin.sin_family, SOCK_STREAM, 0);
			if ((connect(cfd, (struct sockaddr *) & sin, sizeof(sin)))) {
				perror("connect failed");
				return -1;
			}
		}
	}
	while (1) {
		getdata(2, 0, "請輸入聊天代號：", inbuf, CHAT_IDLEN, DOECHO, YEA);
		sprintf(chatid, "%.8s", ((inbuf[0] != '\0' && inbuf[0] != '\n') ? inbuf : cuser.userid));
		fixchatid(chatid);
		sprintf(inbuf, "/! %d %d %s %s %d", uinfo.uid,
			cuser.userlevel, cuser.userid, chatid, HAS_PERM(PERM_CHATCLOAK) ? uinfo.invisible : 0);
		chat_send(cfd, inbuf);
		if (recv(cfd, inbuf, 3, 0) != 3) {
			return 0;
		}
		move(3, 0);
		if (!strcmp(inbuf, CHAT_LOGIN_OK))
			break;
		else if (!strcmp(inbuf, CHAT_LOGIN_EXISTS))
			prints("這個代號已經有人用了");
		else if (!strcmp(inbuf, CHAT_LOGIN_INVALID))
			prints("這個代號是錯誤的");
		else
			prints("你已經有另一個視窗進入此聊天室。");
		clrtoeol();
		refresh();
		sleep(1);
		bell();
	}

	endmsg();
	add_io(cfd, 0);

	newmail = cmdpos = currchar = 0;
	memset(lastcmd, 0, MAXLASTCMD * 80);

	uinfo.in_chat = YEA;
	strcpy(uinfo.chatid, chatid);
	update_utmp();

	clear();
	chatline = 2;
	strcpy(inbuf, chatid);
	stop_line = t_lines - 2;
	move(stop_line, 0);
	outs(msg_seperator);
	move(1, 0);
	outs(msg_seperator);
	print_chatid(chatid);
	memset(inbuf, 0, 80);

	while (chatting) {
		move(b_lines, currchar + 10);
		ch = igetkey();
		talkidletime = 0;
		if (talkrequest)
			page_pending = YEA;
		if (page_pending)
			page_pending = servicepage(0, NULL);

		switch (ch) {
		case KEY_UP:
			cmdpos += MAXLASTCMD - 2;

		case KEY_DOWN:
			cmdpos++;
			cmdpos %= MAXLASTCMD;
			strcpy(inbuf, lastcmd[cmdpos]);
			move(b_lines, 10);
			clrtoeol();
			outs(inbuf);
			currchar = strlen(inbuf);
			continue;

		case KEY_LEFT:
			if (currchar)
				--currchar;
			continue;

		case KEY_RIGHT:
			if (inbuf[currchar])
				++currchar;
			continue;
		}

		if (!newmail && chkmail(0)) {
			newmail = 1;
			printchatline("[1;32m◆ [31m噹！郵差送信來了...[m");
		}
		if (ch == I_OTHERDATA) {	/* incoming */
			if (chat_recv(cfd, chatid) == -1)
				break;
			continue;
		}
#ifdef BIT8
		if (isprint2(ch))
#else
		if (isprint(ch))
#endif

		{
			if (currchar < 68) {
				if (inbuf[currchar]) {	/* insert */
					int     i;
					for (i = currchar; inbuf[i] && i < 68; i++);
					inbuf[i + 1] = '\0';
					for (; i > currchar; i--)
						inbuf[i] = inbuf[i - 1];
				} else {	/* append */
					inbuf[currchar + 1] = '\0';
				}
				inbuf[currchar] = ch;
				move(b_lines, currchar + 10);
				outs(&inbuf[currchar++]);
			}
			continue;
		}
		if (ch == '\n' || ch == '\r') {
			if (currchar) {
				chatting = chat_cmd(inbuf, cfd);
				if (chatting == 0)
					chatting = chat_send(cfd, inbuf);
				if (!strncmp(inbuf, "/b", 2))
					break;

				for (cmdpos = MAXLASTCMD - 1; cmdpos; cmdpos--)
					strcpy(lastcmd[cmdpos], lastcmd[cmdpos - 1]);
				strcpy(lastcmd[0], inbuf);

				inbuf[0] = '\0';
				currchar = cmdpos = 0;
				move(b_lines, 10);
				clrtoeol();
			}
			continue;
		}
		if (ch == Ctrl('H') || ch == '\177') {
			if (currchar) {
				currchar--;
				inbuf[69] = '\0';
				memcpy(&inbuf[currchar], &inbuf[currchar + 1], 69 - currchar);
				move(b_lines, currchar + 10);
				clrtoeol();
				outs(&inbuf[currchar]);
			}
			continue;
		}
		if (ch == Ctrl('Q')) {
			inbuf[0] = '\0';
			currchar = 0;
			move(b_lines, 10);
			clrtoeol();
			continue;
		}
		if (ch == Ctrl('C') || ch == Ctrl('D')) {
			chat_send(cfd, "/b");
			if (recflag == 1) {
				set_rec();
			}
			break;
		}
	}
	close(cfd);
	add_io(0, 0);
	uinfo.in_chat = NA;
	uinfo.chatid[0] = '\0';
	update_utmp();
	clear();
	refresh();
	return 0;
}


int
printuserent(uentp)
struct user_info *uentp;
{
	static char uline[256];
	static int cnt;
	char    pline[50];
	if (!uentp) {
		if (cnt)
			printchatline(uline);
		bzero(uline, 256);
		cnt = 0;
		return 0;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	if (!(HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK)) && uentp->invisible)
		return 0;

#if 0
	if (kill(uentp->pid, 0) == -1)
		return 0;
#endif

	sprintf(pline, " %s%-13s[m%c%-10.10s", myfriend(uentp->userid) ? "[1;32m" : "", uentp->userid, uentp->invisible ? '#' : ' ',
		modestring(uentp->mode, uentp->destuid, 0, NULL));
	if (cnt < 2)
		strcat(pline, "│");
	strcat(uline, pline);
	if (++cnt == 3) {
		printchatline(uline);
		memset(uline, 0, 256);
		cnt = 0;
	}
	return 0;
}


void
chat_help(arg)
char   *arg;
{
	char   *ptr;
	char    buf[256];
	FILE   *fp;
	if (strstr(arg, " op")) {
		if ((fp = fopen("help/chatophelp", "r")) == NULL)
			return;
		while (fgets(buf, 255, fp) != NULL) {
			ptr = strstr(buf, "\n");
			*ptr = '\0';
			printchatline(buf);
		}
		fclose(fp);
	} else {
		if ((fp = fopen("help/chathelp", "r")) == NULL)
			return;
		while (fgets(buf, 255, fp) != NULL) {
			ptr = strstr(buf, "\n");
			*ptr = '\0';
			printchatline(buf);
		}
		fclose(fp);
	}
}
/* youzi 1997.7.25 */
void 
query_user(arg)
char   *arg;
{
	char   *userid, msg[STRLEN * 2];
	int     exp, perf, clr = 0;
	char    qry_mail_dir[STRLEN], buf[50];
	userid = strrchr(arg, ' ');

	if (userid == NULL) {
		printchatline("[1;37m★ [33m請輸入您要查尋的 ID [37m★[m");
		return;
	}
	userid++;
	if (!getuser(userid)) {
		printchatline("[1;31m不正確的使用者代號[m");
		return;
	}
	sprintf(qry_mail_dir, "mail/%c/%s/%s", toupper(lookupuser.userid[0]),
		lookupuser.userid, DOT_DIR);
	exp = countexp(&lookupuser);
	perf = countperf(&lookupuser);

	sprintf(msg, "[1;37m%s[m ([1;33m%s[m) 共上站 [1;32m%d[m 次, 發表"
		"過 [1;32m%d[m 篇文章", lookupuser.userid, lookupuser.username,
		lookupuser.numlogins, lookupuser.numposts);

	printchatline(msg);

	strcpy(genbuf, ctime(&(lookupuser.lastlogin)));

	if ( HAS_DEFINE(lookupuser.userdefine, DEF_COLOREDSEX) )
		clr = (lookupuser.gender == 'F') ? 5 : 6;
	else
		clr = 2;	/* 安能辨我是雄雌 ?! :D */

	if ( strcasecmp(lookupuser.userid, "guest") != 0 )
		sprintf(buf, "[[1;3%dm%s[m] ", clr, horoscope(lookupuser.birthmonth, lookupuser.birthday));
	else
		sprintf(buf, "");
		
	sprintf(msg, "%s上次在 [[1;32m%-24.24s[m] 由 [[1;32m%s[m] 到本站一遊",
		( HAS_DEFINE(lookupuser.userdefine, DEF_S_HOROSCOPE) ) ? buf : "", genbuf,
		(lookupuser.lasthost[0] == '\0' ? "(不詳)" : lookupuser.lasthost));
	printchatline(msg);

	sprintf(msg, "信箱：[[1;5;32m%2s[m]，經驗值：[[1;32m%d[m]([1;36m%s[m)"
		" 表現值：[[1;32m%d[m]([1;36m%s[m) 生命力：[[1;32m%d[m]",
		(check_query_mail(qry_mail_dir) == 1) ? "☉" : "  ", exp, cexp(exp),
		perf, cperf(perf), compute_user_value(&lookupuser));
	printchatline(msg);
	sprintf(msg, "[1;37m目前%s站上[m", (t_search(userid, NA) != NULL) ?
		"在" : "不在");
	printchatline(msg);
}

void
call_user(arg)
char   *arg;
{
	char   *userid, msg[STRLEN * 2];
	struct user_info *uin;
	int     good_id;
	userid = strrchr(arg, ' ');
	if (userid == NULL) {
		printchatline("[1;37m★ [32m請輸入你要邀請的 ID[37m ★[m");
		return;
	} else
		userid += 1;
	if (!strcasecmp(userid, currentuser.userid))
		good_id = NA;
	else {
		uin = t_search(userid, NA);
		if (uin == NULL)
			good_id = NA;
		else
			good_id = YEA;
	}
	if (good_id == YEA && canmsg(uin)) {
		sprintf(msg, "到 %s 的 %s 包廂聊聊天", chat_station, chatroom);
		do_sendmsg(uin, msg, 1, uin->pid);
		sprintf(msg, "[1;37m已經幫你邀請 [32m%s[37m 了[m", uin->userid);
	} else
		sprintf(msg, "[1;32m%s[37m %s[m", userid,
			uin ? "無法呼叫" : "並沒有上站");
	printchatline(msg);
}


void
chat_date()
{
	time_t  thetime;
	time(&thetime);
	sprintf(genbuf, "[1m %s標準時間: [32m%s[m", BoardName, Cdate(&thetime));
	printchatline(genbuf);
}


void
chat_users()
{
	printchatline("");
	sprintf(genbuf, "[1m【 [36m%s [37m的訪客列表 】[m", BoardName);
	printchatline(genbuf);
	printchatline(msg_shortulist);

	if (apply_ulist(printuserent) == -1) {
		printchatline("[1m空無一人[m");
	}
	printuserent(NULL);
}

void
set_rec()
{
	char    fname[STRLEN];
	time_t  now;
	now = time(0);
/*        if(!(HAS_PERM(PERM_SYSOP)||HAS_PERM(PERM_SEECLOAK)))
                        return;                        */

	sprintf(fname, "tmp/chat.%s", currentuser.userid);
	if (recflag == 0) {
		if ((rec = fopen(fname, "w")) == NULL)
			return;

		printchatline("[1;5;32m開始錄音...[m");
		recflag = 1;
		move(0, 0);
		clrtoeol();
		sprintf(genbuf, "房間： [32m%s", chatroom);
		prints("[1;44;33m %-21s  [33m話題：[36m%-47s[5;31m%6s[m", genbuf, buftopic, (recflag == 1) ? "錄音中" : "      ");

		fprintf(rec, "本段由 %s", currentuser.userid);
		fprintf(rec, "所錄下，時間： %s", ctime(&now));
	} else {
		recflag = 0;
		move(0, 0);
		clrtoeol();
		sprintf(genbuf, "房間： [32m%s", chatroom);
		prints("[1;44;33m %-21s  [33m話題：[36m%-47s[5;31m%6s[m", genbuf, buftopic, (recflag == 1) ? "錄音中" : "      ");

		printchatline("[1;5;32m錄音結束...[m");
		fprintf(rec, "結束時間：%s\n", ctime(&now));
		fclose(rec);
		mail_file(fname, currentuser.userid, "錄音結果");
/*                postfile(fname,"syssecurity","錄音結果",2); */
		unlink(fname);
	}
}


void
setpager()
{
	char    buf[STRLEN];
	t_pager();
	sprintf(buf, "[1;32m◆ [31m呼叫器 %s 了[m", (uinfo.pager & ALL_PAGER) ? "打開" : "關閉");
	printchatline(buf);

}

struct chat_command chat_cmdtbl[] = {
	{"pager", setpager},
	{"help", chat_help},
	{"clear", chat_clear},
	{"date", chat_date},
	{"users", chat_users},
	{"set", set_rec},
	{"call", call_user},
	{"query", query_user},	/* 1997.7.25 youzi */
	{NULL, NULL}
};

int
chat_cmd_match(buf, str)
char   *buf;
char   *str;
{
	while (*str && *buf && !isspace(*buf)) {
		if (tolower(*buf++) != *str++)
			return 0;
	}
	return 1;
}


int
chat_cmd(buf)
char   *buf;
{
	int     i;
	if (*buf++ != '/')
		return 0;

	for (i = 0; chat_cmdtbl[i].cmdname; i++) {
		if (*buf != '\0' && chat_cmd_match(buf, chat_cmdtbl[i].cmdname)) {
			chat_cmdtbl[i].cmdfunc(buf);
			return 1;
		}
	}
	return 0;
}
