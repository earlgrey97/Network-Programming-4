# Assignment 4

- Student ID: 20160233
- Your Name: ParkNaHyeon
- Submission date and time: 2020.12.21 around 5pm

## Ethics Oath
I pledge that I have followed all the ethical rules required by this course (e.g., not browsing the code from a disallowed source, not sharing my own code with others, so on) while carrying out this assignment, and that I will not distribute my code to anyone related to this course even after submission of this assignment. I will assume full responsibility if any violation is found later.

Name: ParkNaHyeon
Date: 2020.12.21

## Brief description
Briefly describe here how you implemented the project.
client.cc: sends the file to any super node, receives the translated file, save it with name "translated.txt".

super.cc: receives file from client, parse them and send to another super node. Another super node receives the given part. Then, each super node parses the request to child nodes. When child nodes are all done, a super node who got file from another super node gives file to another super node. A super node who got the file first merges all the translated parts to one file and send the file back to client.

child.cc: receives its own part from super node. parse it and send each to db server. When it gets translated words, merge it to new file. When all translation are done(including DBMISS), it sends the result to super node.

# Misc
Describe here whatever help (if any) you received from others while doing the assignment.
No

How difficult was the assignment? (1-5 scale)
4.5
It's too hard to make asynchronous design. Dealing with multiple threads is so hard....
Mine works well with 1 child per super node, 2 child to super node who gets the file first, and so on on some cases. But it does not work as supernode has more childs. I think it's because of concurrency but I failed to fix them....

How long did you take completing? (in hours)
around 60 hours...(a week)