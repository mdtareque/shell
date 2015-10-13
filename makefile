miniShell: miniShell.c log.c hist.c builtin.c util.c shellinit.c
	     gcc -o miniShell miniShell.c log.c hist.c builtin.c  util.c shellinit.c -I.

