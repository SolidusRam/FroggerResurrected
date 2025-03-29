#include "../include/game.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

void game(int pipein,int pipeToFrog,int num_coccodrilli,int *vite,int pausepipe)
{
    struct position p;
    srand(time(NULL));
    //la rana inizia dal centro dello schermo
    struct position rana_pos = {
        .c = '$', 
        .x = GAME_WIDTH-3, 
        .y = GAME_HEIGHT-2, 
        .width = 2, 
        .height = 5,
        .id = 0
    };
    struct position crocodile_positions [num_coccodrilli];
    struct position bullets[MAX_BULLETS];
    struct tana tane[NUM_TANE];
    init_dens(tane);
    int tane_occupate = 0;
    int max_height_reached = GAME_HEIGHT-2; // Initialize to starting position

    bool game_over = false;
    
    // Imposta il file descriptor pausepipe come non bloccante
    fcntl(pausepipe, F_SETFL, O_NONBLOCK);
    bool pause = false;

    //Inzializzazione delle variabili per la barra del tempo
    int max_time = 30; //ho messo 30 secondi, se necessario si può incrementare
    int remaining_time = max_time;
    time_t last_update = time(NULL);
    draw_time_bar(remaining_time, max_time);

    //Inizializzazione dello score
    int score=0;
    draw_score(score);

    // Initialize all crocodile positions
    // i coccodrilli si dividono in corsie
    for (int i = 0; i < num_coccodrilli; i++) {
            crocodile_positions[i] = (struct position) {
            .c = 'C',
            .height = 2,  // Keep height same as frog
            .id = i
        };
    }

    //inizializzo i proiettili
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
        bullets[i].id = i;
        bullets[i].collision = 0;
    }
    
    while (!game_over && *vite>0)
    {
        time_t current_time = time(NULL);


        // Controlla se è arrivato un comando di pausa
        char pause_cmd;
        if (read(pausepipe, &pause_cmd, sizeof(char)) > 0) {
            if (pause_cmd == 'p') {
                pause = !pause;
                
                if (pause) {
                    // Mostra messaggio di pausa
                    mvprintw(LINES/2, COLS/2-10, "GIOCO IN PAUSA");
                    mvprintw(LINES/2+1, COLS/2-15, "Premi 'p' per continuare");
                    refresh();

                    // Invia SIGSTOP a tutti i processi coccodrillo
                    for (int i = 0; i < num_coccodrilli; i++) {
                        kill(crocodile_positions[i].pid, SIGSTOP);
                    }
                     // Invia SIGSTOP a tutti i processi proiettile attivi
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (bullets[i].active && bullets[i].pid > 0) {
                            // Verifica che il processo sia ancora vivo
                            if (kill(bullets[i].pid, 0) != -1) {
                                kill(bullets[i].pid, SIGSTOP);
                            }
                        }
                    }
                } else {
                    // Aggiorna il tempo per evitare salti
                    last_update = time(NULL);

                    // Cancella messaggio di pausa
                    mvprintw(LINES/2, COLS/2-10, "                 ");
                    mvprintw(LINES/2+1, COLS/2-15, "                        ");
                    refresh();

                    // Invia SIGCONT a tutti i processi coccodrillo
                    for (int i = 0; i < num_coccodrilli; i++) {
                        kill(crocodile_positions[i].pid, SIGCONT);
                    }

                    // Invia SIGCONT a tutti i processi proiettile attivi
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (bullets[i].active && bullets[i].pid > 0) {
                            // Verifica che il processo sia ancora vivo
                            if (kill(bullets[i].pid, 0) != -1) {
                                kill(bullets[i].pid, SIGCONT);
                            }
                        }
                    }

                    // Svuota le pipe per evitare l'accumulo di posizioni obsolete
                    char buffer[1024];
                    fcntl(pipein, F_SETFL, O_NONBLOCK);
                    while (read(pipein, buffer, sizeof(buffer)) > 0) {
                        // Svuota il buffer
                    }
                    fcntl(pipein, F_SETFL, 0); // Ripristina modalità bloccante
                    
                }
            }
        }

        // Se il gioco è in pausa, non fare nulla e controlla solo per comandi di pausa
        if (pause) {
            // Non leggere dalle altre pipe
            // Non aggiornare la posizione
            // Non fare collision detection
            usleep(50000);  // Attesa breve per non consumare troppa CPU
            continue;
        }
      
        // Leggi la posizione dalla pipe
        ssize_t r = read(pipein, &p, sizeof(struct position));
        if (r <= 0) {
            mvprintw(LINES/2, COLS/2-10, "Pipe read error: %s", strerror(errno));
            refresh();
            sleep(1);
            continue;
        }

        //disegno lo score
        draw_score(score);
        //disegno il numero di vite rimanenti
        mvprintw(LINES-1,GAME_WIDTH-20,"Vite: %d",*vite); //ho riadattato a GAME_WIDTH-20 per avere sia vite che score a schermo

        clear_entities(&rana_pos, crocodile_positions, num_coccodrilli, bullets, MAX_BULLETS);

        //aggiorno la posizione in base al carattere letto
        if (p.c == '$') {
            int crocodile_direction = 0;
            bool was_on_crocodile = rana_coccodrillo(&rana_pos, crocodile_positions, num_coccodrilli, &crocodile_direction);
            if (was_on_crocodile) {
                //Aggiorna score
                // Aggiorna la posizione della rana in base all'input del giocatore
                rana_pos.y = p.y;
                rana_pos.x = p.x;
                if (rana_pos.y < max_height_reached) {
                    score += 5;
                    max_height_reached = rana_pos.y;
                }
                // Aggiungi anche il movimento del coccodrillo
                rana_pos.x -= crocodile_direction;
                fcntl(pipeToFrog, F_SETFL, O_NONBLOCK);
                write(pipeToFrog, &rana_pos, sizeof(struct position));
                

            } else {
                //se la rana non è su un coccodrillo, controllo se e caduta in acqua
                if(frog_on_the_water(&rana_pos)){
                     //resetta lo score e riduce di 1 le vite
                    score=0;
                    (*vite)--;
                    max_height_reached = GAME_HEIGHT-2; // Reset max height

                    if (*vite > 0)
                    {
                        //stampa messaggio RAANA IN ACQUA al centro dello schermo
                        mvprintw(LINES/2, COLS/2-10, "RANA IN ACQUA!");
                        score=0;
                        refresh();
                        // Reset only frog position
                        rana_pos.x = GAME_WIDTH/2;
                        rana_pos.y = GAME_HEIGHT-2;
                        write(pipeToFrog, &rana_pos, sizeof(struct position));
                        //reset del timer
                        remaining_time = max_time;
                        napms(2000);

                        continue;
                    }
                    else{
                        game_over = true;
                        mvprintw(LINES/2, COLS/2-10, "GAME OVER!");
                        score=0;
                        refresh();
                        napms(2000);
                        break;
                    }

                }else{
                    rana_pos = p; // Only update position if not on crocodile 
                }
            }
            // Controlla che la rana non esca dallo schermo
            if (rana_pos.x < 1) rana_pos.x = 1;
            if (rana_pos.x > GAME_WIDTH - rana_pos.width - 1) 
                rana_pos.x = GAME_WIDTH - rana_pos.width - 1;
        } else if (p.c == 'C') { // Update crocodile position
            for (int i = 0; i < num_coccodrilli; i++) {
                if (crocodile_positions[i].id == p.id) {
                    crocodile_positions[i] = p;
                    break;
                }
            }
        }else if(p.c == '@' || p.c == '*'){
            // Prima cerchiamo se esiste già un proiettile con lo stesso pid
            int found = 0;

            // Prima rimuovi i proiettili inattivi o morti
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active) {
                    // Verifica se il processo è ancora vivo
                    if (kill(bullets[i].pid, 0) == -1) {
                        bullets[i].active = 0;
                        bullets[i].pid = 0;
                    }
                }
            }
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active && bullets[i].pid == p.pid) {\
                    int was_collided = bullets[i].collision;
                    bullets[i].x = p.x;
                    bullets[i].y = p.y;
                    bullets[i].collision = was_collided;
                    found = 1;
                    break;
                }
            }
            
            // Se non trovato, cerchiamo uno slot libero
            if (!found) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        bullets[i] = p;
                        bullets[i].active = 1;
                        bullets[i].collision = 0;
                        break;
                    }
                }
            }
        }

        //controllo la condizione di vittoria, la rana ha raggiunto la fine dello schermo
        if (rana_pos.y <= 1) { 
            if(check_den_collision(&rana_pos, tane)) {
                score+=100;
                tane_occupate++;
                //reset del timer
                remaining_time = max_time;
                max_height_reached = GAME_HEIGHT-2; // Reset max height

                // Reset rana position after filling a den
                rana_pos.x = GAME_WIDTH/2;
                rana_pos.y = GAME_HEIGHT-2;
                write(pipeToFrog, &rana_pos, sizeof(struct position));
                
                if(tane_occupate == NUM_TANE) {
                    clear();
                    mvprintw(LINES/2, COLS/2-10, "HAI VINTO!");
                    refresh();
                    sleep(2);
                    break;
                }
            }
        }
        

       //collisione proiettili
       //controllo se la rana è stata colpita
        for(int i=0;i<MAX_BULLETS;i++){
            if(bullets[i].c=='@'&&bullets[i].x==rana_pos.x&&bullets[i].y==rana_pos.y&& !bullets[i].collision){
                score=0;
                (*vite)--;
                if(*vite > 0) {
                    // 1. Clear old position
                    clear_frog_position(&rana_pos);
                    
                    // 2. Reset position
                    rana_pos.x = GAME_WIDTH/2;
                    rana_pos.y = GAME_HEIGHT-2;
                    
                    // 3. Write new position to pipe with error check
                    ssize_t w = write(pipeToFrog, &rana_pos, sizeof(struct position));
                    if(w < 0) {
                        mvprintw(0, 0, "Pipe write error: %s", strerror(errno));
                        refresh();
                    }

                    // Si resetta lo score
                    score=0;
                    
                    // 4. Show message
                    mvprintw(LINES/2, COLS/2-10, "RANA COLPITA! Vite: %d", *vite);
                    refresh();
                    napms(2000);
                    
                    // 5. Reset timer
                    remaining_time = max_time;
                    
                    // 6. Clear collision bullet
                    bullets[i].collision = 1;
                    
                    // 7. Redraw frog at new position
                    attron(COLOR_PAIR(1));
                    for (int i = 0; i < rana_pos.height; i++)
                    {
                        for (int j = 0; j < rana_pos.width; j++)
                        {
                            mvaddch(rana_pos.y + i, rana_pos.x + j, rana_sprite[i][j]);
                        }
                    }
                    attroff(COLOR_PAIR(1));
                    refresh();
                    
                    break; // Exit bullet check loop
                } else {
                    game_over = true;
                    mvprintw(LINES/2, COLS/2-10, "GAME OVER!");
                    mvprintw((LINES / 2) + 1, COLS / 2 - 10, "SCORE FINALE: %d", score);
                    refresh();
                    napms(2000);
                    return; // Exit game function
                }
            }
        }
       //collisione due proiettili
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active && bullets[i].c == '@' && !bullets[i].collision) {
                for (int j = 0; j < MAX_BULLETS; j++) {
                    if (bullets[j].active && bullets[j].c == '*'&& !bullets[j].collision) {
                        if (bullets[i].x == bullets[j].x && bullets[i].y == bullets[j].y && !bullets[j].collision) {

                            score+=50;
                          
                            bullets[i].collision = 1;
                            bullets[j].collision = 1;

                            mvaddch(bullets[i].y, bullets[i].x, 'X');
                            mvaddch(bullets[j].y, bullets[j].x, 'X');
                        }
                    }
                }
            }
        }

        // Se è passato un secondo, aggiorna il tempo rimanente

        if (current_time - last_update >= 1) {

        //aggiorna le variabili che gestiscono il tempo
            last_update = current_time;
            remaining_time--;

            // Disegna la barra del tempo
            draw_time_bar(remaining_time, max_time);

            // Controlla se il tempo è finito
            if (remaining_time <= 0) {
                mvprintw(LINES / 2, COLS / 2 - 10, "TEMPO SCADUTO!");
                refresh();
                napms(2000);
                game_over = true;
            }
        }

        // Draw river borders after clearing 
        draw_river_borders();
        draw_game_borders();
        draw_dens(tane);

        //disegno i coccodrilli
        draw_crocodiles(crocodile_positions, num_coccodrilli);
        

        //disegno i proiettili
        void draw_bullets(struct position bullets[], int max_bullets);

        //disegno la rana

        draw_frog(&rana_pos);
        refresh();
    }
}

bool rana_coccodrillo(struct position *rana_pos, struct position crocodile_positions[], int num_coccodrilli, int *direction) {
    for (int i = 0; i < num_coccodrilli; i++) {
        // Check if frog is on crocodile
        if (rana_pos->y == crocodile_positions[i].y && 
            rana_pos->x >= crocodile_positions[i].x && 
            rana_pos->x <= crocodile_positions[i].x + crocodile_positions[i].width - rana_pos->width) {
            
            // Set the direction based on the crocodile's lane
            int lane = (crocodile_positions[i].id/2) % LANES;
            *direction = (lane % 2 == 0) ? -1 : 1;
            return true; // Frog is on crocodile
        }
    }
    return false; // Frog is not on any crocodile
}

bool frog_on_the_water(struct position *rana_pos){
    if (rana_pos->y < FLOOR_HEIGHT && rana_pos->y > 3){
        return true;
    }
    return false;
}
