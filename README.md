process-warrior
===============

A highly effective Linux "Process War" combatant.

What is a Process War?
----------------------
Similarly to the old [Core War](http://en.wikipedia.org/wiki/Core_War), a "Process War" is a programming game for two or more battle process. Each process's objective is to kill all the enemy processes. The winner is the last running process.

The game's environment is any Linux distribution. The rules are simple:
* It's illegal to hang, crash or shut down the system.
* All process must start their logic **only** after `/tmp/GO` is created. If the program is written in C, the first line of the `main()` function has to be `while (access("/tmp/GO", F_OK)) { }`.
* All processes will be put into `/tmp` before starting the game, and will be executed from there.

Besides that, all processes can do **absolutely anything**, and can be written in **any** language.

What does this warrior do?
--------------------------
This process warrior is written in C++11, and is designed to be:

1. As fast as possible
2. Compatible on Linux only

That means that at times, the code may not follow idiomatic C++(11) guidelines (which is only where performance matters), or POSIX portability. This warrior only uses native Linux API's everywhere - it never executes a system command to the job.

The warrior's course of action is divided into three steps: hiding, protecting and infinitely destroying.

###Hiding
###Protecting
###Destroying
