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

// Database callback function
static int DB_check_enter(void *enter, int argc, char **argv, char **colname) {
	*(bool*)enter = true; 

	return 0;
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
	// Create Table
	st = sqlite3_exec(db, user_table.c_str(), DB_check_enter, 0, 0);

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
						send(i, "% ", 2, 0);
					}
				}
			}
		}
	}
	close(sockfd);

	return 0;
}

