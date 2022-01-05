#!/bin/bash
#Usage:
#1.) Start the gunrock_web server in a separate window with your desired parameters. (try: ./gunrock_web -p 8080 -t 3 -b 3 -l log.txt)
#2.) Double check that this file is in the same directory as your gunrock.cpp file and its permissions are set to execute.
#3.) chmod 744 tests.sh (if you didn't know how to do #2).
#4.) ./tests.sh
#5.) If you started the web server and specified a log file, open up the file to see how your threads are behaving!

echo Starting test cases for gunrock_web...

echo ------[TEST SET 1]------
for ((i = 0; i < 10; i++)) ;
	do
	  curl http://localhost:8080/hello_world.html
    	  curl http://localhost:8080/bootstrap.html
	done
echo ----[END TEST SET 1]----

echo Requests for gunrock_web serviced.
