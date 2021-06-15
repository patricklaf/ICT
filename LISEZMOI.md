# ICT
[English version](README.md)

ICT est un testeur de circuits intégrés
## Présentation
ICT permet d'identifier et de vérifier le fonctionnement de circuits logiques des séries [4000](https://fr.wikipedia.org/wiki/Liste_des_circuits_int%C3%A9gr%C3%A9s_de_la_s%C3%A9rie_4000) et [7400](https://fr.wikipedia.org/wiki/Liste_des_circuits_int%C3%A9gr%C3%A9s_de_la_s%C3%A9rie_7400).

Il permet également de vérifier le fonctionnement de composants mémoires.
- Mémoires dynamiques 4116, 4164, 41256, 4416 et 44256.
- Mémoires statiques 2114, 6116 et 61256.
- Mémoires mortes 2764, 27128 et 27256.
- Mémoires ferro-électriques FM1608 et FM1808.
- Mémoires flash SST39SF010A, SST39SF020A et SST39SF040.

Il est possible de supporter de nouveaux composants en ajoutant leur définition dans les fichiers de paramètres.

ICT est inspiré du projet [Smart IC Tester](https://www.instructables.com/id/Smart-IC-Tester/) de Akshay Baweja.

## Installation
ICT est constitué de plusieurs éléments.
### Matériel
- Une carte [Arduino Mega 2560](https://store.arduino.cc/arduino-mega-2560-rev3).
- Un écran TFT tactile avec lecteur de carte micro SD intégré.
- La carte d'extension ICT version 2.0.
- Une carte micro SD.

En ce qui concerne l'écran, il existe de nombreux modèles avec différents contrôleurs. Il faudra en conséquence adapter la gestion de l'écran.
L'écran utilisé est parfaitement géré par la librairie MCUFRIEND_kbv pour l'affichage et la librairie Adafruit TouchScreen pour le tactile.

Les fichiers **logic.ict**, **memory.ict** et **ict.ini** doivent être copiés à la racine de la carte micro SD.
### Logiciel
Pour développer l'application, l'IDE Eclipse, le plugin Sloeber et le core officiel Arduino sont utilisés :
- [Eclipse CDT](https://www.eclipse.org/cdt/) 2020-06
- [Sloeber](https://eclipse.baeyens.it/) 4.3.3
- Core officiel [Arduino AVR](https://github.com/arduino/ArduinoCore-avr) 1.8.3
> Il est toutefois possible d'utiliser l'IDE Arduino pour compiler l'application.

#### Librairies
L'application utilise plusieurs librairies :
- Adafruit BusIO 1.7.5
- Adafruit GFX Library 1.10.10
- Adafruit TouchScreen 1.1.2
- MCUFRIEND_kbv 2.9.9
- SdFat 1.1.4 ou 2.0.6
#### Modifications
Il est nécessaire de modifier le fichier **SdFatConfig.h** de la librairie **SdFat** pour permettre l'accès à la carte micro SD.

En version 1.1.4, il faut définir `ENABLE_SOFTWARE_SPI_CLASS` à 1 :
```cpp
#define ENABLE_SOFTWARE_SPI_CLASS 1
```
En version 2.0.6, il faut définir `SPI_DRIVER_SELECT` à 2 :
```cpp
#define SPI_DRIVER_SELECT 2
```
> En cas de problème d'accès à la carte SD, il est recommandé de formater la carte SD avec l'outil [SD Card Formatter](https://www.sdcard.org/downloads/formatter/).

Si votre écran utilise un composant HX8347, il est nécessaire de décommenter une ligne du fichier **MCUFRIEND_kbv.cpp** pour activer le support de ce composant.
```cpp
#define SUPPORT_8347D
```
Par ailleurs, en fonction de l'écran utilisé, il est possible que le code nécessite des adaptations pour gérer l'orientation et le tactile.
Ces portions de code sont signalées par un commentaire.
```cpp
// TODO adapt code to your display
```
Pour améliorer la précision du tactile, il est possible d'augmenter le nombre d'échantillons utilisés dans le fichier **TouchScreen.cpp**.
```cpp
#define NUMSAMPLES 4
```
## Utilisation
- L'orientation normale du testeur en utilisation est écran en bas, support ZIF en haut.
- La broche 1 du composant à tester est positionnée en haut, à droite du support ZIF, vers le levier de verrouillage, comme indiqué par le pictogramme, à droite du support ZIF.
- Des repères sur le PCB, au bord supérieur du support ZIF indique la position du composant en fonction du nombre de broches. 
- Le bouton poussoir à droite du support ZIF est un bouton utilisateur qui permet d'interrompre une opération.
- Le bouton poussoir en haut, à gauche sous l'écran est un bouton de réinitialisation.

### Identification de circuits logiques
- Insérez le composant à identifier dans le support ZIF.
- Sélectionnez l'option **Logic**.
- Sur l'écran suivant, sélectionnez le nombre de broches du composant.
- L'identification du composant est effectuée.
### Test de circuits logiques
- Insérez le composant à tester dans le support ZIF.
- Sélectionnez l'option **Logic**.
- Sur l'écran suivant, entrez le code du composant à tester. Par exemple **74139** pour un composant **74LS139**.
- Cliquez sur le bouton **ENTER**.
- Le test du circuit est effectué. Pour interrompre l'opération, appuyez sur l'écran. 
### Mémoire vive
- Insérez le composant à tester dans le support ZIF.
- Sélectionnez l'option **Memory**.
- Sur l'écran suivant, entrez le code du composant à tester. Par exemple **4164** ou **tms4164** pour un composant **TMS4164**.
- Cliquez sur le bouton **ENTER**.
- Le test du composant est effectué.
- Un test d'écriture et de relecture de 4 différents motifs de bits est effectué. 
### Mémoire morte
- Insérez le composant à tester dans le support ZIF.
- Sélectionnez l'option **Memory**.
- Sur l'écran suivant, entrez le code du composant à tester. Par exemple **2764** ou **am27c64** pour un composant **AM27C64**.
- Cliquez sur le bouton **ENTER**.
- Sur l'écran suivant, sélectionnez les options :
    - Arrêt si la mémoire n'est pas vierge.
    - Extraction du contenu sur le moniteur série.
    - Extraction du contenu sur la carte SD.
- Cliquez sur le bouton **CONTINUE**.
- Les actions sélectionnées sont effectuées.
### Mémoire ferro-électrique
- Insérez le composant à tester dans le support ZIF.
- Sélectionnez l'option **Memory**.
- Sur l'écran suivant, entrez le code du composant à tester. Par exemple **fm1608** pour un composant **FM1608**.
- Cliquez sur le bouton **ENTER**.
- Sur l'écran suivant, sélectionnez les options :
    - Extraction du contenu sur le moniteur série.
    - Extraction du contenu sur la carte SD.
    - Effacement.
    - Ecriture.
- Cliquez sur le bouton **CONTINUE**.
- Les actions sélectionnées sont effectuées. Si aucune option n'est sélectionnée le composant est testé comme une mémoire vive.
### Mémoire flash
- Insérez le composant à tester dans le support ZIF.
- Sélectionnez l'option **Memory**.
- Sur l'écran suivant, entrez le code du composant à tester. Par exemple **sst39sf040** pour un composant **SST39SF040**.
- Cliquez sur le bouton **ENTER**.
- Sur l'écran suivant, sélectionnez les options :
    - Extraction du contenu sur le moniteur série.
    - Extraction du contenu sur la carte SD.
    - Effacement.
    - Ecriture.
- Cliquez sur le bouton **CONTINUE**.
- Les actions sélectionnées sont effectuées. Si aucune option n'est sélectionnée le composant est testé comme une mémoire vive.
## Résolution de problème
D'une manière générale, si l'extension ICT ne fonctionne pas correctement, il est nécessaire de faire fonctionner chaque élément par étape.
Il faut commencer par utiliser l'écran sur l'Arduino Mega, sans l'extension ICT.
### Fonctionnement de l'écran
Si vous avez un écran blanc, il se peut que l'écran ne soit pas géré par défaut.
Sur le moniteur série, l'identifiant du composant de l'écran est affiché :
```batch
TFT initialized 0x9341
```
Vous pouvez rechercher cet identifiant et vérifier s'il est possible d'activer son support dans la librairie **MCUFRIEND_kbv**.
Une autre solution est d'utiliser le croquis **diagnose_TFT_support.ino** dans les exemples de la librairie **MCUFRIEND_kbv** et de suivre les instructions.
### Fonctionnement du tactile
Le croquis d'exemple **TouchScreen_Calibr_native.ino** de la librairie **MCUFRIEND_kbv** permet de paramétrer et de calibrer le tactile.
### Fonctionnement de la SD
Les problèmes d'accès à la carte SD, en dehors d'une configuration correcte de la librairie **SdFat**, sont liés à un formatage incompatible de la carte.

Une fois que chaque élément fonctionne, la dernière étape est d'intégrer l'extension. A ce stade, le seul point qui peut poser problème est une mauvaise insertion de l'extension.
## Liens
Pour obtenir de l'aide ou se procurer l'extension ICT, vous pouvez consulter le fil de discussion [[Arduino] ICT testeur de circuits intégrés](https://forum.system-cfg.com/viewtopic.php?f=18&t=11417), du forum du site  System.cfg.