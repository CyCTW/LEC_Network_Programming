#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>
#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<sqlite3.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<map>
#include<vector>
#include<sstream>
#include<iomanip>
#include<ctime>
#include<chrono>

using namespace std;

map<int, string> online; //fd, username, check if a user is now online
sqlite3 *db;

string welcome_buf = "*******************************\n"\
					 "**Welcome to the BBS server. **\n"\
					 "*******************************\n";

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

// Database callback function
static int DB_check_enter(void *enter, int argc, char **argv, char **colname) {
	*(bool*)enter = true; 

	return 0;
}

static int DB_list_data(void *fd, int argc, char **argv, char **colname) {
	pair<int, bool>* fd_enter= (pair<int, bool>*)fd;

	// if (fd_enter->second == false) {
	// 	// string col_n = "Index\tName\tModerator\n";
	// 	string col_n;
	// 	for(int i=0; i<argc; i++) {
	// 		col_n += colname[i];
	// 		col_n += '\t';
	// 	}
	// 	col_n += '\n';
	// 	send(fd_enter->first, col_n.c_str(), col_n.size(), 0);
	// }
	// set enter = true
	fd_enter->second = true;
	
	string s;
	int lens;
	for(int i=0; i<argc; i++) {
		string arg = string(argv[i]);

		if (string(colname[i]) == "DATE") {
			arg = arg.substr(5);
			arg.replace(2, 1, "/");
		}
		s += arg; 
		lens = 10 - arg.size();
		while(lens > 0) {
			s += ' ';
			lens--;
		}
		// s += "\t";
	}
	s += '\n';
	send(fd_enter->first, s.c_str(), s.size(), 0);
	return 0;
}
static int DB_read_posts(void *fd, int argc, char **argv, char **colname) {
	pair<int, bool>* fd_enter= (pair<int, bool>*)fd;

	// set enter = true
	fd_enter->second = true;
	string s;
	string ff[5] = {"Author   :", "Title    :", "Date     :", "--\n", "--\n"};
	for(int i=0; i<argc; i++) {
		s += ff[i];
		s += string(argv[i]);
		s += '\n';
	}
	send(fd_enter->first, s.c_str(), s.size(), 0);
	return 0;
}

void print_welcome(int fd) {
	
}

void register_(int sockfd, vector<string> &argu) {
	if(argu.size() != 4) {
		send(sockfd, register_format.c_str(), register_format.size(), 0);
		return;
	}

	string user = argu[1], email = argu[2], password = argu[3];
	string cmd = "INSERT INTO USERS (USERNAME, EMAIL, PASSWORD) VALUES (\'" + user + "\', " + "\'" + email + "\', " + "\'"+ password + "\');";

	int st = sqlite3_exec(db, cmd.c_str(), DB_check_enter, 0, 0);
	if ( st != SQLITE_OK ) {
		string tmp = "Username is already used.\n";
		send(sockfd, tmp.c_str(), tmp.size(), 0);
	}
	else {
		string tmp = "Register successfully.\n";
		send(sockfd, tmp.c_str(), tmp.size(), 0);
	}		
}

void login(int sockfd, vector<string> &argu) {
	if(argu.size() != 3) {
		send(sockfd, login_format.c_str(), login_format.size(), 0);
		return;
	}

	string usern = argu[1], password = argu[2];
	string cmd = "SELECT * FROM USERS WHERE USERNAME= '" + usern + 
				"' AND PASSWORD= '" + password + "';";
	bool enter = false;
	int st = sqlite3_exec(db, cmd.c_str(), DB_check_enter, (void*)&enter, 0);

	if ( online.find(sockfd) != online.end() ) {
		//has already login
		char tmp[] = "Please logout first.\n";
		send(sockfd, tmp, strlen(tmp), 0);
	}
	else {
		if ( !enter ) {
			// can't find
			string tmp = "Login failed.\n";
			send(sockfd, tmp.c_str(), tmp.size(), 0);
		}
		else {
			string tmp = "Welcome, " + usern + '\n';
			online[sockfd] = usern;
			send(sockfd, tmp.c_str(), tmp.size(), 0);
		}
	}
	
}

void logout(int sockfd, vector<string> &argu) {
	if (argu.size() != 1) {
		string tmp = "logout\n";
		send(sockfd, tmp.c_str(), tmp.size(), 0);	
		return;
	}
	if (online.find(sockfd) != online.end()) {
		string tmp = "Bye, " + online[sockfd] + '\n';
		online.erase(sockfd);
		send(sockfd, tmp.c_str(), tmp.size(), 0);
	}
	else {
		string tmp = "Please login first.\n";
		send(sockfd, tmp.c_str(), tmp.size(), 0);
	}
	
}

void whoami(int sockfd, vector<string> &argu) {
	if (argu.size() != 1) {
		string tmp = "whoami\n";
		send(sockfd, tmp.c_str(), tmp.size(), 0);
		return;
	}

	if (online.find(sockfd) != online.end()) {
		string tmp = online[sockfd] + '\n';
		send(sockfd, tmp.c_str(), tmp.size(), 0);
	}
	else {
		string tmp = "Please login first.\n";
		send(sockfd, tmp.c_str(), tmp.size(), 0);
	}

}

// hw2 new functions


void create_board(int sockfd, vector<string> &argu) {
	if (argu.size() != 2) {
		send(sockfd, create_board_format.c_str(), create_board_format.size(), 0);
		return;
	}
	
	string name = argu[1];
	if (online.find(sockfd) != online.end()) {
		// TODO: check duplicate boards	
		// string cmd = "INSERT INTO USERS (USERNAME, EMAIL, PASSWORD) VALUES (\'" + user + "\', " + "\'" + email + "\', " + "\'"+ password + "\');";
		string user_name = online[sockfd];
		string find_board = "INSERT INTO BOARDS (NAME, MODERATOR) VALUES ('" + name + "', '" + user_name + "');"; 
		int st = sqlite3_exec(db, find_board.c_str(), DB_check_enter, 0, 0);
		
		if (st != SQLITE_OK) {
			// already used
			string wan = "Board already exist.\n";
			send(sockfd, wan.c_str(), wan.size(), 0);
		}
		else {
			string wan = "Create board successfully.\n";
			send(sockfd, wan.c_str(), wan.size(), 0);
		}
	}
	else {
		send(sockfd, not_login.c_str(), not_login.size(), 0);
	}

}

void create_post(int sockfd, vector<string> &argu) {
	
	string board_name, title, content;
	bool format = false;
	int content_idx = 1e9;
	bool title_finish = false;
	for(int i=1; i<argu.size(); i++) {
		if (i==1) { 
			board_name = argu[i]; continue;
		}

		if (i==2 && (argu[i] != "--title")) {
			send(sockfd, create_post_format.c_str(), create_post_format.size(), 0);
			return;
		}
		
		if (argu[i] == "--content") {
			format = true; 
			content_idx = i;
			title_finish = true;
			continue;
		}
		// collect title
		if (i > 2 && !title_finish) {
			if (i > 3)
				title += ' ';
			title += argu[i];
		}

		// collect content
		if (i > content_idx) {
			if (i > content_idx+1)
				content += ' ';

			// check <br>
			size_t pos = 0;
			while(1) {
				pos = argu[i].find("<br>", pos);
				if (pos != string::npos) {
					argu[i].replace(pos, 4, "\n");
				}
				else	
					break;
			}
			content += argu[i];
		}
	}
	if (!format){
		send(sockfd, create_post_format.c_str(), create_post_format.size(), 0);
		return;
	}
	string user_name;
	// check login
	if (online.find(sockfd) != online.end()) {
		user_name = online[sockfd];
	}
	else {
		send(sockfd, not_login.c_str(), not_login.size(), 0);
		return;
	}
	// deal with time
	auto now = chrono::system_clock::now();
	auto in_time_t = chrono::system_clock::to_time_t(now);

	stringstream ss;
	string date_;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
	ss >> date_;

	string find_board = "SELECT * FROM BOARDS WHERE NAME='" + board_name + "';";
	bool enter = false;
	sqlite3_exec(db, find_board.c_str(), DB_check_enter, (void*)(&enter), 0);
	if ( !enter ) {
		// can't find
		string wan = "Board does not exist.\n";
		send(sockfd, wan.c_str(), wan.size(), 0);
	}
	else {
		string add_post = "INSERT INTO POSTS (BOARD_NAME, TITLE, AUTHOR, DATE, CONTENT, COMMENT)"\
						  "VALUES ('" + board_name + "', '" + title + "', '" + user_name + "', '" + date_ + "', '" + content + "', '');";
		sqlite3_exec(db, add_post.c_str(), DB_check_enter, 0, 0);
		string wan = "Create post successfully.\n";
		send(sockfd, wan.c_str(), wan.size(), 0);
	}
}

// don't have to login 
void list_board(int sockfd, vector<string> &argu) {
	// Question: ##' ', how to deal with? 
	string key;
	if (argu.size() > 2){
		send(sockfd, list_board_format.c_str(), list_board_format.size(), 0);
		return;
	}

	if (argu.size() == 2) {
		if (argu[1].substr(0, 2) == "##") {
			key = argu[1].substr(2);
		}
		else {
			send(sockfd, list_board_format.c_str(), list_board_format.size(), 0);
			return;
		}
	}
	// TODO: list board
	if( key.empty() ) {
		string list_all_boards = "SELECT * FROM BOARDS;";
		pair<int, bool> fd_enter = {sockfd, false};
		string indexes = "Index     Name      Moderator \n";
		send(sockfd, indexes.c_str(), indexes.size(), 0);
		sqlite3_exec(db, list_all_boards.c_str(), DB_list_data, (void*)&fd_enter, 0);
	}
	else {
		// advanced search
		string list_key_boards = "SELECT * FROM BOARDS WHERE NAME LIKE '%" + key + "%';";
		pair<int, bool> fd_enter = {sockfd, false};
		string indexes = "ID        Title     Author    Date      \n";
		send(sockfd, indexes.c_str(), indexes.size(), 0);
		sqlite3_exec(db, list_key_boards.c_str(), DB_list_data, (void*)&fd_enter, 0);
	}

}

// don't have to login 
void list_post(int sockfd, vector<string> &argu) {
	string key;
	string board_name;

	if (argu.size() > 3) {
		send(sockfd, list_board_format.c_str(), list_board_format.size(), 0);
		return;
	}
	
	if (argu.size() == 3) {
		if(argu[2].substr(0, 2) == "##") {
			key = argu[2].substr(2);
		}
		else {
			send(sockfd, list_board_format.c_str(), list_board_format.size(), 0);	
			return;
		}
	}
	board_name = argu[1];

	pair<int, bool> fd_enter = {sockfd, false};

	// Question: key have space?
	if (key.empty()) {
		string list_key_posts = "SELECT POST_ID, TITLE, AUTHOR, DATE FROM POSTS WHERE BOARD_NAME='" + board_name + "';";

		sqlite3_exec(db, list_key_posts.c_str(), DB_list_data, (void*)&fd_enter, 0);
	}
	else {
		string list_key_posts = "SELECT POST_ID, TITLE, AUTHOR, DATE FROM POSTS WHERE BOARD_NAME='" + board_name + "' AND TITLE LIKE '%" + key + "%';";

		sqlite3_exec(db, list_key_posts.c_str(), DB_list_data, (void*)&fd_enter, 0);
	}
	if(fd_enter.second == false) {
		string no_board = "Board does not exist.\n";
		send(sockfd, no_board.c_str(), no_board.size(), 0);
	}
}

// don't have to login 
void read_(int sockfd, vector<string> &argu) {
	if( argu.size() != 2){
		send(sockfd, read_format.c_str(), read_format.size(), 0);
		return;
	}
	string post_id = argu[1];
	string read_post = "SELECT AUTHOR, TITLE, DATE, CONTENT, COMMENT FROM POSTS WHERE POST_ID=" + post_id + ";";
	pair<int, bool> fd_enter = {sockfd, false};
	
	sqlite3_exec(db, read_post.c_str(), DB_read_posts, (void*)&fd_enter, 0);
	if (fd_enter.second == false) {
		string no_posts = "Post does not exist.\n";
		send(sockfd, no_posts.c_str(), no_posts.size(), 0);
	}
}

void delete_post(int sockfd, vector<string> &argu) {
	if (argu.size() != 2) {
		send(sockfd, delete_post_format.c_str(), delete_post_format.size(), 0);
		return;
	}
	string post_id = argu[1];
	string user_name;
	if (online.find(sockfd) != online.end()) {
		user_name = online[sockfd];
	}
	else {
		send(sockfd, not_login.c_str(), not_login.size(), 0);
	}
	
	string check_autho = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + ";";
	bool enter = false;
	sqlite3_exec(db, check_autho.c_str(), DB_check_enter, (void*)&enter, 0);

	if(enter) {
		string check_post = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + " AND AUTHOR='" + user_name + "';";
		bool enter2 = false;
		sqlite3_exec(db, check_post.c_str(), DB_check_enter, (void*)&enter2, 0);
		if (!enter2) {
			string no_autho = "Not the post owner\n";
			send(sockfd, no_autho.c_str(), no_autho.size(), 0);
		}
		else {
			string del_post = "DELETE FROM POSTS WHERE POST_ID=" + post_id + " AND AUTHOR='" + user_name + "';";
			sqlite3_exec(db, del_post.c_str(), DB_check_enter, 0, 0);

			string succ = "Delete successfully\n";
			send(sockfd, succ.c_str(), succ.size(), 0);
		}		
	}
	else {
		string not_enter = "Post does not exist.\n";
		send(sockfd, not_enter.c_str(), not_enter.size(), 0);
	}



}

void update_post(int sockfd, vector<string> &argu) {
	//Question: can <new> be space?
	string item;
	if (argu.size() < 4) {
		send(sockfd, update_post_format.c_str(), update_post_format.size(), 0);
		return;
	}
	else{
		item = argu[2];
		if(item != "--title" && item != "--content") {
			send(sockfd, update_post_format.c_str(), update_post_format.size(), 0);
			return;
		}
		else {
			if(item == "--title") item = "TITLE";
			else if(item == "--content") item= "CONTENT";
		}
	}
	string post_id = argu[1];
	string new_content;
	// may have <br>
	for(int i=3; i<argu.size(); i++) {
		if (i>3)
			new_content += ' ';
		// check <br>
		size_t pos = 0;
		while(1) {
			pos = argu[i].find("<br>", pos);
			if (pos != string::npos) {
				argu[i].replace(pos, 4, "\n");
			}
			else	
				break;
		}
		new_content += argu[i];
	}

	string user_name;
	if (online.find(sockfd) != online.end()) {
		user_name = online[sockfd];
	}
	else {
		send(sockfd, not_login.c_str(), not_login.size(), 0);
		return;
	}
	
	string check_autho = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + ";";
	bool enter_inner = false;
	sqlite3_exec(db, check_autho.c_str(), DB_check_enter, (void*)&enter_inner, 0);

	if(enter_inner) {
		string check_upd = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + " AND AUTHOR='" + user_name + "';";
		bool enter3 = false;
		sqlite3_exec(db, check_upd.c_str(), DB_check_enter, (void*)&enter3, 0);
		if (enter3) {
			string upd_post = "UPDATE POSTS SET "+ item + " = '" + new_content + "' WHERE POST_ID=" + post_id + " AND AUTHOR='" + user_name + "';";
			sqlite3_exec(db, upd_post.c_str(), DB_check_enter, 0, 0);

			string succ = "Update successfully\n";
			send(sockfd, succ.c_str(), succ.size(), 0);
		}
		else {
			string no_autho = "Not the post owner\n";
			send(sockfd, no_autho.c_str(), no_autho.size(), 0);
		}
	}
	else {
		string not_enter = "Post does not exist.\n";
		send(sockfd, not_enter.c_str(), not_enter.size(), 0);
	}

}
// comment have no <br>
// Question: not print "index"
void comment(int sockfd, vector<string> &argu) {
	if (argu.size() < 3) {
		send(sockfd, comment_format.c_str(), comment_format.size(), 0);
		return;
	}
	string user_name;

	if (online.find(sockfd) != online.end()) {
		user_name = online[sockfd];
	}
	else {
		send(sockfd, not_login.c_str(), not_login.size(), 0);
		return;
	}
	string post_id = argu[1];
	string check_exist = "SELECT * FROM POSTS WHERE POST_ID=" + post_id + ";";
	bool enter = false;
	sqlite3_exec(db, check_exist.c_str(), DB_check_enter, (void*)&enter, 0);
	if (!enter) {
		string not_exist = "Post does not exist.\n";
		send(sockfd, not_exist.c_str(), not_exist.size(), 0);
		return;
	}

	//Question comment have many space ex: hi!      hi!
	string comment;

	for(int i=2; i<argu.size(); i++) {
		if (i > 2)
			comment += ' ';
		comment += argu[i];
	}
	comment = user_name + ": " + comment + "\n";
	// TODO: ADD new comment 
	string add_new_comment = "UPDATE POSTS SET COMMENT = COMMENT || '" + comment + "' WHERE POST_ID=" + post_id +";";
	sqlite3_exec(db, add_new_comment.c_str(), DB_check_enter, 0, 0);

	string succ_upd = "Comment successfully\n";
	send(sockfd, succ_upd.c_str(), succ_upd.size(), 0);
}

int main(int argc, char *argv[]) {
	
	int portnum;
	if (argc != 2) {
		cerr << "Wrong format, should be "<< argv[0] <<" [port number]\n";
		exit(0);
	}
	else {
		portnum = atoi(argv[1]);
	}

	int st;
	// Create Database in memory. (temporary)
	st = sqlite3_open(":memory:", &db);
	
	if (st) {
		printf("Can't open database \n");
	}
	string user_table = "CREATE TABLE USERS (" \
						"UID INTEGER PRIMARY KEY AUTOINCREMENT,"\
						"USERNAME TEXT NOT NULL UNIQUE,"  \
						"EMAIL TEXT NOT NULL," \
						"PASSWORD TEXT NOT NULL);";

	string board_table = "CREATE TABLE BOARDS ("\
						 "BOARD_ID INTEGER PRIMARY KEY AUTOINCREMENT,"
						 "NAME TEXT UNIQUE NOT NULL,"
						 "MODERATOR TEXT NOT NULL);";

	string posts_table = "CREATE TABLE POSTS ("\
						 "POST_ID INTEGER PRIMARY KEY AUTOINCREMENT, "\
						 "BOARD_NAME TEXT NOT NULL,"\
						 "TITLE TEXT NOT NULL,"\
						 "AUTHOR TEXT NOT NULL,"\
						 "DATE NOT NULL,"\
						 "CONTENT TEXT NOT NULL,"\
						 "COMMENT TEXT);";
	// Create Table
	st = sqlite3_exec(db, user_table.c_str(), DB_check_enter, 0, 0);
	st = sqlite3_exec(db, board_table.c_str(), DB_check_enter, 0, 0);
	st = sqlite3_exec(db, posts_table.c_str(), DB_check_enter, 0, 0);

	if (st!=SQLITE_OK) {
		printf("CREATE TABLE ERROR\n");
	}else {
		printf("success create\n");
	}	

	// connect socket
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		cerr << "error opening socket\n"; 
	}

	int on = 1;
	setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	//set IP address
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portnum);

	if ( bind (sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0){ 
		cerr << "Error on binding. \n";
		close(sockfd);
		exit(0);
	}
	
	listen(sockfd, 5);
	int lis_st, n;

	fd_set master, read_fds;
	int fdmax = sockfd;
	FD_SET(sockfd, &master);

	while(true) {
		read_fds = master;

		int select_st = select(fdmax+1, &read_fds, NULL, NULL, NULL);
		if (select_st == -1) {
			cerr << "select error\n";
			exit(1);
		}
		// 0, 1, 2 for input, output, error stream
		for(int i=3; i<=fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				// server has something new -> new connection
				if (i==sockfd) {
					sockaddr_in cli_addr;
					socklen_t cli_addr_len = sizeof(cli_addr);
					int new_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_addr_len);
					if(new_fd == -1) {
						cerr << "accept error";
						exit(1);
					}
					// add new connection fd into fd sets
					FD_SET(new_fd, &master);
					if (new_fd > fdmax) {
						fdmax = new_fd;
					}
					cout << "New connection\n";
					send(new_fd, welcome_buf.c_str(), welcome_buf.size(), 0);
					send(new_fd, "% ", 2, 0);
				}
				// clients has something new -> new input from client
				else {
					char buf[256];
					memset(buf, 0, sizeof(buf));
					int recv_st = recv(i, buf, 255, 0);
					
					if (recv_st <= 0) {
						if (recv_st == 0) 
							cerr << "one client lose connection.\n";			
						else 
							cerr << "recv error\n";
						
						close(i);
						FD_CLR(i, &master);
					}
					else {
						vector<string> argu;
						string buffer = string(buf), tmps;
						stringstream ss(buffer);
						while(ss >> tmps) {
							argu.push_back(tmps);
						}
						if (argu.empty()) {
							send(i, "% ", 2, 0);
							continue;
						}

						if ( argu[0] == "register") {
							register_(i, argu);
						}
						else if( argu[0] == "login") {
							login(i, argu);
						}
						else if( argu[0] == "logout") {
							logout(i, argu);
						}
						else if( argu[0] == "whoami") {
							whoami(i, argu);
						}
						else if( argu[0] == "exit") {
							if (argu.size() != 1) {
								string tmp = "exit\n";
								send(i, tmp.c_str(), tmp.size(), 0);
							}
							else{
								close(i);
								FD_CLR(i, &master);
								cerr << "one client lose connection.\n";
							}
						}
						// homework2 add new function
						else if( argu[0] == "create-board") {
							
							create_board(i, argu);
						}
						else if( argu[0] == "create-post") {
							create_post(i, argu);
						}
						else if( argu[0] == "list-board") {
							list_board(i, argu);
						}
						else if( argu[0] == "list-post") {
							list_post(i, argu);
						}
						else if( argu[0] == "read") {
							read_(i, argu);
						}
						else if( argu[0] == "delete-post") {
							delete_post(i, argu);
						}
						else if( argu[0] == "update-post") {
							update_post(i, argu);
						}
						else if( argu[0] == "comment") {
							comment(i, argu);
						}
						send(i, "% ", 2, 0);
					}
				}
			}
		}
	}
	close(sockfd);

	return 0;
}

