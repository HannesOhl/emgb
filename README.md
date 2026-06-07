Work in progress...

### Overview ###
A small Gameboy emulator for Linux written in C. Currently implements almost all CPU instructions and is able to run Tetris. A console-backend, as  well as a SDL2-backend is provided. 

simply run for example
```
./xemgb path-to-tetris.gb
```



### Controls ###
- d: right
- a: left
- w: up
- s: down
- k: A
- j: B
- m: start
- c: select
- q: quit

### The Terminal Backend ###

Currently tested for gnome-terminal and simple terminal (st). 

xterm is currently not supported.

### Notes ###
Currently no ROM-banking is supported.

Currently no audio is supported. 
