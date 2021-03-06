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
$Id: userinfo.c,v 1.2 2000/01/18 11:22:35 edwardc Exp $
*/

#include "bbs.h"

extern time_t login_start_time;
extern char fromhost[60];
char   *
genpasswd();
char   *sysconf_str();

void
disply_userinfo(u, real)
struct userec *u;
int     real;
{
	int     num, diff;
	int     exp;
#ifdef REG_EXPIRED
	time_t  nextreg, now;
#endif

	move(2, 0);
	clrtobot();
	prints("您的代號     : %-40s", u->userid);
	if (real)
		prints("      性別 : %s", (u->gender == 'M' ? "男" : "女"));
	prints("\n您的暱稱     : %s\n", u->username);
	prints("真實姓名     : %-40s", u->realname);
	if (real)
		prints("  出生日期 : %d/%d/%d", u->birthmonth, u->birthday, u->birthyear + 1900);
	prints("\n居住住址     : %s\n", u->address);
	prints("電子郵件信箱 : %s\n", u->email);
	if (real) {
		prints("真實 E-mail  : %s\n", u->reginfo);
		if HAS_PERM
			(PERM_ADMINMENU)
				prints("Ident 資料   : %s\n", u->ident);
	}
	prints("終端機形態   : %s\n", u->termtype);
	prints("帳號建立日期 : %s", ctime(&u->firstlogin));
	prints("最近光臨日期 : %s", ctime(&u->lastlogin));
	if (real) {

		/* edwardc.990410 顯示下次身份確認時間比較實用 ? :p */
#ifndef REG_EXPIRED
		prints("身份確認日期 : %s", (u->lastjustify == 0) ? "未曾註冊\n" : ctime(&u->lastjustify));
#else
		prints("身份確認     : %s", (u->lastjustify == 0) ? "未曾註冊" : "已完成，有效期限: ");
		nextreg = u->lastjustify + REG_EXPIRED * 86400;
		now = time(0);
		sprintf(genbuf, "，還有 %d 天\n", (nextreg - now) / 86400);
		prints("%s%s", (u->lastjustify == 0) ? "" : (char *) Ctime(&nextreg)
			,(u->lastjustify == 0) ? "\n" : genbuf);
#endif

		prints("最近光臨機器 : %s\n", u->lasthost);
		prints("文章數目     : %d\n", u->numposts);
		prints("私人信箱     : %d 封\n", u->nummails);
	}
	prints("上站次數     : %d 次\n", u->numlogins);
	prints("上站總時數   : %d 小時 %d 分鐘\n", u->stay / 3600, (u->stay / 60) % 60);
	exp = countexp(u);
	prints("經驗值       : %d  (%s)\n", exp, cexp(exp));
	exp = countperf(u);
	prints("表現值       : %d  (%s)\n", exp, cperf(exp));
	if (real) {
		strcpy(genbuf, "bTCPRp#@XWBA#VS-DOM-F012345678\0");
		for (num = 0; num < strlen(genbuf) - 1; num++)
			if (!(u->userlevel & (1 << num)))
				genbuf[num] = '-';
		prints("使用者權限   : %s\n", genbuf);
	} else {
		diff = (time(0) - login_start_time) / 60;
		prints("停留期間     : %d 小時 %02d 分\n", diff / 60, diff % 60);
		prints("螢幕大小     : %dx%d\n", t_lines, t_columns);
	}
	prints("\n");
	if (u->userlevel & PERM_BOARDS) {
		prints("  您是本站的板主, 感謝您的付出.\n");
	} else if (u->userlevel & PERM_LOGINOK) {
		prints("  您的註冊程序已經完成, 歡迎加入本站.\n");
	} else if (u->lastlogin - u->firstlogin < 3 * 86400) {
		prints("  新手上路, 請閱\讀 Announce 討論區.\n");
	} else {
		prints("  註冊尚未成功\, 請參考本站進站畫面說明.\n");
	}
}


int
uinfo_query(u, real, unum)
struct userec *u;
int     real, unum;
{
	struct userec newinfo;
	char    ans[3], buf[STRLEN], genbuf[128];
	char    src[STRLEN], dst[STRLEN];
	int     i, fail = 0, netty_check = 0;
	time_t  now;
	struct tm *tmnow;
	memcpy(&newinfo, u, sizeof(currentuser));
	getdata(t_lines - 1, 0, real ?
		"請選擇 (0)結束 (1)修改資料 (2)設定密碼 (3) 改 ID ==> [0]" :
		"請選擇 (0)結束 (1)修改資料 (2)設定密碼 (3) 選簽名檔 ==> [0]",
		ans, 2, DOECHO, YEA);
	clear();
	refresh();

	now = time(0);
	tmnow = localtime(&now);

	i = 3;
	move(i++, 0);
	if (ans[0] != '3' || real)
		prints("使用者代號: %s\n", u->userid);

	switch (ans[0]) {
	case '1':
		move(1, 0);
		prints("請逐項修改,直接按 <ENTER> 代表使用 [] 內的資料。\n");

		sprintf(genbuf, "暱稱 [%s]: ", u->username);
		getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0])
			strncpy(newinfo.username, buf, NAMELEN);
		if (!real && buf[0])
			strncpy(uinfo.username, buf, 40);

		sprintf(genbuf, "真實姓名 [%s]: ", u->realname);
		getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0])
			strncpy(newinfo.realname, buf, NAMELEN);

		sprintf(genbuf, "居住地址 [%s]: ", u->address);
		getdata(i++, 0, genbuf, buf, STRLEN - 10, DOECHO, YEA);
		if (buf[0])
			strncpy(newinfo.address, buf, NAMELEN);

		sprintf(genbuf, "電子信箱 [%s]: ", u->email);
		getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0]) {
			netty_check = 1;
			strncpy(newinfo.email, buf, NAMELEN);
		}
		sprintf(genbuf, "終端機形態 [%s]: ", u->termtype);
		getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
		if (buf[0])
			strncpy(newinfo.termtype, buf, 16);

		sprintf(genbuf, "出生年 [%d]: ", u->birthyear + 1900);
		getdata(i++, 0, genbuf, buf, 5, DOECHO, YEA);
		if (buf[0] && atoi(buf) > tmnow->tm_year + 1805 && atoi(buf) < tmnow->tm_year + 1897)
			newinfo.birthyear = atoi(buf) - 1900;

		sprintf(genbuf, "出生月 [%d]: ", u->birthmonth);
		getdata(i++, 0, genbuf, buf, 3, DOECHO, YEA);
		if (buf[0] && atoi(buf) >= 1 && atoi(buf) <= 12)
			newinfo.birthmonth = atoi(buf);

		sprintf(genbuf, "出生日 [%d]: ", u->birthday);
		getdata(i++, 0, genbuf, buf, 3, DOECHO, YEA);
		if (buf[0] && atoi(buf) >= 1 && atoi(buf) <= 31)
			newinfo.birthday = atoi(buf);

		sprintf(genbuf, "性別 [%c]: ", u->gender);
		getdata(i++, 0, genbuf, buf, 2, DOECHO, YEA);
		if (buf[0]) {
			if (strchr("MmFf", buf[0]))
				newinfo.gender = toupper(buf[0]);
		}
		if (real) {
			sprintf(genbuf, "真實Email[%s]: ", u->reginfo);
			getdata(i++, 0, genbuf, buf, 62, DOECHO, YEA);
			if (buf[0])
				strncpy(newinfo.reginfo, buf, STRLEN - 16);

			sprintf(genbuf, "上線次數 [%d]: ", u->numlogins);
			getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.numlogins = atoi(buf);

			sprintf(genbuf, "文章數目 [%d]: ", u->numposts);
			getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.numposts = atoi(buf);

		}
		break;
	case '2':
		if (!real) {
			getdata(i++, 0, "請輸入原密碼: ", buf, PASSLEN, NOECHO, YEA);
			if (*buf == '\0' || !checkpasswd(u->passwd, buf)) {
				prints("\n\n很抱歉, 您輸入的密碼不正確。\n");
				fail++;
				break;
			}
		}
		getdata(i++, 0, "請設定新密碼: ", buf, PASSLEN, NOECHO, YEA);
		if (buf[0] == '\0') {
			prints("\n\n密碼設定取消, 繼續使用舊密碼\n");
			fail++;
			break;
		}
		strncpy(genbuf, buf, PASSLEN);

		getdata(i++, 0, "請重新輸入新密碼: ", buf, PASSLEN, NOECHO, YEA);
		if (strncmp(buf, genbuf, PASSLEN)) {
			prints("\n\n新密碼確認失敗, 無法設定新密碼。\n");
			fail++;
			break;
		}
		buf[8] = '\0';
		strncpy(newinfo.passwd, genpasswd(buf), ENCPASSLEN);
		break;
	case '3':
		if (!real) {
			sprintf(genbuf, "目前使用簽名檔 [%d]: ", u->signature);
			getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.signature = atoi(buf);
		} else {
			getdata(i++, 0, "新的使用者代號: ", genbuf, IDLEN + 1, DOECHO, YEA);
			if (*genbuf != '\0') {
				if (getuser(genbuf)) {
					prints("\n錯誤! 已經有同樣 ID 的使用者\n");
					fail++;
				} else {
					strncpy(newinfo.userid, genbuf, IDLEN + 2);
				}
			}
		}
		break;
	default:
		clear();
		return 0;
	}
	if (fail != 0) {
		pressreturn();
		clear();
		return 0;
	}
	if (askyn("確定要改變嗎", NA, YEA) == YEA) {
		if (real) {
			char    secu[STRLEN];
			sprintf(secu, "修改 %s 的基本資料或密碼。", u->userid);
			securityreport(secu);
		}
		if (strcmp(u->userid, newinfo.userid)) {

			sprintf(src, "mail/%c/%s", toupper(u->userid[0]), u->userid);
			sprintf(dst, "mail/%c/%s", toupper(newinfo.userid[0]), newinfo.userid);
			rename(src, dst);
			sethomepath(src, u->userid);
			sethomepath(dst, newinfo.userid);
			rename(src, dst);
			sethomefile(src, u->userid, "register");
			unlink(src);
			sethomefile(src, u->userid, "register.old");
			unlink(src);
			setuserid(unum, newinfo.userid);
		}
		if ((netty_check == 1)) {
			sprintf(genbuf, "%s", BBSHOST);
			if ((sysconf_str("EMAILFILE") != NULL) &&
				(!strstr(newinfo.email, genbuf)) &&
				(!invalidaddr(newinfo.email)) &&
				(!invalid_email(newinfo.email))) {
				strcpy(u->email, newinfo.email);
				send_regmail(u);
			} else {
				if (sysconf_str("EMAILFILE") != NULL) {
					move(t_lines - 5, 0);
					prints("\n您所填的電子郵件地址 【[1;33m%s[m】\n", newinfo.email);
					prints("恕不受本站承認，系統不會投遞註冊信，請把它修正好...\n");
					pressanykey();
					/*
					 * edwardc.990521 if there's wrong
					 * email, do not update anything.
					 */
					return 0;
				}
			}
		}
		memcpy(u, &newinfo, sizeof(newinfo));
		set_safe_record();
		if (netty_check == 1) {
			newinfo.userlevel &= ~(PERM_LOGINOK | PERM_PAGE);
			sethomefile(src, newinfo.userid, "register");
			sethomefile(dst, newinfo.userid, "register.old");
			rename(src, dst);
		}
		substitute_record(PASSFILE, &newinfo, sizeof(newinfo), unum);
	}
	clear();
	return 0;
}

void
x_info()
{
	modify_user_mode(GMENU);
	if (!strcmp("guest", currentuser.userid)) {
		disply_userinfo(&currentuser, 0);
		pressreturn();
		return;
	}
	disply_userinfo(&currentuser, 1);
	uinfo_query(&currentuser, 0, usernum);
}

void
getfield(line, info, desc, buf, len)
int     line, len;
char   *info, *desc, *buf;
{
	char    prompt[STRLEN];
	sprintf(genbuf, "  原先設定: %-46.46s [1;32m(%s)[m",
		(buf[0] == '\0') ? "(未設定)" : buf, info);
	move(line, 0);
	prints(genbuf);
	sprintf(prompt, "  %s: ", desc);
	getdata(line + 1, 0, prompt, genbuf, len, DOECHO, YEA);
	if (genbuf[0] != '\0') {
		strncpy(buf, genbuf, len);
	}
	move(line, 0);
	clrtoeol();
	prints("  %s: %s\n", desc, buf);
	clrtoeol();
}

void
x_fillform()
{
	char    rname[NAMELEN], addr[STRLEN];
	char    phone[STRLEN], dept[STRLEN], assoc[STRLEN];
	char    ans[5], *mesg, *ptr;
	FILE   *fn;
	time_t  now;
	modify_user_mode(NEW);
	move(3, 0);
	clrtobot();
	if (!strcmp("guest", currentuser.userid)) {
		prints("抱歉, 請用 new 申請一個新帳號後再填申請表.");
		pressreturn();
		return;
	}
	if (currentuser.userlevel & PERM_LOGINOK) {
		prints("您已經完成本站的使用者註冊手續, 歡迎加入本站的行列.");
		pressreturn();
		return;
	}
	if ((fn = fopen("new_register", "r")) != NULL) {
		while (fgets(genbuf, STRLEN, fn) != NULL) {
			if ((ptr = strchr(genbuf, '\n')) != NULL)
				*ptr = '\0';
			if (strncmp(genbuf, "userid: ", 8) == 0 &&
				strcmp(genbuf + 8, currentuser.userid) == 0) {
				fclose(fn);
				prints("站長尚未處理您的註冊申請單, 請耐心等候.");
				pressreturn();
				return;
			}
		}
		fclose(fn);
	}
	move(3, 0);
	if (askyn("您確定要填寫註冊單嗎", NA, NA) == NA)
		return;
	strncpy(rname, currentuser.realname, NAMELEN);
	strncpy(addr, currentuser.address, STRLEN);
	dept[0] = phone[0] = assoc[0] = '\0';
	while (1) {
		move(3, 0);
		clrtoeol();
		prints("%s 您好, 請據實填寫以下的資料:\n", currentuser.userid);
		getfield(6, "請用中文", "真實姓名", rname, NAMELEN);
		getfield(8, "學校系級或公司職稱", "學校系級", dept, STRLEN);
		getfield(10, "包括寢室或門牌號碼", "目前住址", addr, STRLEN);
		getfield(12, "包括可聯絡時間", "聯絡電話", phone, STRLEN);
#ifdef NEED_ASSOC		/* edwardc.990410 i think it's not general
				 * enough */
		getfield(14, "校友會或畢業學校", "校 友 會", assoc, STRLEN);
#endif
		mesg = "以上資料是否正確, 按 Q 放棄註冊 (Y/N/Quit)? [N]: ";
		getdata(t_lines - 1, 0, mesg, ans, 3, DOECHO, YEA);
		if (ans[0] == 'Q' || ans[0] == 'q')
			return;
		if (ans[0] == 'Y' || ans[0] == 'y')
			break;
	}
	strncpy(currentuser.realname, rname, NAMELEN);
	strncpy(currentuser.address, addr, STRLEN);
	if ((fn = fopen("new_register", "a")) != NULL) {
		now = time(NULL);
		fprintf(fn, "usernum: %d, %s", usernum, ctime(&now));
		fprintf(fn, "userid: %s\n", currentuser.userid);
		fprintf(fn, "realname: %s\n", rname);
		fprintf(fn, "dept: %s\n", dept);
		fprintf(fn, "addr: %s\n", addr);
		fprintf(fn, "phone: %s\n", phone);
#ifdef NEED_ASSOC
		fprintf(fn, "assoc: %s\n", assoc);
#endif
		fprintf(fn, "----\n");
		fclose(fn);
	}
	setuserfile(genbuf, "mailcheck");
	if ((fn = fopen(genbuf, "w")) == NULL) {
		fclose(fn);
		return;
	}
	fprintf(fn, "usernum: %d\n", usernum);
	fclose(fn);
}
