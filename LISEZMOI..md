# ICT
[English version](README.md)

ICT est un testeur de circuits intégrés
## Présentation
ICT permet d'identifier et de vérifier le fonctionnement de circuits logiques des séries [4000](https://fr.wikipedia.org/wiki/Liste_des_circuits_int%C3%A9gr%C3%A9s_de_la_s%C3%A9rie_4000) et [7400](https://fr.wikipedia.org/wiki/Liste_des_circuits_int%C3%A9gr%C3%A9s_de_la_s%C3%A9rie_7400).

Il permet également de vérifier le fonctionnement de composants mémoires.
- Mémoires dynamiques 4116, 4164, 41256, 4416 et 44256.
- Mémoires statiques 2114, 6116 et 61256.

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

Les fichiers **logic.txt** et **ram.txt** doivent être copiés à la racine de la carte micro SD.
### Logiciel
Pour développer l'application, l'IDE Eclipse, le plugin Sloeber et le core officiel Arduino sont utilisés :
- [Eclipse CDT](https://www.eclipse.org/cdt/) 2020-06
- [Sloeber](https://eclipse.baeyens.it/) 4.3.3
- Core officiel [Arduino AVR](https://github.com/arduino/ArduinoCore-avr) 1.8.3
> Il est toutefois possible d'utiliser l'IDE Arduino pour compiler l'application.

#### Librairies
L'application utilise plusieurs librairies :
- Adafruit BusIO 1.7.2
- Adafruit GFX Library 1.10.4
- Adafruit TouchScreen 1.1.1
- MCUFRIEND_kbv 2.9.9
- SdFat 1.1.4 ou 2.0.4
#### Modifications
Il est nécessaire de modifier le fichier **SdFatConfig.h** de la librairie SdFat pour permettre l'accès à la carte micro SD.

En version 1.1.4, il faut définir `ENABLE_SOFTWARE_SPI_CLASS` à 1 :
```cpp
#define ENABLE_SOFTWARE_SPI_CLASS 1
```
En version 2.0.4, il faut définir `SPI_DRIVER_SELECT` à 2 :
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
- Sur l'écran suivant, entrez le code du composant à tester. Par exemple **4164** ou **tms4164** pour un composant **TMS4164**.
- Cliquez sur le bouton **ENTER**.
- Le test du composant est effectué.
> Il est possible d'entrer des codes alphanumériques de composant mémoire.
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

# ICT
ICT is a tester for integrated circuits

[Français](/#french)
## Presentation
ICT allows to identify and verify integrated circuits of the [4000](https://en.wikipedia.org/wiki/List_of_4000-series_integrated_circuits) and [7400](https://en.wikipedia.org/wiki/List_of_7400-series_integrated_circuits) series.

It also allos to test memory components.
- Dynamic memories 4116, 4164, 41256, 4416 and 44256.
- Static memories 2114, 6116 et 61256.

It is possible to handle new components by adding definitions into the files of parameters.

ICT is inspired by [Smart IC Tester](https://www.instructables.com/id/Smart-IC-Tester/) project from Akshay Baweja.

## Installation
ICT is made of several items.
### Hardware
- An [Arduino Mega 2560](https://store.arduino.cc/arduino-mega-2560-rev3) card.
- A TFT touch screen with integrated micro SD card reader.
- The ICT version 2.0 extension card.
- A micro SD card.

As for the screen, there are many models with different controllers. It will therefore be necessary to adapt the management of the screen.
The screen used is perfectly managed by the MCUFRIEND_kbv library for display and the Adafruit TouchScreen library for touch.

The files **logic.txt** and **ram.txt** must be copied to the root of the micro SD card.
### Software
To develop the application, the Eclipse IDE, the Sloeber plugin and the official Arduino core are used:
- [Eclipse CDT](https://www.eclipse.org/cdt/) 2020-06
- [Sloeber](https://eclipse.baeyens.it/) 4.3.3
- Official core [Arduino AVR](https://github.com/arduino/ArduinoCore-avr) 1.8.3
> It is however possible to use the Arduino IDE to compile the application.

#### Libraries
The application uses several libraries:
- Adafruit BusIO 1.7.2
- Adafruit GFX Library 1.10.4
- Adafruit TouchScreen 1.1.1
- MCUFRIEND_kbv 2.9.9
- SdFat 1.1.4 ou 2.0.4
#### Modifications
It is necessary to modify the file ** SdFatConfig.h ** of the SdFat library to allow access to the micro SD card.

In version 1.1.4, you must set `ENABLE_SOFTWARE_SPI_CLASS` to 1:
```cpp
#define ENABLE_SOFTWARE_SPI_CLASS 1
```
In version 2.0.4, you must set `SPI_DRIVER_SELECT` to 2:
```cpp
#define SPI_DRIVER_SELECT 2
```
> In case of problem accessing the SD card, it is recommended to format the SD card with the [SD Card Formatter](https://www.sdcard.org/downloads/formatter/) tool.

If your screen uses an HX8347 component, it is necessary to uncomment a line of the **MCUFRIEND_kbv.cpp** file to activate support for this component.
```cpp
#define SUPPORT_8347D
```
In addition, depending on the screen used, the code may require adaptations to manage orientation and touch.
These portions of code are indicated by a comment.
```cpp
// TODO adapt code to your display
```
To improve touch accuracy, it is possible to increase the number of samples used in the **TouchScreen.cpp** file.
```cpp
#define NUMSAMPLES 4
```
## Usage
- The normal orientation of the tester in use is screen at the bottom, ZIF support at the top.
- Pin 1 of the component to be tested is positioned at the top, to the right of the ZIF support, towards the locking lever, as indicated by the pictogram, to the right of the ZIF support.
- Markings on the PCB at the upper edge of the ZIF support indicate the position of the component according to the number of pins.
- The push button to the right of the ZIF support is a user button that allows you to interrupt an operation.
- The push button at the top left below the screen is a reset button.

### Identification of logic circuits
- Insert the circuit to be identified in the ZIF support.
- Select the option **identify logic**.
- On the next screen, select the number of pins, 14 or 16 of the circuit.
- Component identification is performed.
### Logic circuit test
- Insert the circuit to be tested in the ZIF socket.
- Select the option **test logic**.
- On the next screen, enter the code of the circuit to be tested. For example ** 74139 ** for a component **74LS139**.
- Click on the **ENTER** button.
- The circuit test is carried out. To interrupt the operation, tap the screen.
### Memory component test
- Insert the component to be tested in the ZIF support.
- Select the option **RAM test**.
- On the next screen, enter the code of the component to be tested. For example **4164** or **tms4164** for a component **TMS4164**.
- Click on the **ENTER** button.
- Component test is performed.
> It is possible to enter alphanumeric memory component codes.
## Troubleshooting
In general, if the ICT extension does not work properly, it is necessary to make each element work step by step.
You have to start by using the screen on the Arduino Mega, without the ICT extension.
### Screen operation
If you have a white screen, the screen may not be handled by default.
On the serial monitor, the screen component identifier is displayed:
```batch
TFT initialized 0x9341
```
You can search for this identifier and check if it is possible to activate its support in the **MCUFRIEND_kbv** library.
Another solution is to use the **diagnose_TFT_support.ino** sketch in the examples of the **MCUFRIEND_kbv** library and to follow the instructions.
### Touch operation
The **TouchScreen_Calibr_native.ino** example sketch of the **MCUFRIEND_kbv** library is used to configure and calibrate the touch screen.
### SD operation
Problems accessing the SD card, apart from a correct configuration of the ** SdFat ** library, are linked to incompatible formatting of the card.

Once every element works, the last step is to integrate the extension. At this stage, the only point that can cause problems is improper insertion of the extension.### Fonctionnement de la SD
## Links
For help or to obtain the ICT extension, you can consult the thread [[Arduino] ICT testeur de circuits intégrés](https://forum.system-cfg.com/viewtopic.php?f=18&t=11417), from the System.cfg site forum.