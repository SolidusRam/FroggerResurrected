Base progetto SO

Il progetto consiste nel realizzare il gioco frogger in C utilizzando la libreria ncurses.
il gioco consiste nel far attraversare una strada ad un rospo evitando che questo affoghi in un fiume.
il gioco ha 2 versioni: una utilizza i threads e una i processi. Verrà implementata una pipe per la comunicazione tra i processi.

Ci sono diversi oggetti, chiascuno è rappresentato da un processo:
Il giocatore(Rana) che si muove nelle 4 direzioni
I coccodrilli(tronchi) che si muovono in orizzontale, minimo 8 corsie
I proiettili, in questa versione i coccodrilli sparano proiettili, la rana ha dei proiettili a disposizione per distruggerli
Il gestore delle stampe e del tempo

FroggerResurrected/
├── src/
│   ├── main.c          # Punto di ingresso del programma
│   ├── game.c          # Logica generale del gioco
│   ├── player.c        # Gestione della rana (giocatore)
│   ├── crocodile.c     # Gestione dei coccodrilli
│   ├── projectiles.c   # Gestione dei proiettili
│   └── utils.c         # Funzioni di utilità
├── include/
│   ├── game.h          # Dichiarazioni delle funzioni e strutture del gioco
│   ├── player.h        # Dichiarazioni delle funzioni e strutture del giocatore
│   ├── crocodile.h     # Dichiarazioni delle funzioni e strutture dei coccodrilli
│   ├── projectiles.h   # Dichiarazioni delle funzioni e strutture dei proiettili
│   └── utils.h         # Dichiarazioni delle funzioni di utilità
├── assets/             # Eventuali risorse grafiche o audio
├── Makefile            # File per automatizzare la compilazione
└── README.md           # Documentazione del progetto
