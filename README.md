# ICT
ICT est un testeur de circuits intégrés
## Présentation
ICT permet d'identifier et de vérifier le fonctionnement de circuits logiques des séries [4000](https://fr.wikipedia.org/wiki/Liste_des_circuits_int%C3%A9gr%C3%A9s_de_la_s%C3%A9rie_4000) et [7400](https://fr.wikipedia.org/wiki/Liste_des_circuits_int%C3%A9gr%C3%A9s_de_la_s%C3%A9rie_7400).

Il permet également de vérifier le fonctionnement de composants mémoires.
- Mémoires dynamiques 4164, 41256, 4416 et 44256.
- Mémoires statiques 2114.

ICT est une évolution du logiciel du projet [Smart IC Tester](https://www.instructables.com/id/Smart-IC-Tester/) de Akshay Baweja.

La partie matérielle est identique.

## Installation
ICT est constitué de plusieurs éléments.
### Matériel
- Une carte [Arduino Mega 2560](https://store.arduino.cc/arduino-mega-2560-rev3).
- Un écran TFT tactile avec lecteur de carte micro SD intégré.
- La carte d'extension [IC Tester](https://www.instructables.com/id/Smart-IC-Tester/) de Akshay Baweja.
- Une carte micro SD.

En ce qui concerne l'écran, il existe de nombreux modèles avec différents contrôleurs. Il faudra en conséquence adapter la gestion de l'écran.
L'écran utilisé est parfaitement géré par la librairie MCUFRIEND_kbv pour l'affichage et la librairie Adafruit TouchScreen pour le tactile.

Les fichiers **logic.txt** et **ram.txt** doivent être copiés à la racine de la carte micro SD.
### Logiciel
Pour développer l'application, l'IDE Eclipse, le plugin Sloeber et le core officiel Arduino sont utilisés :
- [Eclipse CDT](https://www.eclipse.org/cdt/) 2020-06
- [Sloeber](https://eclipse.baeyens.it/) 4.3.3
- Core officiel [Arduino AVR](https://github.com/arduino/ArduinoCore-avr) 1.8.2
> Il est toutefois possible d'utiliser l'IDE Arduino pour compiler l'application.

#### Librairies
L'application utilise plusieurs librairies :
- Adafruit BusIO 1.5.0
- Adafruit GFX Library 1.10.1
- Adafruit TouchScreen 1.1.0
- MCUFRIEND_kbv 2.9.9
- SdFat 1.1.4
#### Modifications
Il est nécessaire de modifier le fichier **SdFatConfig.h** de la librairie SdFat pour permettre l'accès à la carte micro SD.
```cpp
#define ENABLE_SOFTWARE_SPI_CLASS 1
```
Si votre écran utilise un composant HX8347, il est nécessaire de décommenter une ligne du fichier **MCUFRIEND_kbv.cpp** pour activer le support de ce composant.
```cpp
#define SUPPORT_8347D
```
Par ailleurs, en fonction de l'écran utilisé, il est possible que le code nécessite des adaptations pour gérer l'orientation et le tactile.
Ces portions de code sont signalées par un commentaire.
```cpp
// TODO adapt code to your display
```
## Utilisation
### Identification de circuits logiques
- Insérez le circuit à identifier dans le support ZIF.
- Sélectionnez l'option **identify logic**.
- Sur l'écran suivant, sélectionnez le nombre de broches, 14 ou 16 du circuit.
- L'identification du composant est effectuée.
### Test de circuits logiques
- Insérez le circuit à tester dans le support ZIF.
- Sélectionnez l'option **test logic**.
- Sur l'écran suivant, entrez le code du circuit à tester. Par exemple **74139** pour un composant **74LS139**.
- Cliquez sur le bouton **ENTER**.
- Le test du circuit est effectué. Pour interrompre l'opération, appuyez sur l'écran. 
### Test de composants mémoire
- Insérez le composant à tester dans le support ZIF.
- Sélectionnez l'option **test RAM**.
- Sur l'écran suivant, entrez le code du composant à tester. Par exemple **4164** pour un composant **TMS4164**.
- Cliquez sur le bouton **ENTER**.
- Le test du composant est effectué.
