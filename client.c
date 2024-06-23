#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>


#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_ATTENTE 300
#define MAX_CONNECTIONS 7

//pour stocker les messages en cas d'arrêt des réceptions le temps d'envoyer un message
int bloque = 0;
int attendus = 0;
int ignore = 0;
int personnes[MAX_CONNECTIONS];
int p = 0;
int destination;
char patience[MAX_ATTENTE][BUFFER_SIZE];


void *communication_continue(void *arg) {
    int c_sock = *((int *)arg);
    char buff[BUFFER_SIZE];     //reçoit message serveur

    while (1) {
        int rec = recv(c_sock, buff, BUFFER_SIZE, 0);
        if (rec < 0) {
            perror("Erreur lors de la réception du msg du serveur");
            exit(EXIT_FAILURE);
        }

        buff[rec] = '\0';

        // dans le cas où on envoie à un.e utilisateur.ice précis.e, le message reçu est la liste des personnes connectées
        if (ignore) {
            printf("%s\n", buff);        // on affiche les utilisateur.ices online

            char *ligne = strtok(buff, "\n");
            while (ligne != NULL) {
                char *apres = ligne;

                while (*apres != '\0' && *(apres+1) != ')') {  // on cherche le caractère avant la parenthèse (i.e num client)
                    apres++;
                }

                sscanf(apres, "%d", &destination);
                personnes[p++] = destination;
                ligne = strtok(NULL, "\n");
            }
            // on stocke leurs numéros dans une liste
            ignore = 0;
            continue;
        }

        // si user appuie sur 2 pour ne pas être interrompu
        if (bloque) {
            strcpy(patience[attendus++], buff);
        }

        else {
            printf("%s\n", buff);
        }

      }
      return NULL;
}


// pour savoir si un nombre fait partie d'une liste de taille len
int in_list(int nb, int* list, int len) {
    for (int i = 0; i < len; i++) {
        if (list[i] == nb) {
            return 1;
        }
    }
    return 0;
}


int main() {
    int c_sock;
    struct sockaddr_in s_add;
    char msg[BUFFER_SIZE];   //stocker messsage à envoyer
    char action[50];         //stocke action de user (0 pour quitter, 1 pour envoyer, 2 sans interruption, 3 pour mp)

    c_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (c_sock == -1) {
        perror("Erreur lors de la création de la socket");
        exit(EXIT_FAILURE);
    }

    memset(&s_add, 0, sizeof(s_add));
    s_add.sin_family = AF_INET;
    s_add.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &s_add.sin_addr) <= 0) {
        perror("Adresse IP invalide ou non supportée");
        exit(EXIT_FAILURE);
    }

    if (connect(c_sock, (struct sockaddr *)&s_add, sizeof(s_add)) < 0) {
        perror("Erreur lors de la connexion au serveur");
        exit(EXIT_FAILURE);
    }

    //thread pour recevoir message du serveur à tout moment
    pthread_t tid;
    if (pthread_create(&tid, NULL, communication_continue, &c_sock) != 0) {
        perror("Erreur lors de la création du thread pour la communication continue avec le serveur");
        close(c_sock);
        exit(EXIT_FAILURE);
    }

    //choix du pseudo
    char pseudo[50];

    printf("\nVeuillez choisir un pseudo : ");
    fgets(pseudo, BUFFER_SIZE, stdin);
    char *newline = strchr(pseudo, '\n');  //pour retirer le saut de ligne final

    if (newline != NULL) {
        *newline = '\0';
    }

    send(c_sock, pseudo, strlen(pseudo), 0);

    //interface client
    printf("\nBienvenue %s !\nVoici les commandes :\n",pseudo);
    printf("\t - rentrer '1' pour écrire un message\n");
    printf("\t - rentrer '2' pour écrire un message sans interruption\n");
    printf("\t - rentrer '3' pour écrire un message privé à un utilisateur\n");
    printf("\t - rentrer '0' pour sortir\n");
    printf("---------------------------------------------------------------------------\n\n");

    //tant qu'on ne décide pas de quitter (avec 0) on reste dans la boucle
    while (1) {
        fgets(action, BUFFER_SIZE, stdin);

        char *newline = strchr(action, '\n');  //pour retirer le saut de ligne final
        if (newline != NULL) {
            *newline = '\0';
        }

        int num = -1;

        if (sscanf(action, "%d", &num) != 1) {
            printf("ENTREE INVALIDE\n\n");
            continue;   //on recommence la boucle
        }

        if (num == 0) {   //pour quitter le serveur
            printf("Voulez-vous vraiment quitter la messagerie ? [Y/N]\n");
            fgets(msg, BUFFER_SIZE, stdin);
            char *newline = strchr(msg, '\n');  //pour retirer le saut de ligne final
            if (newline != NULL) {
                *newline = '\0';
            }
            //on passe le premier char en maj (pour accepter y et Y)
            if (msg[0] >= 'a' && msg[0] <= 'z') {
                msg[0] = toupper(msg[0]);
            }

            if (strcmp(msg, "Y") == 0) {
              printf("Fermeture du socket\n");
              break;
            }
            else{
              printf("Merci de rester :)\n");
            }
        }

        else if (num == 1) {  //pour envoyer un msg
            printf("vous: ");
            fgets(msg, BUFFER_SIZE, stdin);

            char *newline = strchr(msg, '\n');  //pour retirer le saut de ligne final
            if (newline != NULL) {
                *newline = '\0';
            }

            size_t len = strlen(msg);

            if (send(c_sock, msg, len, 0) == -1) {
                perror("Erreur lors de l'envoi du message");
                break;
            }
        }

        else if (num == 2) {    //envoyer sans interruption
            bloque = 1;
            attendus = 0;
            printf("vous: ");
            fgets(msg, BUFFER_SIZE, stdin);

            char *newline = strchr(msg, '\n');  //pour retirer le saut de ligne final
            if (newline != NULL) {
                *newline = '\0';
            }

            size_t len = strlen(msg);

            if (send(c_sock, msg, len, 0) == -1) {
                perror("Erreur lors de l'envoi du message");
                break;
            }

            printf("\n");

            if (attendus > 0) {   //au moins un message en attente
                for (int i = 0; i < attendus; i++) {
                    printf(patience[i]);
                    printf("\n");
                    sleep(0.5);
                }
            }
            bloque = 0;
        }

        else if (num == 3) {
            strcpy(msg, "135213521352");
            size_t len = strlen(msg);
            ignore = 1;
            if (send(c_sock, msg, len, 0) == -1) {       // on envoie un codage spécial, pour indiquer qu'on est dans le cas 3
                perror("Erreur lors de l'envoi du message");
                break;
            }

            printf("A quel.le utilisateur.ice envoyer ce message ? Voici celleux connecté.es : \n");
            sleep(2);
            // on a affiché dans une autre fonction avant la liste des personnes connectées

            while (1) {
                fgets(action, BUFFER_SIZE, stdin);

                char *newline = strchr(action, '\n');  // Retirer le saut de ligne final
                if (newline != NULL) {
                    *newline = '\0';
                }

                if (sscanf(action, "%d", &destination) != 1) {
                    printf("ENTREE INVALIDE\n\n");
                    continue;   // Recommencez la boucle
                }


                if (in_list(destination, personnes, p)) {
                    break; // Le nombre est dans la liste d'utilisateurs possibles
                }

                printf("CE N'EST PAS UN UTILISATEUR VALIDE.\n\n");
            }

            // maintenant on envoit le num de la personne visée au serveur
            sprintf(msg, "%d", destination);
            len = strlen(msg);
            if (send(c_sock, msg, len, 0) == -1) {
                perror("Erreur lors de l'envoi du message");
                break;
            }

            // on peut ENFIN ecrire notre message et l'envoyer au serveur, qui saura à qui l'envoyer
            printf("vous : ");
            fgets(msg, BUFFER_SIZE, stdin);

            char *newline = strchr(msg, '\n');  //pour retirer le saut de ligne final
            if (newline != NULL) {
                *newline = '\0';
            }

            len = strlen(msg);

            if (send(c_sock, msg, len, 0) == -1) {
                perror("Erreur lors de l'envoi du message");
                break;
            }

        }

        else {  //ni 0, ni 1, ni 2, ni 3
          printf("ENTREE INVALIDE :\n\n");
        }
    }

    //on a choisi de quitter le serveur
    close(c_sock);
    return 0;
}
