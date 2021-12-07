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
#define TAILLE_TAMPON 64

/**
 * @brief Construit un array {@code argv} à donner à l'{@code execvp} pour
 * lancer chaque nouveau processus.
 * 
 * @param argc nombre de valeurs de l'argv du {@code main}.
 * @param argv l'argv du main.
 * @param argvCurseur emplaceemnt de la tête de lecture dans l'argv du main.
 * @param commands le nouvel array argv à passer en argument à execvp.
 */
void buildNewArgv( int argc, char** argv, int *argvCurseur, char** commands );

/**
 * @brief Redirige {@code stdout} vers le socket1 et/ou le socket2 vers stdin, 
 * dépendant de la situation
 * 
 * @param redirCount indique à quelle redirections nous sommes rendus.
 * @param argvCurseur emplaceemnt de la tête de lecture dans l'{@code argv} du main.
 * @param argc nombre de valeurs de l'argv du main.
 * @param socket1 la socketpair reliant {@code stdout} au compteur.
 * @param socket2 la socketpair reliant le compteur au {@code stdin} du prochain
 *                processus.
 */
void prepareIO( int redirCount, int argvCurseur, int argc,
                      int *socket1, int *socket2 );

/**
 * @brief Compte le nombre d'octets qui transitent par chaque redirection.
 * 
 * Cette fonction établie le lien entre le {@code socket1} et le {@code socket2}
 * (au besoin).
 * 
 * @param redirCount indique à quelle redirections nous sommes rendus.
 * @param socket1 la socketpair reliant {@code stdout} au compteur.
 * @param socket2 la socketpair reliant le compteur au {@code stdin} du prochain
 *                processus. 
 */
void counter( int redirCount, int *socket1, int *socket2 );

/**
 * @brief Ouvre ou crée un fichier texte sortie pour le compte
 * 
 * @param redirCount 
 * @param bytesCopied 
 */
void writeFile( int redirCount, int bytesCopied );


void buildNewArgv( int argc, char** argv, int *argvCurseur, char** commands ){
    int positionInArray = 0;

    while ( *argvCurseur < argc && strcmp( argv[*argvCurseur], ":" )) {  

        commands[positionInArray] = argv[*argvCurseur];
        positionInArray++;
        ( *argvCurseur )++;
    }
    commands[positionInArray] = '\0';
    ( *argvCurseur )++;
}

void prepareIO( int redirCount, int argvCurseur, int argc,
                      int *socket1, int *socket2 ){
    // Si ce n'est pas la première redirection, utlilise le socket2 comme stdin
    if (redirCount > 1) { 
        dup2( socket2[0], 0 );
        close( socket2[0] ); close( socket2[1] );
    }
    // Si ce n'est pas la dernière redirection, utilise le socket 1 comme stdin
    if ( argvCurseur < argc ) { 
        dup2( socket1[1], 1 );
        close( socket1[0] ); close( socket1[1] );
    }
}

void counter( int redirCount, int *socket1, int *socket2 ){
    char tampon[TAILLE_TAMPON] = {'\0'};
    int bytesRead = 0;
    int bytesCopied = 0;

    close(socket1[1]); close(socket2[0]);

    while(( bytesRead = read( socket1[0], tampon, TAILLE_TAMPON )) > 0 ){
        bytesCopied += write( socket2[1], tampon, bytesRead );
        bytesRead = 0;
    }
    close( socket1[0]); close( socket2[1] );

    if ( bytesCopied > 0 )
        writeFile( redirCount, bytesCopied );
}

void writeFile( int redirCount, int bytesCopied ){
    char outputLine[24];
    
    int outputFile = open( FICHIER_SORTIE, O_CREAT | O_APPEND | O_WRONLY, 0666 );
    if ( outputFile == -1 )
        exit(1);

    int longeur = sprintf( outputLine, "%d : %d\n", redirCount, bytesCopied );

    if (( write( outputFile, outputLine, longeur ) == -1 ) || ( close( outputFile ) == -1 ))
        exit(1);
}


int main(int argc, char **argv){
    pid_t childPid = -1;
    int argvCurseur = 1;
    int redirCount = 0;
    int childCount = 0;
    int socket1[2], socket2[2];

    while ( argvCurseur < argc ){

        char **commands = calloc( argc, sizeof( char * ));
        buildNewArgv( argc, argv, &argvCurseur, commands );

        if (socketpair( AF_UNIX, SOCK_STREAM, 0, socket1 ) == -1)
            return 1;

        // Fork un fils
        redirCount++;
        childCount++;
        childPid = fork();
        if ( childPid == -1 ){
            return 1;

        // Si c'est le fils
        } else if ( childPid == 0 ) {  
            prepareIO( redirCount, argvCurseur, argc, socket1, socket2 );            
            execvp( commands[0], commands );
            return 127;

        // Si c'est le pere
        } else {    
            free( commands );

            // Si ce n'est pas la dernière redirection:
            if ( argvCurseur < argc) { 
                if ( socketpair( AF_UNIX, SOCK_STREAM, 0, socket2 ) == -1 )
                    return 1;
                
                // Fork un compteur
                childCount++;
                pid_t counterPid = fork();
                if ( counterPid == -1 ){
                    return 1;

                // Si c'est le compteur
                } else if ( counterPid == 0 ) { 
                    counter( redirCount, socket1, socket2 );
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
    int waitState;
    int returnValue = 127;

    do {
        if ( waitpid(childPid, &waitState, WUNTRACED ) == -1 )
            return 1;

        /* En cas de succès, on retournera le code de retour de la dernière commande. */
        if ( WIFEXITED( waitState )) {
            returnValue = WEXITSTATUS( waitState );

        /* Si la dernière commande se termine à cause d'un signal,
           le code de retour est 128 + numéro du signal. */
        } else if ( WIFSIGNALED( waitState ))
            returnValue = 128 + WTERMSIG( waitState );

    } while ( !WIFEXITED( waitState ) && !WIFSIGNALED( waitState ) ); 


    /* Attend que toutes les autres commands se soient terminées. */
    for( argvCurseur = 0; argvCurseur < childCount -1; argvCurseur++ )
        wait( NULL );

    return returnValue;
}