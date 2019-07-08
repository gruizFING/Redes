Para poder compilar una aplicacion con pct se debe editar el script makefile.
En el mismo hay 2 variables, emisor y receptor que se corresponden con las aplicaciones que van a hacer el papel de enviar y recibir datos utilizando PCT.
Estas variables se cargan con el nombre de las aplicaciones a utilizar. Deben tener extension .cc.
Ejemplo: Utilizando los programas enviaFile.cc y recibeFile.cc.

Makefile:

emisor=enviaFile

receptor=recibeFile
