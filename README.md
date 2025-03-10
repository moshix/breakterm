[![Hits](https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2Fmoshix%2Fbreakterm&count_bg=%2379C83D&title_bg=%23E0D8D8&icon=epicgames.svg&icon_color=%23E65656&title=hits&edge_flat=false)](https://hits.seeyoufarm.com)

BREAK OUT for the terminal (also in ssh, mosh or telnet)
========================================================

This is an ad-hoc implementation of the famous breakout game for the *NIX terminal, in 400 lines of C, or so.  I wrote it so it would work also on z/OS USS. but it runs on every type of *NIX, such as Linux, AIX, Macos, NetBSD, FreeBSD, Solaris, HP-UX, and Android. 

z/OS USS sports subtle differences from other, 'normal' *NIXES that make terminal stuff more challenging. This works on z/OS USS. 

To compile the program, make sure you have a c compiler, libncurses and libncurses-dev installed:
<pre>
sudo apt install libncurses5 libncurses5-dev # on Debian distros  
sudo yum install ncurses ncurses-devel       # on RHEL distros  
</pre> 

Then just type **make**. It should compile without warnings, or errors, Gd forbid. 

Game Play
=========
  
P for Pause  
B for Boss screen  
Q for Quit    
Left cursor for left  
Right cursor for right  

You can also invoke the program and pass it how many starting lives you want (with -l 5 for 5 lives), and with -a 20% (for ball acceleration percent at each new game level), like this:
<pre>
./breakterm -l 5 -a 20
</pre>
  
You start by default with 4 lives and at level 1. Each time you finish a level, the ball speeds up by 15%! Can you survive the next level?   
  
  
**Have fun!**

Moshix  
March 2025 - Barolo  

![Actual game screenshot ](https://github.com/moshix/breakterm/blob/main/screenshot2.png)


