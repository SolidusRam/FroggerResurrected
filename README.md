Base progetto SO

Il progetto consiste nel realizzare il gioco frogger in C utilizzando la libreria ncurses.
il gioco consiste nel far attraversare una strada ad un rospo evitando che questo affoghi in un fiume.
il gioco ha 2 versioni: una utilizza i threads e una i processi. Verrà implementata una pipe per la comunicazione tra i processi.

Ci sono diversi oggetti, chiascuno è rappresentato da un processo:
Il giocatore(Rana) che si muove nelle 4 direzioni
I coccodrilli(tronchi) che si muovono in orizzontale, minimo 8 corsie
I proiettili, in questa versione i coccodrilli sparano proiettili, la rana ha dei proiettili a disposizione per distruggerli
Il gestore delle stampe e del tempo

FroggerResurrected/ <br>
├── src/<br>
│   ├── main.c # Punto di ingresso del programma <br>
│   ├── game.c # Logica generale del gioco <br>
│   ├── player.c # Gestione della rana (giocatore) <br>
│   ├── crocodile.c # Gestione dei coccodrilli <br>
│   ├── projectiles.c # Gestione dei proiettili <br>
│   └── utils.c # Funzioni di utilità <br>
├── include/ <br>
│   ├── game.h # Dichiarazioni delle funzioni e strutture del gioco <br>
│   ├── player.h # Dichiarazioni delle funzioni e strutture del giocatore <br>
│   ├── crocodile.h # Dichiarazioni delle funzioni e strutture dei coccodrilli <br>
│   ├── projectiles.h # Dichiarazioni delle funzioni e strutture dei proiettili <br>
│   └── utils.h # Dichiarazioni delle funzioni di utilità <br>
├── assets/ # Eventuali risorse grafiche o audio <br>
├── Makefile # File per automatizzare la compilazione <br>
└── README.md # Documentazione del progetto<br>


ESPANSIONE MULTIPLAYER
- Shared: estrai state e costanti dal singleplayer in un modulo senza ncurses.
- Server: loop headless che gira e stampa snapshot su stdout (per debug).
- Client: riceve snapshot e renderizza (niente input all’inizio → spettatore).
- Input: invia intenti e controlla un player sul server.
- Secondo client: gestisci 2 player.
- Interpolazione leggera e heartbeat.
- Pulizia: riconnessione, game over, scoreboard.