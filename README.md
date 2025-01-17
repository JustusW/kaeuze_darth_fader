# Darth Fader #
Dies ist die Software für den MIDI Controller der 2024 für die Käuze entwickelt wurde. Der Controller besteht aus einem Teensy 3.2, 6 MX Yellow Tasten und einem Standard Fader Potentiometer. Der Controller steuert über das MIDI Interface das SCS System des Theaters bei Aufführungen.

## Setup ##
Der Controller wird über die Arduino IDE programmiert. Hierzu sind die jeweiligen Board Libraries von Espressif zu laden.
Die IDE muss auf Teensy 3.2 eingestellt sein, als USB Type muss Serial + MIDI ausgewählt sein.
Alle anderen Bibliotheken sollten bereits mit der Board Lib installiert sein.

Der Teensy Controller wird dann über USB angeschlossen, der korrekte USB2Serial Port ausgewählt und dann die Sketch kompiliert und auf den Controller hochgeladen.

