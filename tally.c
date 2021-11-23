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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define FICHIER_SORTIE "count"

int main(int argc, char **argv) {
    int i = 1;  // compteur argv
    int socket1[2];
    int socket2[2];
    pid_t pfils;
    int compte_forks = 0;

    while ( i < argc){
        char **commandes = malloc(( argc ) * sizeof( char * ));

        int j = 0; // place dans array
        while ( i < argc && strcmp( argv[i], ":" )) {  
            commandes[j] = argv[i];
            //*(commandes + j) = argv[i];
            i++;
            j++;
        }
        i++;
        //j++;
        //commandes[j] = NULL;

        socketpair( AF_UNIX, SOCK_STREAM, 0, socket1 ); // verifier erreurs

        compte_forks++;
        pfils = fork();

        if ( pfils == -1 ) {
            return 1;

        } else if ( pfils == 0 ) {  // Si c'est le fils
            if (compte_forks > 1) { // Verifie si c'est le premier
                //printf("pas le premier\n");
                dup2(socket2[0], 0);
                close(socket2[0]); close(socket2[1]);

            }

            if ( i < argc) {   // Verifie si c'est le dernier
                //printf("pas le dernier\n");
                dup2(socket1[1], 1);
                close(socket1[0]); close(socket1[1]);

            }
                //printf("exec\n");
            execvp( commandes[0], commandes);
            //perror("Could not execve");
            return 1;   // ne devrait jamais se rendre ici!

        } else {    // Si c'est le pere
            free(commandes);
        
                // ferme tube 2 (au besoin)
            //close(socket2[0]); close(socket2[1]);
            //close(socket2[0]);

            if ( i < argc) {   // Verifie si c'est le dernier
                socketpair( AF_UNIX, SOCK_STREAM, 0, socket2 ); // verifier erreurs
                
                pid_t pcompt = fork();  // fork un compteur
                if ( pcompt == -1 ) {
                    return 1;

                } else if ( pcompt == 0 ) {  // Si c'est le compteur

                    close(socket1[1]);
                    close(socket2[0]);

                    // char *buffer = calloc(32, sizeof(char));
                    char *buffer[32] = {'\0'};
                    // int bytesCopied = read(socket1[0], socket2[1], sizeof(socket1[0]));
                    int bytesCopied = read(socket1[0], buffer, sizeof(buffer));
                    close(socket1[0]);

                    //printf("-->%s\n", buffer);

                    if (bytesCopied > 0){
                        write(socket2[1], buffer, sizeof(buffer));
                        // printf("passe par le compteur...\n");

                        // free(buffer);

        
                        //     Ouvre un fichier texte sortie
                        // char * outputPath = concatenatePath( OUTPUT_DIR, basename( path ));
                        int outputFile = open( FICHIER_SORTIE, O_CREAT | O_APPEND | O_WRONLY, 0666 );
                        if ( outputFile == -1 ){
                            exit(1);
                        }

                        char resultat[10];
                        sprintf(resultat, "%d : %d\n", compte_forks, bytesCopied );

                        if ( write( outputFile, resultat, sizeof(resultat) ) == -1 ){
                            exit(1);
                        } 

                        if ( close( outputFile ) == -1 ){ 
                            exit(1);
                        }
                    }

                    close(socket2[1]);
                    // p.s. c'est le compteur qui attend le processus fils. wait
                    // i = argc + 1  // ne retourne pas au while...
                    exit(1);

                } else {    // Si c'est toujours le pere
                    close(socket1[1]); close(socket1[0]);
                    close(socket2[1]);
                }
            }
        }   
    } // Fin de la boucle while




    // wait d'alcatraz.c :
    // Attend le retour du dernier fork...
    int etatAttente;

    do {
        if ( waitpid(pfils, &etatAttente, WUNTRACED) == -1 ){
            return 1;
        }

        // si le fils s'est terminé normalement, afficher sa valeur de retour
        if ( WIFEXITED( etatAttente )) {
            // printf( "%d\n", WEXITSTATUS( etatAttente ));

            // Si le fils s'est terminé à cause d'un signal reçu, affiche le no
            // du signal et retourne 1
        } else if ( WIFSIGNALED( etatAttente )) {
            // printf( "%d\n", WTERMSIG( etatAttente ));
            return 1;
        }
    } while ( !WIFEXITED( etatAttente ) && !WIFSIGNALED( etatAttente ));


    return 0;
}