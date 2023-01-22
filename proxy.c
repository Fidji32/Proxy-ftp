#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port

void fils();

int main(){
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    pid_t idProc ;                   // variables pour fork
    socklen_t len;                   // Variable utilisée pour stocker les longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur
    
    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
        perror("Erreur création socket RDV\n");
        exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par 
				                      // la fonction getaddrinfo

    // Récupération des informations du serveur
    ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
    if (ecode) {
        fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }
    // Publication de la socket
    ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
    if (ecode == -1) {
        perror("Erreur liaison de la socket de RDV");
        exit(3);
    }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
    freeaddrinfo(res);

     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
    len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
    if (ecode == -1)
    {
        perror("SERVEUR: getsockname");
        exit(4);
    }
    ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ecode != 0) {
            fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
            exit(4);
    }
    printf("L'adresse d'ecoute est: %s\n", serverAddr);
    printf("Le port d'ecoute est: %s\n", serverPort);

     // Definition de la taille du tampon contenant les demandes de connexion
    ecode = listen(descSockRDV, LISTENLEN);
    if (ecode == -1) {
        perror("Erreur initialisation buffer d'écoute");
        exit(5);
    }

	len = sizeof(struct sockaddr_storage);

    //! Boucle pour que le père attende un connexion
    while (1){

        // Attente connexion du client
        // Lorsque demande de connexion, creation d'une socket de communication avec le client
        descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
        if (descSockCOM == -1){
            perror("Erreur accept\n");
            exit(6);
        }

        //! /////////////////////////
        //! Connections multiples ///
        //! /////////////////////////

        idProc = fork();
        switch(idProc){
            case -1 : // la création du processus enfant a échoué affichage d’un message d’erreur et arrêt du processus parent 
                perror("Echec fork") ;
                exit(1) ;
            case 0 : // la création du processus enfant a réussi par souci de lisibilité appel d’une fonction dans laquelle est codé son traitement
                fils(descSockCOM);
                exit(0);
        }
    }
    // close(descSockRDV);
}

void fils(int descSockCOM){

    struct addrinfo hintsClient;
    char buffer[MAXBUFFERLEN]; // Tampon de communication entre le client et le serveur
    int ecode; // Code retour des fonctions

    // Echange de données avec le client connecté
    strcpy(buffer, "220 Proxy FTP prêt\n");
    write(descSockCOM, buffer, strlen(buffer));

    //! -------------------------------------------------------- Début du code

    // Lire USER Login@ServerName
    ecode = read(descSockCOM,buffer,MAXBUFFERLEN-1);
    if(ecode == -1) {
        perror("Erreur lecture login@serveur"); 
        exit(1);
    }   
    buffer[ecode]='\0';
    char login[MAXBUFFERLEN/2-1];
    char adresse[MAXBUFFERLEN/2-1];
    sscanf(buffer,"%[^@]@%s", login, adresse);
    strcat(login,"\r\n");
    printf("login: %sadresse: %s\n", login, adresse);


    // Connexion sur le serveur FTP (CTRL)
    int SockSERV;
    char Port[] = "21";
    ecode = connect2Server(adresse,Port,&SockSERV);
    if(ecode == -1) {
        perror("Erreur lors de la connexion"); 
        exit(1);
    }
    printf("Connexion établie !\n");

    // S -> P : Lire 220
    bzero(buffer, MAXBUFFERLEN);
    ecode = read(SockSERV,buffer,MAXBUFFERLEN-1);
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }
    buffer[ecode] = '\0';

    // P -> S : Envoyer login
    ecode = write(SockSERV, login, strlen(login));
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }

    // S -> P : Lire 331 Please specify the password
    ecode = read(SockSERV,buffer,MAXBUFFERLEN-1);
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }
    buffer[ecode] = '\0';

    // P -> C : 331 Please specify the password
    ecode = write(descSockCOM,buffer,strlen(buffer));
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }

    // C : Lire PASS pass
    bzero(buffer, MAXBUFFERLEN);
    ecode = read(descSockCOM,buffer,MAXBUFFERLEN-1);
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }
    buffer[ecode] = '\0';

    // C -> S : PASS pass
    ecode = write(SockSERV,buffer,strlen(buffer));
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }

    // S : Lire 230 Login successful
    bzero(buffer, MAXBUFFERLEN);
    ecode = read(SockSERV,buffer,MAXBUFFERLEN-1);
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }
    buffer[ecode] = '\0';

    // S -> C : 230 Login successful
    ecode = write(descSockCOM,buffer,strlen(buffer));
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }

    // C : Lire SYST
    bzero(buffer, MAXBUFFERLEN);
    ecode = read(descSockCOM,buffer,MAXBUFFERLEN-1);
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }
    buffer[ecode] = '\0';

    // C -> S : SYST
    ecode = write(SockSERV,buffer,strlen(buffer));
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }

    // S : 215 UNIX Type
    bzero(buffer, MAXBUFFERLEN);
    ecode = read(SockSERV,buffer,MAXBUFFERLEN-1);
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }
    buffer[ecode] = '\0';

    // S -> C : UNIX Type
    ecode = write(descSockCOM,buffer,strlen(buffer));
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }

    // C : Lire Commande 
    bzero(buffer, MAXBUFFERLEN);
    ecode = read(descSockCOM,buffer,MAXBUFFERLEN-1);
    if(ecode == -1) {
        perror("Erreur"); 
        exit(1);
    }
    buffer[ecode] = '\0';

    //! ///////////////////////
    //! Boucle de commandes ///
    //! ///////////////////////

    while (strstr(buffer,"QUIT") == NULL){

        //! //////////////////////////////
        //! Vérification commande PORT ///
        //! //////////////////////////////

        // Vérification si commande demande PORT
        if (strstr(buffer,"PORT") == NULL){ //** Commande normale
            
            // C -> S : Commande
            ecode = write(SockSERV,buffer,strlen(buffer));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }

            // S : Lire réponse commande
            bzero(buffer, MAXBUFFERLEN);
            ecode = read(SockSERV,buffer,MAXBUFFERLEN-1);
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            buffer[ecode] = '\0';

            // S -> C : réponse commande
            ecode = write(descSockCOM,buffer,strlen(buffer));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }

            // C : Lire Commande
            bzero(buffer, MAXBUFFERLEN);
            ecode = read(descSockCOM,buffer,MAXBUFFERLEN-1);
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            buffer[ecode] = '\0';

        } 

        //! ////////////////////////////////
        //! Commande necessitant un PORT ///
        //! ////////////////////////////////

        else { 
            
            printf("PORT : %s" , buffer);
            // Formatage du port et l'ip pour PasvSockCOM
            int n1, n2, n3, n4, n5, n6;
            sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &n1, &n2, &n3, &n4, &n5, &n6);
            char AdresseClient[25];
            char PortClient[10];
            sprintf(AdresseClient, "%d.%d.%d.%d", n1, n2, n3, n4);
            sprintf(PortClient, "%d", n5 * 256 + n6);
            printf("Adresse du client : %s Port : %s\n", AdresseClient, PortClient);
            memset( &hintsClient, 0, sizeof(hintsClient));
            hintsClient.ai_socktype = SOCK_STREAM;
            hintsClient.ai_family = AF_INET;

            int PasvSockCOM;
            ecode = connect2Server(AdresseClient,PortClient,&PasvSockCOM);
            if(ecode == -1) {
                perror("Erreur lors de la connexion"); 
                exit(1);
            }

            // -> S : PASV
            ecode = write(SockSERV,"PASV\r\n",strlen("PASV\r\n"));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            bzero(buffer, MAXBUFFERLEN);

            // S : 227 Enterring Passive Mode
            bzero(buffer, MAXBUFFERLEN);
            ecode = read(SockSERV,buffer,MAXBUFFERLEN-1);
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            printf("PASV : %s" , buffer);
            buffer[ecode] = '\0';

            // Formatage du port et l'ip pour PasvSockSERV
            n1, n2, n3, n4, n5, n6 = 0;
            sscanf(buffer, "%*[^(](%d,%d,%d,%d,%d,%d", &n1, &n2, &n3, &n4, &n5, &n6);
            char AdresseServeur[25];
            char PortServeur[10];
            sprintf(AdresseServeur, "%d.%d.%d.%d", n1, n2, n3, n4);
            sprintf(PortServeur, "%d", n5 * 256 + n6);
            printf("Adresse server %s", AdresseServeur);
            printf("Port server %s", PortServeur);

            int PasvSockSERV;
            ecode = connect2Server(AdresseServeur,PortServeur,&PasvSockSERV);
            if(ecode == -1) {
                perror("Erreur lors de la connexion"); 
                exit(1);
            }

            // -> C : 200 PORT command successful
            ecode = write(descSockCOM,"200 PORT command successful\r\n",strlen("200 PORT command successful\r\n"));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }

            // C : Lire LIST
            bzero(buffer, MAXBUFFERLEN);
            ecode = read(descSockCOM,buffer,MAXBUFFERLEN-1);
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            buffer[ecode] = '\0';
            
            // C -> S : LIST
            ecode = write(SockSERV,buffer,strlen(buffer));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            bzero(buffer, MAXBUFFERLEN);

            // S : Lire 150 Opening ASCII mode data connection for file list
            ecode = read(SockSERV,buffer,MAXBUFFERLEN-1);
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            buffer[ecode] = '\0';

            // S -> C : 150 Opening ASCII mode data connection for file list
            ecode = write(descSockCOM,buffer,strlen(buffer));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }

            //! ////////////////////////
            //! Transfert de données ///
            //! ////////////////////////

            // PS : Lire réponse commande
            bzero(buffer, MAXBUFFERLEN);
            ecode = read(PasvSockSERV,buffer,MAXBUFFERLEN-1);
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            buffer[ecode] = '\0';

            //Pour rentrer dans la boucle la premiere fois on change la valeur de ecode mais pas a 0
            ecode = 1;
            while (ecode != 0) {
                ecode = write(PasvSockCOM, buffer, strlen(buffer));
                memset(buffer, 0, MAXBUFFERLEN);
                ecode = read(PasvSockSERV, buffer, MAXBUFFERLEN - 1);
                if (ecode < 0) {
                    perror("Erreur lecture pasv serveur");
                    exit(-1);
                }
            }

            // PS -> PC : Réponse commande
            ecode = write(PasvSockCOM,buffer,strlen(buffer));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }

            bzero(buffer, MAXBUFFERLEN);
            ecode = read(SockSERV,buffer,strlen(buffer));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }
            printf(" hello %s", buffer);

            ecode = write(descSockCOM,buffer,strlen(buffer));
            if(ecode == -1) {
                perror("Erreur"); 
                exit(1);
            }

            bzero(buffer, MAXBUFFERLEN);
            close(PasvSockCOM);
            close(PasvSockSERV);

        }

    }

    //Fermeture de la connexion
    close(descSockCOM);

    //! -------------------------------------------------------- Fin du code 
}

