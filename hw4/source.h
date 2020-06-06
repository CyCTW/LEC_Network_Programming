#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>
#include<sys/types.h>
#include <sys/stat.h>

#include<unistd.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sqlite3.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<map>
#include<unordered_map>
#include<vector>
#include<sstream>
#include<iomanip>
#include<ctime>
#include<chrono>
using namespace std;

unordered_map<int, string> online; //fd, username, check if a user is now online
unordered_map<string, string> user_bucket_tb;
unordered_map<string, int> user_mailid;
unordered_map<int, string> postid_name;
int POST_ID = 1;
string post_username;
sqlite3 *db;

string welcome_buf = "********************************\n"\
					 "** Welcome to the BBS server. **\n"\
					 "********************************\n% ";

string register_format = "register <username> <email> <password>\n";
string login_format = "login <username> <password>\n";
string create_board_format = "create-board <name>\n";
string create_post_format = "create-post <board-name> --title <title> --content <content>\n";
string list_board_format = "list-board ##<key>\n";
string list_post_format = "list-post <board-name> ##<key>\n";
string read_format = "read <post-id>\n";
string delete_post_format = "delete-post <post-id>\n";
string update_post_format = "update-post <post-id> --title/content <new>\n";
string comment_format = "comment <post-id> <comment>\n";
string not_login = "Please login first.\n";

string mail_to_format = "mail-to <username> --subject <subject> --content <content>\n";
string list_mail_format = "list-mail\n";
string retr_mail_format = "retr-mail <mail#>\n";
string delete_mail_format = "delete-mail <mail#>\n";

string subscribe_format = "usage: subscribe --board <board-name> --keyword <keyword>\n";
string already_sub = "Already subscribed.\n";
string non_subscirbe_format = "You haven't subscribe.\n";
string sub_succ = "Subscribe successfully.\n";
string unsub_succ = "Unsubscribe successfully.\n";

static int DB_check_enter(void *enter, int argc, char **argv, char **colname) {
	*(bool*)enter = true; 
	// if (colname[argc-1] == "AUTHOR") {
	// 	post_username = argv[argc-1];
	// }
	return 0;
}
string glo_ret;
int id_idx = 1;
int board_index = 1;
int mail_index = 1;

static int DB_list_data(void *enter, int argc, char **argv, char **colname) {
	// pair<int, bool >* fd_enter= (pair<int, bool >*)fd;
	*(bool*)enter = true;
	// fd_enter->second = true;
	string s = "   ";
	int lens;
	for(int i=0; i<argc; i++) {
		string arg = string(argv[i]);

		if (string(colname[i]) == "DATE") {
			arg = arg.substr(5);
			arg.replace(2, 1, "/");
		}
		else if (string(colname[i]) == "BOARD_ID") {
			arg = to_string(board_index++);
		}
		else if (string(colname[i]) == "OBJECT_NAME") {
			arg = to_string(mail_index++);
		}

        s += arg; 
   		lens = 10 - arg.size();
		do {
			s += ' ';
			lens--;
		} while(lens > 0);
		// s += "\t";
	}
	s += '\n';
	// ret += s;
	glo_ret += s;
	// send(fd_enter->first, s.c_str(), s.size(), 0);
	return 0;
}
static int DB_read_posts(void *fd, int argc, char **argv, char **colname) {
	pair<int, bool>* fd_enter= (pair<int, bool>*)fd;

	// set enter = true
	fd_enter->second = true;
	string s;
	string ff[6] = {"  Author   :", "  Title    :", "  Date     :", "  Subject   :", "  From      :", "  Date      :"};
	for(int i=0; i<argc; i++) {
		s += ff[(3 * fd_enter->first) + i];
		s += string(argv[i]);
		s += '\n';
	}
	glo_ret += s;

	return 0;
}

static int DB_get_mailname(void *fd, int argc, char **argv, char **colname) {
	*(string*)fd = argv[0];
}


bool check_online(int sockfd, string &username, string &ret) {
	if (online.find(sockfd) != online.end()) {
		username = online[sockfd];
		return true;
	}
	else {
		ret += not_login;
		return false;
	}
}