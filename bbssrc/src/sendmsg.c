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
$Id: sendmsg.c,v 1.2 2000/01/30 15:24:12 edwardc Exp $
*/

#include "bbs.h"
#ifdef lint
#include <sys/uio.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

extern int RMSG;
extern int msg_num;
char    buf2[STRLEN];
struct user_info *t_search();
extern struct UTMPFILE *utmpshm;

int
get_msg(uid, msg, line)
char   *msg, *uid;
int     line;
{
	char    genbuf[3];
	move(line, 0);
	clrtoeol();
	prints("送音信給：%s", uid);
	memset(msg, 0, sizeof(msg));
	while (1) {
		getdata(line + 1, 0, "音信 : ", msg, 55, DOECHO, NA);
		if (msg[0] == '\0')
			return NA;
		getdata(line + 2, 0, "確定要送出嗎(Y)是的 (N)不要 (E)再編輯? [Y]: ",
			genbuf, 2, DOECHO, YEA);
		if (genbuf[0] == 'e' || genbuf[0] == 'E')
			continue;
		if (genbuf[0] == 'n' || genbuf[0] == 'N')
			return NA;
		else
			return YEA;
	}
}

char
msgchar(uin)
struct user_info *uin;
{
	if (isreject(uin))
		return '*';
	if ((uin->pager & ALLMSG_PAGER))
		return ' ';
	if (hisfriend(uin)) {
		if ((uin->pager & FRIENDMSG_PAGER))
			return 'O';
		else
			return '#';
	}
	return '*';
}

int
canmsg(uin)
struct user_info *uin;
{
	if (isreject(uin))
		return NA;
	if ((uin->pager & ALLMSG_PAGER) || HAS_PERM(PERM_SYSOP | PERM_FORCEPAGE))
		return YEA;
	if ((uin->pager & FRIENDMSG_PAGER) && hisfriend(uin))
		return YEA;
	return NA;
}
s_msg()
{
	do_sendmsg(NULL, NULL, 0, 0);
}

int
show_allmsgs()
{
	char    fname[STRLEN];
#ifdef LOG_MY_MESG
	setuserfile(fname, "msgfile.me");
#else
	setuserfile(fname, "msgfile");
#endif
	clear();
	modify_user_mode(LOOKMSGS);
	if (dashf(fname)) {
		mesgmore(fname, YEA, 0, 9999);
		clear();
	} else {
		move(5, 30);
		prints("沒有任何的訊息存在！！");
		pressanykey();
		clear();
	}
}

int
do_sendmsg(uentp, msgstr, mode, userpid)
struct user_info *uentp;
char    msgstr[256];
int     mode;
int     userpid;
{
	char    uident[STRLEN], ret_str[20];
	time_t  now;
	struct user_info *uin;
	char    buf[80], *msgbuf, *timestr;
#ifdef LOG_MY_MESG
	char   *mymsg, buf2[80];
	int     ishelo = 0;	/* 是不是好友上站通知訊息 */
	mymsg = (char *) malloc(256);
#endif
	msgbuf = (char *) malloc(256);

	if (mode == 0) {
		move(2, 0);
		clrtobot();
		if (uinfo.invisible && !HAS_PERM(PERM_SYSOP)) {
			move(2, 0);
			prints("抱歉, 此功\能在隱身狀態下不能執行...\n");
			pressreturn();
			return 0;
		}
		modify_user_mode(MSG);
	}
	if (uentp == NULL) {
		prints("<輸入使用者代號>\n");
		move(1, 0);
		clrtoeol();
		prints("送訊息給: ");
		creat_list();
		namecomplete(NULL, uident);
		if (uident[0] == '\0') {
			clear();
			return 0;
		}
		uin = t_search(uident, NA);
		if (uin == NULL) {
			move(2, 0);
			prints("對方目前不在線上...\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
		if (uin->mode == IRCCHAT || uin->mode == BBSNET || uin->mode == WWW
			|| uin->mode == HYTELNET || uin->mode == GAME || uin->mode == PAGE
			|| !canmsg(uin)) {
			move(2, 0);
			prints("目前無法傳送訊息給對方.\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
	} else {
		if (!strcmp(uentp->userid, currentuser.userid))
			return 0;
		uin = uentp;
		if (uin->mode == IRCCHAT || uin->mode == BBSNET || uin->mode == WWW
			|| uin->mode == HYTELNET || uin->mode == GAME || uin->mode == PAGE
			|| !canmsg(uin))
			return 0;
		strcpy(uident, uin->userid);
	}
	if (msgstr == NULL) {
		if (!get_msg(uident, buf, 1)) {
			move(1, 0);
			clrtoeol();
			move(2, 0);
			clrtoeol();
			return 0;
		}
	}
	now = time(0);
	timestr = ctime(&now) + 11;
	*(timestr + 8) = '\0';
	strcpy(ret_str, "^Z回");
	if (msgstr == NULL || mode == 2) {
		sprintf(msgbuf, "[0;1;44;36m%-12.12s[33m([36m%-5.5s[33m):[37m%-54.54s[31m(%s)[m[%05dm\n",
			currentuser.userid, timestr, (msgstr == NULL) ? buf : msgstr, ret_str, uinfo.pid);
#ifdef LOG_MY_MESG
		sprintf(mymsg, "[1;32;40mTo [1;33;40m%s[m (%-5.5s):%-59.59s\n",
			uin->userid, timestr, (msgstr == NULL) ? buf : msgstr);

		sprintf(buf2, "你的好朋友 %s 已經上站囉！", currentuser.userid);

		if (msgstr != NULL)
			if (strcmp(msgstr, buf2) == 0)
				ishelo = 1;
			else if (strcmp(buf, buf2) == 0)
				ishelo = 1;
#endif
	} else if (mode == 0) {
		sprintf(msgbuf, "[0;1;5;44;33m站長 於[36m %8.8s [33m廣播：[m[1;37;44m%-57.57s[m[%05dm\n",
			timestr, msgstr, uinfo.pid);
	} else if (mode == 1) {
		sprintf(msgbuf, "[0;1;44;36m%-12.12s[37m([36m%-5.5s[37m) 邀請你[37m%-48.48s[31m(%s)[m[%05dm\n",
			currentuser.userid, timestr, msgstr, ret_str, uinfo.pid);
	} else if (mode == 3) {
		sprintf(msgbuf, "[0;1;45;36m%-12.12s[33m([36m%-5.5s[33m):[37m%-54.54s[31m(%s)[m[%05dm\n",
			currentuser.userid, timestr, (msgstr == NULL) ? buf : msgstr, ret_str, uinfo.pid);
	}
	if (userpid) {
		if (userpid != uin->pid) {
			saveline(0, 0);	/* Save line */
			move(0, 0);
			clrtoeol();
			prints("[1m對方已經離線...[m\n");
			sleep(1);
			saveline(0, 1);	/* restore line */
			return -1;
		}
	}
	if (!uin->active || kill(uin->pid, 0) == -1) {
		if (msgstr == NULL) {
			prints("\n對方已經離線...\n");
			pressreturn();
			clear();
		}
		return -1;
	}
	sethomefile(buf, uident, "msgfile");
	file_append(buf, msgbuf);

#ifdef LOG_MY_MESG
	/*
	 * 990610.edwardc 除了我直接送訊息給某人外, 其他如廣撥給站上
	 * 撥好友一律不紀錄 .. :)
	 */

	if (mode == 2 || (mode == 0 && msgstr == NULL)) {
		if (ishelo == 0) {
			sethomefile(buf, currentuser.userid, "msgfile.me");
			file_append(buf, mymsg);
		}
	}
	sethomefile(buf, uident, "msgfile.me");
	file_append(buf, msgbuf);
	free(mymsg);

#endif
	free(msgbuf);

	kill(uin->pid, SIGTTOU);
	kill(uin->pid, SIGUSR2);
	if (msgstr == NULL) {
		prints("\n已送出訊息...\n");
		pressreturn();
		clear();
	}
	return 1;
}

int
dowall(uin)
struct user_info *uin;
{
	if (!uin->active || !uin->pid)
		return -1;
	move(1, 0);
	clrtoeol();
	prints("[1;32m正對 %s 廣播.... Ctrl-D 停止對此位 User 廣播。[m", uin->userid);
	refresh();
	do_sendmsg(uin, buf2, 0, uin->pid);
}

int
myfriend_wall(uin)
struct user_info *uin;
{
	if ((uin->pid - uinfo.pid == 0) || !uin->active || 
		!uin->pid || isreject(uin) || uin->invisible )
		return -1;
	if (myfriend(uin->uid)) {
		move(1, 0);
		clrtoeol();
		prints("[1;32m正在送訊息給 %s...  [m", uin->userid);
		refresh();
		do_sendmsg(uin, buf2, 3, uin->pid);
	}
}

int
hisfriend_wall(uin)
struct user_info *uin;
{
	if ((uin->pid - uinfo.pid == 0) || !uin->active ||
		 !uin->pid || isreject(uin) || uin->invisible )
		return -1;
	if (hisfriend(uin)) {
		move(1, 0);
		clrtoeol();
		prints("[1;32m正在送訊息給 %s...  [m", uin->userid);
		refresh();
		do_sendmsg(uin, buf2, 3, uin->pid);
	}
}

int
friend_wall()
{
	char    buf[3];
	char    msgbuf[256], filename[80];
	time_t  now;
	char   *timestr;
	now = time(0);
	timestr = ctime(&now) + 11;
	*(timestr + 8) = '\0';

	if (uinfo.invisible) {
		move(2, 0);
		prints("抱歉, 此功\能在隱身狀態下不能執行...\n");
		pressreturn();
		return 0;
	}
	modify_user_mode(MSG);
	move(2, 0);
	clrtobot();
	getdata(4, 0, "送訊息給 [1] 我的好朋友，[2] 與我為友者: ",
		buf, 2, DOECHO, YEA);
	switch (buf[0]) {
	case '1':
		if (!get_msg("我的好朋友", buf2, 1))
			return 0;
		if (apply_ulist(myfriend_wall) == -1) {
			move(2, 0);
			prints("線上空無一人\n");
			pressanykey();
		} else {
			/* 記錄送訊息給好友 */
			sprintf(msgbuf, "[0;1;45;36m送訊息給好友[33m([36m%-5.5s[33m):[37m%-54.54s[31m(^Z回)[m[%05dm\n",
				timestr, buf2, uinfo.pid);
			setuserfile(filename, "msgfile.me");
			file_append(filename, msgbuf);
		}
		break;
	case '2':
		if (!get_msg("與我為友者", buf2, 1))
			return 0;
		if (apply_ulist(hisfriend_wall) == -1) {
			move(2, 0);
			prints("線上空無一人\n");
			pressanykey();
		} else {
			/* 記錄送訊息給與我為友者 */
			sprintf(msgbuf, "[0;1;45;36m送給與我為友[33m([36m%-5.5s[33m):[37m%-54.54s[31m(^Z回)[m[%05dm\n",
				timestr, buf2, uinfo.pid);
			setuserfile(filename, "msgfile.me");
			file_append(filename, msgbuf);

		}
		break;
	default:
		return 0;
	}
	move(6, 0);
	prints("訊息傳送完畢...");
	pressanykey();
	return 1;
}

void
r_msg2()
{
	FILE   *fp;
	char    buf[256];
	char    msg[256];
	char    fname[STRLEN];
	int     line, tmpansi;
	int     y, x, ch, i, j;
	int     MsgNum;
	getyx(&y, &x);
	if (uinfo.mode == TALK)
		line = t_lines / 2 - 1;
	else
		line = 0;
	setuserfile(fname, "msgfile");
	i = get_num_records(fname, 129);
	if (i == 0)
		return;
	tmpansi = showansi;
	showansi = 1;
	oflush();
	if (RMSG == NA) {
		saveline(line, 0);
		saveline(line + 1, 2);
	}
	MsgNum = 0;
	RMSG = YEA;
	while (1) {
		MsgNum = (MsgNum % i);
		if ((fp = fopen(fname, "r")) == NULL) {
			RMSG = NA;
			return;
		}
		for (j = 0; j < (i - MsgNum); j++) {
			if (fgets(buf, 256, fp) == NULL)
				break;
			else
				strcpy(msg, buf);
		}
		fclose(fp);
		move(line, 0);
		clrtoeol();
		prints("%s", msg);
		refresh();
		{
			struct user_info *uin;
			char    msgbuf[STRLEN];
			int     good_id, send_pid;
			char   *ptr, usid[STRLEN];
			ptr = strrchr(msg, '[');
			send_pid = atoi(ptr + 1);
			ptr = strtok(msg + 12, " [");
			if (ptr == NULL)
				good_id = NA;
			else if (!strcmp(ptr, currentuser.userid))
				good_id = NA;
			else {
				strcpy(usid, ptr);
				uin = t_search(usid, send_pid);
				if (uin == NULL)
					good_id = NA;
				else
					good_id = YEA;
			}
			if (good_id == YEA && canmsg(uin)) {
				int     userpid;
				userpid = uin->pid;
				move(line + 1, 0);
				clrtoeol();
				sprintf(msgbuf, "回訊息給 %s: ", usid);
				getdata(line + 1, 0, msgbuf, buf, 55, DOECHO, YEA);
				if (buf[0] == Ctrl('Z')) {
					MsgNum++;
					continue;
				} else if (buf[0] == Ctrl('A')) {
					MsgNum--;
					if (MsgNum < 0)
						MsgNum = i - 1;
					continue;
				}
				if (buf[0] != '\0') {
					if (do_sendmsg(uin, buf, 2, userpid) == 1)
						sprintf(msgbuf, "[1;32m幫你送出訊息給 %s 了![m", usid);
					else
						sprintf(msgbuf, "[1;32m訊息無法送出.[m");
				} else
					sprintf(msgbuf, "[1;33m空訊息, 所以不送出.[m");
				refresh();
				move(line + 1, 0);
				clrtoeol();
				refresh();
				prints("%s", msgbuf);
				refresh();
				if (!strstr(msgbuf, "幫你"))
					sleep(1);
			} else {
				sprintf(msgbuf, "[1;32m找不到發訊息的 %s! 請按上:[^Z ↑] 或下:[^A ↓] 或其他鍵離開.[m", usid);
				move(line + 1, 0);
				clrtoeol();
				refresh();
				prints("%s", msgbuf);
				refresh();
				if ((ch = igetkey()) == Ctrl('Z') || ch == KEY_UP) {
					MsgNum++;
					continue;
				}
				if (ch == Ctrl('A') || ch == KEY_DOWN) {
					MsgNum--;
					if (MsgNum < 0)
						MsgNum = i - 1;
					continue;
				}
			}
		}
		break;
	}
	saveline(line, 1);
	saveline(line + 1, 3);
	showansi = tmpansi;
	move(y, x);
	refresh();
	RMSG = NA;
	return;
}

void
count_msg()
{
	signal(SIGTTOU, count_msg);
	msg_num++;
}

void
r_msg()
{
	FILE   *fp;
	char    buf[256];
	char    msg[256];
	char    fname[STRLEN];
	int     line, tmpansi;
	int     y, x, i, j, premsg;
	char    ch;
	signal(SIGUSR2, r_msg);
	getyx(&y, &x);
	tmpansi = showansi;
	showansi = 1;
	if (uinfo.mode == TALK)
		line = t_lines / 2 - 1;
	else
		line = 0;
	if (DEFINE(DEF_MSGGETKEY)) {
		oflush();
		saveline(line, 0);
		premsg = RMSG;
	}
	while (msg_num) {
		if (DEFINE(DEF_SOUNDMSG)) {
			bell();
		}
		setuserfile(fname, "msgfile");
		i = get_num_records(fname, 129);
		if ((fp = fopen(fname, "r")) == NULL)
			return;
		for (j = 0; j <= (i - msg_num); j++) {
			if (fgets(buf, 256, fp) == NULL)
				break;
			else
				strcpy(msg, buf);
		}
		fclose(fp);
		move(line, 0);
		clrtoeol();
		prints("%s", msg);
		refresh();
		msg_num--;
		if (DEFINE(DEF_MSGGETKEY)) {
			RMSG = YEA;
			ch = 0;
#ifdef MSG_CANCLE_BY_CTRL_C
			while (ch != Ctrl('C'))
#else
			while (ch != '\r' && ch != '\n')
#endif
			{
				ch = igetkey();
#ifdef MSG_CANCLE_BY_CTRL_C
				if (ch == Ctrl('C'))
					break;
#else
				if (ch == '\r' || ch == '\n')
					break;
#endif
				else if (ch == Ctrl('R') || ch == 'R' || ch == 'r' || ch == Ctrl('Z')) {
					struct user_info *uin;
					char    msgbuf[STRLEN];
					int     good_id, send_pid;
					char   *ptr, usid[STRLEN];
					ptr = strrchr(msg, '[');
					send_pid = atoi(ptr + 1);
					ptr = strtok(msg + 12, " [");
					if (ptr == NULL)
						good_id = NA;
					else if (!strcmp(ptr, currentuser.userid))
						good_id = NA;
					else {
						strcpy(usid, ptr);
						uin = t_search(usid, send_pid);
						if (uin == NULL)
							good_id = NA;
						else
							good_id = YEA;
					}
					oflush();
					saveline(line + 1, 2);
					if (good_id == YEA) {
						int     userpid;
						userpid = uin->pid;
						move(line + 1, 0);
						clrtoeol();
						sprintf(msgbuf, "立即回訊息給 %s: ", usid);
						getdata(line + 1, 0, msgbuf, buf, 55, DOECHO, YEA);
						if (buf[0] != '\0' && buf[0] != Ctrl('Z') && buf[0] != Ctrl('A')) {
							if (do_sendmsg(uin, buf, 2, userpid))
								sprintf(msgbuf, "[1;32m幫你送出訊息給 %s 了![m", usid);
							else
								sprintf(msgbuf, "[1;32m訊息無法送出.[m");
						} else
							sprintf(msgbuf, "[1;33m空訊息, 所以不送出.[m");
					} else {
						sprintf(msgbuf, "[1;32m找不到發訊息的 %s.[m", usid);
					}
					refresh();
					move(line + 1, 0);
					clrtoeol();
					refresh();
					prints("%s", msgbuf);
					refresh();
					if (!strstr(msgbuf, "幫你"))
						sleep(1);
					saveline(line + 1, 3);
					refresh();
					break;
				}	/* if */
			}	/* while */
		}		/* if */
	}			/* while */
	if (DEFINE(DEF_MSGGETKEY)) {
		RMSG = premsg;
		saveline(line, 1);
	}
	showansi = tmpansi;
	move(y, x);
	refresh();
	return;
}

int
friend_login_wall(pageinfo)
struct user_info *pageinfo;
{
	char    msg[STRLEN];
	int     x, y;
	if (!pageinfo->active || !pageinfo->pid || isreject(pageinfo))
		return 0;
	if (hisfriend(pageinfo)) {
		if (getuser(pageinfo->userid) <= 0)
			return 0;
		if (!(lookupuser.userdefine & DEF_LOGINFROM))
			return 0;
		if (!strcmp(pageinfo->userid, currentuser.userid))
			return 0;
		/* edwardc.990427 好友隱身就不顯示送出上站通知 */
		if (pageinfo->invisible)
			return 0;
		getyx(&y, &x);
		if (y > 22) {
			pressanykey();
			move(7, 0);
			clrtobot();
		}
		prints("送出好友上站通知給 %s\n", pageinfo->userid);
		sprintf(msg, "你的好朋友 %s 已經上站囉！", currentuser.userid);
		do_sendmsg(pageinfo, msg, 2, pageinfo->pid);
	}
	return 0;
}
