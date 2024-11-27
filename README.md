Base progetto SO

Il progetto consiste nel realizzare il gioco frogger in C utilizzando la libreria ncurses.
il gioco consiste nel far attraversare una strada ad un rospo evitando che questo affoghi in un fiume.
il gioco ha 2 versioni: una utilizza i threads e una i processi. Verrà implementata una pipe per la comunicazione tra i processi.

Ci sono diversi oggetti, chiascuno è rappresentato da un processo:
Il giocatore(Rana) che si muove nelle 4 direzioni
I coccodrilli(tronchi) che si muovono in orizzontale, minimo 8 corsie
I proiettili, in questa versione i coccodrilli sparano proiettili, la rana ha dei proiettili a disposizione per distruggerli
Il gestore delle stampe e del tempo
