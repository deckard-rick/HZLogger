Stammdatenverwaltung
====================

Version 1.0
Man kann seine Basis/Stammdaten für kleine Projekte natürlich in irgendwelche-Ini Dateien schreiben,
denn sie ändern sich vermutlich selten. Ich bin aber so gestrickt das ich alles gerne in passenden
Datenbank-Tabellen ablege. Dann besteht aber das Problem, deren Inhalt schnell leicht und einfach 
bearbeiten zu können.

Daher habe ich hier ein Satz PHP-Klassen entworfen, die ohne jedes andere Framework, aus sich selbst
heraus Stammdaten-Bearbeitung zulassen.

baseStamController - bearbeitet die Anfragen die an den LAMP-Stack gestellt werden.
baseModel - Model welches mit der MYSQL Tabelle umgehen kann und das neu/speichern/löschen vornimmt
baseView - zur Anzeige der Listen und Formulare
baseView.phtml - ein minimales HTML-Layout

iotdeviceModel - das erste konkrete Model auf einer einfachen Datenbank-Tabellen
iotdevice.php - die Seite, die von außen angesprochen wird

25.06.2018-tg

Version 1.1
Erweiterung um select-Option bei Reference-Keys, Read-Only in der Form und kleinere Verbesserungen

Version 1.2
Ausgabe der View nicht mehr direkt in baseStamController, damit flexibler
Einführung eines baseRequest Objectes, das vereinfacht die Übergaben und zentralisiert die Analyse
