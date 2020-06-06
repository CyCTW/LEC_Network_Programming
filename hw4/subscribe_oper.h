#include <vector>
#include "source.h"
using namespace std;

std::unordered_map<string, std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string> > > > sub_list(20);

void sub_init(string username) {
    sub_list[username] = {};
    sub_list[username]["Author"] = {};
    sub_list[username]["Board"] = {};

}
string subscribe_(int sockfd, vector<string> &argu) {
    string ret;
    if (argu.size() != 5) {
        return subscribe_format;
    }
    else if (argu[1] == "--board" && argu[3] != "--keyword") {
        return subscribe_format;
    }
    else if (argu[1] == "--author" && argu[3] != "--keyword") {
        return subscribe_format;
    }
    else if (argu[1] != "--board" || argu[1] != "--author") {
        return subscribe_format;
    }
    string user_name;
    if ( !check_online(sockfd, user_name, ret) ) {
        return ret;
    }

    if (argu[1] == "--board") {
        string board_name = argu[2];
        string keyword = argu[4];
        
        if ( sub_list[user_name]["Board"].count(board_name) ) {
            sub_list[user_name]["Board"][board_name] = {};
            // kafka_sub( board_name );
        }
        vector<string> &tmp = sub_list[user_name]["Board"][board_name];
        for(auto &v : tmp) {
            if (v == keyword) {
                return already_sub;
            }
        }
        tmp.push_back(keyword);

    }
    else if(argu[1] == "--author") {
        string author = argu[2];
        string keyword = argu[4];
        
        if (!sub_list[user_name]["Author"].count(author) ) {
            sub_list[user_name]["Author"][author] = {};
            // kafka_sub( author );

        }
        
        vector<string> &tmp = sub_list[user_name]["Author"][author];
        for(auto &v : tmp) {
            if (v == keyword) {
                return already_sub;
            }
        }
        tmp.push_back(keyword);
    }
    else {
        return subscribe_format;
    }
    return sub_succ;
}

string unsubscribe_(int sockfd, vector<string> &argu) {
    string ret;
    string user_name;
    string keyname = argu[2];
    if (argu.size() != 3) {
        return subscribe_format;
    }
    else if (argu[1] != "--board" || argu[1] != "--author") {
        return subscribe_format;
    }
    if (!check_online(sockfd, user_name, ret)) {
        return ret;
    }

    bool succ = false;
    if (argu[1] == "--board") {
        for(auto & name : sub_list[user_name]["Board"]) {
            if (keyname == name.first) {
                sub_list[user_name]["Board"].erase(keyname);
                succ = true;
                break;
            }
        }
    }
    else if (argu[1] == "--author") {
        for(auto & name : sub_list[user_name]["Author"]) {
            if (keyname == name.first) {
                sub_list[user_name]["Author"].erase(keyname);
                succ = true;
            }
        }
    }

    if (!succ) {
        return non_subscirbe_format;
    }
    else {
        return unsub_succ;
    }
}

string list_sub(int sockfd, vector<string> &argu) {
    string user_name, ret;
    if (!check_online(sockfd, user_name, ret)) {
        return ret;
    }

    for(auto &sub : sub_list[user_name]) {
        if (sub.second.empty())
            continue;
        // cout << sub.first << ": ";
        ret += (sub.first + ": ");
        for(auto &name: sub.second) {
            // cout << name.first << ": ";
            ret += (name.first + ": ");
            for(auto &v : name.second) {
                // cout << v << ' ';
                ret += (v + ' ');
            }
            // cout << "; ";
            ret += "; ";
        }
        ret += '\n';
        // cout << '\n';
    }
    return ret;
}