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
#define MAX_CONNECTIONS 7
#define TAILLE_MAX 1024
#define MAX_MSG 100
#define RESET   "\033[0m" //pour retourner à la couleur normale après un printf

//liste de 7 couleurs (une par user)
const char colors[][9] = {
    "\033[31m", //rouge
    "\033[34m", //bleu
    "\033[32m", //vert
    "\033[33m", //jaune
    "\033[35m", //lagenta
    "\033[36m", //cyan
    "\033[37m"  //gris
};

// pour stocker info du client et le passer dans le thread utilisation
typedef struct {
    int c_sock;
    int c_num;                  //numéro propre à chaque client
    int online;                 //initialisé à 1, passe à 0 si le socket se ferme
    char c_pseudo[TAILLE_MAX];  //pseudo du client à afficher dans la messagerie
    char c_color[9];
} thread_args;

thread_args clients[MAX_CONNECTIONS];                     // liste des infos des clients
char tout_message[MAX_CONNECTIONS][MAX_MSG][TAILLE_MAX];  // liste de tous les messages envoyés

static void* utilisation(void* arg) {

    //on récupère les infos du client passées en argument
    thread_args *args = (thread_args *)arg;
    int c_sock = args->c_sock;
    int c_num = args->c_num;
    char c_pseudo[strlen(args->c_pseudo)];
    strncpy(c_pseudo, args->c_pseudo, strlen(args->c_pseudo));
    char c_color[9];
    strcpy(c_color, args->c_color);

    char buffer[TAILLE_MAX];      //pour recuperer le message du client
    int mess = 0;                 //cpt pour ajouter les messages dans la matrice stockant les msg

    //pour l'affichage de chaque msg, on écrit "pseudo: message"
    size_t current_len = strlen(c_pseudo);
    strcat(c_pseudo, ": ");
    c_pseudo[current_len + 2] = '\0';

    //pour l'option 3
    int present = 0;
    int personnel = 0;
    int destination = 0;

    while (1) {
            int bytes_received = recv(c_sock, buffer, TAILLE_MAX - 1, 0);
            if (bytes_received < 0) {
                perror("Erreur lors de la réception du message du client");
                close(c_sock);
                pthread_exit(NULL);
            }

            else if (bytes_received == 0) {
                c_pseudo[current_len] = '\0';         //car sinon c_pseudo = "pseudo: "
                sprintf(buffer, "%s%s%s s'est déconnecté(e)\n",c_color,c_pseudo,RESET);
                printf(buffer);

                //on prévient les clients d'une nouvelle déconnexion
                for (int i = 0; i < MAX_CONNECTIONS; i++) {
                  if ((clients[i].c_num != args->c_num)&&(clients[i].online == 1)) {  //on vérifie qu'on n'envoie pas à l'envoyeur originel + que le destinataire est en ligne
                        send(clients[i].c_sock, buffer, strlen(buffer), 0);
                    }
                }

                clients[c_num].online = 0;            // le client n'est plus en ligne
                close(c_sock);
                pthread_exit(NULL);
            }
            buffer[bytes_received] = '\0';            //pour ne lire que les données reçues (et pas le reste du tampon)


            char colored_pseudo[strlen(c_pseudo)+20];
            sprintf(colored_pseudo, "%s%s%s", c_color,c_pseudo, RESET); // on colorise le pseudo

            //cas où client envoie un message privé
            // on prévient que le prochain message sera celui du numéro à qui envoyer
            if (strcmp(buffer, "135213521352") == 0) {
                personnel = 1;
                strcpy(buffer, "");
                // ici, on renvoie une chaine avec les utilisateur.ices disponibles
                for (int i = 0; i < MAX_CONNECTIONS; i++) {
                    thread_args user = clients[i];
                    if (user.online) {                // si la personne est connectée, on l'ajoute à la liste
                        char remplissage[strlen(args->c_pseudo)+strlen(colors[i])+strlen(RESET)];
                        sprintf(remplissage, "%s%s%s (%d)\n", colors[i],user.c_pseudo,RESET, i);    // on met le pseudo et le numéro
                        strcat(buffer, remplissage);
                    }
                }
                // on renvoit une chaine indiquant les personnes presentes
                send(clients[c_num].c_sock, buffer, strlen(buffer), 0);
                continue;
            }

            // naturellement, cette itération est la suivante. Le texte reçu est forcément le numéro d'utilisateur à qui envoyer
            else if (personnel) {
                destination = atoi(buffer);    // on enregistre l'utilisateur destinataire
                personnel = 0;
                present = 1;
                continue;
            }

            // prochaine itération, on a reçu le texte à envoyer à destination précise
            else if (present) {
                strcpy(tout_message[c_num][mess++], buffer);
                printf("%s@%s%s%s %s\n", colored_pseudo,colors[destination],clients[destination].c_pseudo,RESET, buffer);        // affichage du message (+pseudo) sur le terminal du serveur
                send(clients[destination].c_sock, colored_pseudo, strlen(colored_pseudo), 0);         //on envoie déjà le pseudo

                char prive[] = "(Message privé) ";
                send(clients[destination].c_sock, prive, strlen(prive), 0);
                send(clients[destination].c_sock, buffer, strlen(buffer), 0);
                // on envoie le message au client numero destination
                present = 0;
                continue;
            }

            strcpy(tout_message[c_num][mess++], buffer);  //on stocke le message dans la matrice
            printf("%s%s\n",colored_pseudo, buffer);      //affichage du message (+pseudo en couleur) sur le terminal du serveur

            //on renvoie le message à tous les autres clients
            for (int i = 0; i < MAX_CONNECTIONS; i++) {
                if ((clients[i].c_num != args->c_num)&&(clients[i].online == 1)) {  //on vérifie qu'on n'envoie pas à l'envoyeur originel + que le destinataire est en ligne
                    send(clients[i].c_sock, colored_pseudo, strlen(colored_pseudo), 0);         //on envoie déjà le pseudo colorisé
                    send(clients[i].c_sock, buffer, strlen(buffer), 0);             //on envoie ensuite le message
                }
            }
   }
    close(c_sock);
    pthread_exit(NULL);
    return NULL;
}

int main() {
    pthread_t c_threads[MAX_CONNECTIONS];
    int c_sock[MAX_CONNECTIONS];
    struct sockaddr_in s_add, c_add;
    char errorMessage[TAILLE_MAX];
    char buffer[TAILLE_MAX];
    char rep[10];            //stocker réponse Y/N pour fermer le serveur
    int c_acceptes = 0;      //cpt le nombre de connexions acceptées
    int fermeture = 0;       //1 si on peut fermer le serveur, 0 sinon

    socklen_t c_len = sizeof(c_add);
    int s_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (s_sock == -1) {
        perror("Erreur lors de la création de la socket");
        exit(EXIT_FAILURE);
    }

    memset(&s_add, 0, sizeof(s_add));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = INADDR_ANY;   //adresse ip
    s_add.sin_port = htons(PORT);

    if (bind(s_sock, (struct sockaddr *)&s_add, sizeof(s_add)) == -1) {
        perror("Erreur lors de l'attachement de la socket au port");
        exit(EXIT_FAILURE);
    }

    if (listen(s_sock, MAX_CONNECTIONS) == -1) {
        perror("Erreur lors de la mise en écoute de la socket");
        exit(EXIT_FAILURE);
    }

    printf("Serveur en attente de connexions...\n\n");

    //tant qu'on peut accepter des connexions et qu'on ne souhaite pas fermer le serveur
    while (c_acceptes < MAX_CONNECTIONS && !fermeture){

      //ensemble de fd à surveiller pour le décompte (juste s_sock)
      fd_set readfds;
      FD_ZERO(&readfds);
      FD_SET(s_sock, &readfds);

      //décompte d'attente de connexion
      struct timeval timeout;
      timeout.tv_sec = 30;
      timeout.tv_usec = 0;

      int activity = select(s_sock + 1, &readfds, NULL, NULL, &timeout);    //renvoie le nb de fd prêts à fonctionner (0 si aucune connexion, 1 sinon)
      if (activity == -1) {
          perror("Erreur lors de la vérification des descripteurs de fichier avec select");
          exit(EXIT_FAILURE);

      } else if (activity == 0) {   //cas ou aucune connexion avant 30sec
        //propose de fermer le serveur si aucun client n'est connecté
        if (c_acceptes > 0){
              fermeture = 1;
              for (int i = 0 ; i < c_acceptes ; i++){
                if (clients[i].online == 1){  //au moins un client connecté
                  fermeture = 0;
                }
              }

              if (fermeture){    //aucun client connecté
                printf("Voulez-vous fermer le serveur (prochaine demande dans 30sec)? [Y/N]\n");
                fgets(rep, TAILLE_MAX, stdin);

                //on passe le premier char en maj (pour accepter y et Y)
                if (rep[0] >= 'a' && rep[0] <= 'z') {
                    rep[0] = toupper(rep[0]);
                }
                char *newline = strchr(rep, '\n');  //pour retirer le saut de ligne final de rep
                if (newline != NULL) {
                    *newline = '\0';
                }

                if (strcmp(rep, "Y") == 0) {
                  printf("Fermeture du serveur\n");
                  fermeture = 1;
                }
                else{
                  fermeture = 0;
                  printf("Serveur en attente de nouvelles connexions...\n\n");
                }
              }
          }
          continue;   //on recommence la boucle (et le décompte)
      }

      //attente de connexions
      if ((c_sock[c_acceptes] = accept(s_sock, (struct sockaddr *)&c_add, &c_len)) == -1) {
            sprintf(errorMessage, "Erreur lors de l'acceptation de la connexion numero %d", c_acceptes);
            perror(errorMessage);
            exit(EXIT_FAILURE);
        }

        //on attend le pseudo du client
        char pseudo[50];
        int nb = recv(c_sock[c_acceptes], pseudo, TAILLE_MAX, 0);
        pseudo[nb] = '\0';

        sprintf(buffer, "%s%s%s s'est connecté(e)\n\n", colors[c_acceptes], pseudo, RESET);
        printf(buffer);

        //on prévient les clients d'une nouvelle connexion
        for (int i = 0; i < c_acceptes; i++) {
            if (clients[i].online == 1) {           //on vérifie que le destinataire est en ligne
                send(clients[i].c_sock, buffer, strlen(buffer), 0);
            }
        }

        //stocke les infos du client pour les passer dans le thread
        strncpy(clients[c_acceptes].c_pseudo, pseudo, TAILLE_MAX);
        clients[c_acceptes].c_sock = c_sock[c_acceptes];
        clients[c_acceptes].c_num = c_acceptes;
        clients[c_acceptes].online = 1;
        strcpy(clients[c_acceptes].c_color, colors[c_acceptes]);

        //on lance un thread utilisation pour le nouveau client
        if (pthread_create(&c_threads[c_acceptes], NULL, utilisation, (void *)&clients[c_acceptes]) != 0) {
            perror("Erreur lors de la création du thread du client");
            close(c_sock[c_acceptes]);
            exit(EXIT_FAILURE);
        }
        c_acceptes++;
    }

    close(s_sock);

    //en théorie pas de problemes, car tous les clients doivent être offline pour que le serveur se ferme
    for (int i = 0; i < c_acceptes; i++) {
        pthread_join(c_threads[i], NULL);
    }
    return 0;
}
