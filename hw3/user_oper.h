#include "source.h"

// funciton: register, login, logout, whoami;

string register_(int sockfd, vector<string> &argu) {
	string ret;
	if(argu.size() != 4) {
		ret += register_format;
		// send(sockfd, register_format.c_str(), register_format.size(), 0);
		return ret;
	}

	string user = argu[1], email = argu[2], password = argu[3];
	string cmd = "INSERT INTO USERS (USERNAME, EMAIL, PASSWORD) VALUES (\'" + user + "\', " + "\'" + email + "\', " + "\'"+ password + "\');";

	int st = sqlite3_exec(db, cmd.c_str(), DB_check_enter, 0, 0);
	if ( st != SQLITE_OK ) {
		string tmp = "Username is already used.\n";
		ret += tmp;
		// send(sockfd, tmp.c_str(), tmp.size(), 0);
	}
	else {
		string tmp = "Register successfully.\n";
		ret += tmp;
		// send(sockfd, tmp.c_str(), tmp.size(), 0);
	}	
	return ret;	
}

string login(int sockfd, vector<string> &argu) {
	string ret;
	if(argu.size() != 3) {
		ret += login_format;
		// send(sockfd, login_format.c_str(), login_format.size(), 0);
		return ret;
	}

	string usern = argu[1], password = argu[2];
	string cmd = "SELECT * FROM USERS WHERE USERNAME= '" + usern + 
				"' AND PASSWORD= '" + password + "';";
	bool enter = false;
	int st = sqlite3_exec(db, cmd.c_str(), DB_check_enter, (void*)&enter, 0);
    
	if ( online.find(sockfd) != online.end() ) {
		//has already login
		char tmp[] = "Please logout first.\n";
		ret += tmp;
		// send(sockfd, tmp, strlen(tmp), 0);
	}
	else {
		if ( !enter ) {
			// can't find
			string tmp = "Login failed.\n";
			ret += tmp;
			// send(sockfd, tmp.c_str(), tmp.size(), 0);
		}
		else {
			string tmp = "Welcome, " + usern + '\n';
			online[sockfd] = usern;
			ret += tmp;
			// send(sockfd, tmp.c_str(), tmp.size(), 0);
		}
	}
	return ret;
	
}

string logout(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() != 1) {
		string tmp = "logout\n";
		ret += tmp;

		return ret;
	}
    string user_name;
    if ( !check_online(sockfd, user_name, ret) )
        return ret;
	
    string tmp = "Bye, " + online[sockfd] + '\n';
    online.erase(sockfd);
    ret += tmp;
    // send(sockfd, tmp.c_str(), tmp.size(), 0);
	

	return ret;
	
}

string whoami(int sockfd, vector<string> &argu) {
	string ret;
	if (argu.size() != 1) {
		string tmp = "whoami\n";
		ret += tmp;
		// send(sockfd, tmp.c_str(), tmp.size(), 0);
		return ret;
	}
    string user_name;
    if ( !check_online(sockfd, user_name, ret) )
        return ret;
	
    string tmp = online[sockfd] + '\n';
	ret += tmp;
	
	return ret;

}