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

int main(int argc, char **argv) {
    int i = 1;  // compteur argv
    int socket1[2];
    int socket2[2];
    pid_t pfils;
    int compte_forks = 0;

    while ( i < argc){

        char *ligneCommandes = argv[i];
        char *arguments = malloc(0);
        int taille_arguments = 0;
        while ( i < argc && strcmp( argv[i], ":" )) {
            taille_arguments = taille_arguments + strlen(argv[i]) +1;
            arguments = realloc(arguments, taille_arguments );
            strcat( arguments, argv[i]);
            strcat( arguments, " ");
            i++;
        }
        i++;

        socketpair( AF_UNIX, SOCK_STREAM, 0, socket1 ); // verifier erreurs

        compte_forks++;
        pfils = fork();

        


        if ( pfils == -1 ) {
            return 1;

        } else if ( pfils == 0 ) {  // Si c'est le fils

            if (compte_forks > 1) { // Verifie si c'est le premier
                close(socket2[1]);
                dup2(socket2[0], 0);
                close(socket2[0]);
            }

            if ( i < argc - 1) {   // Verifie si c'est le dernier
                close(socket1[0]);
                dup2(socket1[1], 1);
                close(socket1[1]);
            }

            execve( ligneCommandes, &arguments, NULL );
            return 1;   // ne devrait jamais se rendre ici!

        } else {    // Si c'est le pere

        

            close(socket1[1]);
            dup2(socket1[0], 0);
            close(socket1[0]);

                // ferme tube 2 (au besoin)
            // Crée un tube 2 (vers execve) 
            socketpair( AF_UNIX, SOCK_STREAM, 0, socket2 ); // verifier erreurs
            // fork un compteur

            pid_t pcompt = fork();


            if ( pcompt == -1 ) {
                return 1;
            } else if ( pcompt == 0 ) {  // Si c'est le compteur

                char buffer[1024];
                read(socket1[0], buffer, 1024);

                // compte les bytes
                int bytesCopied = 0;


                write(socket2[1], buffer, sizeof(buffer));

   
                //     Ouvre un fichier texte sortie
                // char * outputPath = concatenatePath( OUTPUT_DIR, basename( path ));
                int outputFile = open( "count", O_CREAT | O_APPEND | O_WRONLY, 0666 );
                if ( outputFile == -1 ){
                    exit(1);
                }
                // write ...
                //     écrit le compte dans le fichier


                if ( close( outputFile ) == -1 ){ 
                    exit(1);
                }

                // p.s. c'est le compteur qui attend le processus fils. wait
                // i = argc + 1  // ne retourne pas au while...

                // Ferme le 1er tube
                // Ferme le 2e tube
                // Exit

            } else {    // Si c'est toujours le pere
                // rien de prévu
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
            //printf( "%d\n", WEXITSTATUS( etatAttente ));

            // Si le fils s'est terminé à cause d'un signal reçu, affiche le no
            // du signal et retourne 1
        } else if ( WIFSIGNALED( etatAttente )) {
            //printf( "%d\n", WTERMSIG( etatAttente ));
            return 1;
        }
    } while ( !WIFEXITED( etatAttente ) && !WIFSIGNALED( etatAttente ));


    return 0;
}