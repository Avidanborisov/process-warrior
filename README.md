process-warrior
===============

A highly effective Linux "Process War" combatant.

What is Process War?
--------------------
Similarly to the old [Core War](http://en.wikipedia.org/wiki/Core_War), "Process War" is a programming game for two or more battle process. Each process's objective is to kill all the enemy processes. The winner is the last running process.

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

That means that at times, the code may favor native system API's instead of C++(11) standard library features and idioms (due to performance). In addition, the code isn't meant to be POSIX compliant, it's supposed to be running on Linux only. This warrior only uses native Linux API's everywhere - it never executes a system command to the job.

The warrior's course of action is divided into three steps: hiding, protecting and infinitely destroying.

###Hiding
The first thing to do is to hide the process from the other processes, making it "invisible" to them. If done correctly, this essentially eliminates the chance of being killed by other processes, since they won't even know your process exists. 

How can other processes know what processes are their competitors? Well, the rules say all processes will be run from `/tmp`, so that means all them will have at least one of the following properties:
* `/proc/<pid>/exe` that links to an executable file in `/tmp`
* `/proc/<pid>/cwd` that links to `/tmp`
* `/proc/<pid>/cmdline` with the first argument being an executable file from `/tmp`

There are no other 'sure' ways of identifying competitor processes. Therefore, the first objective of our warrior is to change all of these properties to something else. The last two properties are changed easily by calling `chdir()` and modifying the process's `argv` (I chose the fake name of `uwotm8` for the process). The first property, which is the path to process's executable image, is changed by making a hardlink to the warrior's executable image somewhere else, and then executing that hardlink, which executes the same program, but now with a different executable image path.

###Protecting
Even after the process is hidden, some basic protection for the process is still provided. We can never know if the process was hidden successfully - a clever warrior could maybe find a way to track us down.

The protection boils down to handling signals sent to the process that normally terminate it. Although a clever warrior would just send a `SIGKILL` which can't be ignored (although here it is, in some circumstances. see next section), it is beneficial to ignore all other signals that can be ignored, such as `SIGQUIT` or `SIGINT`.

But those signals aren't just ignored. In fact, the handlers are designed to kill the signal sender if possible (quite clever ha?).

###Destroying
This is the core of the program. After the process is hidden and protected from various signals, it's job is to actually start and terminate the enemies.

This part essentially infinitely iterates on directory entries in `/proc`, killing all processes which have a `/proc/<pid>/exe` link to a program in `/tmp` (and if possible, not only the matching process is `SIGKILL`ed, but it's entire process group too). We could look at other properties in `/proc` as described before, but those aren't as reliable and generally not worth the time. 

But that's not all. They key strength of this warrior is the fact that it `fork`s itself periodically (with each child getting a different process group, to not associate them together), while running in a distributed fashion. That means that as long as not all processes are killed, they will continue to fork themselves and run without a problem. Even if the original father process is killed, the children will continue to fork to 'complete' the missing process. Yes, that means that as long as not all processes are killed at the same time, this warrior can essentially ignore `SIGKILL`*.

Does the process just fork itself infinitely? No, that would be against the rules ("you shall not crash"). There's a constant value for the maximum number of processes that can run at all times. But how can the processes know how many copies of themselves are currently running?

That problems took me years to find a solution for, till I finally came across a gem in some manpage - the `shm_nattch` property of [POSIX shared memory](http://man7.org/linux/man-pages/man7/shm_overview.7.html) regions. All the processes of our warrior use a shared memory region, and the `shm_nattch` property of that region specifies the current number of processes running at any time (it's updated by the kernel automatically with each process's termination). Therefore, with each periodic callback taken place, this property is checked to see whether there's a need to `fork()` another process.

<sub>* One can try to execute a `killall -9 uwotm8` and see whether the program is in fact killed. It can take a couple of `killall` commands (one right after the other) to kill all the processes of the warrior. The exact number of depends on the value specified in the `Config::forkDelayMicrosecs` and `Config::maxProcesses` constants.</sub>
