# RTerm

![example.png](https://github.com/Rainfxxk/rterm/blob/main/img/example.png?raw=true)

A expriment project to learn the machanism under terminal which I use frequently in my study and work. I hope I can make it better enough for my daily use step by step. This is my little goal.

## Build

I just write this in Linux and never test it in other platform.

In Linux, make you have the SDL2 and SDL2_TTF development evironment, then run command:

```sh
$ git clone https://github.com/Rainfxxk/rterm.git
$ make
```

Now, rterm support some color display experimentally, you can run command `export TERM=xterm-256colr` or change the `env` variable in function `pty.c: int spawn(PTY *pty)` to try it, but it still not work well.

## TODO

- let rterm run successfully vim (now we can seemingly start vim currectly, see [img](https://github.com/Rainfxxk/rterm/blob/main/img/vim.png))
- support the [xterm control sequences](https://www.xfree86.org/current/ctlseqs.html)

## Thanks

This project is inspired by 

- [eduterm](https://www.uninformativ.de/git/eduterm)
- [libtmt](https://github.com/hardentoo/libtmt)
