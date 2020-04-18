#!/bin/bash
host=localhost\ 7788
{
sleep 1
#printf "create-board NP_HW" >> output.txt
echo create-board NP_HW
sleep 1
echo register Bob bob@qwer.asdf 123456
sleep 1
echo register Sam sam@qwer.com 654321
sleep 1
echo login Bob 123456
sleep 1
echo create-board NP_HW
sleep 1
echo create-board NP_HW
sleep 1
echo create-board OS_HW
sleep 1
echo create-board FF
sleep 1
echo list-board
sleep 1
echo "list-board ##HW"
sleep 1
echo "create-post NCTU --title About NP HW_2 --content Help!<br>I have some problem!"
sleep 1
echo "create-post NP_HW --title About NP HW_2 --content Help!<br>I have some problem!"
sleep 1
echo "create-post NP_HW --title HW_3 --content Ask!<br>Is NP HW_3 Released?"
sleep 1
echo list-post NP
sleep 1
echo list-post NP_HW
sleep 1
echo "list-post NP_HW ##HW_2"
sleep 1
echo read 888
sleep 1
echo read 1
sleep 1
echo update-post 888 --title NP HW_2
sleep 1
echo update-post 1 --title NP HW_2
sleep 1
echo read 1
sleep 1
echo logout
sleep 1
echo login Sam 654321
sleep 1
echo "update-post 1 --content Ha!<br>ha!<br>ha!"
sleep 1
echo delete-post 1
sleep 1
echo comment 888 Ha! ha! ha!
sleep 1
echo comment 1 Ha! ha! ha!
sleep 1
echo read 1
sleep 1
echo create-board Hello
sleep 1
echo list-board
sleep 1
echo logout
sleep 1
echo login Bob 123456
sleep 1
echo delete-post 1
sleep 1
echo read 1
sleep 1
echo logout
sleep 1
echo exit
sleep 1
echo register Bob 3 4
sleep 1
echo exit
} | telnet $host 
