# Observator
Ein einfacher Markdown-Dokumentbetrachter.

Das Dokument läßt sich extern bearbeiten und wird nach jeder
Speicherung automatisch und gänzlich formatiert angezeigt.
Es kann außerdem zu einem fertigen Dokumentformat wie etwa .pdf
exportiert werden.

Der Observator nutzt Pandoc, python3, sowie MathJax zur formatierung
mathematischer Formeln. Diese müssen sich im Pfad befinden.

Note: At this time, this is a personal project and is only available in
German.

## Tabellen
Der Observator kann aus einer externen Textdatei Markdown-Tabellen
generieren, wobei die Spalten hier nacheinander angegeben werden,
was zur guten Lesbarkeit beiträgt.

Die Tabellen werden bei jeder Änderung mitgeneriert.
Im Markdown wird dazu ein Kommentar wie folgt eingefügt:

```
<!--TABLE "test.table"-->
```

Die dazugehörige Datei:

```
# Spaltentitel 1
Zeile 1
Zeile 2
# Spaltentitel 2
Zeile 1
Zeile 2
```

