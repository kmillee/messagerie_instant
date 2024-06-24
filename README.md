# Serveur de discussion
Ce programme C utilise des sockets pour simuler le fonctionnement d'un serveur de messagerie instantannée. 
Ce code a été créé dans le contexte d'un projet noté de notre cours de Systèmes d'exploitation.

## GUIDE D'UTILISATION

### Prérequis
* Ce code est écrit en C, vous devez avoir un compilateur adapté.
* Ce code utilise des bibliothèques et des mécanismes propres à un environnement type UNIX, il ne fonctionnera pas sous Windows.

### Installation

#### 1. Cloner le dépôt
Dans un terminal, écrivez l'instruction suivante :
```sh
git clone https://github.com/kmillee/messagerie_instant.git 
cd messagerie_instant
```
#### 2. Compiler le programme
Deux fichiers doivent être compilés :
```sh
gcc serveur.c -o serveur
gcc client.c -o client
```
#### 3. Exécuter le programme
Pour exécuter le programme, lancez dans un premier terminal le code du serveur :
```sh
./serveur
```
Puis pour chaque nouvel utilisateur, ouvrez un nouveau terminal et lancez le code client :
```sh
./client
```
### Détails
* Quitter le serveur est définitif : si un client quitte le serveur puis se reconnecte, cela créera un nouvel utilisateur.
* Le nombre maximum de connexions est de 8. (en cumulé sur une session, pas nécessairement en simultané)
* Pour fermer le serveur, il faut que tous les clients se déconnectent.

## CONTACTS
N'hésitez pas à nous contacter pour plus d'informations :
* Adam Kaufman alias [@ockedman](https://github.com/ockedman)
  * mail : adam.kaufman@dauphine.eu
* Camille Jouanet alias [@kmillee](https://github.com/kmillee)
  * mail : jouanet.camille@gmail.com
  * [Linkedin](https://fr.linkedin.com/in/camillejouanet)

Merci de vous être interessé.e à notre projet ! :)
