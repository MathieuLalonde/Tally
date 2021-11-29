/*  INF3173 - TP2
 *  Session : automne 2021
 *  Tous les groupes
 *  
 *  IDENTIFICATION.
 *
 *      Nom : Lalonde
 *      Prénom : Mathieu
 *      Code permanent : LALM14127501
 *      Groupe : 20
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define FICHIER_SORTIE "count"
#define TAILLE_TAMPON 32

/**
 * @brief Construit un array {@code argv} à donner à l'{@code execvp} pour
 * lancer chaque nouveau processus.
 * 
 * @param argc nombre de valeurs de l'argv du {@code main}.
 * @param argv l'argv du main.
 * @param argv_select emplaceemnt de la tête de lecture dans l'argv du main.
 * @param commands le nouvel array argv à passer en argument à execvp.
 */
void build_new_argv( int argc, char** argv, int *argv_select, char** commands );

/**
 * @brief Redirige {@code stdout} vers le socket1 et/ou le socket2 vers stdin, 
 * dépendant de la situation
 * 
 * @param redirection_count indique à quelle redirections nous sommes rendus.
 * @param argv_select emplaceemnt de la tête de lecture dans l'{@code argv} du main.
 * @param argc nombre de valeurs de l'argv du main.
 * @param socket1 la socketpair reliant {@code stdout} au compteur.
 * @param socket2 la socketpair reliant le compteur au {@code stdin} du prochain
 *                processus.
 */
void prepare_io( int redirection_count, int argv_select, int argc,
                      int *socket1, int *socket2 );

/**
 * @brief Compte le nombre d'octets qui transitent par chaque redirection.
 * 
 * Cette fonction établie le lien entre le {@code socket1} et le {@code socket2}
 * (au besoin).
 * 
 * @param redirection_count indique à quelle redirections nous sommes rendus.
 * @param socket1 la socketpair reliant {@code stdout} au compteur.
 * @param socket2 la socketpair reliant le compteur au {@code stdin} du prochain
 *                processus. 
 */
void counter( int redirection_count, int *socket1, int *socket2 );

/**
 * @brief Ouvre ou crée un fichier texte sortie pour le compte
 * 
 * @param redirection_count 
 * @param bytesCopied 
 */
void write_file( int redirection_count, int bytesCopied );


void build_new_argv( int argc, char** argv, int *argv_select, char** commands ){
    int position_in_array = 0;

    while ( *argv_select < argc && strcmp( argv[*argv_select], ":" )) {  
        
        commands[position_in_array] = argv[*argv_select];
        position_in_array++;
        ( *argv_select )++;
    }
    commands[position_in_array] = '\0';
    ( *argv_select )++;
}

void prepare_io( int redirection_count, int argv_select, int argc,
                      int *socket1, int *socket2 ){
    // Si ce n'est pas la première redirection, utlilise le socket2 comme stdin
    if (redirection_count > 1) { 
        dup2( socket2[0], 0 );
        close( socket2[0] ); close( socket2[1] );
    }
    // Si ce n'est pas la dernière redirection, utilise le socket 1 comme stdin
    if ( argv_select < argc ) { 
        dup2( socket1[1], 1 );
        close( socket1[0] ); close( socket1[1] );
    }
}

void counter( int redirection_count, int *socket1, int *socket2 ){
    char tampon[TAILLE_TAMPON] = {'\0'};
    int bytesRead = 0;
    int bytesCopied = 0;

    close(socket1[1]); close(socket2[0]);

    while(( bytesRead = read( socket1[0], tampon, TAILLE_TAMPON )) > 0 ) {
        bytesCopied += write( socket2[1], tampon, bytesRead );
        bytesRead = 0;
    }
    close( socket1[0]); close( socket2[1] );

    if ( bytesCopied > 0 ){
        write_file( redirection_count, bytesCopied );
    }
}

void write_file( int redirection_count, int bytesCopied ){
    char ligne_info[24];
    
    int outputFile = open( FICHIER_SORTIE, O_CREAT | O_APPEND | O_WRONLY, 0666 );
    if ( outputFile == -1 ){
        exit(1);
    }

    int longeur = sprintf( ligne_info, "%d : %d\n", redirection_count, bytesCopied );

    if ( write( outputFile, ligne_info, longeur ) == -1 ){
        exit(1);
    } 
    if ( close( outputFile ) == -1 ){ 
        exit(1);
    }
}


int main(int argc, char **argv) {
    pid_t child_pid = -1;
    int argv_select = 1;
    int socket1[2], socket2[2];
    int redirection_count = 0;
    int child_count = 0;

    while ( argv_select < argc ){

        char **commands = calloc( argc, sizeof( char * ));
        build_new_argv( argc, argv, &argv_select, commands );

        if (socketpair( AF_UNIX, SOCK_STREAM, 0, socket1 ) == -1){
            return 1;
        }

        // Fork un fils
        redirection_count++;
        child_count++;
        child_pid = fork();
        if ( child_pid == -1 ) {
            return 1;

        // Si c'est le fils
        } else if ( child_pid == 0 ) {  
            prepare_io( redirection_count, argv_select, argc, socket1, socket2 );            
            execvp( commands[0], commands );
            return 127;

        // Si c'est le pere
        } else {    
            free( commands );

            // Si ce n'est pas la dernière redirection:
            if ( argv_select < argc) { 
                if (socketpair( AF_UNIX, SOCK_STREAM, 0, socket2 ) == -1){
                    return 1;
                }
                
                // Fork un compteur
                child_count++;
                pid_t counter_pid = fork();
                if ( counter_pid == -1 ) {
                    return 1;

                // Si c'est le compteur
                } else if ( counter_pid == 0 ) { 
                    counter( redirection_count, socket1, socket2 );
                    exit(1);

                // Si c'est toujours le pere
                } else {    
                    close( socket1[1] ); close( socket1[0] );
                    close( socket2[1] );
                }
            }
        }   
    } // Fin boucle

    /* Attend le retour de la dernière commande.
    Si elle n'existe pas, le code de retour est 127. */ 
    int wait_state;
    int return_value = 127;

    do {
        if ( waitpid(child_pid, &wait_state, WUNTRACED ) == -1 ){
            return 1;
        }
        /* En cas de succès, tally retourne le code de retour de la dernière commande. */
        if ( WIFEXITED( wait_state )) {
            return_value = WEXITSTATUS( wait_state );

        /* Si la dernière commande se termine à cause d'un signal,
           le code de retour est 128 + numéro du signal. */
        } else if ( WIFSIGNALED( wait_state )) {
            return_value = 128 + WTERMSIG( wait_state );
        }
    } while ( !WIFEXITED( wait_state ) && !WIFSIGNALED( wait_state ) ); 


    /* Attend que toutes les autres commands se soient terminées. */
    for( argv_select = 0; argv_select < child_count -1; argv_select++ ){
        wait( NULL );
    }

    return return_value;
}