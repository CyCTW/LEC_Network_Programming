
#include "user_oper.h"
#include "post_oper.h"
#include "mail_oper.h"

int main(int argc, char *argv[]) {
	
	int portnum;
	if (argc != 2) {
		cerr << "Wrong format, should be "<< argv[0] <<" [port number]\n";
		exit(0);
	}
	else {
		portnum = atoi(argv[1]);
	}

	struct stat bufr;
	bool exist = true;
	if (stat("mydbf.db", &bufr) == -1) {
		exist = false;
	}
	// Create Database in memory. (temporary)
	int st;
	st = sqlite3_open(":memory:", &db);
	if (st) {
		printf("Can't open database \n");
	}

	if (!exist)	{
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
							"DATE TEXT NOT NULL);";
		string mail_table = "CREATE TABLE MAILS ("\
							"TO_USER TEXT NOT NULL,"\
							"MAILID INTEGER NOT NULL,"\
							"SUBJECT TEXT NOT NULL, "\
							"FROM_USER TEXT NOT NULL, "\
							"DATE TEXT NOT NULL);";
		// Create Table
		st = sqlite3_exec(db, user_table.c_str(), DB_check_enter, 0, 0);
		st = sqlite3_exec(db, board_table.c_str(), DB_check_enter, 0, 0);
		st = sqlite3_exec(db, posts_table.c_str(), DB_check_enter, 0, 0);
		st = sqlite3_exec(db, mail_table.c_str(), 0, 0, 0);

		// set case-sensitive
		string sensitive = "PRAGMA case_sensitive_like = true;";
		sqlite3_exec(db, sensitive.c_str(), 0, 0, 0);

		if (st!=SQLITE_OK) {
			cout << "CREATE TABLE ERROR\n";
		}else {
			cout << "success create\n";
		}	
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
	addr.sin_addr.s_addr = inet_addr("140.113.123.236");
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portnum);

	if ( bind (sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0){ 
		cerr << "Error on binding. \n";
		close(sockfd);
		exit(0);
	}
	
	listen(sockfd, 5);

	fd_set master, read_fds;
	int fdmax = sockfd;
	FD_SET(sockfd, &master);
	FD_SET(0, &master);
	bool close_server = false;

	while(!close_server) {
		read_fds = master;
		int select_st = select(fdmax+1, &read_fds, NULL, NULL, NULL);
		if (select_st == -1) {
			cerr << "select error\n";
			exit(1);
		}
		// 0, 1, 2 for input, output, error stream
		for(int i=0; i<=fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				// server has something new -> new connection
				if (i==0) {
					close_server=true;
					cerr << "Server close...\n";
					break;
				}
				else if (i==sockfd) {
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
					// send(new_fd, "% ", 2, 0);
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
						cout << "User input: " << buffer << '\n';
						stringstream ss(buffer);
						while(ss >> tmps) {
							argu.push_back(tmps);
						}
						string ret;

						if (argu.empty()) {
							send(i, "% ", 2, 0);
							// ret += "% ";
							continue;
						}

						if ( argu[0] == "register") {
							ret += register_(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
							char r_buf[256];
							memset(r_buf, 0, sizeof(r_buf));
							if (ret[0] == 'R') {
								recv(i, r_buf, 255, 0);
								// store metadata
								user_bucket_tb[ argu[1] ]= string(r_buf);
								string tmp = "#";
								send(i, tmp.c_str(), tmp.size(), 0);
							}

						}
						else if( argu[0] == "login") {
							ret += login(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);

						}
						else if( argu[0] == "logout") {
							ret += logout(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
						}
						else if( argu[0] == "whoami") {
							ret += whoami(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
						}
						else if( argu[0] == "exit") {
							if (argu.size() != 1) {
								string tmp = "exit\n";
								ret += tmp;
								ret += "% ";
								send(i, ret.c_str(), ret.size(), 0);
								// send(i, tmp.c_str(), tmp.size(), 0);
							}
							else{
								close(i);
								FD_CLR(i, &master);
								cerr << "one client lose connection.\n";
							}
						}
						// homework2 add new function
						else if( argu[0] == "create-board") {
							ret += create_board(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
						}
						else if( argu[0] == "create-post") {
							ret += create_post(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
							char c_buf[256];
							if (ret[0] == 'C') {
								recv(i, c_buf, 255, 0);
								// store metadata
								string user_name = online[i];
								string bucket_name = user_bucket_tb[ user_name ];
								string post_id = to_string(POST_ID-1);
								cout << "bucket_name: " << bucket_name << '\n';
								cout << "PostId: " << POST_ID-1 << '\n';
								// stringstream ss;
								// ss << (POST_ID-1); ss >> post_id;
								string msg = bucket_name + ' ' + post_id; 
								send(i, msg.c_str(), msg.size(), 0);

							}
						}
						else if( argu[0] == "list-board") {
							ret += list_board(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
						}
						else if( argu[0] == "list-post") {
							ret += list_post(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
						}
						else if( argu[0] == "read") {
							ret += read_(i, argu);
							// ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
							char c_buf[256];

							if (ret[0] == 'A') {
								recv(i, c_buf, 255, 0);
								// give bucket_name
								string user_name = postid_name[atoi(argu[1].c_str())];
								string bucket_name = user_bucket_tb[ user_name ];
								cout << "bucket_name: " << bucket_name << '\n';
								send(i, bucket_name.c_str(), bucket_name.size(), 0);
							}
						}
						else if( argu[0] == "delete-post") {
							ret += delete_post(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
							char c_buf[256];
							memset(c_buf, 0, sizeof(c_buf));
							if (ret[0] == 'D') {
								recv(i, c_buf, 255, 0);
								// give bucket_name
								string user_name = online[i];
								string bucket_name = user_bucket_tb[ user_name ];
								cout << "bucket_name: " << bucket_name << '\n';
								send(i, bucket_name.c_str(), bucket_name.size(), 0);

							}
						}
						else if( argu[0] == "update-post") {
							ret += update_post(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);

							char c_buf[256];
							memset(c_buf, 0, sizeof(c_buf));
							if (ret[0] == 'U' && argu[2]=="--content") {
								recv(i, c_buf, 255, 0);
								// give bucket_name
								string user_name = online[i];
								string bucket_name = user_bucket_tb[ user_name ];
								cout << "bucket_name: " << bucket_name << '\n';
								send(i, bucket_name.c_str(), bucket_name.size(), 0);

							}
						}
						else if( argu[0] == "comment") {
							ret += comment(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);

							char c_buf[256];
							memset(c_buf, 0, sizeof(c_buf));
							if (ret[0] == 'C') {
								recv(i, c_buf, 255, 0);
								// give bucket_name
								string user_name = online[i];
								string postuser_name = postid_name[atoi(argu[1].c_str())];
								string bucket_name = user_bucket_tb[ postuser_name ];
								cout << "bucket_name: " << bucket_name << '\n';
								string msg = user_name + ' ' + bucket_name;
								send(i, msg.c_str(), msg.size(), 0);

							}
						}
						else if( argu[0] == "mail-to") {
							ret += mail_to(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
							char c_buf[256];
							memset(c_buf, 0, sizeof(c_buf));
							if (ret.substr(0, 4) == "Sent") {
								recv(i, c_buf, 255, 0);
								// give bucket_name of specific user

								string user_name = argu[1]; // receiver's name
								string bucket_name = user_bucket_tb[ user_name ];
								int mail_id = user_mailid[user_name];
								cout << "bucket_name: " << bucket_name << '\n';
								cout << "recv user: " << to_string(mail_id) << '\n';
								string msg = bucket_name + ' ' + to_string(mail_id);
								send(i, msg.c_str(), msg.size(), 0);
							}
						}

						else if( argu[0] == "list-mail") {
							ret += list_mail(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);

						}
						else if( argu[0] == "retr-mail") {
							ret += retr_mail(i, argu);
							// ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
							char c_buf[256];
							memset(c_buf, 0, sizeof(c_buf));
							if (ret.substr(0, 1) == "S") {
								recv(i, c_buf, 255, 0);
								// give bucket_name of specific user
								string user_name = online[i];
								string bucket_name = user_bucket_tb[ user_name ];
								cout << "bucket_name: " << bucket_name << '\n';
								send(i, bucket_name.c_str(), bucket_name.size(), 0);
							}
						}
						else if( argu[0] == "delete-mail") {
							ret += delete_mail(i, argu);
							ret += "% ";
							send(i, ret.c_str(), ret.size(), 0);
							char c_buf[256];
							memset(c_buf, 0, sizeof(c_buf));
							if (ret.substr(0, 1) == "M") {
								recv(i, c_buf, 255, 0);
								// give bucket_name of specific user
								string user_name = online[i];
								string bucket_name = user_bucket_tb[ user_name ];
								cout << "bucket_name: " << bucket_name << '\n';
								send(i, bucket_name.c_str(), bucket_name.size(), 0);
							}
						}
						else {
 							string tmp = "% ";
							send(i, tmp.c_str(), tmp.size(), 0);
						}
						// cout << "Return value: " << ret << '\n';
						
					}
				}
			}
		}
	}
	close(sockfd);
	sqlite3_close(db);

	return 0;
}

